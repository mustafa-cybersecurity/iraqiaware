#include "AIServiceLayer.h"

#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrl>
#include <QDebug>
#include <QTimer>

// ── Constructor / Destructor ──────────────────────────────────────────────────

AIServiceLayer::AIServiceLayer(QObject *parent)
    : QObject(parent)
{
    connect(&m_networkManager, &QNetworkAccessManager::finished,
            this,              &AIServiceLayer::onReplyFinished);
}

AIServiceLayer::~AIServiceLayer() = default;

// ── Configuration ─────────────────────────────────────────────────────────────

void AIServiceLayer::setProvider(Provider provider)
{
    m_provider = provider;

    // Set default model for each provider
    switch (provider) {
    case Provider::OpenAI:    m_model = "gpt-4o";                  break;
    case Provider::Gemini:    m_model = "gemini-1.5-pro";          break;
    case Provider::Anthropic: m_model = "claude-3-5-sonnet-20241022"; break;
    case Provider::Ollama:    m_model = "llava";                   break;
    case Provider::Custom:    /* keep whatever the user set */     break;
    }
}

void AIServiceLayer::setApiKey(const QString &apiKey)  { m_apiKey = apiKey; }
void AIServiceLayer::setModel(const QString &model)    { m_model = model; }
void AIServiceLayer::setCustomEndpoint(const QString &url) { m_customEndpoint = url; }
void AIServiceLayer::setTimeout(int ms)                { m_timeoutMs = ms; }
AIServiceLayer::Provider AIServiceLayer::provider() const { return m_provider; }
QString AIServiceLayer::model() const                  { return m_model; }

bool AIServiceLayer::isConfigured() const
{
    if (m_provider == Provider::Ollama) return true; // no key needed
    if (m_provider == Provider::Custom) return !m_customEndpoint.isEmpty();
    return !m_apiKey.isEmpty();
}

void AIServiceLayer::cancelPendingRequests()
{
    // Qt cleans up automatically; we could iterate manager replies if needed.
}

// ── Public API ────────────────────────────────────────────────────────────────

void AIServiceLayer::analyzeScreenshot(const QByteArray &imageBytes)
{
    if (!isConfigured()) {
        emit analysisError(QStringLiteral("AI service not configured. Please set your API key in Settings."));
        return;
    }

    const QString url = endpointUrl();
    QNetworkRequest request{QUrl(url)};
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Accept", "application/json");

    // Provider-specific auth headers
    switch (m_provider) {
    case Provider::OpenAI:
        request.setRawHeader("Authorization",
                             ("Bearer " + m_apiKey).toUtf8());
        break;
    case Provider::Gemini:
        // Key goes in query string for Gemini; handled in endpointUrl()
        break;
    case Provider::Anthropic:
        request.setRawHeader("x-api-key", m_apiKey.toUtf8());
        request.setRawHeader("anthropic-version", "2023-06-01");
        break;
    case Provider::Ollama:
    case Provider::Custom:
        if (!m_apiKey.isEmpty())
            request.setRawHeader("Authorization",
                                 ("Bearer " + m_apiKey).toUtf8());
        break;
    }

    QByteArray body = buildRequestBody(imageBytes);
    m_networkManager.post(request, body);
    emit requestProgress(10);
}

// ── Private helpers ───────────────────────────────────────────────────────────

QString AIServiceLayer::endpointUrl() const
{
    switch (m_provider) {
    case Provider::OpenAI:
        return QStringLiteral("https://api.openai.com/v1/chat/completions");
    case Provider::Gemini:
        return QStringLiteral("https://generativelanguage.googleapis.com/v1beta/models/")
               + m_model
               + QStringLiteral(":generateContent?key=")
               + m_apiKey;
    case Provider::Anthropic:
        return QStringLiteral("https://api.anthropic.com/v1/messages");
    case Provider::Ollama:
        return QStringLiteral("http://localhost:11434/api/chat");
    case Provider::Custom:
        return m_customEndpoint;
    }
    return {};
}

QString AIServiceLayer::buildPrompt() const
{
    return QStringLiteral(
        "You are a cybersecurity expert analysing a screenshot of a user's screen. "
        "Identify any security risks present in the image such as:\n"
        "- Weak or exposed passwords\n"
        "- Phishing or suspicious websites\n"
        "- Exposed sensitive personal data (credit cards, IDs, private keys)\n"
        "- Unsafe browsing habits\n"
        "- Any other security threats\n\n"
        "For each finding, respond with exactly this format:\n"
        "SEVERITY: [INFO|LOW|MEDIUM|HIGH|CRITICAL]\n"
        "CATEGORY: <category name>\n"
        "DESCRIPTION: <brief description>\n"
        "RECOMMENDATION: <actionable advice>\n"
        "---\n"
        "If no threats are detected, respond with exactly: NO_THREATS_DETECTED"
    );
}

QByteArray AIServiceLayer::buildRequestBody(const QByteArray &imageBytes) const
{
    const QString base64Image = QString::fromLatin1(imageBytes.toBase64());
    const QString prompt      = buildPrompt();

    QJsonDocument doc;

    if (m_provider == Provider::OpenAI) {
        QJsonObject imageUrlObj;
        imageUrlObj["url"] = "data:image/jpeg;base64," + base64Image;

        QJsonObject imageContentItem;
        imageContentItem["type"] = "image_url";
        imageContentItem["image_url"] = imageUrlObj;

        QJsonObject textContentItem;
        textContentItem["type"] = "text";
        textContentItem["text"] = prompt;

        QJsonArray contentArray;
        contentArray.append(textContentItem);
        contentArray.append(imageContentItem);

        QJsonObject userMessage;
        userMessage["role"]    = "user";
        userMessage["content"] = contentArray;

        QJsonArray messages;
        messages.append(userMessage);

        QJsonObject root;
        root["model"]      = m_model;
        root["messages"]   = messages;
        root["max_tokens"] = 1024;

        doc = QJsonDocument(root);

    } else if (m_provider == Provider::Anthropic) {
        QJsonObject imageSource;
        imageSource["type"]       = "base64";
        imageSource["media_type"] = "image/jpeg";
        imageSource["data"]       = base64Image;

        QJsonObject imageBlock;
        imageBlock["type"]   = "image";
        imageBlock["source"] = imageSource;

        QJsonObject textBlock;
        textBlock["type"] = "text";
        textBlock["text"] = prompt;

        QJsonArray contentArray;
        contentArray.append(imageBlock);
        contentArray.append(textBlock);

        QJsonObject userMessage;
        userMessage["role"]    = "user";
        userMessage["content"] = contentArray;

        QJsonArray messages;
        messages.append(userMessage);

        QJsonObject root;
        root["model"]      = m_model;
        root["max_tokens"] = 1024;
        root["messages"]   = messages;

        doc = QJsonDocument(root);

    } else if (m_provider == Provider::Gemini) {
        QJsonObject inlineData;
        inlineData["mime_type"] = "image/jpeg";
        inlineData["data"]      = base64Image;

        QJsonObject imagePart;
        imagePart["inline_data"] = inlineData;

        QJsonObject textPart;
        textPart["text"] = prompt;

        QJsonArray parts;
        parts.append(textPart);
        parts.append(imagePart);

        QJsonObject content;
        content["parts"] = parts;

        QJsonArray contents;
        contents.append(content);

        QJsonObject root;
        root["contents"] = contents;

        doc = QJsonDocument(root);

    } else {
        // Ollama / Custom – use OpenAI-compatible format
        QJsonObject imageContentItem;
        imageContentItem["type"] = "image_url";
        QJsonObject imgUrl;
        imgUrl["url"] = "data:image/jpeg;base64," + base64Image;
        imageContentItem["image_url"] = imgUrl;

        QJsonObject textContentItem;
        textContentItem["type"] = "text";
        textContentItem["text"] = prompt;

        QJsonArray contentArray;
        contentArray.append(textContentItem);
        contentArray.append(imageContentItem);

        QJsonObject userMessage;
        userMessage["role"]    = "user";
        userMessage["content"] = contentArray;

        QJsonArray messages;
        messages.append(userMessage);

        QJsonObject root;
        root["model"]    = m_model;
        root["messages"] = messages;
        root["stream"]   = false;

        doc = QJsonDocument(root);
    }

    return doc.toJson(QJsonDocument::Compact);
}

void AIServiceLayer::onReplyFinished(QNetworkReply *reply)
{
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        emit analysisError(QStringLiteral("Network error: ") + reply->errorString());
        return;
    }

    emit requestProgress(80);

    const QByteArray responseData = reply->readAll();
    const QJsonDocument doc       = QJsonDocument::fromJson(responseData);

    if (doc.isNull()) {
        emit analysisError(QStringLiteral("Invalid JSON response from AI service"));
        return;
    }

    QString analysisText;

    if (m_provider == Provider::OpenAI || m_provider == Provider::Ollama
        || m_provider == Provider::Custom)
    {
        analysisText = doc["choices"][0]["message"]["content"].toString();
    } else if (m_provider == Provider::Anthropic) {
        analysisText = doc["content"][0]["text"].toString();
    } else if (m_provider == Provider::Gemini) {
        analysisText = doc["candidates"][0]["content"]["parts"][0]["text"].toString();
    }

    if (analysisText.isEmpty()) {
        emit analysisError(QStringLiteral("Empty response from AI service"));
        return;
    }

    emit requestProgress(100);
    emit analysisComplete(analysisText);
}

#pragma once

#include <QObject>
#include <QString>
#include <QByteArray>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <functional>

/**
 * @brief Thin abstraction layer that talks to AI REST APIs.
 *
 * Supports multiple providers (OpenAI, Gemini, Anthropic, Ollama …) through
 * a simple provider-name / base-URL / API-key configuration.
 * All network calls are asynchronous; results are returned via signals.
 */
class AIServiceLayer : public QObject
{
    Q_OBJECT

public:
    /** The AI providers we have built-in prompt templates for. */
    enum class Provider {
        OpenAI,     ///< GPT-4o / GPT-4-vision
        Gemini,     ///< Google Gemini 1.5 Pro
        Anthropic,  ///< Claude 3 Opus / Sonnet
        Ollama,     ///< Local Ollama (llava / bakllava)
        Custom      ///< User-defined endpoint
    };

    explicit AIServiceLayer(QObject *parent = nullptr);
    ~AIServiceLayer() override;

    // ── Configuration ────────────────────────────────────────────────────────
    void setProvider(Provider provider);
    void setApiKey(const QString &apiKey);
    void setModel(const QString &model);
    void setCustomEndpoint(const QString &url);
    void setTimeout(int ms);

    Provider provider() const;
    QString  model() const;

    // ── Main API ─────────────────────────────────────────────────────────────
    /**
     * Send @p imageBytes (a JPEG screenshot) to the configured AI endpoint and
     * request a security analysis.  Results arrive via analysisComplete().
     */
    void analyzeScreenshot(const QByteArray &imageBytes);

    /** Cancel any in-flight request. */
    void cancelPendingRequests();

    bool isConfigured() const;
    bool isBusy() const;

signals:
    /** Emitted when the AI returns a response.
     *  @param analysisText  Raw text returned by the AI model.
     */
    void analysisComplete(const QString &analysisText);

    /** Emitted on any network or API error. */
    void analysisError(const QString &errorMessage);

    /** Emitted while a request is in progress (0-100). */
    void requestProgress(int percent);
    void busyChanged(bool busy);

private slots:
    void onReplyFinished(QNetworkReply *reply);

private:
    QString buildPrompt() const;
    QByteArray buildRequestBody(const QByteArray &imageBytes) const;
    QString    endpointUrl() const;
    void       logRequest(const QString &url) const;

    QNetworkAccessManager m_networkManager;
    Provider m_provider{Provider::OpenAI};
    QString  m_apiKey;
    QString  m_model{"gpt-4o"};
    QString  m_customEndpoint;
    int      m_timeoutMs{30000};
    bool     m_requestInFlight{false};
    QByteArray m_pendingImageBytes;
};

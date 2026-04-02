#include "AIServiceLayer.h"
#include "LoggingSystem.h"

#include <nlohmann/json.hpp>
#include <curl/curl.h>

#include <chrono>
#include <stdexcept>
#include <sstream>
#include <thread>
#include <vector>

using json = nlohmann::json;

namespace {

// cURL write callback – accumulates response body in a std::string
size_t curlWriteCallback(char* ptr, size_t size, size_t nmemb, void* userdata) {
    auto* buf = static_cast<std::string*>(userdata);
    buf->append(ptr, size * nmemb);
    return size * nmemb;
}

} // anonymous namespace

// ─────────────────────────────────────────────────────────────────────────────

AIServiceLayer::AIServiceLayer() {
    curl_global_init(CURL_GLOBAL_DEFAULT);
}

AIServiceLayer::~AIServiceLayer() {
    curl_global_cleanup();
}

void AIServiceLayer::setProvider(const std::string& providerName) {
    m_provider = providerName;
}

void AIServiceLayer::setApiKey(const std::string& key) {
    m_apiKey = key;
}

void AIServiceLayer::setModel(const std::string& model) {
    m_model = model;
}

bool AIServiceLayer::isConfigured() const {
    return !m_apiKey.empty() && !m_provider.empty();
}

std::vector<std::string> AIServiceLayer::getSupportedProviders() {
    return {"openai", "google"};
}

// ── Analyse a screenshot ──────────────────────────────────────────────────────

void AIServiceLayer::analyzeScreenshot(const std::string& base64Image,
                                        ResponseCallback   callback) {
    if (!isConfigured()) {
        LOG_WARNING("AIServiceLayer: not configured – skipping analysis");
        if (callback) callback(false, "", "AI service not configured. Please set provider and API key.");
        return;
    }

    std::string body;
    try {
        body = buildRequestBody(base64Image);
    } catch (const std::exception& e) {
        if (callback) callback(false, "", e.what());
        return;
    }

    std::map<std::string, std::string> headers;
    headers["Content-Type"] = "application/json";
    if (m_provider == "openai") {
        headers["Authorization"] = "Bearer " + m_apiKey;
    }

    const std::string url = getEndpointUrl();

    for (int attempt = 0; attempt <= m_maxRetries; ++attempt) {
        if (attempt > 0) {
            // Exponential backoff: 1 s, 2 s, 4 s, …
            const int delaySec = (1 << (attempt - 1));
            LOG_INFO("AIServiceLayer: retry " + std::to_string(attempt) +
                     "/" + std::to_string(m_maxRetries) +
                     " after " + std::to_string(delaySec) + " s");
            std::this_thread::sleep_for(std::chrono::seconds(delaySec));
        }

        try {
            std::string responseBody;
            const long status = httpPost(url, body, headers, responseBody);

            if (status >= 200 && status < 300) {
                std::string parsed = parseResponse(responseBody);
                if (callback) callback(true, parsed, "");
                return;
            }

            // Rate-limited or server error → retry
            if (status == 429 || status >= 500) {
                LOG_WARNING("AIServiceLayer: HTTP " + std::to_string(status) +
                            " – will retry");
                continue;
            }

            // Any other client-side error → do not retry
            std::string errMsg = "HTTP " + std::to_string(status) + ": " + responseBody;
            LOG_ERROR("AIServiceLayer request failed: " + errMsg);
            if (callback) callback(false, "", errMsg);
            return;

        } catch (const std::exception& e) {
            LOG_ERROR(std::string("AIServiceLayer exception: ") + e.what());
            if (attempt == m_maxRetries) {
                if (callback) callback(false, "", e.what());
                return;
            }
            // network error – retry
        }
    }

    if (callback) callback(false, "", "Max retries exceeded");
}

// ── Build request body ────────────────────────────────────────────────────────

std::string AIServiceLayer::buildRequestBody(const std::string& base64Image) const {
    if (m_provider == "openai") {
        json req;
        req["model"] = m_model;
        req["messages"] = json::array({
            {
                {"role", "system"},
                {"content", kSystemPrompt}
            },
            {
                {"role", "user"},
                {"content", json::array({
                    {
                        {"type", "image_url"},
                        {"image_url", {
                            {"url", "data:image/png;base64," + base64Image},
                            {"detail", "high"}
                        }}
                    },
                    {
                        {"type", "text"},
                        {"text", "Analyze this screenshot for cybersecurity threats."}
                    }
                })}
            }
        });
        req["max_tokens"] = 1000;
        return req.dump();

    } else if (m_provider == "google") {
        json req;
        req["requests"] = json::array({
            {
                {"image", {{"content", base64Image}}},
                {"features", json::array({
                    {{"type", "TEXT_DETECTION"}},
                    {{"type", "SAFE_SEARCH_DETECTION"}}
                })}
            }
        });
        return req.dump();
    }

    throw std::runtime_error("Unsupported AI provider: " + m_provider);
}

// ── Parse response ────────────────────────────────────────────────────────────

std::string AIServiceLayer::parseResponse(const std::string& rawJson) const {
    try {
        auto j = json::parse(rawJson);

        if (m_provider == "openai") {
            // Extract content from choices[0].message.content
            if (j.contains("choices") && j["choices"].is_array()
                    && !j["choices"].empty()) {
                auto& choice = j["choices"][0];
                if (choice.contains("message") &&
                    choice["message"].contains("content")) {
                    return choice["message"]["content"].get<std::string>();
                }
            }
        } else if (m_provider == "google") {
            // Return full response for SecurityAnalyzer to interpret
            return rawJson;
        }
    } catch (...) {}

    return rawJson; // fall-through: return raw JSON
}

// ── HTTP POST via cURL ────────────────────────────────────────────────────────

long AIServiceLayer::httpPost(const std::string& url,
                               const std::string& body,
                               const std::map<std::string, std::string>& headers,
                               std::string& outBody) {
    CURL* curl = curl_easy_init();
    if (!curl) throw std::runtime_error("curl_easy_init failed");

    curl_slist* headerList = nullptr;
    for (auto& [k, v] : headers) {
        headerList = curl_slist_append(headerList, (k + ": " + v).c_str());
    }

    curl_easy_setopt(curl, CURLOPT_URL,            url.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS,     body.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER,     headerList);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,  curlWriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA,      &outBody);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT,        static_cast<long>(m_timeoutSec));
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);

    CURLcode res  = curl_easy_perform(curl);
    long     code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);

    curl_slist_free_all(headerList);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        throw std::runtime_error(
            std::string("cURL error: ") + curl_easy_strerror(res));
    }

    return code;
}

// ── Endpoint URL ──────────────────────────────────────────────────────────────

std::string AIServiceLayer::getEndpointUrl() const {
    if (m_provider == "openai") {
        return "https://api.openai.com/v1/chat/completions";
    } else if (m_provider == "google") {
        return "https://vision.googleapis.com/v1/images:annotate?key=" + m_apiKey;
    }
    throw std::runtime_error("No endpoint defined for provider: " + m_provider);
}

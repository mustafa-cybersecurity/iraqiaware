#pragma once

#include <string>
#include <functional>
#include <memory>
#include <map>
#include <vector>

/**
 * AIServiceLayer
 * --------------
 * Provides a unified REST-API client for multiple AI vision providers
 * (OpenAI GPT-4 Vision, Google Cloud Vision, etc.).
 *
 * Responsibilities:
 *  - Build and send HTTP requests with cURL
 *  - Handle API key management (read from ConfigManager)
 *  - Parse JSON responses into raw text ready for SecurityAnalyzer
 *  - Implement basic retry / rate-limiting logic
 */
class AIServiceLayer {
public:
    // Async completion callback: (success, responseText, errorMsg)
    using ResponseCallback = std::function<void(bool success,
                                                const std::string& response,
                                                const std::string& error)>;

    explicit AIServiceLayer();
    ~AIServiceLayer();

    // Configure active provider ("openai", "google", …)
    void setProvider(const std::string& providerName);
    std::string getProvider() const { return m_provider; }

    // Set / retrieve the API key for the active provider
    void setApiKey(const std::string& key);
    std::string getApiKey() const { return m_apiKey; }

    // Set model identifier (e.g. "gpt-4o", "gpt-4-vision-preview")
    void setModel(const std::string& model);
    std::string getModel() const { return m_model; }

    // Set request timeout in seconds
    void setTimeout(int seconds) { m_timeoutSec = seconds; }

    /**
     * Analyse a screenshot (supplied as Base64-encoded PNG/JPEG).
     * The prompt instructs the model to look for cybersecurity threats.
     * Calls \p callback on the calling thread (synchronous cURL call).
     */
    void analyzeScreenshot(const std::string& base64Image,
                           ResponseCallback   callback);

    // Validate that the current configuration is complete
    bool isConfigured() const;

    // Retrieve the list of supported provider names
    static std::vector<std::string> getSupportedProviders();

private:
    // Build request body JSON for the active provider
    std::string buildRequestBody(const std::string& base64Image) const;

    // Parse raw JSON response from the active provider → plain text
    std::string parseResponse(const std::string& rawJson) const;

    // Perform HTTP POST; returns HTTP status code, body in \p outBody
    long httpPost(const std::string& url,
                  const std::string& body,
                  const std::map<std::string, std::string>& headers,
                  std::string& outBody);

    // Endpoint URL for the active provider
    std::string getEndpointUrl() const;

private:
    std::string m_provider   = "openai";
    std::string m_apiKey;
    std::string m_model      = "gpt-4o";
    int         m_timeoutSec = 30;

    // System prompt template sent to the vision model
    static constexpr const char* kSystemPrompt =
        "You are a cybersecurity expert analyzing a screenshot for security threats. "
        "Identify any of the following issues:\n"
        "1. Weak or exposed passwords in visible form fields\n"
        "2. Phishing indicators (suspicious URLs, fake login pages, warning banners)\n"
        "3. Sensitive data exposure (credit card numbers, SSNs, private keys)\n"
        "4. Unsafe browsing (HTTP sites handling sensitive data, unsecured connections)\n"
        "5. Malware indicators (suspicious pop-ups, download prompts)\n\n"
        "Respond in JSON format:\n"
        "{\n"
        "  \"threats\": [\n"
        "    {\n"
        "      \"category\": \"<WeakPassword|PhishingDetected|SensitiveDataExposed|UnsafeBrowsing|MalwareIndicator>\",\n"
        "      \"severity\": \"<Low|Medium|High|Critical>\",\n"
        "      \"title\": \"<short title>\",\n"
        "      \"description\": \"<detailed description>\",\n"
        "      \"recommendation\": \"<action to take>\"\n"
        "    }\n"
        "  ]\n"
        "}\n"
        "If no threats are found, return {\"threats\": []}.";
};

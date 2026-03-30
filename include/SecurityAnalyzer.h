#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QDateTime>
#include <vector>

/**
 * @brief Parses AI analysis text and classifies security findings.
 */
class SecurityAnalyzer : public QObject
{
    Q_OBJECT

public:
    /** Severity levels used for threat classification. */
    enum class Severity {
        Info,
        Low,
        Medium,
        High,
        Critical
    };

    /** A single security finding extracted from AI output. */
    struct Threat {
        Severity    severity;
        QString     category;   ///< e.g. "Weak Password", "Phishing"
        QString     description;
        QString     recommendation;
        QDateTime   detectedAt;
    };

    explicit SecurityAnalyzer(QObject *parent = nullptr);
    ~SecurityAnalyzer() override;

    /**
     * Parse raw AI response text and emit threatsDetected() for any findings.
     * @param aiResponse  Plain text returned by AIServiceLayer.
     */
    void analyze(const QString &aiResponse);

    /** Returns all threats found in the last analysis pass. */
    const std::vector<Threat> &lastThreats() const;

    /** Utility: human-readable severity label. */
    static QString severityLabel(Severity s);

    /** Utility: CSS class name for severity colour. */
    static QString severityClass(Severity s);

signals:
    /** Emitted once per analyze() call with every finding (may be empty). */
    void threatsDetected(const std::vector<SecurityAnalyzer::Threat> &threats);

    /** Emitted when the AI says the screen looks clean. */
    void noThreatsDetected();

private:
    // ── Keyword-based heuristics (backup when AI gives unstructured text) ──
    Severity    classifySeverity(const QString &text) const;
    QString     extractCategory(const QString &line) const;
    QStringList splitIntoBlocks(const QString &text) const;

    std::vector<Threat> m_lastThreats;
};

Q_DECLARE_METATYPE(SecurityAnalyzer::Threat)
Q_DECLARE_METATYPE(std::vector<SecurityAnalyzer::Threat>)

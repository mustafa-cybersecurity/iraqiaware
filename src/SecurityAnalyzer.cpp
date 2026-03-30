#include "SecurityAnalyzer.h"

#include <QRegularExpression>
#include <QStringList>
#include <QDebug>

// ── Constructor / Destructor ──────────────────────────────────────────────────

SecurityAnalyzer::SecurityAnalyzer(QObject *parent)
    : QObject(parent)
{
    qRegisterMetaType<SecurityAnalyzer::Threat>();
    qRegisterMetaType<std::vector<SecurityAnalyzer::Threat>>();
}

SecurityAnalyzer::~SecurityAnalyzer() = default;

// ── Public API ────────────────────────────────────────────────────────────────

void SecurityAnalyzer::analyze(const QString &aiResponse)
{
    m_lastThreats.clear();

    const QString trimmed = aiResponse.trimmed();

    if (trimmed.contains(QStringLiteral("NO_THREATS_DETECTED"),
                         Qt::CaseInsensitive))
    {
        emit noThreatsDetected();
        return;
    }

    // Parse structured blocks separated by "---"
    const QStringList blocks = splitIntoBlocks(trimmed);

    for (const QString &block : blocks) {
        if (block.trimmed().isEmpty()) continue;

        Threat threat;
        threat.detectedAt = QDateTime::currentDateTime();

        // Extract SEVERITY
        QRegularExpression severityRe(
            QStringLiteral("SEVERITY\\s*:\\s*(INFO|LOW|MEDIUM|HIGH|CRITICAL)"),
            QRegularExpression::CaseInsensitiveOption);
        auto severityMatch = severityRe.match(block);
        if (severityMatch.hasMatch()) {
            const QString s = severityMatch.captured(1).toUpper();
            if      (s == "INFO")     threat.severity = Severity::Info;
            else if (s == "LOW")      threat.severity = Severity::Low;
            else if (s == "MEDIUM")   threat.severity = Severity::Medium;
            else if (s == "HIGH")     threat.severity = Severity::High;
            else if (s == "CRITICAL") threat.severity = Severity::Critical;
            else                      threat.severity = Severity::Low;
        } else {
            threat.severity = classifySeverity(block);
        }

        // Extract CATEGORY
        QRegularExpression categoryRe(
            QStringLiteral("CATEGORY\\s*:\\s*(.+)"),
            QRegularExpression::CaseInsensitiveOption);
        auto categoryMatch = categoryRe.match(block);
        threat.category = categoryMatch.hasMatch()
                          ? categoryMatch.captured(1).trimmed()
                          : extractCategory(block);

        // Extract DESCRIPTION
        QRegularExpression descRe(
            QStringLiteral("DESCRIPTION\\s*:\\s*(.+)"),
            QRegularExpression::CaseInsensitiveOption);
        auto descMatch = descRe.match(block);
        threat.description = descMatch.hasMatch()
                             ? descMatch.captured(1).trimmed()
                             : block.trimmed().left(200);

        // Extract RECOMMENDATION
        QRegularExpression recRe(
            QStringLiteral("RECOMMENDATION\\s*:\\s*(.+)"),
            QRegularExpression::CaseInsensitiveOption);
        auto recMatch = recRe.match(block);
        if (recMatch.hasMatch()) {
            threat.recommendation = recMatch.captured(1).trimmed();
        }

        if (!threat.description.isEmpty()) {
            m_lastThreats.push_back(std::move(threat));
        }
    }

    if (m_lastThreats.empty()) {
        // Fallback: treat entire text as a single finding
        Threat t;
        t.severity    = classifySeverity(trimmed);
        t.category    = extractCategory(trimmed);
        t.description = trimmed.left(500);
        t.detectedAt  = QDateTime::currentDateTime();
        m_lastThreats.push_back(t);
    }

    emit threatsDetected(m_lastThreats);
}

const std::vector<SecurityAnalyzer::Threat> &SecurityAnalyzer::lastThreats() const
{
    return m_lastThreats;
}

// ── Static utilities ──────────────────────────────────────────────────────────

QString SecurityAnalyzer::severityLabel(Severity s)
{
    switch (s) {
    case Severity::Info:     return QStringLiteral("INFO");
    case Severity::Low:      return QStringLiteral("LOW");
    case Severity::Medium:   return QStringLiteral("MEDIUM");
    case Severity::High:     return QStringLiteral("HIGH");
    case Severity::Critical: return QStringLiteral("CRITICAL");
    }
    return QStringLiteral("UNKNOWN");
}

QString SecurityAnalyzer::severityClass(Severity s)
{
    switch (s) {
    case Severity::Info:     return QStringLiteral("severity-info");
    case Severity::Low:      return QStringLiteral("severity-low");
    case Severity::Medium:   return QStringLiteral("severity-medium");
    case Severity::High:     return QStringLiteral("severity-high");
    case Severity::Critical: return QStringLiteral("severity-critical");
    }
    return {};
}

// ── Private helpers ───────────────────────────────────────────────────────────

SecurityAnalyzer::Severity SecurityAnalyzer::classifySeverity(const QString &text) const
{
    const QString lower = text.toLower();
    if (lower.contains("critical") || lower.contains("severe"))
        return Severity::Critical;
    if (lower.contains("high") || lower.contains("password") ||
        lower.contains("phishing") || lower.contains("credential"))
        return Severity::High;
    if (lower.contains("medium") || lower.contains("sensitive") ||
        lower.contains("unsafe") || lower.contains("exposed"))
        return Severity::Medium;
    if (lower.contains("low") || lower.contains("warning"))
        return Severity::Low;
    return Severity::Info;
}

QString SecurityAnalyzer::extractCategory(const QString &line) const
{
    const QString lower = line.toLower();
    if (lower.contains("password"))           return QStringLiteral("Weak Password");
    if (lower.contains("phishing") ||
        lower.contains("suspicious"))         return QStringLiteral("Phishing/Suspicious Site");
    if (lower.contains("sensitive") ||
        lower.contains("personal") ||
        lower.contains("credit") ||
        lower.contains("private key"))        return QStringLiteral("Sensitive Data Exposure");
    if (lower.contains("browsing") ||
        lower.contains("http://"))            return QStringLiteral("Unsafe Browsing");
    if (lower.contains("malware") ||
        lower.contains("virus"))              return QStringLiteral("Malware/Virus Risk");
    return QStringLiteral("Security Risk");
}

QStringList SecurityAnalyzer::splitIntoBlocks(const QString &text) const
{
    // Split on one or more consecutive "-" separator lines.
    // The AI prompt instructs it to use exactly "---" between findings,
    // but we accept "----" or longer to be tolerant of minor AI formatting variations.
    return text.split(QRegularExpression(QStringLiteral("-{3,}")),
                      Qt::SkipEmptyParts);
}

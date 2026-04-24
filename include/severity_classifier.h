#ifndef _SEVERITY_CLASSIFIER_H
#define _SEVERITY_CLASSIFIER_H

#include "log_entry.h"
#include <string>
#include <vector>
#include <utility>

/**
 * @brief Rule that maps a keyword found in a log payload to a minimum severity.
 *
 * If the payload contains `keyword` (case-insensitive) and the current
 * severity is below `minSeverity`, the entry is escalated.
 */
struct EscalationRule {
    std::string keyword;
    Severity    minSeverity;
};

/**
 * @brief Classifies / escalates the severity of a LogEntry based on payload
 *        keywords and log-type heuristics.
 *
 * Thread-safe: the rule table is read-only after construction.
 */
class SeverityClassifier {
public:
    SeverityClassifier();

    void addRule(const std::string& keyword, Severity minSev);

    /**
     * @brief Return the (possibly escalated) severity for `entry`.
     *
     * The original entry is not mutated; the caller decides whether
     * to overwrite `entry.severity`.
     */
    Severity classify(const LogEntry& entry) const;

private:
    std::vector<EscalationRule> rules_;
    void loadDefaultRules();
    static bool containsCI(const std::string& haystack,
                           const std::string& needle);
};

#endif

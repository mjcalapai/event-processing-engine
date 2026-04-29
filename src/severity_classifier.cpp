#include "severity_classifier.h"
#include <algorithm>
#include <cctype>

// Helper

bool SeverityClassifier::containsCI(const std::string& haystack,
                                    const std::string& needle) {
    if (needle.empty()) return false;
    auto it = std::search(
        haystack.begin(), haystack.end(),
        needle.begin(),   needle.end(),
        [](char a, char b) { return std::tolower(a) == std::tolower(b); });
    return it != haystack.end();
}

// Construction

SeverityClassifier::SeverityClassifier() {
    loadDefaultRules();
}

void SeverityClassifier::loadDefaultRules() {
    // Security-critical keywords -> CRITICAL
    rules_.push_back({"port scan",         Severity::CRITICAL});
    rules_.push_back({"oom killer",        Severity::CRITICAL});
    rules_.push_back({"buffer overflow",   Severity::CRITICAL});
    rules_.push_back({"privilege escalation", Severity::CRITICAL});
    rules_.push_back({"root logged in",    Severity::CRITICAL});
    rules_.push_back({"brute force",       Severity::CRITICAL});

    // Warning-level keywords -> ERROR
    rules_.push_back({"failed login",      Severity::ERROR});
    rules_.push_back({"authentication failure", Severity::ERROR});
    rules_.push_back({"access denied",     Severity::ERROR});
    rules_.push_back({"connection refused", Severity::ERROR});
    rules_.push_back({"timeout",           Severity::ERROR});

    // Informational escalations -> WARNING
    rules_.push_back({"packet dropped",    Severity::WARNING});
    rules_.push_back({"retry",             Severity::WARNING});
    rules_.push_back({"disk usage",        Severity::WARNING});
}

void SeverityClassifier::addRule(const std::string& keyword, Severity minSev) {
    rules_.push_back({keyword, minSev});
}

// Classification

Severity SeverityClassifier::classify(const LogEntry& entry) const {
    Severity result = entry.severity;

    for (const auto& rule : rules_) {
        if (containsCI(entry.payload, rule.keyword)) {
            if (static_cast<int>(rule.minSeverity) > static_cast<int>(result)) {
                result = rule.minSeverity;
            }
        }
    }

    if (entry.type == LogType::AUTH &&
        static_cast<int>(result) < static_cast<int>(Severity::WARNING)) {
        result = Severity::WARNING;
    }

    return result;
}

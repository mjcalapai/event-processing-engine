#ifndef _LOG_ENTRY_H
#define _LOG_ENTRY_H

#include <string>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <cstdint>
#include <arpa/inet.h>  // for IP validation
#include <atomic>

uint64_t nextLogID();

enum class Severity {
    DEBUG    = 0,
    INFO     = 1,
    WARNING  = 2,
    ERROR    = 3,
    CRITICAL = 4
};

std::string severityToString(Severity s);
Severity    severityFromString(const std::string& s);// throws on bad input

enum class LogType {
    AUTH,
    NETWORK,
    SYSTEM,
    APPLICATION,
    UNKNOWN
};

std::string logTypeToString(LogType t);
LogType     logTypeFromString(const std::string& t);

using Timestamp = std::chrono::system_clock::time_point;

Timestamp   now();
std::string timestampToString(const Timestamp& ts);
Timestamp   timestampFromString(const std::string& s); // ISO 8601




struct LogEntry {
    uint64_t    id; // unique monotonic ID
    Timestamp   timestamp;
    std::string source;// hostname / process name
    std::string source_ip;// dotted-decimal IPv4 or IPv6
    Severity    severity;
    LogType     type;
    std::string payload;// raw log message body

    // Formatting
    std::string toString() const;

    // Validation — returns false + sets `error` if malformed
    bool validate(std::string& error) const;
};

// Parse one line of your log format into a LogEntry.
// Returns false and sets `error` on failure.
bool parseLogLine(const std::string& line, LogEntry& out, std::string& error);

//  Generator (for testing) 
LogEntry generateRandomLog(uint64_t id);

#endif
#include "log_entry.h"
#include <sstream>
#include <stdexcept>
#include <random>
//#include <regex> // Not needed since we use inet_pton for IP validation



static std::atomic<uint64_t> id_counter{0};

uint64_t nextLogID() {
    return id_counter.fetch_add(1);
}

std::string severityToString(Severity s) {
    switch (s) {
        case Severity::DEBUG:    return "DEBUG";
        case Severity::INFO:     return "INFO";
        case Severity::WARNING:  return "WARNING";
        case Severity::ERROR:    return "ERROR";
        case Severity::CRITICAL: return "CRITICAL";
    }
    return "UNKNOWN";
}

Severity severityFromString(const std::string& s) {
    if (s == "DEBUG")    return Severity::DEBUG;
    if (s == "INFO")     return Severity::INFO;
    if (s == "WARNING")  return Severity::WARNING;
    if (s == "ERROR")    return Severity::ERROR;
    if (s == "CRITICAL") return Severity::CRITICAL;
    throw std::invalid_argument("Unknown severity: " + s);
}


std::string logTypeToString(LogType t) {
    switch (t) {
        case LogType::AUTH:        return "AUTH";
        case LogType::NETWORK:     return "NETWORK";
        case LogType::SYSTEM:      return "SYSTEM";
        case LogType::APPLICATION: return "APPLICATION";
        default:                   return "UNKNOWN";
    }
}

LogType logTypeFromString(const std::string& t) {
    if (t == "AUTH")        return LogType::AUTH;
    if (t == "NETWORK")     return LogType::NETWORK;
    if (t == "SYSTEM")      return LogType::SYSTEM;
    if (t == "APPLICATION") return LogType::APPLICATION;
    return LogType::UNKNOWN;
}


Timestamp now() {
    return std::chrono::system_clock::now();
}

std::string timestampToString(const Timestamp& ts) {
    auto t  = std::chrono::system_clock::to_time_t(ts);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                  ts.time_since_epoch()) % 1000;
    std::ostringstream oss;
    oss << std::put_time(std::gmtime(&t), "%Y-%m-%dT%H:%M:%S")
        << '.' << std::setfill('0') << std::setw(3) << ms.count() << 'Z';
    return oss.str();
}

Timestamp timestampFromString(const std::string& s) {
    std::tm tm{};
    std::istringstream iss(s);
    iss >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%S");
    if (iss.fail()) throw std::invalid_argument("Bad timestamp: " + s);
    return std::chrono::system_clock::from_time_t(std::mktime(&tm));
}

std::string LogEntry::toString() const {
    std::ostringstream oss;
    oss << "[" << timestampToString(timestamp) << "] "
        << "[" << severityToString(severity)   << "] "
        << "[" << logTypeToString(type)         << "] "
        << "ID:" << id << " "
        << "SRC:" << source << "(" << source_ip << ") "
        << "| " << payload;
    return oss.str();
}


bool LogEntry::validate(std::string& error) const {
    if (source.empty()) {
        error = "source is empty";
        return false;
    }
    // Basic IPv4 check via inet_pton
    struct in_addr addr4;
    struct in6_addr addr6;
    if (inet_pton(AF_INET, source_ip.c_str(), &addr4) != 1 &&
        inet_pton(AF_INET6, source_ip.c_str(), &addr6) != 1) {
        error = "invalid source_ip: " + source_ip;
        return false;
    }
    if (payload.empty()) {
        error = "payload is empty";
        return false;
    }
    return true;
}


// Expected format (pipe-delimited):
//   ID|TIMESTAMP|SOURCE|SOURCE_IP|SEVERITY|TYPE|PAYLOAD
bool parseLogLine(const std::string& line, LogEntry& out, std::string& error) {
    std::istringstream iss(line);
    std::string id_s, ts_s, sev_s, type_s;

    if (!std::getline(iss, id_s,       '|') ||
        !std::getline(iss, ts_s,       '|') ||
        !std::getline(iss, out.source, '|') ||
        !std::getline(iss, out.source_ip, '|') ||
        !std::getline(iss, sev_s,      '|') ||
        !std::getline(iss, type_s,     '|') ||
        !std::getline(iss, out.payload)) {
        error = "malformed line: " + line;
        return false;
    }
    try {
        out.id        = std::stoull(id_s);
        out.timestamp = timestampFromString(ts_s);
        out.severity  = severityFromString(sev_s);
        out.type      = logTypeFromString(type_s);
    } catch (const std::exception& e) {
        error = e.what();
        return false;
    }
    return out.validate(error);
}

LogEntry generateRandomLog(uint64_t id) {
    static std::mt19937 rng(42);
    static const char* sources[]  = {"webserver", "firewall", "auth-daemon", "kernel"};
    static const char* ips[]      = {"192.168.1.1", "10.0.0.5", "172.16.0.3", "127.0.0.1"};
    static const char* payloads[] = {
        "Connection accepted",
        "Failed login attempt",
        "Packet dropped: port scan detected",
        "User root logged in",
        "OOM killer invoked"
    };

    std::uniform_int_distribution<int> src_d(0, 3), ip_d(0, 3),
                                       sev_d(0, 4), type_d(0, 3), pay_d(0, 4);
    LogEntry e;
    e.id        = id;
    e.timestamp = now();
    e.source    = sources[src_d(rng)];
    e.source_ip = ips[ip_d(rng)];
    e.severity  = static_cast<Severity>(sev_d(rng));
    e.type      = static_cast<LogType>(type_d(rng));
    e.payload   = payloads[pay_d(rng)];
    return e;
}
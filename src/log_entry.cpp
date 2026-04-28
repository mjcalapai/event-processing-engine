#include "log_entry.h"
#include <sstream>
#include <stdexcept>
#include <random>


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
    // CHANGED: was std::mktime(&tm) which interprets tm as local time.
    // timestampToString formats in UTC via gmtime, so we must parse as UTC
    // too. timegm() is the POSIX inverse of gmtime_r.
    return std::chrono::system_clock::from_time_t(timegm(&tm));
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

// NEW: pipe-delimited serialisation — mirrors parseLogLine() field order:
//   ID|TIMESTAMP|SOURCE|SOURCE_IP|SEVERITY|TYPE|PAYLOAD
std::string LogEntry::toLogLine() const {
    std::ostringstream oss;
    oss << id                       << '|'
        << timestampToString(timestamp) << '|'
        << source                   << '|'
        << source_ip                << '|'
        << severityToString(severity) << '|'
        << logTypeToString(type)    << '|'
        << payload;
    return oss.str();
}


bool LogEntry::validate(std::string& error) const {
    if (source.empty()) {
        error = "source is empty";
        return false;
    }
    // Basic IPv4/IPv6 check via inet_pton
    struct in_addr  addr4;
    struct in6_addr addr6;
    if (inet_pton(AF_INET,  source_ip.c_str(), &addr4) != 1 &&
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

    if (!std::getline(iss, id_s,          '|') ||
        !std::getline(iss, ts_s,          '|') ||
        !std::getline(iss, out.source,    '|') ||
        !std::getline(iss, out.source_ip, '|') ||
        !std::getline(iss, sev_s,         '|') ||
        !std::getline(iss, type_s,        '|') ||
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


// ---------------------------------------------------------------------------
// Internal helper — type-coherent random entry
// ---------------------------------------------------------------------------
namespace {

// CHANGED: each LogType now has its own pool of realistic payloads, so a
// generated entry never mixes e.g. AUTH type with a SYSTEM payload.
struct ScenarioPool {
    const char* source;
    Severity    severity;
    LogType     type;
    const char* payload;
};

static const ScenarioPool kNormalPool[] = {
    { "webserver",   Severity::INFO,    LogType::APPLICATION, "GET /index.html 200"              },
    { "webserver",   Severity::INFO,    LogType::APPLICATION, "POST /api/login 200"              },
    { "auth-daemon", Severity::INFO,    LogType::AUTH,        "User alice logged in"             },
    { "auth-daemon", Severity::DEBUG,   LogType::AUTH,        "Session token refreshed"          },
    { "firewall",    Severity::INFO,    LogType::NETWORK,     "Connection accepted on port 443"  },
    { "firewall",    Severity::DEBUG,   LogType::NETWORK,     "Outbound connection established"  },
    { "kernel",      Severity::INFO,    LogType::SYSTEM,      "Disk flush completed"             },
    { "kernel",      Severity::DEBUG,   LogType::SYSTEM,      "CPU frequency scaled"             },
};

static const char* kInternalIPs[] = {
    "192.168.1.10", "192.168.1.20", "10.0.0.5", "10.0.0.8", "172.16.0.3"
};

} // namespace


// generateRandomLog — unchanged semantics, now type-coherent (CHANGED)
// Seed is fixed at 42 for reproducibility in unit tests.
LogEntry generateRandomLog(uint64_t id) {
    static std::mt19937 rng(42); // reproducible; change seed for variety
    std::uniform_int_distribution<int> pool_d(0, 7);
    std::uniform_int_distribution<int> ip_d(0, 4);

    const auto& p = kNormalPool[pool_d(rng)];
    LogEntry e;
    e.id        = id;
    e.timestamp = now();
    e.source    = p.source;
    e.source_ip = kInternalIPs[ip_d(rng)];
    e.severity  = p.severity;
    e.type      = p.type;
    e.payload   = p.payload;
    return e;
}


// NEW: generateNormalTraffic — alias for type-coherent random log
LogEntry generateNormalTraffic(uint64_t id) {
    return generateRandomLog(id);
}


// NEW: generateFailedLoginBurst
// Produces a single CRITICAL AUTH event from attacker_ip.
// Call repeatedly with the same IP to build a burst workload.
LogEntry generateFailedLoginBurst(uint64_t id, const std::string& attacker_ip) {
    LogEntry e;
    e.id        = id;
    e.timestamp = now();
    e.source    = "auth-daemon";
    e.source_ip = attacker_ip;
    e.severity  = Severity::CRITICAL;
    e.type      = LogType::AUTH;
    e.payload   = "Failed login attempt: invalid credentials";
    return e;
}


// NEW: generatePortScanEvent
// Produces a single WARNING NETWORK event probing `port` from attacker_ip.
// Call in a loop over a port range to simulate a scan sequence.
LogEntry generatePortScanEvent(uint64_t id,
                               const std::string& attacker_ip,
                               uint16_t port) {
    LogEntry e;
    e.id        = id;
    e.timestamp = now();
    e.source    = "firewall";
    e.source_ip = attacker_ip;
    e.severity  = Severity::WARNING;
    e.type      = LogType::NETWORK;

    std::ostringstream pay;
    pay << "Packet dropped: connection attempt on port " << port;
    e.payload   = pay.str();
    return e;
}


// NEW: generateMixedNoise
// Low-severity DEBUG/INFO chatter from varied internal sources.
// Used to stress-test anti-starvation in the priority buffer (Phase 2).
LogEntry generateMixedNoise(uint64_t id) {
    static std::mt19937 rng(7); // separate seed from generateRandomLog
    static const ScenarioPool kNoisePool[] = {
        { "webserver",   Severity::DEBUG, LogType::APPLICATION, "Health check OK"            },
        { "kernel",      Severity::DEBUG, LogType::SYSTEM,      "Timer interrupt handled"    },
        { "webserver",   Severity::INFO,  LogType::APPLICATION, "Static asset served"        },
        { "auth-daemon", Severity::DEBUG, LogType::AUTH,        "Token validation succeeded" },
        { "firewall",    Severity::INFO,  LogType::NETWORK,     "Keep-alive ACK received"    },
    };
    std::uniform_int_distribution<int> pool_d(0, 4);
    std::uniform_int_distribution<int> ip_d(0, 4);

    const auto& p = kNoisePool[pool_d(rng)];
    LogEntry e;
    e.id        = id;
    e.timestamp = now();
    e.source    = p.source;
    e.source_ip = kInternalIPs[ip_d(rng)];
    e.severity  = p.severity;
    e.type      = p.type;
    e.payload   = p.payload;
    return e;
}
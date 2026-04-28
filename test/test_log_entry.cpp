// test_log_entry.cpp
// Compile: g++ -std=c++17 -Wall -o test_log_entry test_log_entry.cpp log_entry.cpp
//
// Each TEST() block prints PASS or FAIL and a description.
// A final summary line reports total pass/fail counts.

#include "log_entry.h"
#include <cassert>
#include <iostream>
#include <string>
#include <unordered_set>
#include <vector>
#include <cmath>

// ---------------------------------------------------------------------------
// Minimal test harness
// ---------------------------------------------------------------------------
static int s_pass = 0, s_fail = 0;

#define TEST(name, expr)                                          \
    do {                                                          \
        if (expr) {                                               \
            std::cout << "PASS  " << (name) << "\n";             \
            ++s_pass;                                             \
        } else {                                                  \
            std::cout << "FAIL  " << (name) << "\n";             \
            ++s_fail;                                             \
        }                                                         \
    } while (false)

// ---------------------------------------------------------------------------
// 1. Timestamp round-trip (was broken — mktime vs timegm)
// ---------------------------------------------------------------------------
void test_timestamp_roundtrip() {
    // Truncate to seconds to match parse precision
    auto original = std::chrono::system_clock::from_time_t(
        std::chrono::system_clock::to_time_t(now()));

    std::string s  = timestampToString(original);
    Timestamp   rt = timestampFromString(s);

    // Allow ±1 second tolerance (milliseconds are stripped during parse)
    auto diff = std::chrono::duration_cast<std::chrono::seconds>(rt - original).count();
    TEST("timestamp round-trip within 1 second", std::abs(diff) <= 1);

    // Verify UTC formatting — string must end with 'Z'
    TEST("timestamp string ends with Z", !s.empty() && s.back() == 'Z');

    // Verify known fixed time: 2024-01-01T00:00:00.000Z → epoch offset
    Timestamp known = timestampFromString("2024-01-01T00:00:00.000Z");
    std::string known_s = timestampToString(known);
    TEST("timestamp known value parses correctly",
         known_s.substr(0, 10) == "2024-01-01");
}

// ---------------------------------------------------------------------------
// 2. toLogLine() / parseLogLine() round-trip
// ---------------------------------------------------------------------------
void test_logline_roundtrip() {
    LogEntry orig;
    orig.id        = 42;
    orig.timestamp = std::chrono::system_clock::from_time_t(
        std::chrono::system_clock::to_time_t(now()));
    orig.source    = "webserver";
    orig.source_ip = "192.168.1.1";
    orig.severity  = Severity::ERROR;
    orig.type      = LogType::NETWORK;
    orig.payload   = "Connection refused";

    std::string line = orig.toLogLine();

    // Must contain exactly 6 pipe separators (7 fields)
    int pipes = 0;
    for (char c : line) if (c == '|') ++pipes;
    TEST("toLogLine produces 6 pipe separators", pipes == 6);

    LogEntry parsed;
    std::string err;
    bool ok = parseLogLine(line, parsed, err);
    TEST("parseLogLine accepts toLogLine output", ok);
    TEST("round-trip id",        parsed.id        == orig.id);
    TEST("round-trip source",    parsed.source    == orig.source);
    TEST("round-trip source_ip", parsed.source_ip == orig.source_ip);
    TEST("round-trip severity",  parsed.severity  == orig.severity);
    TEST("round-trip type",      parsed.type      == orig.type);
    TEST("round-trip payload",   parsed.payload   == orig.payload);
}

// ---------------------------------------------------------------------------
// 3. validate() — good and bad entries
// ---------------------------------------------------------------------------
void test_validate() {
    LogEntry e;
    e.id        = 1;
    e.timestamp = now();
    e.source    = "firewall";
    e.source_ip = "10.0.0.1";
    e.severity  = Severity::INFO;
    e.type      = LogType::NETWORK;
    e.payload   = "ok";

    std::string err;
    TEST("valid entry passes validate", e.validate(err));

    LogEntry bad = e;
    bad.source = "";
    TEST("empty source fails validate", !bad.validate(err));

    bad = e;
    bad.source_ip = "999.999.999.999";
    TEST("bad IPv4 fails validate", !bad.validate(err));

    bad = e;
    bad.source_ip = "not-an-ip";
    TEST("non-IP string fails validate", !bad.validate(err));

    bad = e;
    bad.payload = "";
    TEST("empty payload fails validate", !bad.validate(err));

    // IPv6 should pass
    bad = e;
    bad.source_ip = "::1";
    TEST("IPv6 loopback passes validate", bad.validate(err));
}

// ---------------------------------------------------------------------------
// 4. parseLogLine() — malformed inputs
// ---------------------------------------------------------------------------
void test_parse_bad_inputs() {
    LogEntry out;
    std::string err;

    TEST("empty string fails parse",
         !parseLogLine("", out, err));

    TEST("too few fields fails parse",
         !parseLogLine("1|2024-01-01T00:00:00.000Z|src", out, err));

    TEST("bad severity fails parse",
         !parseLogLine("1|2024-01-01T00:00:00.000Z|src|127.0.0.1|MEGA|AUTH|msg",
                       out, err));

    TEST("bad source_ip fails parse",
         !parseLogLine("1|2024-01-01T00:00:00.000Z|src|bad-ip|INFO|AUTH|msg",
                       out, err));
}

// ---------------------------------------------------------------------------
// 5. generateFailedLoginBurst — same IP, correct fields
// ---------------------------------------------------------------------------
void test_failed_login_burst() {
    const std::string attacker = "203.0.113.5";
    std::vector<LogEntry> burst;
    for (int i = 0; i < 10; ++i)
        burst.push_back(generateFailedLoginBurst(nextLogID(), attacker));

    bool all_ip   = true;
    bool all_auth = true;
    bool all_crit = true;
    for (const auto& e : burst) {
        if (e.source_ip != attacker)        all_ip   = false;
        if (e.type      != LogType::AUTH)   all_auth = false;
        if (e.severity  != Severity::CRITICAL) all_crit = false;
    }
    TEST("burst: all events share attacker IP",       all_ip);
    TEST("burst: all events are LogType::AUTH",       all_auth);
    TEST("burst: all events are Severity::CRITICAL",  all_crit);

    // IDs must be unique (monotonic counter)
    std::unordered_set<uint64_t> ids;
    for (const auto& e : burst) ids.insert(e.id);
    TEST("burst: all IDs unique", ids.size() == burst.size());
}

// ---------------------------------------------------------------------------
// 6. generatePortScanEvent — same IP, sequential ports in payload
// ---------------------------------------------------------------------------
void test_port_scan() {
    const std::string attacker = "198.51.100.7";
    uint16_t start_port = 80;

    LogEntry e = generatePortScanEvent(nextLogID(), attacker, start_port);
    std::string err;

    TEST("port scan: correct source_ip",         e.source_ip == attacker);
    TEST("port scan: LogType::NETWORK",          e.type      == LogType::NETWORK);
    TEST("port scan: Severity::WARNING",         e.severity  == Severity::WARNING);
    TEST("port scan: payload contains port",
         e.payload.find("80") != std::string::npos);
    TEST("port scan: validates cleanly",         e.validate(err));

    // Generate a small scan sequence — payload ports should differ
    auto e2 = generatePortScanEvent(nextLogID(), attacker, 443);
    TEST("port scan: different ports produce different payloads",
         e.payload != e2.payload);
}

// ---------------------------------------------------------------------------
// 7. generateMixedNoise — low severity, validates, varied payloads
// ---------------------------------------------------------------------------
void test_mixed_noise() {
    const int N = 20;
    bool all_low    = true;
    bool all_valid  = true;
    std::unordered_set<std::string> payloads;

    for (int i = 0; i < N; ++i) {
        LogEntry e = generateMixedNoise(nextLogID());
        std::string err;
        if (!e.validate(err))                      all_valid = false;
        if (e.severity > Severity::INFO)           all_low   = false;
        payloads.insert(e.payload);
    }
    TEST("noise: all entries validate",                       all_valid);
    TEST("noise: all entries are DEBUG or INFO severity",     all_low);
    TEST("noise: payload variety across 20 entries",          payloads.size() >= 2);
}

// ---------------------------------------------------------------------------
// 8. generateRandomLog — type-coherent (AUTH source doesn't get SYSTEM payload)
// ---------------------------------------------------------------------------
void test_random_log_coherence() {
    // Run 50 entries and check that AUTH type only has AUTH sources, etc.
    bool coherent = true;
    for (int i = 0; i < 50; ++i) {
        LogEntry e = generateRandomLog(nextLogID());
        if (e.type == LogType::AUTH &&
            e.source != "auth-daemon") {
            coherent = false;
        }
        if (e.type == LogType::SYSTEM &&
            e.source != "kernel") {
            coherent = false;
        }
    }
    TEST("generateRandomLog: type/source coherent over 50 entries", coherent);
}

// ---------------------------------------------------------------------------
// 9. Severity / LogType enum conversions
// ---------------------------------------------------------------------------
void test_enum_conversions() {
    TEST("severityToString DEBUG",    severityToString(Severity::DEBUG)    == "DEBUG");
    TEST("severityToString CRITICAL", severityToString(Severity::CRITICAL) == "CRITICAL");
    TEST("severityFromString WARNING", severityFromString("WARNING") == Severity::WARNING);

    bool threw = false;
    try { severityFromString("GARBAGE"); } catch (...) { threw = true; }
    TEST("severityFromString throws on unknown input", threw);

    TEST("logTypeToString AUTH",      logTypeToString(LogType::AUTH)    == "AUTH");
    TEST("logTypeFromString NETWORK", logTypeFromString("NETWORK") == LogType::NETWORK);
    TEST("logTypeFromString unknown returns UNKNOWN",
         logTypeFromString("NOTYPE") == LogType::UNKNOWN);
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------
int main() {
    std::cout << "=== log_entry tests ===\n\n";

    test_timestamp_roundtrip();
    std::cout << "\n";
    test_logline_roundtrip();
    std::cout << "\n";
    test_validate();
    std::cout << "\n";
    test_parse_bad_inputs();
    std::cout << "\n";
    test_failed_login_burst();
    std::cout << "\n";
    test_port_scan();
    std::cout << "\n";
    test_mixed_noise();
    std::cout << "\n";
    test_random_log_coherence();
    std::cout << "\n";
    test_enum_conversions();

    std::cout << "\n=== Results: "
              << s_pass << " passed, "
              << s_fail << " failed ===\n";
    return s_fail == 0 ? 0 : 1;
}
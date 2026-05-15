//./event_engine_test
#include <gtest/gtest.h>

#include "boundedBuffer.h"
#include "log_entry.h"
#include "mlfq_scheduler.h"
#include "correlation_engine.h"
#include "metrics.h"

static LogEntry* makeLog(uint64_t id, Severity severity) {
    LogEntry* log = new LogEntry();

    log->id = id;
    log->timestamp = now();
    log->source = "test-source";
    log->source_ip = "192.168.1.10";
    log->severity = severity;
    log->type = LogType::APPLICATION;
    log->payload = "test payload";

    return log;
}

TEST(BoundedBufferTest, StartsEmpty) {
    BoundedBuffer<int> bb(5);
    EXPECT_TRUE(bb.isEmpty());
}

TEST(BoundedBufferTest, AppendRemove) {
    BoundedBuffer<int> bb(5);
    bb.append(42);
    EXPECT_EQ(bb.remove(), 42);
}

TEST(LogEntryTest, SeverityConversion) {
    EXPECT_EQ(severityToString(Severity::ERROR), "ERROR");
    EXPECT_EQ(severityFromString("CRITICAL"), Severity::CRITICAL);
}

TEST(MLFQSchedulerTest, RRProcessesAllSeverities) {
    MLFQScheduler scheduler(10, MLFQPolicy::RR);

    scheduler.append(makeLog(1, Severity::DEBUG));
    scheduler.append(makeLog(2, Severity::INFO));
    scheduler.append(makeLog(3, Severity::WARNING));
    scheduler.append(makeLog(4, Severity::ERROR));
    scheduler.append(makeLog(5, Severity::CRITICAL));

    int count = 0;

    for (int i = 0; i < 5; i++) {
        LogEntry* item = scheduler.remove();

        ASSERT_NE(item, nullptr);
        count++;

        delete item;
    }

    EXPECT_EQ(count, 5);
    EXPECT_TRUE(scheduler.isEmpty());
}

TEST(MLFQSchedulerTest, WeightedPrioritizesCritical) {
    MLFQScheduler scheduler(10, MLFQPolicy::WEIGHTED);

    scheduler.append(makeLog(1, Severity::DEBUG));
    scheduler.append(makeLog(2, Severity::CRITICAL));

    LogEntry* first = scheduler.remove();

    ASSERT_NE(first, nullptr);
    EXPECT_EQ(first->severity, Severity::CRITICAL);

    delete first;

    LogEntry* second = scheduler.remove();

    ASSERT_NE(second, nullptr);
    EXPECT_EQ(second->severity, Severity::DEBUG);

    delete second;
}

TEST(MLFQSchedulerTest, PoisonPillReturnsNullptr) {
    MLFQScheduler scheduler(5, MLFQPolicy::WEIGHTED);

    scheduler.append(nullptr);

    LogEntry* item = scheduler.remove();

    EXPECT_EQ(item, nullptr);
}

TEST(CorrelationEngineTest, NoAlertOnNormalAuth) {
    int initialAlerts = metrics.getAlerts();
    CorrelationEngine engine;
    
    LogEntry log;
    log.type = LogType::AUTH;
    log.source_ip = "10.0.0.1";
    log.payload = "successful login";
    log.timestamp = now();
    
    for (int i = 0; i < 5; i++) {
        engine.process(&log);
    }
    
    EXPECT_EQ(metrics.getAlerts(), initialAlerts);
}

TEST(CorrelationEngineTest, AlertOnFailedLoginBurst) {
    int initialAlerts = metrics.getAlerts();
    CorrelationEngine engine;
    
    LogEntry log;
    log.type = LogType::AUTH;
    log.source_ip = "10.0.0.2";
    log.payload = "Failed Login for user admin";

    log.timestamp = now();
    engine.process(&log);
    engine.process(&log);
    EXPECT_EQ(metrics.getAlerts(), initialAlerts);
    
    engine.process(&log);
    EXPECT_EQ(metrics.getAlerts(), initialAlerts + 1);
}

TEST(CorrelationEngineTest, NoAlertIfFailedLoginsAreSpreadOut) {
    int initialAlerts = metrics.getAlerts();
    CorrelationEngine engine;
    
    LogEntry log;
    log.type = LogType::AUTH;
    log.source_ip = "10.0.0.3";
    log.payload = "failed login";
    
    log.timestamp = now() - std::chrono::seconds(70);
    engine.process(&log);
    
    log.timestamp = now() - std::chrono::seconds(35);
    engine.process(&log);
    
    log.timestamp = now();
    engine.process(&log);
    
    EXPECT_EQ(metrics.getAlerts(), initialAlerts);
}

TEST(CorrelationEngineTest, AlertOnPortScan) {
    int initialAlerts = metrics.getAlerts();
    CorrelationEngine engine;
    
    LogEntry log;
    log.type = LogType::NETWORK;
    log.source_ip = "10.0.0.4";
    log.payload = "port 80 connection attempt";
    
    for (int i = 0; i < 4; i++) {
        log.timestamp = now();
        engine.process(&log);
    }
    EXPECT_EQ(metrics.getAlerts(), initialAlerts);
    
    log.timestamp = now();
    engine.process(&log);
    EXPECT_EQ(metrics.getAlerts(), initialAlerts + 1);
}

TEST(CorrelationEngineTest, AlertOnPortScanPacketDropped) {
    int initialAlerts = metrics.getAlerts();
    CorrelationEngine engine;
    
    LogEntry log;
    log.type = LogType::NETWORK;
    log.source_ip = "10.0.0.44";
    log.payload = "port 80 packet dropped";
    
    for (int i = 0; i < 4; i++) {
        log.timestamp = now();
        engine.process(&log);
    }
    EXPECT_EQ(metrics.getAlerts(), initialAlerts);
    
    log.timestamp = now();
    engine.process(&log);
    EXPECT_EQ(metrics.getAlerts(), initialAlerts + 1);
}

TEST(CorrelationEngineTest, NoAlertOnIrrelevantNetworkEvents) {
    int initialAlerts = metrics.getAlerts();
    CorrelationEngine engine;
    
    LogEntry log;
    log.type = LogType::NETWORK;
    log.source_ip = "10.0.0.5";
    log.payload = "normal traffic";
    
    for (int i = 0; i < 10; i++) {
        log.timestamp = now();
        engine.process(&log);
    }
    EXPECT_EQ(metrics.getAlerts(), initialAlerts);
}

TEST(CorrelationEngineTest, CaseInsensitiveMatch) {
    int initialAlerts = metrics.getAlerts();
    CorrelationEngine engine;
    
    LogEntry log;
    log.type = LogType::AUTH;
    log.source_ip = "10.0.0.6";

    log.payload = "FaIlEd LoGiN attempt";
    log.timestamp = now();
    
    engine.process(&log);
    engine.process(&log);
    engine.process(&log);
    
    EXPECT_EQ(metrics.getAlerts(), initialAlerts + 1);
}

int main(int argc, char** argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
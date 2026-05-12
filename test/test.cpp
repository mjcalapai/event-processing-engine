#include <gtest/gtest.h>

#include "boundedBuffer.h"
#include "log_entry.h"

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
}

int main(int argc, char** argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
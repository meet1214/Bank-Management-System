#include <gtest/gtest.h>
#include "BackgroundWorker.h"
#include "DatabaseManager.h"

TEST(BackgroundWorkerTest, StartsAndStopsCleanly) {
    BackgroundWorker worker;
    worker.start();
    worker.stop();
    SUCCEED();
}

TEST(BackgroundWorkerTest, StopsWithinReasonableTime) {
    DatabaseManager::open(":memory:");
    BackgroundWorker worker;
    worker.start();

    auto start = std::chrono::steady_clock::now();
    worker.stop();
    auto end = std::chrono::steady_clock::now();

    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>
                (end - start).count();

    EXPECT_LT(ms, 1000);
    DatabaseManager::close();
}

TEST(BackgroundWorkerTest, CleanExpiredSessions_RunsWithoutCrash) {
    DatabaseManager::open(":memory:");
    EXPECT_NO_THROW(DatabaseManager::cleanExpiredSessions());
    DatabaseManager::close();
}
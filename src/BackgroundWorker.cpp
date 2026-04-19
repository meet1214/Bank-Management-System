#include "BackgroundWorker.h"
#include "DatabaseManager.h"
#include "Logger.h"

#include <chrono>

using namespace std;

BackgroundWorker::BackgroundWorker() : stopFlag(false) {}

BackgroundWorker::~BackgroundWorker() {
    stop();
}

void BackgroundWorker::start() {
    if (worker.joinable()) return;
    stopFlag = false;
    worker = std::thread(&BackgroundWorker::run, this);
}

void BackgroundWorker::stop() {
    stopFlag = true;
    cv.notify_one();
    if (worker.joinable()) {
        worker.join();
    }
}

void BackgroundWorker::run() {
    Logger::getInstance().info("Background worker started.");

    while (!stopFlag) {

        // Wait 30 seconds OR until stop is signaled
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait_for(lock, 
                    std::chrono::seconds(30),
                    [this] { return stopFlag.load(); });

        if (stopFlag) break;

        // Do background work
        Logger::getInstance().info("[BG] Running cleanup...");
        DatabaseManager::cleanExpiredSessions();

    }

    Logger::getInstance().info("Background worker stopped.");
}
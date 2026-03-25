#ifndef BACKGROUNDWORKER_H
#define BACKGROUNDWORKER_H

#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>

class BackgroundWorker {
private:
    std::thread worker;
    std::atomic<bool> stopFlag;
    std::mutex mtx;
    std::condition_variable cv;

    void run();

public:
    BackgroundWorker();
    ~BackgroundWorker();

    void start();
    void stop();
};

#endif
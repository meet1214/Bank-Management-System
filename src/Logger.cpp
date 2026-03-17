#include "Logger.h"
#include <iostream>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <filesystem>

using namespace std;

// Constructor
Logger::Logger() {
    filesystem::create_directories("data");
    logFile.open("data/bank.log", ios::app);
    if (!logFile)
    cerr << "Warning: Could not open log file.\n";
}

// Destructor
Logger::~Logger() {
    logFile.close();
}

// log()
void Logger::log(const string& level, const string& message) {
    auto now   = chrono::system_clock::now();
    time_t t   = chrono::system_clock::to_time_t(now);
    tm* lt     = localtime(&t);
    
    ostringstream timestamp;
    timestamp << (lt->tm_year + 1900);
    timestamp << "-";
    timestamp << setw(2) << setfill('0') << (lt->tm_mon + 1);
    timestamp << "-";
    timestamp << setw(2) << setfill('0') << (lt->tm_mday) << " ";
    timestamp << setw(2) << setfill('0') << (lt->tm_hour) << ":";
    timestamp << setw(2) << setfill('0') << (lt->tm_min) << ":";
    timestamp << setw(2) << setfill('0') << (lt->tm_sec);

    string line = "[" + timestamp.str()+ "] [" + level + "] " + message;
    
    cout    << line << "\n";
    logFile << line << "\n";
    logFile.flush();
}

// info()
void Logger::info(const string& message) {
    log("INFO", message);
}

// warn()
void Logger::warn(const string& message) {
    log("WARN", message);
}

// error()
void Logger::error(const string& message) {
    log("ERROR", message);
}

// admin()
void Logger::admin(const string& message) {
    log("ADMIN", message);
}
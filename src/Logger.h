#ifndef LOGGER_H
#define LOGGER_H

#include <fstream>
#include <string>

class Logger{
    private:
        Logger();
        static Logger* instance;
        std::ofstream logFile;
    
    public:
        static Logger& getInstance() {
            static Logger instance;
            return instance;
        }
        
        void log(const std::string& level, const std::string& message);
        void info(const std::string& message);
        void warn(const std::string& message);
        void error(const std::string& message);
        void admin(const std::string& message);
        
        ~Logger();
};

#endif
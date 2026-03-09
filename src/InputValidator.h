#ifndef INPUTVALIDATOR_H
#define INPUTVALIDATOR_H

#include <string>

class InputValidator{
    public:
        static int getInt(const std::string& prompt);
        static double getDouble(const std::string& prompt);
        static double getPositiveDouble(const std::string& prompt);
        static std::string getString(const std::string& prompt);
        static std::string getPin(const std::string& prompt);
        static char getChar(const std::string& prompt);
};

#endif
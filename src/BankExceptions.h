#ifndef BANKEXCEPTIONS_H
#define BANKEXCEPTIONS_H

#include <exception>
#include <string>

class BankException : public std::exception {
protected:
    std::string message;
public:
    explicit BankException(const std::string& msg) : message(msg) {}
    const char* what() const noexcept override {
        return message.c_str();
    }
};

class InsufficientFundsException : public BankException {
public:
    explicit InsufficientFundsException(const std::string& msg) : BankException(msg) {}
};

class AccountLockedException : public BankException {
public:
    explicit AccountLockedException(const std::string& msg) : BankException(msg) {}
};

class DailyLimitExceededException : public BankException {
public:
    explicit DailyLimitExceededException(const std::string& msg) : BankException(msg) {}
};

class LoanException : public BankException {
public:
    explicit LoanException(const std::string& msg) : BankException(msg) {}
};

#endif
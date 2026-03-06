#ifndef BANKACCOUNT_H
#define BANKACCOUNT_H

#include <cstddef>
#include <string>
#include <vector>

struct Transaction {
    int transactionId;
    std::string dateTime;
    std::string type;
    double amount;
    double balance;
};

class BankAccount {
private:
    std::string accountNumber;
    std::string name;
    size_t pinHash;
    double balance;
    std::vector<Transaction> transactionHistory;
    int lastTransactionId;
    std::string role;   // "user" or "admin"
    bool isLocked;

public:
    BankAccount(const std::string& accNo,
                const std::string& n,
                size_t hash,
                double b,
                const std::string& r,
                bool locked);

    // Basic Getters
    std::string getName() const;
    std::string getAccountNumber() const;
    double getBalance() const;
    size_t getPinHash() const;

    // Authentication
    bool authenticatePin(int enteredPin) const;

    // PIN Setter (if needed)
    void setPinHash(size_t hashValue);

    // Role Handling
    std::string getRole() const { return role; }
    void setRole(const std::string& r) { role = r; }

    // Lock Handling
    bool getLockStatus() const { return isLocked; }
    void setLockStatus(bool status) { isLocked = status; }

    // Account Operations
    void depositMoney(double amount);
    bool withdrawMoney(double amount);
    bool transferMoney(BankAccount &receiver, double amount);

    void showBalance() const;

    // Transaction Handling
    void showTransactionHistory() const;
    void loadTransactionsFromFile();
    void saveTransactionToFile(const Transaction &t);
    std::string getCurrentDateTime() const;
};

#endif
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
    std::string pinHash;   // SHA-256 hex digest (64 chars)
    std::string salt;      // random per-account salt
    double balance;
    std::vector<Transaction> transactionHistory;
    int lastTransactionId;
    std::string role;      // "user" or "admin"
    bool isLocked;

    // ---- Per-account limits (set by admin) ----
    double depositLimit;       // max Rs. per deposit transaction
    double withdrawLimit;      // max Rs. per withdrawal transaction
    int    dailyTxnLimit;      // max transactions per day
    double dailyTransferLimit; // max Rs. transferred out per day

    // ---- Daily tracking (resets each new day) ----
    int    dailyTxnCount;      // transactions done today
    double dailyTransferUsed;  // transfer amount used today
    std::string lastTxnDate;   // date of last transaction (YYYY-MM-DD)

    // Daily reset helper
    void resetDailyCountersIfNeeded();

public:
    BankAccount(const std::string& accNo,
                const std::string& n,
                const std::string& pinHash,
                const std::string& salt,
                double b,
                const std::string& r,
                bool locked,
                double depositLimit    = 100000.0,
                double withdrawLimit   = 50000.0,
                int    dailyTxnLimit   = 10,
                double dailyTransferLimit = 200000.0);

    // Getters
    std::string getName() const;
    std::string getAccountNumber() const;
    double getBalance() const;
    std::string getPinHash() const;
    std::string getSalt() const;

    // Authentication
    bool authenticatePin(int enteredPin) const;

    // PIN Setter
    void setPinHash(const std::string& hash, const std::string& newSalt);

    //Change Pin generation
    bool changePin(int currentPin, int newPin);

    // Role Handling
    std::string getRole() const { return role; }
    void setRole(const std::string& r) { role = r; }

    // Limit Getters
    double getDepositLimit()       const { return depositLimit; }
    double getWithdrawLimit()      const { return withdrawLimit; }
    int    getDailyTxnLimit()      const { return dailyTxnLimit; }
    double getDailyTransferLimit() const { return dailyTransferLimit; }

    // Limit Setters (called by admin)
    void setDepositLimit(double v)       { depositLimit = v; }
    void setWithdrawLimit(double v)      { withdrawLimit = v; }
    void setDailyTxnLimit(int v)         { dailyTxnLimit = v; }
    void setDailyTransferLimit(double v) { dailyTransferLimit = v; }

    // Daily tracking (for display)
    int    getDailyTxnCount()      const { return dailyTxnCount; }
    double getDailyTransferUsed()  const { return dailyTransferUsed; }

    // Lock Handling
    bool getLockStatus() const { return isLocked; }
    void setLockStatus(bool status) { isLocked = status; }

    // Account Operations
    void depositMoney(double amount);
    bool withdrawMoney(double amount);
    bool transferMoney(BankAccount& receiver, double amount);
    void showBalance() const;
    void showLimits() const;
    void showMiniStatement() const;
    void addInterest(double amount);
    void showAccountSummary() const;

    // Transaction Handling
    void showTransactionHistory() const;
    void loadTransactionsFromFile();
    void saveTransactionToFile(const Transaction& t);
    std::string getCurrentDateTime() const;
};

#endif
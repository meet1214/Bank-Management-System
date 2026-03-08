#include "BankAccount.h"
#include "Sha256.h"
#include "Logger.h"

#include <chrono>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

using namespace std;

// ================= CONSTRUCTOR =================
BankAccount::BankAccount(const std::string& accNo,
                         const std::string& n,
                         const std::string& pinHash,
                         const std::string& salt,
                         double b,
                         const std::string& r,
                         bool locked,
                         double depositLim,
                         double withdrawLim,
                         int    dailyTxnLim,
                         double dailyTransferLim)
    : accountNumber(accNo),
      name(n),
      pinHash(pinHash),
      salt(salt),
      balance(b),
      transactionHistory(),
      lastTransactionId(0),
      role(r),
      isLocked(locked),
      depositLimit(depositLim),
      withdrawLimit(withdrawLim),
      dailyTxnLimit(dailyTxnLim),
      dailyTransferLimit(dailyTransferLim),
      dailyTxnCount(0),
      dailyTransferUsed(0.0),
      lastTxnDate("")
{
}

// ================= GETTERS =================
string BankAccount::getName()           const { return name; }
string BankAccount::getAccountNumber()  const { return accountNumber; }
double BankAccount::getBalance()        const { return balance; }
string BankAccount::getPinHash()        const { return pinHash; }
string BankAccount::getSalt()           const { return salt; }

void BankAccount::setPinHash(const std::string& hash,
                              const std::string& newSalt) {
    pinHash = hash;
    salt    = newSalt;
}

// ================= AUTHENTICATION =================
bool BankAccount::authenticatePin(int enteredPin) const {
    return verifyPin(enteredPin, pinHash, salt);
}

// ================= HELPER: GET CURRENT TIME =================
string BankAccount::getCurrentDateTime() const {
    auto now     = chrono::system_clock::now();
    time_t nowTime = chrono::system_clock::to_time_t(now);
    string timeStr = ctime(&nowTime);
    timeStr.pop_back(); // remove trailing newline
    return timeStr;
}

// ================= HELPER: GET TODAY'S DATE (YYYY-MM-DD) =================
static string getTodayDate() {
    auto now = chrono::system_clock::now();
    time_t t = chrono::system_clock::to_time_t(now);
    tm* lt   = localtime(&t);
    ostringstream oss;
    oss << (1900 + lt->tm_year) << "-"
        << setw(2) << setfill('0') << (1 + lt->tm_mon) << "-"
        << setw(2) << setfill('0') << lt->tm_mday;
    return oss.str();
}

// ================= HELPER: RESET DAILY COUNTERS IF NEW DAY =================
void BankAccount::resetDailyCountersIfNeeded() {
    string today = getTodayDate();
    if (lastTxnDate != today) {
        dailyTxnCount    = 0;
        dailyTransferUsed = 0.0;
        lastTxnDate      = today;
    }
}

// ================= DEPOSIT =================
void BankAccount::depositMoney(double amount) {

    resetDailyCountersIfNeeded();

    // Limit checks
    if (amount > depositLimit) {
        Logger::getInstance()->warn("Deposit denied for " + 
            accountNumber + ": exceeds limit of Rs." + to_string(depositLimit)); 
        return;
    }
    if (dailyTxnCount >= dailyTxnLimit) {
        Logger::getInstance()->warn("Deposit denied for " + 
            accountNumber + ": daily limit of " + to_string(dailyTxnLimit) + " reached."); 
        return;
    }

    balance += amount;
    ++lastTransactionId;
    ++dailyTxnCount;

    Transaction t;
    t.transactionId = lastTransactionId;
    t.dateTime      = getCurrentDateTime();
    t.type          = "Deposit";
    t.amount        = amount;
    t.balance       = balance;

    transactionHistory.push_back(t);
    saveTransactionToFile(t);
    Logger::getInstance()->info("Deposit of Rs." + to_string(amount) + " on account " + accountNumber);
}

// ================= WITHDRAW =================
bool BankAccount::withdrawMoney(double amount) {

    resetDailyCountersIfNeeded();

    if (amount > withdrawLimit) {
        Logger::getInstance()->warn("Withdrawal denied for " + accountNumber + 
            ": exceeds limit of Rs." + to_string(withdrawLimit)); 
        return false;
    }
    if (dailyTxnCount >= dailyTxnLimit) {
        Logger::getInstance()->warn("Withdrawal denied for " + accountNumber + 
            ": daily limit of " + to_string(dailyTxnLimit) + " reached."); 
        return false;
    }
    if (amount > balance) {
        Logger::getInstance()->warn("Insufficient balance in " + accountNumber);
        return false;
    }

    balance -= amount;
    ++lastTransactionId;
    ++dailyTxnCount;

    Transaction t;
    t.transactionId = lastTransactionId;
    t.dateTime      = getCurrentDateTime();
    t.type          = "Withdraw";
    t.amount        = -amount;
    t.balance       = balance;

    transactionHistory.push_back(t);
    saveTransactionToFile(t);
    Logger::getInstance()->info("Withdrawal of Rs." + to_string(amount) + " on account " + accountNumber);
    return true;
}

// ================= SAVE TRANSACTION TO FILE =================
void BankAccount::saveTransactionToFile(const Transaction& t) {
    filesystem::create_directories("data/transactions");

    string filePath = "data/transactions/" + accountNumber + ".txt";
    ofstream file(filePath, ios::app);
    if (!file) {
        Logger::getInstance()->error("Failed to open transaction file: " + filePath);
        return;
    }

    file << t.transactionId << "|"
         << t.dateTime      << "|"
         << t.type          << "|"
         << t.amount        << "|"
         << t.balance       << "\n";
}

// ================= LOAD TRANSACTIONS =================
void BankAccount::loadTransactionsFromFile() {
    transactionHistory.clear();
    lastTransactionId = 0;

    string filePath = "data/transactions/" + accountNumber + ".txt";
    ifstream file(filePath);
    if (!file) return;

    string line;
    while (getline(file, line)) {
        if (line.empty()) continue;

        stringstream ss(line);
        string idStr, dateTime, type, amountStr, balanceStr;

        if (!getline(ss, idStr,     '|')) continue;
        if (!getline(ss, dateTime,  '|')) continue;
        if (!getline(ss, type,      '|')) continue;
        if (!getline(ss, amountStr, '|')) continue;
        if (!getline(ss, balanceStr,'|')) continue;

        Transaction t;
        try {
            t.transactionId = stoi(idStr);
            t.amount        = stod(amountStr);
            t.balance       = stod(balanceStr);
        } catch (...) {
            continue; // skip malformed lines
        }

        t.dateTime = dateTime;
        t.type     = type;
        transactionHistory.push_back(t);

        if (t.transactionId > lastTransactionId)
            lastTransactionId = t.transactionId;
    }
}

// ================= SHOW BALANCE =================
void BankAccount::showBalance() const {
    cout << "Current Balance: Rs."
         << fixed << setprecision(2) << balance << "\n";
}

// ================= TRANSFER =================
bool BankAccount::transferMoney(BankAccount& receiver, double amount) {

    resetDailyCountersIfNeeded();

    if (amount <= 0) {
        Logger::getInstance()->warn("Invalid transfer amount." + accountNumber);
        return false;
    }
    if (amount > balance) {
        Logger::getInstance()->warn("Insufficient balance in " + accountNumber);
        return false;
    }
    if (receiver.getLockStatus()) {
        Logger::getInstance()->warn("Transfer denied from " + 
            accountNumber + " to frozen account " + receiver.getAccountNumber());
        return false;
    }
    if (dailyTxnCount >= dailyTxnLimit) {
        Logger::getInstance()->warn("Transfer denied for " + accountNumber + ": daily limit of " + 
            to_string(dailyTxnLimit) + " reached."); 
        return false;
    }
    if (dailyTransferUsed + amount > dailyTransferLimit) {
        double remaining = dailyTransferLimit - dailyTransferUsed;
        Logger::getInstance()->warn("Transfer denied for " + 
            accountNumber + ": exceeds limit of Rs." + to_string(dailyTransferLimit) + 
            " would be exceeded. Remaining today: Rs." + to_string(remaining)); 
        return false;
    }

    string currentTime = getCurrentDateTime();

    // Deduct from sender
    balance -= amount;
    ++lastTransactionId;
    ++dailyTxnCount;
    dailyTransferUsed += amount;

    Transaction senderTxn;
    senderTxn.transactionId = lastTransactionId;
    senderTxn.dateTime      = currentTime;
    senderTxn.type          = "Transfer To " + receiver.getName() +
                               " (" + receiver.getAccountNumber() + ")";
    senderTxn.amount  = -amount;
    senderTxn.balance = balance;

    transactionHistory.push_back(senderTxn);
    saveTransactionToFile(senderTxn);

    // Credit receiver
    receiver.balance += amount;
    ++receiver.lastTransactionId;

    Transaction receiverTxn;
    receiverTxn.transactionId = receiver.lastTransactionId;
    receiverTxn.dateTime      = currentTime;
    receiverTxn.type          = "Received From " + name +
                                 " (" + accountNumber + ")";
    receiverTxn.amount  = amount;
    receiverTxn.balance = receiver.balance;

    receiver.transactionHistory.push_back(receiverTxn);
    receiver.saveTransactionToFile(receiverTxn);

    Logger::getInstance()->info("Transfer Successful to " + receiver.accountNumber);
    return true;
}

// ================= SHOW LIMITS =================
void BankAccount::showLimits() const {
    cout << "\n========== ACCOUNT LIMITS ==========\n";
    cout << fixed << setprecision(2);
    cout << "Deposit limit (per txn):    Rs." << depositLimit       << "\n";
    cout << "Withdrawal limit (per txn): Rs." << withdrawLimit      << "\n";
    cout << "Daily transaction limit:    "    << dailyTxnLimit      << " txns\n";
    cout << "Daily transfer limit:       Rs." << dailyTransferLimit << "\n";
    cout << "\n--- Today's Usage ---\n";
    cout << "Transactions done:    " << dailyTxnCount
         << " / " << dailyTxnLimit << "\n";
    cout << "Transfer amount used: Rs." << dailyTransferUsed
         << " / Rs." << dailyTransferLimit << "\n";
    cout << "=====================================\n";
}
void BankAccount::showTransactionHistory() const {
    cout << "\n================ TRANSACTION HISTORY ================\n\n";

    if (transactionHistory.empty()) {
        cout << "No transactions found.\n";
        return;
    }

    cout << left << setw(6)  << "S.No"
                 << setw(8)  << "TxnID"
                 << setw(25) << "Date & Time"
                 << setw(35) << "Type"
                 << setw(14) << "Amount"
                 << setw(14) << "Balance" << "\n";

    cout << string(102, '-') << "\n";

    int serial = 1;
    for (const auto& entry : transactionHistory) {
        cout << left << setw(6)  << serial++
                     << setw(8)  << entry.transactionId
                     << setw(25) << entry.dateTime
                     << setw(35) << entry.type;

        cout << fixed << setprecision(2)
             << "Rs." << setw(11) << entry.amount
             << "Rs." << setw(11) << entry.balance << "\n";
    }

    cout << string(102, '-') << "\n";
    cout << "Current Balance: Rs."
         << fixed << setprecision(2) << balance << "\n";
}
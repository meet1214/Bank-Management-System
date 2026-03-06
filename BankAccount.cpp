#include "BankAccount.h"
#include "Sha256.h"

#include <chrono>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>

using namespace std;

// ================= CONSTRUCTOR =================
BankAccount::BankAccount(const std::string& accNo,
                         const std::string& n,
                         const std::string& pinHash,
                         const std::string& salt,
                         double b,
                         const std::string& r,
                         bool locked)
    : accountNumber(accNo),
      name(n),
      pinHash(pinHash),
      salt(salt),
      balance(b),
      transactionHistory(),
      lastTransactionId(0),
      role(r),
      isLocked(locked)
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

// ================= DEPOSIT =================
void BankAccount::depositMoney(double amount) {
    balance += amount;
    ++lastTransactionId;

    Transaction t;
    t.transactionId = lastTransactionId;
    t.dateTime      = getCurrentDateTime();
    t.type          = "Deposit";
    t.amount        = amount;
    t.balance       = balance;

    transactionHistory.push_back(t);
    saveTransactionToFile(t);
}

// ================= WITHDRAW =================
bool BankAccount::withdrawMoney(double amount) {
    if (amount > balance) {
        cout << "Insufficient balance.\n";
        return false;
    }

    balance -= amount;
    ++lastTransactionId;

    Transaction t;
    t.transactionId = lastTransactionId;
    t.dateTime      = getCurrentDateTime();
    t.type          = "Withdraw";
    t.amount        = -amount;
    t.balance       = balance;

    transactionHistory.push_back(t);
    saveTransactionToFile(t);
    return true;
}

// ================= SAVE TRANSACTION TO FILE =================
void BankAccount::saveTransactionToFile(const Transaction& t) {
    filesystem::create_directories("data/transactions");

    string filePath = "data/transactions/" + accountNumber + ".txt";
    ofstream file(filePath, ios::app);
    if (!file) return;

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
    if (amount <= 0) {
        cout << "Invalid transfer amount.\n";
        return false;
    }
    if (amount > balance) {
        cout << "Insufficient balance.\n";
        return false;
    }
    if (receiver.getLockStatus()) {
        cout << "Cannot transfer to a frozen account.\n";
        return false;
    }

    string currentTime = getCurrentDateTime();

    // Deduct from sender
    balance -= amount;
    ++lastTransactionId;

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

    cout << "Transfer successful.\n";
    return true;
}

// ================= TRANSACTION HISTORY =================
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
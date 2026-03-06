#include "BankAccount.h"

#include <chrono>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <sstream>

using namespace std;

// ================= CONSTRUCTOR =================
BankAccount::BankAccount(const std::string& accNo,
                         const std::string& n,
                         size_t hash,
                         double b,
                         const std::string& r,
                         bool locked)
    : accountNumber(accNo),
      name(n),
      pinHash(hash),
      balance(b),
      role(r),
      isLocked(locked),
      lastTransactionId(0)   // initialize transaction counter
{
}

// ================= GETTERS =================
string BankAccount::getName() const { return name; }
string BankAccount::getAccountNumber() const { return accountNumber; }
double BankAccount::getBalance() const { return balance; }
size_t BankAccount::getPinHash() const { return pinHash; }
void BankAccount::setPinHash(size_t hashValue) { pinHash = hashValue; }

// ================= AUTHENTICATION =================
bool BankAccount::authenticatePin(int enteredPin) const
{
    // Use the same hashing scheme as in AccountManager::createAccount
    std::hash<std::string> hasher;
    return hasher(std::to_string(enteredPin)) == pinHash;
}

// ================= HELPER: GET CURRENT TIME =================
string BankAccount::getCurrentDateTime() const
{
    auto now = chrono::system_clock::now();
    time_t nowTime = chrono::system_clock::to_time_t(now);

    string timeStr = ctime(&nowTime);
    timeStr.pop_back(); // remove newline
    return timeStr;
}

// ================= DEPOSIT =================
void BankAccount::depositMoney(double amount)
{
    balance += amount;

    lastTransactionId++;

    Transaction t;
    t.transactionId = lastTransactionId;
    t.dateTime = getCurrentDateTime();
    t.type = "Deposit";
    t.amount = amount;
    t.balance = balance;

    transactionHistory.push_back(t);

    saveTransactionToFile(t);
}

// ================= WITHDRAW =================
bool BankAccount::withdrawMoney(double amount)
{
    if (amount > balance)
    {
        cout << "Insufficient balance.\n";
        return false;
    }

    balance -= amount;

    lastTransactionId++;

    Transaction t;
    t.transactionId = lastTransactionId;
    t.dateTime = getCurrentDateTime();
    t.type = "Withdraw";
    t.amount = -amount;
    t.balance = balance;
    transactionHistory.push_back(t);

    saveTransactionToFile(t);

    return true;
}

// ================= SAVE TRANSACTION TO USER FILE =================
void BankAccount::saveTransactionToFile(const Transaction &t)
{
    filesystem::create_directories("data/transactions");

    string filePath = "data/transactions/" + accountNumber + ".txt";

    ofstream file(filePath, ios::app);

    if (!file)
        return;

    file << t.transactionId << "|"
         << t.dateTime << "|"
         << t.type << "|"
         << t.amount << "|"
         << t.balance << endl;

    file.close();
}

// ================= LOAD TRANSACTIONS =================
void BankAccount::loadTransactionsFromFile()
{
    transactionHistory.clear();
    lastTransactionId = 0;

    string filePath = "data/transactions/" + accountNumber + ".txt";

    ifstream file(filePath);

    if (!file)
        return; // No transactions yet

    string line;

    while (getline(file, line))
    {
        if (line.empty())
            continue;

        stringstream ss(line);

        string idStr, dateTime, type, amountStr,balanceStr;

        if (!getline(ss, idStr, '|'))
            continue;
        if (!getline(ss, dateTime, '|'))
            continue;
        if (!getline(ss, type, '|'))
            continue;
        if (!getline(ss, amountStr, '|'))
            continue;
        if (!getline(ss, balanceStr, '|'))
            continue; 

        Transaction t;

        try
        {
            t.transactionId = stoi(idStr);
            t.amount        = stod(amountStr);
            t.balance       = stod(balanceStr);
        }
        catch (...)
        {
            // Skip malformed lines instead of crashing
            continue;
        }

        t.dateTime = dateTime;
        t.type     = type;

        transactionHistory.push_back(t);

        if (t.transactionId > lastTransactionId)
            lastTransactionId = t.transactionId;
    }

    file.close();
}

// ================= SHOW BALANCE =================
void BankAccount::showBalance() const
{
    cout << "Current Balance: ₹"
         << fixed << setprecision(2)
         << balance << endl;
}

//=================TRANSFER MONEY FROM ACCOUNT TO ACCOUNT===========
bool BankAccount::transferMoney(BankAccount &receiver, double amount)
{
    if (amount <= 0)
    {
        cout << "Invalid transfer amount.\n";
        return false;
    }

    if (amount > balance)
    {
        cout << "Insufficient balance.\n";
        return false;
    }
    if(receiver.getLockStatus()) {
        cout << "Cannot transfer Money to frozen account.\n";
        return false;
    }

    string currentTime = getCurrentDateTime();

    // ===== Deduct From Sender =====
    balance -= amount;
    ++lastTransactionId;

    Transaction senderTxn;
    senderTxn.transactionId = lastTransactionId;
    senderTxn.dateTime = currentTime;
    senderTxn.type = "Transfer To " + receiver.getName() +
                     " (" + receiver.getAccountNumber() + ")";
    senderTxn.amount = -amount;
    senderTxn.balance = balance;

    transactionHistory.push_back(senderTxn);
    saveTransactionToFile(senderTxn);

    // ===== Credit Receiver =====
    receiver.balance += amount;
    ++receiver.lastTransactionId;

    Transaction receiverTxn;
    receiverTxn.transactionId = receiver.lastTransactionId;
    receiverTxn.dateTime = currentTime;
    receiverTxn.type = "Transfer From " + name +
                       " (" + accountNumber + ")";
    receiverTxn.amount = amount;
    receiverTxn.balance = receiver.balance;

    receiver.transactionHistory.push_back(receiverTxn);
    receiver.saveTransactionToFile(receiverTxn);

    cout << "Transfer successful.\n";
    return true;
}

// ================= SHOW TRANSACTION HISTORY =================
void BankAccount::showTransactionHistory() const
{
    cout << "\n================ TRANSACTION HISTORY ================\n\n";

    if (transactionHistory.empty())
    {
        cout << "No transactions found.\n";
        return;
    }

    cout << left << setw(6)  << "S.No"
         << setw(8)  << "TxnID"
         << setw(25) << "Date & Time"
         << setw(12) << "Type"
         << setw(14) << "Amount"
         << setw(14) << "Balance"
         << "\n";

    cout << "-----------------------------------------------------------------------------------\n";

    int serial = 1;

    for (const auto &entry : transactionHistory)
    {
        cout << left << setw(6)  << serial++
             << setw(8)  << entry.transactionId
             << setw(25) << entry.dateTime
             << setw(12) << entry.type;

        cout << "₹"
             << fixed << setprecision(2)
             << setw(13) << entry.amount
             << "₹"
             << fixed << setprecision(2)
             << setw(13) << entry.balance
             << "\n";
    }

    cout << "-----------------------------------------------------------------------------------\n";

    cout << "Current Balance: ₹"
         << fixed << setprecision(2)
         << balance << "\n";
}
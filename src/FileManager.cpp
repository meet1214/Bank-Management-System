#include "FileManager.h"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <string>

using namespace std;

// File format (11 pipe-separated fields):
// accountNo | name | pinHash | salt | balance | lock | role
// | depositLimit | withdrawLimit | dailyTxnLimit | dailyTransferLimit

void FileManager::saveAccounts(
    const unordered_map<string, BankAccount>& users) {

    filesystem::create_directories("data");
    ofstream file("data/accounts.txt");

    if (!file) { cout << "Error saving file.\n"; return; }

    for (const auto& [accNo, user] : users) {
        file << user.getAccountNumber()    << "|"
             << user.getName()             << "|"
             << user.getPinHash()          << "|"
             << user.getSalt()             << "|"
             << user.getBalance()          << "|"
             << user.getLockStatus()       << "|"
             << user.getRole()             << "|"
             << user.getAccountType()      << "|"
             << user.getDepositLimit()     << "|"
             << user.getWithdrawLimit()    << "|"
             << user.getDailyTxnLimit()    << "|"
             << user.getDailyTransferLimit()<< "\n";
    }
}

void FileManager::loadAccounts(
    unordered_map<string, BankAccount>& users,
    long long& lastSequenceNumber,
    const string& branchCode) {

    users.clear();
    lastSequenceNumber = 0;
    filesystem::create_directories("data");

    ifstream file("data/accounts.txt");
    if (!file) return;

    string line;
    while (getline(file, line)) {
        if (line.empty()) continue;

        stringstream ss(line);
        string accNo, name, pinHash, salt,
               balanceStr, lockStr, role, accType,
               depLimStr, withLimStr, txnLimStr, transferLimStr;

        getline(ss, accNo,          '|');
        getline(ss, name,           '|');
        getline(ss, pinHash,        '|');
        getline(ss, salt,           '|');
        getline(ss, balanceStr,     '|');
        getline(ss, lockStr,        '|');
        getline(ss, role,           '|');
        getline(ss, accType,        '|');
        // Limit fields — optional for backward compatibility
        getline(ss, depLimStr,      '|');
        getline(ss, withLimStr,     '|');
        getline(ss, txnLimStr,      '|');
        getline(ss, transferLimStr, '|');

        try {
            double balance = stod(balanceStr);
            bool isLocked  = (lockStr == "1");

            // Use saved limits if present, otherwise use defaults
            double depLim      = depLimStr.empty()      ? 100000.0 : stod(depLimStr);
            double withLim     = withLimStr.empty()     ? 50000.0  : stod(withLimStr);
            int    txnLim      = txnLimStr.empty()      ? 10       : stoi(txnLimStr);
            double transferLim = transferLimStr.empty() ? 200000.0 : stod(transferLimStr);

            if (accType.empty()) accType = "Savings";

            users.emplace(accNo,
                BankAccount(accNo, name, pinHash, salt,
                            balance, role, isLocked, accType,
                            depLim, withLim, txnLim, transferLim));

            // Track highest sequence number for account numbering
            if (accNo.length() > branchCode.length()) {
                string seqPart = accNo.substr(branchCode.length());
                if (!seqPart.empty() &&
                    all_of(seqPart.begin(), seqPart.end(), ::isdigit)) {
                    long long seq = stoll(seqPart);
                    if (seq > lastSequenceNumber)
                        lastSequenceNumber = seq;
                }
            }
        } catch (...) {
            cout << "Skipping corrupted entry.\n";
        }
    }
}
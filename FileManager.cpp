#include "FileManager.h"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>

using namespace std;

void FileManager::saveAccounts(
    const unordered_map<string, BankAccount>& users) {

    filesystem::create_directories("data");

    ofstream file("data/accounts.txt");

    if (!file) {
        cout << "Error saving file.\n";
        return;
    }

    for (const auto& [accNo, user] : users) {

        file << user.getAccountNumber() << "|"
             << user.getName() << "|"
             << user.getPinHash() << "|"
             << user.getBalance() << "|"
             << user.getLockStatus() << "|"
             << user.getRole() << "\n";
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

    if (!file)
        return;

    string line;

    while (getline(file, line)) {

        if (line.empty())
            continue;

        stringstream ss(line);

        string accNo, name,
               pinHashStr, balanceStr,
               lockStr, role;

        getline(ss, accNo, '|');
        getline(ss, name, '|');
        getline(ss, pinHashStr, '|');
        getline(ss, balanceStr, '|');
        getline(ss, lockStr, '|');
        getline(ss, role, '|');

        try {
            size_t pinHash = stoull(pinHashStr);
            double balance = stod(balanceStr);
            bool isLocked = (lockStr == "1");

            users.emplace(
                accNo,
                BankAccount(accNo, name, pinHash, balance, role, isLocked)
            );

            if (accNo.length() > branchCode.length()) {

                string seqPart =
                    accNo.substr(branchCode.length());

                if (!seqPart.empty() &&
                    all_of(seqPart.begin(),
                           seqPart.end(),
                           ::isdigit)) {

                    long long seq = stoll(seqPart);

                    if (seq > lastSequenceNumber)
                        lastSequenceNumber = seq;
                }
            }
        }
        catch (...) {
            cout << "Skipping corrupted entry.\n";
        }
    }
}
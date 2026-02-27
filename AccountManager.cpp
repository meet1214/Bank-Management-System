#include "AccountManager.h"
#include "BankAccount.h"
#include <filesystem>
#include <iostream>
#include <fstream>
#include <sstream>
#include <functional>

using namespace std;

// ================= FIND USER =================
int AccountManager::findUserIndex(const string& accNo) const {
    for (size_t i = 0; i < users.size(); ++i) {
        if (users[i].getAccountNumber() == accNo) {
            return i;
        }
    }
    return -1;
}

// ================= GENERATE ACCOUNT NUMBER =================
string AccountManager::generateAccountNumber() {

    lastSequenceNumber++;

    string sequence = to_string(lastSequenceNumber);

    while (sequence.length() < 8) {
        sequence = "0" + sequence;
    }

    return branchCode + sequence;   // 12-digit final number
}

// ================= CREATE ACCOUNT =================
bool AccountManager::createAccount(const string& name, int pin) {

    string accNo = generateAccountNumber();

    size_t hashValue = std::hash<std::string>{}(std::to_string(pin));

    users.push_back(
        BankAccount(accNo, name, hashValue, 0.0, "user", false)
    );

    cout << "Account created successfully!\n";
    cout << "Your Account Number is: " << accNo << "\n";

    saveToFile();
    return true;
}

// ================= LOGIN =================
BankAccount* AccountManager::loginAccount(const string& accNo, int pin) {

    for (auto& user : users) {

        if (user.getAccountNumber() == accNo) {

            if (user.getLockStatus()) {
                cout << "Account is locked.\n";
                return nullptr;
            }

            if (user.authenticatePin(pin))
                return &user;

            return nullptr;
        }
    }

    return nullptr;
}

// ================= SAVE TO FILE =================
void AccountManager::saveToFile() const {

    std::filesystem::create_directories("data");
    ofstream file("data/accounts.txt");

    if (!file) {
        cout << "Error saving file.\n";
        return;
    }

    for (const auto& user : users) {
        file << user.getAccountNumber() << "|"
             << user.getName() << "|"
             << user.getPinHash() << "|"
             << user.getBalance() << "|"
             << user.getLockStatus() << "|"
             << user.getRole() << "\n";
    }

    file.close();
}

// ================= LOAD FROM FILE =================
void AccountManager::loadFromFile() {

    users.clear();
    lastSequenceNumber = 0;

    // Ensure data directory exists
    std::filesystem::create_directories("data");

    ifstream file("data/accounts.txt");

    if (file) {

        string line;

        while (getline(file, line)) {

            stringstream ss(line);

            string accNo, name, pinHashStr, balanceStr, role, lockStr;

            getline(ss, accNo, '|');
            getline(ss, name, '|');
            getline(ss, pinHashStr, '|');
            getline(ss, balanceStr, '|');
            getline(ss, role, '|');
            getline(ss, lockStr, '|');

            if (accNo.empty())
                continue;

            size_t pinHash = stoull(pinHashStr);
            double balance = stod(balanceStr);
            bool isLocked = (lockStr == "1");

            // Create account using full constructor
            BankAccount account(accNo, name, pinHash, balance, role, isLocked);

            users.push_back(account);

            // Extract numeric sequence from account number
            if (accNo.length() >= 12) {
                string sequencePart = accNo.substr(4);
                long long seq = stoll(sequencePart);

                if (seq > lastSequenceNumber)
                    lastSequenceNumber = seq;
            }
        }

        file.close();
    }

    // ================= Ensure at least one admin exists =================

    bool adminExists = false;

    for (const auto& user : users) {
        if (user.getRole() == "admin") {
            adminExists = true;
            break;
        }
    }

    if (!adminExists) {

        // Default admin PIN: 1234 (hashed as string)
        size_t defaultAdminHash = std::hash<std::string>{}(std::to_string(1234));

        users.push_back(
            BankAccount("ADMIN00000001",
                        "SystemAdmin",
                        defaultAdminHash,
                        0.0,
                        "admin",
                        false)
        );

        // This will create data/accounts.txt on first run
        saveToFile();
    }
}

// ================= GET ACCOUNT BY INDEX =================
BankAccount* AccountManager::getAccountByIndex(int index) {
    if (index < 0 || index >= static_cast<int>(users.size())) {
        return nullptr;
    }
    return &users[index];
}

//================SHOW ADMIN MENU============
void AccountManager::showAdminMenu() {

    int choice;

    do {
        cout << "\n====== ADMIN PANEL ======\n";
        cout << "1. View All Accounts\n";
        cout << "2. Freeze Account\n";
        cout << "3. Unfreeze Account\n";
        cout << "4. Delete Account\n";
        cout << "5. View Total Bank Balance\n";
        cout << "6. Logout\n";
        cout << "Enter choice: ";
        cin >> choice;

        string accNo;

        switch (choice) {

            case 1:
                viewAllAccounts();
                break;

            case 2:
                cout << "Enter Account Number to Freeze: ";
                cin >> accNo;
                freezeAccount(accNo);
                break;

            case 3:
                cout << "Enter Account Number to Unfreeze: ";
                cin >> accNo;
                unfreezeAccount(accNo);
                break;

            case 4:
                cout << "Enter Account Number to Delete: ";
                cin >> accNo;
                deleteAccount(accNo);
                break;

            case 5:
                showTotalBankBalance();
                break;

            case 6:
                cout << "Logging out...\n";
                break;

            default:
                cout << "Invalid choice.\n";
        }

    } while (choice != 6);
}

//=========VIEW ALL ACCOUNTS================
void AccountManager::viewAllAccounts() const {

    cout << "\n----- All User Accounts -----\n";

    for (const auto& user : users) {

        if (user.getRole() == "admin")
            continue;

        cout << "Account No: " << user.getAccountNumber() << "\n";
        cout << "Name: " << user.getName() << "\n";
        cout << "Balance: ₹" << user.getBalance() << "\n";
        cout << "Status: "
             << (user.getLockStatus() ? "Locked" : "Active")
             << "\n-----------------------------\n";
    }
}

//=========FREEZE ACCOUNTS================
void AccountManager::freezeAccount(const string& accNo) {

    int index = findUserIndex(accNo);

    if (index == -1) {
        cout << "Account not found.\n";
        return;
    }

    if (users[index].getRole() == "admin") {
        cout << "Cannot freeze admin account.\n";
        return;
    }

    users[index].setLockStatus(true);
    saveToFile();

    cout << "Account frozen successfully.\n";
}

//============UNFREEZE ACCOUNTS===============
void AccountManager::unfreezeAccount(const string& accNo) {

    int index = findUserIndex(accNo);

    if (index == -1) {
        cout << "Account not found.\n";
        return;
    }

    users[index].setLockStatus(false);
    saveToFile();

    cout << "Account unfrozen successfully.\n";
}

//==========DELETE ACCOUNT=================
void AccountManager::deleteAccount(const string& accNo) {

    int index = findUserIndex(accNo);

    if (index == -1) {
        cout << "Account not found.\n";
        return;
    }

    if (users[index].getRole() == "admin") {
        cout << "Cannot delete admin account.\n";
        return;
    }

    // Delete per-account transaction file
    string filePath = "data/transactions/" + accNo + ".txt";
    std::filesystem::remove(filePath);

    users.erase(users.begin() + index);
    saveToFile();

    cout << "Account deleted successfully.\n";
}

//========SHOW TOTAL BANK BALANCE===============
void AccountManager::showTotalBankBalance() const {

    double total = 0;

    for (const auto& user : users) {
        if (user.getRole() == "user")
            total += user.getBalance();
    }

    cout << "Total Bank Balance (All Users): ₹" << total << "\n";
}


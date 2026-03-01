#include "AccountManager.h"
#include "BankAccount.h"
#include <filesystem>
#include <iostream>
#include <fstream>
#include <sstream>
#include <functional>
#include <algorithm>

using namespace std;

// ================= FIND USER =================
int AccountManager::findUserIndex(const string& accNo) const {
    for (size_t i = 0; i < users.size(); ++i) {
        if (users[i].getAccountNumber() == accNo)
            return i;
    }
    return -1;
}

// ================= GENERATE ACCOUNT NUMBER =================
string AccountManager::generateAccountNumber() {

    lastSequenceNumber++;

    string sequence = to_string(lastSequenceNumber);

    while (sequence.length() < 8)
        sequence = "0" + sequence;

    return branchCode + sequence;
}

// ================= CREATE ACCOUNT =================
bool AccountManager::createAccount(const string& name, int pin) {

    string accNo = generateAccountNumber();

    size_t hashValue =
        std::hash<std::string>{}(std::to_string(pin));

    users.push_back(
        BankAccount(accNo, name, hashValue, 0.0, "user", false)
    );

    cout << "Account created successfully!\n";
    cout << "Your Account Number is: " << accNo << "\n";

    saveToFile();
    return true;
}

// ================= LOGIN =================
BankAccount* AccountManager::loginAccount(
        const string& accNo, int pin) {

    for (auto& user : users) {

        if (user.getAccountNumber() == accNo) {

            if (user.getLockStatus()) {
                cout << "Account is locked.\n";
                return nullptr;
            }

            if (user.authenticatePin(pin))
                return &user;

            cout << "Invalid PIN.\n";
            return nullptr;
        }
    }

    cout << "Account not found.\n";
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

    std::filesystem::create_directories("data");

    ifstream file("data/accounts.txt");

    if (file) {

        string line;

        while (getline(file, line)) {

            if (line.empty())
                continue;

            stringstream ss(line);

            string accNo, name,
                   pinHashStr, balanceStr,
                   lockStr, role;

            // Correct order (must match saveToFile)
            getline(ss, accNo, '|');
            getline(ss, name, '|');
            getline(ss, pinHashStr, '|');
            getline(ss, balanceStr, '|');
            getline(ss, lockStr, '|');
            getline(ss, role, '|');

            if (accNo.empty())
                continue;

            try {

                size_t pinHash = stoull(pinHashStr);
                double balance = stod(balanceStr);
                bool isLocked = (lockStr == "1");

                BankAccount account(
                    accNo,
                    name,
                    pinHash,
                    balance,
                    role,
                    isLocked
                );

                users.push_back(account);

                // Extract numeric sequence safely
                if (accNo.length() > branchCode.length()) {

                    string sequencePart =
                        accNo.substr(branchCode.length());

                    bool isNumeric =
                        !sequencePart.empty() &&
                        all_of(sequencePart.begin(),
                               sequencePart.end(),
                               ::isdigit);

                    if (isNumeric) {
                        long long seq = stoll(sequencePart);

                        if (seq > lastSequenceNumber)
                            lastSequenceNumber = seq;
                    }
                }
            }
            catch (...) {
                cout << "Warning: Skipping corrupted account entry.\n";
                continue;
            }
        }

        file.close();
    }

    // ================= ENSURE ADMIN EXISTS =================
    bool adminExists = false;

    for (const auto& user : users) {
        if (user.getRole() == "admin") {
            adminExists = true;
            break;
        }
    }

    if (!adminExists) {

        size_t defaultAdminHash =
            std::hash<std::string>{}(std::to_string(1234));

        users.push_back(
            BankAccount("ADMIN00000001",
                        "SystemAdmin",
                        defaultAdminHash,
                        0.0,
                        "admin",
                        false)
        );

        saveToFile();
    }
}

// ================= GET ACCOUNT BY INDEX =================
BankAccount* AccountManager::getAccountByIndex(int index) {

    if (index < 0 ||
        index >= static_cast<int>(users.size()))
        return nullptr;

    return &users[index];
}

// ================= ADMIN MENU =================
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
                cout << "Enter Account Number: ";
                cin >> accNo;
                freezeAccount(accNo);
                break;

            case 3:
                cout << "Enter Account Number: ";
                cin >> accNo;
                unfreezeAccount(accNo);
                break;

            case 4:
                cout << "Enter Account Number: ";
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

// ================= VIEW ALL ACCOUNTS =================
void AccountManager::viewAllAccounts() const {

    cout << "\n----- All User Accounts -----\n";

    for (const auto& user : users) {

        if (user.getRole() == "admin")
            continue;

        cout << "Account No: "
             << user.getAccountNumber() << "\n";
        cout << "Name: "
             << user.getName() << "\n";
        cout << "Balance: ₹"
             << user.getBalance() << "\n";
        cout << "Status: "
             << (user.getLockStatus()
                 ? "Locked"
                 : "Active")
             << "\n-----------------------------\n";
    }
}

// ================= FREEZE ACCOUNT =================
void AccountManager::freezeAccount(
        const string& accNo) {

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

// ================= UNFREEZE ACCOUNT =================
void AccountManager::unfreezeAccount(
        const string& accNo) {

    int index = findUserIndex(accNo);

    if (index == -1) {
        cout << "Account not found.\n";
        return;
    }

    users[index].setLockStatus(false);
    saveToFile();

    cout << "Account unfrozen successfully.\n";
}

// ================= DELETE ACCOUNT =================
void AccountManager::deleteAccount(
        const string& accNo) {

    int index = findUserIndex(accNo);

    if (index == -1) {
        cout << "Account not found.\n";
        return;
    }

    if (users[index].getRole() == "admin") {
        cout << "Cannot delete admin account.\n";
        return;
    }

    string filePath =
        "data/transactions/" + accNo + ".txt";

    std::filesystem::remove(filePath);

    users.erase(users.begin() + index);

    saveToFile();

    cout << "Account deleted successfully.\n";
}

// ================= TOTAL BANK BALANCE =================
void AccountManager::showTotalBankBalance() const {

    double total = 0;

    for (const auto& user : users) {
        if (user.getRole() == "user")
            total += user.getBalance();
    }

    cout << "Total Bank Balance: ₹"
         << total << "\n";
}
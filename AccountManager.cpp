#include "AccountManager.h"
#include "BankAccount.h"
#include "FileManager.h"

#include <filesystem>
#include <iostream>
#include <functional>


using namespace std;

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
        hash<string>{}(to_string(pin));

    auto result = users.emplace(
        accNo,
        BankAccount(accNo, name, hashValue, 0.0, "user", false)
    );

    if (!result.second) {
        cout << "Account number collision occurred.\n";
        return false;
    }

    save();

    cout << "Account created successfully!\n";
    cout << "Your Account Number is: " << accNo << "\n";

    return true;
}

// ================= LOGIN =================
string AccountManager::loginAccount(
        const string& accNo, int pin) {

    auto it = users.find(accNo);

    if (it == users.end()) {
        cout << "Account not found.\n";
        return "";
    }

    BankAccount& user = it->second;

    if (user.getLockStatus()) {
        cout << "Account is locked.\n";
        return "";
    }

    if (!user.authenticatePin(pin)) {
        cout << "Invalid PIN.\n";
        return "";
    }

    return accNo;  // return account ID instead of pointer
}

// ================= SAVE TO FILE =================
void AccountManager::save() const {
    FileManager::saveAccounts(users);
}

// ================= LOAD FROM FILE =================
void AccountManager::load() {
    FileManager::loadAccounts(users,
                              lastSequenceNumber,
                              branchCode);

    ensureAdminExists();
}
//======ENSURE ADMIN EXISTENCE=============
void AccountManager::ensureAdminExists() {

    for (const auto& [accNo, user] : users) {
        if (user.getRole() == "admin")
            return;
    }

    size_t defaultAdminHash =
        std::hash<std::string>{}(std::to_string(1234));

    users.emplace(
        "ADMIN00000001",
        BankAccount("ADMIN00000001",
                    "SystemAdmin",
                    defaultAdminHash,
                    0.0,
                    "admin",
                    false)
    );

    save();
}

// ================= GET ACCOUNT =================
BankAccount* AccountManager::getAccountByAccountNumber(
        const string& accNo) {

    auto it = users.find(accNo);

    if (it == users.end())
        return nullptr;

    return &it->second;
}

// ================= VIEW ALL ACCOUNTS =================
void AccountManager::viewAllAccounts() const {

    cout << "\n----- All User Accounts -----\n";

    for (const auto& [accNo, user] : users) {

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

// ================= FREEZE ACCOUNT =================
void AccountManager::freezeAccount(const string& accNo) {

    auto it = users.find(accNo);

    if (it == users.end()) {
        cout << "Account not found.\n";
        return;
    }

    if (it->second.getRole() == "admin") {
        cout << "Cannot freeze admin account.\n";
        return;
    }

    it->second.setLockStatus(true);
    save();

    cout << "Account frozen successfully.\n";
}

// ================= UNFREEZE ACCOUNT =================
void AccountManager::unfreezeAccount(const string& accNo) {

    auto it = users.find(accNo);

    if (it == users.end()) {
        cout << "Account not found.\n";
        return;
    }

    it->second.setLockStatus(false);
    save();

    cout << "Account unfrozen successfully.\n";
}

// ================= DELETE ACCOUNT =================
void AccountManager::deleteAccount(const string& accNo) {

    auto it = users.find(accNo);

    if (it == users.end()) {
        cout << "Account not found.\n";
        return;
    }

    if (it->second.getRole() == "admin") {
        cout << "Cannot delete admin account.\n";
        return;
    }

    // Remove transaction file
    string filePath = "data/transactions/" + accNo + ".txt";
    std::filesystem::remove(filePath);

    users.erase(it);
    save();

    cout << "Account deleted successfully.\n";
}

// ================= TOTAL BANK BALANCE =================
void AccountManager::showTotalBankBalance() const {

    double total = 0;

    for (const auto& [accNo, user] : users) {
        if (user.getRole() == "user")
            total += user.getBalance();
    }

    cout << "Total Bank Balance: ₹" << total << "\n";
}
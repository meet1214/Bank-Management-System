#include "AccountManager.h"
#include "BankAccount.h"
#include "FileManager.h"
#include "Sha256.h"

#include <filesystem>
#include <iomanip>
#include <iostream>

using namespace std;

// ================= GENERATE ACCOUNT NUMBER =================
string AccountManager::generateAccountNumber() {
    ++lastSequenceNumber;
    string sequence = to_string(lastSequenceNumber);
    while (sequence.length() < 8)
        sequence = "0" + sequence;
    return branchCode + sequence;
}

// ================= CREATE ACCOUNT =================
bool AccountManager::createAccount(const string& name, int pin) {
    string accNo = generateAccountNumber();

    // Generate unique salt and hash the PIN
    string salt      = generateSalt();
    string pinHashed = hashPin(to_string(pin), salt);

    auto result = users.emplace(
        accNo,
        BankAccount(accNo, name, pinHashed, salt, 0.0, "user", false)
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
string AccountManager::loginAccount(const string& accNo, int pin) {
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

    return accNo;
}

// ================= SAVE =================
void AccountManager::save() const {
    FileManager::saveAccounts(users);
}

// ================= LOAD =================
void AccountManager::load() {
    FileManager::loadAccounts(users, lastSequenceNumber, branchCode);
    ensureAdminExists();
}

// ================= ENSURE ADMIN EXISTS =================
void AccountManager::ensureAdminExists() {
    for (const auto& [accNo, user] : users) {
        if (user.getRole() == "admin") return;
    }

    // Default admin PIN: 1234 — uses a fixed salt so it's
    // reproducible across restarts without storing state
    string adminSalt = "ADMIN_FIXED_SALT_v1";
    string adminHash = hashPin("1234", adminSalt);

    users.emplace(
        "ADMIN00000001",
        BankAccount("ADMIN00000001", "SystemAdmin",
                    adminHash, adminSalt,
                    0.0, "admin", false)
    );

    save();
}

// ================= GET ACCOUNT =================
BankAccount* AccountManager::getAccountByAccountNumber(const string& accNo) {
    auto it = users.find(accNo);
    if (it == users.end()) return nullptr;
    return &it->second;
}

// ================= VIEW ALL ACCOUNTS =================
void AccountManager::viewAllAccounts() const {
    cout << "\n----- All User Accounts -----\n";
    for (const auto& [accNo, user] : users) {
        if (user.getRole() == "admin") continue;
        cout << "Account No: " << user.getAccountNumber() << "\n"
             << "Name: "       << user.getName()           << "\n"
             << "Balance: Rs." << user.getBalance()         << "\n"
             << "Status: "     << (user.getLockStatus() ? "Locked" : "Active")
             << "\n-----------------------------\n";
    }
}

// ================= FREEZE =================
void AccountManager::freezeAccount(const string& accNo) {
    auto it = users.find(accNo);
    if (it == users.end()) { cout << "Account not found.\n"; return; }
    if (it->second.getRole() == "admin") { cout << "Cannot freeze admin account.\n"; return; }
    it->second.setLockStatus(true);
    save();
    cout << "Account frozen successfully.\n";
}

// ================= UNFREEZE =================
void AccountManager::unfreezeAccount(const string& accNo) {
    auto it = users.find(accNo);
    if (it == users.end()) { cout << "Account not found.\n"; return; }
    it->second.setLockStatus(false);
    save();
    cout << "Account unfrozen successfully.\n";
}

// ================= DELETE =================
void AccountManager::deleteAccount(const string& accNo) {
    auto it = users.find(accNo);
    if (it == users.end()) { cout << "Account not found.\n"; return; }
    if (it->second.getRole() == "admin") { cout << "Cannot delete admin account.\n"; return; }
    std::filesystem::remove("data/transactions/" + accNo + ".txt");
    users.erase(it);
    save();
    cout << "Account deleted successfully.\n";
}

// ================= SET ACCOUNT LIMITS =================
void AccountManager::setAccountLimits(const string& accNo) {

    auto it = users.find(accNo);
    if (it == users.end()) {
        cout << "Account not found.\n";
        return;
    }
    if (it->second.getRole() == "admin") {
        cout << "Cannot set limits on admin account.\n";
        return;
    }

    BankAccount& acc = it->second;

    cout << "\n--- Current Limits for " << acc.getName() << " ---\n";
    acc.showLimits();

    cout << "\nEnter new limits (press Enter to keep current):\n";

    // Helper lambda to read optional double input
    auto readDouble = [](const string& prompt, double current) -> double {
        cout << prompt << " [current: Rs." << fixed
             << setprecision(2) << current << "]: ";
        string input;
        getline(cin >> ws, input);
        if (input.empty()) return current;
        try { return stod(input); }
        catch (...) { cout << "Invalid, keeping current.\n"; return current; }
    };

    auto readInt = [](const string& prompt, int current) -> int {
        cout << prompt << " [current: " << current << "]: ";
        string input;
        getline(cin >> ws, input);
        if (input.empty()) return current;
        try { return stoi(input); }
        catch (...) { cout << "Invalid, keeping current.\n"; return current; }
    };

    double newDep      = readDouble("Max deposit per transaction",    acc.getDepositLimit());
    double newWith     = readDouble("Max withdrawal per transaction",  acc.getWithdrawLimit());
    int    newTxn      = readInt   ("Max transactions per day",        acc.getDailyTxnLimit());
    double newTransfer = readDouble("Max total transfer per day",      acc.getDailyTransferLimit());

    // Validate — no negative or zero limits
    if (newDep <= 0 || newWith <= 0 || newTxn <= 0 || newTransfer <= 0) {
        cout << "Limits must be greater than zero. No changes saved.\n";
        return;
    }

    acc.setDepositLimit(newDep);
    acc.setWithdrawLimit(newWith);
    acc.setDailyTxnLimit(newTxn);
    acc.setDailyTransferLimit(newTransfer);

    save();
    cout << "Limits updated successfully.\n";
}

// ================= TOTAL BALANCE =================
void AccountManager::showTotalBankBalance() const {
    double total = 0;
    for (const auto& [accNo, user] : users)
        if (user.getRole() == "user") total += user.getBalance();
    cout << "Total Bank Balance: Rs." << total << "\n";
}
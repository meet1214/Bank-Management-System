#include "AccountManager.h"
#include "BankAccount.h"
#include "Sha256.h"
#include "Logger.h"
#include "DatabaseManager.h"

#include <algorithm>
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
bool AccountManager::createAccount(const string& name, int pin, const string& accType) {
    string accNo = generateAccountNumber();

    // Generate unique salt and hash the PIN
    string salt      = generateSalt();
    string pinHashed = hashPin(to_string(pin), salt);

    auto result = users.emplace(
        accNo,
        BankAccount(accNo, name, pinHashed, salt, 0.0, "user", false,accType)
    );

    if (!result.second) {
        Logger::getInstance()->error("Account number Collision:" + accNo);
        return false;
    }

    save();
    Logger::getInstance()->info("Account created: " + accNo + " for " + name);
    return true;
}

// ================= LOGIN =================
string AccountManager::loginAccount(const string& accNo, int pin) {
    auto it = users.find(accNo);

    if (it == users.end()) {
        Logger::getInstance()->warn("Account " + accNo + " not found");
        return "";
    }

    BankAccount& user = it->second;

    if (user.getLockStatus()) {
        Logger::getInstance()->warn("Account "+ accNo + " is locked.");
        return "";
    }

    if (!user.authenticatePin(pin)) {
        Logger::getInstance()->warn("Invalid pin for " +accNo );
        return "";
    }
        
    Logger::getInstance()->info("Login successful");
    return accNo;
}

// ================= SAVE =================
void AccountManager::save() const {
    DatabaseManager::saveAccounts(users);
}

// ================= LOAD =================
void AccountManager::load() {
    DatabaseManager::open();
    DatabaseManager::loadAccounts(users, lastSequenceNumber, branchCode);
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
                    0.0, "admin", false,"Current")
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
    if (users.empty()) {
        Logger::getInstance()->info("No users found.");
    }
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
    if (it == users.end()) { 
        Logger::getInstance()->warn("Account " + accNo + " not found "); 
        return; 
    }
    if (it->second.getRole() == "admin") { 
        Logger::getInstance()->error("Cannot freeze admin account."); 
        return; 
    }
    it->second.setLockStatus(true);
    save();
    Logger::getInstance()->admin("Account " + accNo + " frozen successfully.");
}

// ================= UNFREEZE =================
void AccountManager::unfreezeAccount(const string& accNo) {
    auto it = users.find(accNo);
    if (it == users.end()) { 
        Logger::getInstance()->warn("Account " + accNo + " not found ");  
        return; 
    }
    it->second.setLockStatus(false);
    save();
    Logger::getInstance()->admin("Account " + accNo + " unfrozen successfully.");
}

// ================= DELETE =================
void AccountManager::deleteAccount(const string& accNo) {
    auto it = users.find(accNo);
    if (it == users.end()) { 
        Logger::getInstance()->warn("Account " + accNo + " not found ");
        return;
    }
    if (it->second.getRole() == "admin") { 
        Logger::getInstance()->error("Cannot delete admin account.");  
        return; 
    }
    DatabaseManager::deleteTransactions(accNo);
    users.erase(it);
    save();
    Logger::getInstance()->admin("Account " + accNo + " deleted successfully.");
}

// ================= SET ACCOUNT LIMITS =================
void AccountManager::setAccountLimits(const string& accNo) {

    auto it = users.find(accNo);
    if (it == users.end()) {
        Logger::getInstance()->warn("Account " + accNo + " not found ");
        return;
    }
    if (it->second.getRole() == "admin") {
        Logger::getInstance()->error("Cannot set limits for the admin account."); 
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
        if (input.empty()) 
            return current;
        try { 
            return stod(input); 
        }
        catch (...) { 
            Logger::getInstance()->error("Invalid, keeping current."); 
            return current; 
        }
    };

    auto readInt = [](const string& prompt, int current) -> int {
        cout << prompt << " [current: " << current << "]: ";
        string input;
        getline(cin >> ws, input);
        if (input.empty()) 
            return current;
        try { 
            return stoi(input); 
        }
        catch (...) { 
            Logger::getInstance()->error("Invalid, keeping current."); 
            return current; 
        }
    };

    double newDep      = readDouble("Max deposit per transaction",    acc.getDepositLimit());
    double newWith     = readDouble("Max withdrawal per transaction",  acc.getWithdrawLimit());
    int    newTxn      = readInt   ("Max transactions per day",        acc.getDailyTxnLimit());
    double newTransfer = readDouble("Max total transfer per day",      acc.getDailyTransferLimit());

    // Validate — no negative or zero limits
    if (newDep <= 0 || newWith <= 0 || newTxn <= 0 || newTransfer <= 0) {
        Logger::getInstance()->error("Limits must be greater than 0.");
        return;
    }

    acc.setDepositLimit(newDep);
    acc.setWithdrawLimit(newWith);
    acc.setDailyTxnLimit(newTxn);
    acc.setDailyTransferLimit(newTransfer);

    save();
    Logger::getInstance()->admin("Limits for " + accNo + " is updated successfully.");
}

// ================= TOTAL BALANCE =================
void AccountManager::showTotalBankBalance() const {
    double total = 0;
    for (const auto& [accNo, user] : users)
        if (user.getRole() == "user") total += user.getBalance();
    cout << "Total Bank Balance: Rs." << total << "\n";
}

//===================APPLY INEREST TO ALL==========================
void AccountManager::applyInterestToAll() {  // ← Remove the rate parameter
    
    int accountsProcessed = 0;
    
    for (auto& [accNo, user] : users) {
        
        if (user.getRole() == "admin") continue;
        
        if (user.getBalance() <= 0) continue;
        
        double rate = user.getInterestRate();
        

        if (rate <= 0) {
            Logger::getInstance()->admin("Skipped " + accNo + " (" + 
                user.getAccountType() + ") - No interest for this account type");
            continue;
        }
        
        double interest = (user.getBalance() * rate) / 100;
        
        user.addInterest(interest);
        accountsProcessed++;
        
        Logger::getInstance()->admin("Interest of Rs." + to_string(interest) + 
            " (" + to_string(rate) + "%) applied to " + accNo + 
            " (" + user.getAccountType() + ")");    
    }
    
    save();
    Logger::getInstance()->admin("Interest applied to " + to_string(accountsProcessed) + " accounts.");
}

//=============VIEW ALL ACCOUNTS SORTED====================
void AccountManager::viewAllAccountsSorted(const string& sortBy) const {
    
    if (users.empty()) {
        Logger::getInstance()->info("No users found.");
        return;
    }
    
    vector<const BankAccount*> accountList;
    for (const auto& [accNo, user] : users) {
        if (user.getRole() == "user") {
            accountList.push_back(&user);
        }
    }
    
    // Sort based on sortBy parameter
    if (sortBy == "account") {
        sort(accountList.begin(), accountList.end(),
             [](const BankAccount* a, const BankAccount* b) {
                 return a->getAccountNumber() < b->getAccountNumber();
             });
    }
    else if (sortBy == "name") {
        sort(accountList.begin(), accountList.end(),
             [](const BankAccount* a, const BankAccount* b) {
                 return a->getName() < b->getName();
             });
    }
    else if (sortBy == "balance_high") {
        sort(accountList.begin(), accountList.end(),
             [](const BankAccount* a, const BankAccount* b) {
                 return a->getBalance() > b->getBalance();
             });
    }
    else if (sortBy == "balance_low") {
        sort(accountList.begin(), accountList.end(),
             [](const BankAccount* a, const BankAccount* b) {
                 return a->getBalance() < b->getBalance();
             });
    }
    else {
        cout << "Invalid sort option.\n";
        return;
    }
    
    cout << "\n----- All User Accounts (Sorted) -----\n";
    for (const BankAccount* user : accountList) {
        cout << "Account No: " << user->getAccountNumber() << "\n"
             << "Name: "       << user->getName()           << "\n"
             << "Balance: Rs." << fixed << setprecision(2) 
             << user->getBalance() << "\n"
             << "Status: "     << (user->getLockStatus() ? "Locked" : "Active")
             << "\n-----------------------------\n";
    }
}
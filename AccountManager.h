#ifndef ACCOUNTMANAGER_H
#define ACCOUNTMANAGER_H

#include "BankAccount.h"
#include <vector>
#include <string>
#include <unordered_map>

class AccountManager{
private:
    std::unordered_map<std::string, BankAccount> users;

    const std::string branchCode = "BNGL";
    long long lastSequenceNumber = 0;

    std::string generateAccountNumber();


public:
    //==========FILE STORAGE=============
    void load();
    void save() const;
    void ensureAdminExists();

    //==========USER PANEL=============
    bool createAccount(const std::string& name, int pin);
    std::string loginAccount(const std::string& accNo, int pin);
    BankAccount* getAccountByAccountNumber(const std::string& accNo);

    //=========ADMIN PANEL==============
    void showAdminMenu();
    void viewAllAccounts() const;
    void freezeAccount(const std::string& accNo);
    void unfreezeAccount(const std::string& accNo);
    void deleteAccount(const std::string& accNo);
    void showTotalBankBalance() const;
};


#endif
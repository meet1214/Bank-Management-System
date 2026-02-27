#ifndef ACCOUNTMANAGER_H
#define ACCOUNTMANAGER_H

#include "BankAccount.h"
#include <vector>
#include <string>

class AccountManager{
private:
    std::vector<BankAccount> users;

    const std::string branchCode = "BNGL";
    long long lastSequenceNumber = 0;

    std::string generateAccountNumber();


public:
    //==========FILE STORAGE=============
    void loadFromFile();
    void saveToFile() const;

    //==========USER PANEL=============
    bool createAccount(const std::string& name, int pin);
    BankAccount* loginAccount(const std::string& name, int pin);
    int findUserIndex(const std::string& name) const;
    BankAccount* getAccountByIndex(int index);

    //=========ADMIN PANEL==============
    void showAdminMenu();
    void viewAllAccounts() const;
    void freezeAccount(const std::string& accNo);
    void unfreezeAccount(const std::string& accNo);
    void deleteAccount(const std::string& accNo);
    void showTotalBankBalance() const;
};


#endif
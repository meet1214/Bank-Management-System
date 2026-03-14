#ifndef DATABASEMANAGER_H
#define DATABASEMANAGER_H

#include <sqlite3.h>
#include <unordered_map>
#include <string>
#include <vector>

#include "BankAccount.h"

using namespace std;

class DatabaseManager {
    private:
        static sqlite3* db_;
        static void initSchema();
    
    public:
        static void open(const string& path = "data/bank.db");
        static void close();

        static void saveAccounts(const unordered_map<string, BankAccount>& users);
        static void loadAccounts(unordered_map<string, BankAccount>& users,
                                long long& lastSeq, const string& branchCode);

        static void saveTransaction(const string& accNo, const Transaction& t);
        static void loadTransactions(const string& accNo,vector<Transaction>& out,int& lastTxnId);

        static void deleteTransactions(const string& accNo);


};
#endif
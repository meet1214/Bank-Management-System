#ifndef DATABASEMANAGER_H
#define DATABASEMANAGER_H

#include <sqlite3.h>
#include <unordered_map>
#include <string>
#include <vector>

#include "BankAccount.h"
#include "Loan.h"

using namespace std;

class DatabaseManager {
    private:
        static sqlite3* db_;
        static void initSchema();
    
    public:
        static void open(const string& path = "data/bank.db");
        static void close();

        //ACCOUNTS OPERATIONS
        static void saveAccounts(const unordered_map<string, BankAccount>& users);
        static void loadAccounts(unordered_map<string, BankAccount>& users,
                                long long& lastSeq, const string& branchCode);

        //TRANSACTION OPERATIONS
        static void saveTransaction(const string& accNo, const Transaction& t);
        static void loadTransactions(const string& accNo,vector<Transaction>& out,int& lastTxnId);
        static void deleteTransactions(const string& accNo);

        //LOAN OPERATIONS
        static void saveLoans(const unordered_map<string, Loan>& loans);
        static void loadLoans(unordered_map<string, Loan>& loans, int& lastSeq);
        static void saveLoanPayment(const string& loanId, const LoanPayment& p);
        static void deleteLoan(const string& loanId);


};
#endif
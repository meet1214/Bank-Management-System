#ifndef DATABASEMANAGER_H
#define DATABASEMANAGER_H

#include <sqlite3.h>
#include <unordered_map>
#include <string>
#include <vector>

#include "BankAccount.h"
#include "Loan.h"
#include "RD.h"
#include "StandingInstruction.h"

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
        
        //SECURITY OPERATIONS
        static string createSession(const string& accountNumber, int expiryMinutes = 15);
        static string validateSession(const string& token);
        static void   deleteSession(const string& token);
        static void   logAudit(const string& accountNumber, const string& token,
                            const string& action, const string& details,
                            const string& status);
        static bool   checkRateLimit(const string& accountNumber, int maxAttempts = 5, int windowSeconds = 60);
        static void   recordFailedAttempt(const string& accountNumber);

        // RD OPERATIONS
        static void saveRD(const RecurringDeposit& rd);
        static void loadRDs(std::vector<RecurringDeposit>& rds);
        static void updateRD(const RecurringDeposit& rd);
        static void deleteRD(const std::string& rdId);

        // STANDING INSTRUCTION OPERATIONS
        static void saveSI(const StandingInstruction& si);
        static void loadSIs(vector<StandingInstruction>& sis);
        static void updateSI(const StandingInstruction& si);
        static void deleteSI(const string& siId);

        //TRANSACTION ROLLBACK OPERATIONS
        static void beginTransaction();
        static void commitTransaction();
        static void rollbackTransaction();

        //CLEANUP OPERATIONS
        static void cleanExpiredSessions();

};
#endif
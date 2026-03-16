#include "DatabaseManager.h"
#include "BankAccount.h"
#include "Logger.h"
#include "RD.h"

#include <sstream>
#include <iomanip>
#include <random>
#include <ctime>
#include <algorithm>
#include <filesystem>
#include <iostream>
#include <sqlite3.h>
#include <vector>

//===========STATIC MEMBER DEFINITION=================
sqlite3* DatabaseManager::db_ = nullptr;

//===========OPEN FUNCTION================
void DatabaseManager::open(const string& path) {
    if(db_ != NULL){
        return;
    }
    std::filesystem::create_directories("data/");
    int rc = sqlite3_open(path.c_str(), &db_);
    if (rc != SQLITE_OK) {
        cout << "Error opening the database file.\n";
        return;
    }
    initSchema();

    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db_, "PRAGMA integrity_check;", -1, &stmt, nullptr);
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        string result = (const char*)sqlite3_column_text(stmt, 0);
        if (result == "ok") {
            Logger::getInstance()->info("Database integrity check passed.");
        } else {
            Logger::getInstance()->error("Database integrity check FAILED: " + result);
        }
    }
    sqlite3_finalize(stmt);

    Logger::getInstance()->info("The file is opened successfully");
}

void DatabaseManager::initSchema() {
    const char* sql = R"(
        CREATE TABLE IF NOT EXISTS accounts (
        account_number      TEXT    PRIMARY KEY,
        name                TEXT    NOT NULL,
        pin_hash            TEXT    NOT NULL,
        salt                TEXT    NOT NULL,
        balance             REAL    NOT NULL DEFAULT 0.0,
        role                TEXT    NOT NULL DEFAULT 'user',
        is_locked           INTEGER NOT NULL DEFAULT 0,
        acc_type            TEXT    NOT NULL DEFAULT 'Savings',
        deposit_limit       REAL    NOT NULL DEFAULT 100000.0,
        withdraw_limit      REAL    NOT NULL DEFAULT 50000.0,
        daily_txn_limit     INTEGER NOT NULL DEFAULT 10,
        daily_transfer_lim  REAL    NOT NULL DEFAULT 200000.0
        );
    )";
    sqlite3_exec(db_, sql, nullptr, nullptr, nullptr);

    sql = R"(
        CREATE TABLE IF NOT EXISTS transactions (
        id              INTEGER PRIMARY KEY AUTOINCREMENT,
        account_number  TEXT    NOT NULL,
        txn_id          INTEGER NOT NULL,
        date_time       TEXT    NOT NULL,
        type            TEXT    NOT NULL,
        amount          REAL    NOT NULL,
        balance         REAL    NOT NULL,
        FOREIGN KEY (account_number)
            REFERENCES accounts(account_number)
            ON DELETE CASCADE
        );
    )";
    sqlite3_exec(db_, sql, nullptr, nullptr, nullptr);

    sql = R"(
        CREATE INDEX IF NOT EXISTS idx_txn_account 
            ON transactions(account_number, txn_id);
    )";
    sqlite3_exec(db_, sql, nullptr, nullptr, nullptr);

    sql = R"(
        CREATE TABLE IF NOT EXISTS loans (
            loan_id             TEXT    PRIMARY KEY,
            account_number      TEXT    NOT NULL,
            loan_type           TEXT    NOT NULL,
            principal_amount    REAL    NOT NULL,
            interest_rate       REAL    NOT NULL,
            tenure_months       INTEGER NOT NULL,
            emi_amount          REAL    NOT NULL,
            outstanding_amount  REAL    NOT NULL,
            status              TEXT    NOT NULL,
            rejection_reason    TEXT,
            application_date    TEXT,
            disbursement_date   TEXT,
            next_emi_date       TEXT,
            payments_made       INTEGER NOT NULL DEFAULT 0,
            FOREIGN KEY (account_number)
                REFERENCES accounts(account_number)
                ON DELETE CASCADE
        );    
    )";
    sqlite3_exec(db_, sql, nullptr, nullptr, nullptr);

    sql = R"(
        CREATE TABLE IF NOT EXISTS loan_payments (
            id                  INTEGER PRIMARY KEY AUTOINCREMENT,
            loan_id             TEXT    NOT NULL,
            payment_number      INTEGER NOT NULL,
            payment_date        TEXT    NOT NULL,
            emi_amount          REAL    NOT NULL,
            principal_paid      REAL    NOT NULL,
            interest_paid       REAL    NOT NULL,
            outstanding_balance REAL    NOT NULL,
            remarks             TEXT,
            FOREIGN KEY (loan_id)
                REFERENCES loans(loan_id)
                ON DELETE CASCADE
        );
    )";
    sqlite3_exec(db_, sql, nullptr, nullptr, nullptr);

    sql = R"(
        CREATE TABLE IF NOT EXISTS sessions (
            token           TEXT    PRIMARY KEY,
            account_number  TEXT    NOT NULL,
            created_at      INTEGER NOT NULL,
            expires_at      INTEGER NOT NULL,
            FOREIGN KEY (account_number)
                REFERENCES accounts(account_number)
                ON DELETE CASCADE
        );
    )";
    sqlite3_exec(db_, sql, nullptr, nullptr, nullptr);

    sql = R"(
        CREATE TABLE IF NOT EXISTS audit_log (
            id              INTEGER PRIMARY KEY AUTOINCREMENT,
            timestamp       INTEGER NOT NULL,
            account_number  TEXT,
            token           TEXT,
            action          TEXT    NOT NULL,
            details         TEXT,
            status          TEXT    NOT NULL
        );
    )";
    sqlite3_exec(db_, sql, nullptr, nullptr, nullptr);

    sql = R"(
        CREATE TABLE IF NOT EXISTS login_attempts (
            account_number  TEXT    NOT NULL,
            attempt_time    INTEGER NOT NULL
        );
    )";
    sqlite3_exec(db_, sql, nullptr, nullptr, nullptr);

    sql = R"(
        CREATE TABLE IF NOT EXISTS recurring_deposits (
            rd_id            TEXT    PRIMARY KEY,
            account_number   TEXT    NOT NULL,
            monthly_amount   REAL    NOT NULL,
            tenure_months    INTEGER NOT NULL,
            interest_rate    REAL    NOT NULL,
            start_date       TEXT    NOT NULL,
            next_debit_date  TEXT    NOT NULL,
            total_deposited  REAL    NOT NULL DEFAULT 0,
            months_paid      INTEGER NOT NULL DEFAULT 0,
            maturity_amount  REAL    NOT NULL,
            status           TEXT    NOT NULL DEFAULT 'ACTIVE',
            FOREIGN KEY (account_number)
                REFERENCES accounts(account_number)
                ON DELETE CASCADE
        );
    )";

    sqlite3_exec(db_, sql, nullptr, nullptr, nullptr);

}

//==========CLOSE FUNCTION==========
void DatabaseManager::close() {
    if (db_) {
        sqlite3_close(db_);
        db_ = nullptr;
    }
}

//==========SAVE ACCOUNTS===================
void DatabaseManager::saveAccounts(const unordered_map<string, BankAccount> &users){

    if (!db_) return;
    sqlite3_exec(db_, "BEGIN;", nullptr, nullptr, nullptr);
    const char* sql = R"(
        INSERT OR REPLACE INTO accounts
        (account_number, name, pin_hash, salt, balance, role, is_locked, acc_type,
        deposit_limit, withdraw_limit, daily_txn_limit, daily_transfer_lim)
        VALUES (?, ?, ?, ?, ?,?, ?, ?, ?, ?, ?, ?);
    )";

    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);


    for (const auto& [accNo, user] : users){
        sqlite3_bind_text   (stmt, 1,  user.getAccountNumber().c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text   (stmt, 2,  user.getName().c_str(),          -1, SQLITE_TRANSIENT);
        sqlite3_bind_text   (stmt, 3,  user.getPinHash().c_str(),       -1, SQLITE_TRANSIENT);
        sqlite3_bind_text   (stmt, 4,  user.getSalt().c_str(),          -1, SQLITE_TRANSIENT);
        sqlite3_bind_double (stmt, 5,  user.getBalance());
        sqlite3_bind_text   (stmt, 6,  user.getRole().c_str(),          -1, SQLITE_TRANSIENT);
        sqlite3_bind_int    (stmt, 7,  user.getLockStatus() ? 1 : 0);
        sqlite3_bind_text   (stmt, 8,  user.getAccountType().c_str(),   -1, SQLITE_TRANSIENT);
        sqlite3_bind_double (stmt, 9,  user.getDepositLimit());
        sqlite3_bind_double (stmt, 10, user.getWithdrawLimit());
        sqlite3_bind_int    (stmt, 11, user.getDailyTxnLimit());
        sqlite3_bind_double (stmt, 12, user.getDailyTransferLimit());


        sqlite3_step(stmt);
        sqlite3_reset(stmt);
        sqlite3_clear_bindings(stmt);
    }

    sqlite3_finalize(stmt);
    sqlite3_exec(db_, "COMMIT;", nullptr, nullptr, nullptr); 
}

//===========LOAD ACCOUNTS===================================
void DatabaseManager::loadAccounts(unordered_map<string, BankAccount> &users, long long &lastSeq, const string &branchCode) {

    users.clear();
    lastSeq = 0;

    const char* sql = R"(
    SELECT account_number, name, pin_hash, salt, balance,
       role, is_locked, acc_type, deposit_limit, withdraw_limit,
       daily_txn_limit, daily_transfer_lim
    FROM accounts;
    )";

    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        string accNo   = (const char*)  sqlite3_column_text  (stmt, 0);
        string name    = (const char*)  sqlite3_column_text  (stmt, 1);
        string pinHash = (const char*)  sqlite3_column_text  (stmt, 2);
        string salt    = (const char*)  sqlite3_column_text  (stmt, 3);
        double balance =                sqlite3_column_double(stmt, 4);
        string role    = (const char*)  sqlite3_column_text  (stmt, 5);
        bool isLocked  =                sqlite3_column_int   (stmt, 6) != 0;
        string acc_type = (const char*) sqlite3_column_text  (stmt, 7);
        double depLim  =                sqlite3_column_double(stmt, 8);
        double withLim =                sqlite3_column_double(stmt, 9);
        int    txnLim  =                sqlite3_column_int   (stmt, 10);
        double transLim=                sqlite3_column_double(stmt, 11);

        users.emplace(accNo,
        BankAccount(accNo, name, pinHash, salt,
                balance, role, isLocked, acc_type,
                depLim, withLim, txnLim, transLim));

        if (accNo.length() > branchCode.length()) {
                string seqPart = accNo.substr(branchCode.length());
                if (!seqPart.empty() &&
                    all_of(seqPart.begin(), seqPart.end(), ::isdigit)) {
                    long long seq = stoll(seqPart);
                    if (seq > lastSeq)
                        lastSeq = seq;
                }
        }
    }
    sqlite3_finalize(stmt);
}

//================SAVE TRANSACTIONS=======================
void DatabaseManager::saveTransaction(const string &accNo, const Transaction &t) {
    
    const char* sql = R"(
    INSERT INTO transactions
        (account_number, txn_id, date_time, type, amount, balance)
        VALUES (?, ?, ?, ?, ?, ?);
    )";

    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    
    sqlite3_bind_text   (stmt, 1, accNo.c_str(),      -1, SQLITE_TRANSIENT);
    sqlite3_bind_int    (stmt, 2, t.transactionId);
    sqlite3_bind_text   (stmt, 3, t.dateTime.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text   (stmt, 4, t.type.c_str(),     -1, SQLITE_TRANSIENT);
    sqlite3_bind_double (stmt, 5, t.amount);
    sqlite3_bind_double (stmt, 6, t.balance);

    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}

//=================LOAD TRANSACTIONS=======================
void DatabaseManager::loadTransactions(const string &accNo, vector<Transaction> &out, int &lastTxnId) {

    out.clear();
    lastTxnId = 0;

    const char* sql = R"(
        SELECT txn_id, date_time, type, amount, balance
        FROM transactions
        WHERE account_number = ?
        ORDER BY txn_id ASC;
    )";

    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);

    sqlite3_bind_text(stmt, 1, accNo.c_str(), -1, SQLITE_TRANSIENT);

    while (sqlite3_step(stmt) == SQLITE_ROW) {

        int txn_id       =               sqlite3_column_int   (stmt, 0);
        string date_time = (const char*) sqlite3_column_text  (stmt, 1);
        string type      = (const char*) sqlite3_column_text  (stmt, 2);
        double amount    =               sqlite3_column_double(stmt, 3);
        double balance   =               sqlite3_column_double(stmt, 4);

        Transaction t;
        t.transactionId = txn_id;
        t.dateTime      = date_time;
        t.type          = type;
        t.amount        = amount;
        t.balance       = balance;
        out.push_back(t);

        if (txn_id > lastTxnId)
            lastTxnId = txn_id;

    }
    sqlite3_finalize(stmt);
}

//=================DELETE TRANSACTIONS===============
void DatabaseManager::deleteTransactions(const string &accNo) {

    const char* sql = R"(
        DELETE FROM transactions WHERE account_number = ?;
    )";
    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);

    sqlite3_bind_text(stmt, 1, accNo.c_str(), -1, SQLITE_TRANSIENT);

    sqlite3_step(stmt);

    sqlite3_finalize(stmt);

}

//===================SAVE LOANS=======================
void DatabaseManager::saveLoans(const unordered_map<string, Loan> &loans) {

    if (!db_) return;
    sqlite3_exec(db_, "BEGIN;", nullptr, nullptr, nullptr);

    const char* sql = R"(
        INSERT OR REPLACE INTO loans
        (loan_id, account_number, loan_type, principal_amount, interest_rate,
        tenure_months, emi_amount, outstanding_amount, status, rejection_reason,
        application_date, disbursement_date, next_emi_date, payments_made)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);
    )";

    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);

    for(const auto& [loanId,loan]:loans) {
        sqlite3_bind_text   (stmt,  1,  loan.loanId.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text   (stmt,  2,  loan.accountNumber.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text   (stmt,  3,  loan.loanType.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_double (stmt,  4,  loan.principalAmount);
        sqlite3_bind_double (stmt,  5,  loan.interestRate);
        sqlite3_bind_int    (stmt,  6,  loan.tenureInMonths);
        sqlite3_bind_double (stmt,  7,  loan.emiAmount);
        sqlite3_bind_double (stmt,  8,  loan.outstandingAmount);
        sqlite3_bind_text   (stmt,  9,  loan.status.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text   (stmt, 10,  loan.rejectionReason.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text   (stmt, 11,  loan.applicationDate.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text   (stmt, 12,  loan.disbursementDate.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text   (stmt, 13,  loan.nextEmiDate.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int    (stmt, 14,  loan.paymentsMade);

        sqlite3_step(stmt);
        sqlite3_reset(stmt);
        sqlite3_clear_bindings(stmt);
    }

    sqlite3_finalize(stmt);
    sqlite3_exec(db_, "COMMIT;", nullptr, nullptr, nullptr);
}

//=================SAVE LOAN PAYMENT=========================
void DatabaseManager::saveLoanPayment(const string &loanId, const LoanPayment &p) {

    const char* sql = R"(
        INSERT INTO loan_payments
        (loan_id, payment_number, payment_date, emi_amount,
        principal_paid, interest_paid, outstanding_balance, remarks)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?);
    )";

    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    
    sqlite3_bind_text   (stmt, 1, loanId.c_str(),      -1, SQLITE_TRANSIENT);
    sqlite3_bind_int    (stmt, 2, p.paymentNumber);
    sqlite3_bind_text   (stmt, 3, p.paymentDate.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_double (stmt, 4, p.emiAmount);
    sqlite3_bind_double (stmt, 5, p.principalPaid);
    sqlite3_bind_double (stmt, 6, p.interestPaid);
    sqlite3_bind_double (stmt, 7, p.outstandingBalance);
    sqlite3_bind_text   (stmt, 8, p.remarks.c_str(), -1, SQLITE_TRANSIENT);

    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
} 

//==================LOAD LOANS=====================
void DatabaseManager::loadLoans(unordered_map<string, Loan>& loans, int& lastSeq) {
    loans.clear();
    lastSeq = 0;

    const char* sql = R"(
        SELECT loan_id, account_number, loan_type, principal_amount, interest_rate,
               tenure_months, emi_amount, outstanding_amount, status, rejection_reason,
               application_date, disbursement_date, next_emi_date, payments_made
        FROM loans;
    )";

    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);

    while (sqlite3_step(stmt) == SQLITE_ROW) {

        string loanId                      = (const char*)sqlite3_column_text(stmt, 0);
        string account_number              = (const char*)sqlite3_column_text(stmt, 1);
        string loanType                    = (const char*)sqlite3_column_text(stmt, 2);
        double principalAmount             =              sqlite3_column_double(stmt,3);
        double interestRate                =              sqlite3_column_double(stmt,4);
        int tenureInMonths                 =              sqlite3_column_int(stmt,5);
        double emiAmount                   =              sqlite3_column_double(stmt,6);
        double outstandingAmount           =              sqlite3_column_double(stmt,7);
        string status                      = (const char*)sqlite3_column_text(stmt, 8);
        string rejectionReason             = (const char*)sqlite3_column_text(stmt, 9);
        string applicationDate             = (const char*)sqlite3_column_text(stmt, 10);
        string disbursementDate            = (const char*)sqlite3_column_text(stmt, 11);
        string nextEmiDate                 = (const char*)sqlite3_column_text(stmt, 12);
        int paymentsMade                   =              sqlite3_column_int(stmt,13);

        // Step 2 — build the Loan struct
        Loan loan;
        loan.loanId = loanId;
        loan.accountNumber = account_number;
        loan.loanType = loanType;
        loan.principalAmount = principalAmount;
        loan.interestRate = interestRate;
        loan.tenureInMonths = tenureInMonths;
        loan.emiAmount = emiAmount;
        loan.outstandingAmount = outstandingAmount;
        loan.status = status;
        loan.rejectionReason = rejectionReason;
        loan.applicationDate = applicationDate;
        loan.disbursementDate = disbursementDate;
        loan.nextEmiDate = nextEmiDate;
        loan.paymentsMade = paymentsMade;

        const char* pSql = R"(
            SELECT payment_number, payment_date, emi_amount, principal_paid,
                   interest_paid, outstanding_balance, remarks
            FROM loan_payments
            WHERE loan_id = ?
            ORDER BY payment_number ASC;
        )";
        sqlite3_stmt* pStmt = nullptr;
        sqlite3_prepare_v2(db_, pSql, -1, &pStmt, nullptr);
        sqlite3_bind_text(pStmt, 1, loanId.c_str(), -1, SQLITE_TRANSIENT);

        while (sqlite3_step(pStmt) == SQLITE_ROW) {
            LoanPayment p;
            p.paymentNumber      = sqlite3_column_int(pStmt, 0);
            p.paymentDate        = (const char*) sqlite3_column_text(pStmt,1);
            p.emiAmount          = sqlite3_column_double(pStmt, 2);
            p.principalPaid      = sqlite3_column_double(pStmt, 3);
            p.interestPaid       = sqlite3_column_double(pStmt, 4);
            p.outstandingBalance = sqlite3_column_double(pStmt, 5);
            p.remarks            = (const char*) sqlite3_column_text(pStmt,6);
            loan.paymentHistory.push_back(p);
        }
        sqlite3_finalize(pStmt);

        if (loanId.length() > 4 && loanId.substr(0, 4) == "LOAN") {
                string seqPart = loanId.substr(4);
                try {
                    int seq = stoi(seqPart);
                    if (seq > lastSeq) {
                        lastSeq = seq;
                    }
                } catch (...) {
                    // Invalid format, skip
                }
            }

        loans.emplace(loanId, loan);
    }

    sqlite3_finalize(stmt);
}

//==================DELETE LOAN========================
void DatabaseManager::deleteLoan(const string& loanId){
    const char* sql = R"(
        DELETE FROM loans WHERE loan_id = ?;
    )";
    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);

    sqlite3_bind_text(stmt, 1, loanId.c_str(), -1, SQLITE_TRANSIENT);

    sqlite3_step(stmt);

    sqlite3_finalize(stmt);
}

//===============CREATE SESSION===================
string DatabaseManager::createSession(const string &accountNumber, int expiryMinutes){
    std::random_device rd;
    std::mt19937_64 gen(rd());
    std::uniform_int_distribution<uint64_t> dist;

    std::ostringstream oss;
    oss << std::hex << std::setw(16) << std::setfill('0') << dist(gen)
                    << std::setw(16) << std::setfill('0') << dist(gen);
    string token = oss.str();

    time_t now       = time(0);
    time_t expiresAt = now + (expiryMinutes * 60);

    const char* sql = R"(
        INSERT INTO sessions (token, account_number, created_at, expires_at)
            VALUES (?, ?, ?, ?);
    )";

    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);

    sqlite3_bind_text  (stmt, 1, token.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text  (stmt, 2, accountNumber.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64 (stmt, 3, now);
    sqlite3_bind_int64 (stmt, 4, expiresAt);

    sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    return token;
}

//===============VALIDATE SESSION======================
string DatabaseManager::validateSession(const string& token) {

    const char* sql = R"(
        SELECT account_number, expires_at
        FROM sessions
        WHERE token = ?;
    )";

    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);

    sqlite3_bind_text(stmt, 1, token.c_str(), -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) != SQLITE_ROW) {
        sqlite3_finalize(stmt);
        return "";
    }

    time_t now       = time(0);
    time_t expiresAt = sqlite3_column_int64(stmt, 1);

    if (now > expiresAt) {
        sqlite3_finalize(stmt);
        deleteSession(token);   // clean up expired token
        return "";
    }

    string accountNumber = (const char*)sqlite3_column_text(stmt, 0);
    sqlite3_finalize(stmt);
    return accountNumber;

}

//================DELETE SESSION=======================
void DatabaseManager::deleteSession(const string& token) {
    const char* sql = R"(
        DELETE FROM sessions WHERE token = ?;
    )";
    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);

    sqlite3_bind_text(stmt, 1, token.c_str(), -1, SQLITE_TRANSIENT);

    sqlite3_step(stmt);

    sqlite3_finalize(stmt);
}

//===================LOG AUDIT============================
void DatabaseManager::logAudit(const string& accountNumber,
                                const string& token,const string& action,
                                const string& details,const string& status) {

    const char* sql = R"(
        INSERT INTO audit_log
        (timestamp, account_number, token, action, details, status)
        VALUES (?, ?, ?, ?, ?, ?);
    )";

    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);

    sqlite3_bind_int64 (stmt, 1, time(0));
    sqlite3_bind_text  (stmt, 2, accountNumber.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text  (stmt, 3, token.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text  (stmt, 4, action.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text  (stmt, 5, details.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text  (stmt, 6, status.c_str(), -1, SQLITE_TRANSIENT);
    
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}                               

//======================CHECK RATE LIMIT===========================
bool DatabaseManager::checkRateLimit(const string& accountNumber, int maxAttempts, int windowSeconds) {

    const char* sql = R"(
        SELECT COUNT(*) FROM login_attempts
        WHERE account_number = ?
        AND attempt_time > ?;
    )";

    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);

    sqlite3_bind_text(stmt, 1, accountNumber.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt,2, time(0)-windowSeconds);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        int count = sqlite3_column_int(stmt, 0);
        sqlite3_finalize(stmt);
        return count >= maxAttempts; 
    }
    sqlite3_finalize(stmt);
    return false;
}

//=====================RECORD FAILED ATTEMPT======================
void DatabaseManager::recordFailedAttempt(const string& accountNumber){

    const char* sql = R"(
        INSERT INTO login_attempts (account_number, attempt_time)
        VALUES (?, ?);
    )";

    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);

    sqlite3_bind_text  (stmt, 1, accountNumber.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64 (stmt, 2, time(0));
    
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}

//===========================SAVE RD==============================
void DatabaseManager::saveRD(const RecurringDeposit &rd) {

    const char* sql = R"(
        INSERT OR REPLACE INTO recurring_deposits
        (rd_id, account_number, monthly_amount, tenure_months, interest_rate,
        start_date, next_debit_date, total_deposited, months_paid, maturity_amount, status)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);
    )";

    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);

    sqlite3_bind_text   (stmt,  1,  rd.rdId.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text   (stmt,  2,  rd.accountNumber.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_double (stmt,  3,  rd.monthlyAmount);
    sqlite3_bind_int    (stmt,  4,  rd.tenureMonths);
    sqlite3_bind_double (stmt,  5,  rd.interestRate);
    sqlite3_bind_text   (stmt,  6,  rd.startDate.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text   (stmt,  7,  rd.nextDebitDate.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_double (stmt,  8,  rd.totalDeposited);
    sqlite3_bind_int    (stmt,  9,  rd.monthsPaid);
    sqlite3_bind_double (stmt, 10,  rd.maturityAmount);
    sqlite3_bind_text   (stmt, 11,  rd.status.c_str(), -1, SQLITE_TRANSIENT);

    sqlite3_step(stmt);
    sqlite3_finalize(stmt);   

}

//===================LOAD RD========================
void DatabaseManager::loadRDs(vector<RecurringDeposit>& rds) {
    rds.clear();

    const char* sql = R"(
        SELECT rd_id, account_number, monthly_amount, tenure_months, interest_rate,
               start_date, next_debit_date, total_deposited, months_paid,
               maturity_amount, status
        FROM recurring_deposits;
    )";

    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        // read columns
        RecurringDeposit rd;
        rd.rdId           = (const char*)sqlite3_column_text  (stmt, 0);
        rd.accountNumber  = (const char*)sqlite3_column_text  (stmt, 1);
        rd.monthlyAmount  =              sqlite3_column_double(stmt, 2);
        rd.tenureMonths   =              sqlite3_column_int   (stmt, 3);
        rd.interestRate   =              sqlite3_column_double(stmt, 4);
        rd.startDate      = (const char*)sqlite3_column_text  (stmt, 5);
        rd.nextDebitDate  = (const char*)sqlite3_column_text  (stmt, 6);
        rd.totalDeposited =              sqlite3_column_double(stmt, 7);
        rd.monthsPaid     =              sqlite3_column_int   (stmt, 8);
        rd.maturityAmount =              sqlite3_column_double(stmt, 9);
        rd.status         = (const char*)sqlite3_column_text  (stmt, 10);
        rds.push_back(rd);
    }

    sqlite3_finalize(stmt);
}

//====================UPDATE RD======================
void DatabaseManager::updateRD(const RecurringDeposit& rd) {
    const char* sql = R"(
        UPDATE recurring_deposits
        SET next_debit_date = ?, total_deposited = ?,
            months_paid = ?, status = ?
        WHERE rd_id = ?;
    )";
    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    
    sqlite3_bind_text(stmt, 1, rd.nextDebitDate.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_double(stmt, 2, rd.totalDeposited);
    sqlite3_bind_int(stmt, 3, rd.monthsPaid);
    sqlite3_bind_text(stmt, 4, rd.status.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 5, rd.rdId.c_str(), -1, SQLITE_TRANSIENT);

    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}

//====================DELETE RD==============================
void DatabaseManager::deleteRD(const string& rdId) {
    
    const char* sql = R"(
        DELETE FROM recurring_deposits WHERE rd_id = ?;
    )";

    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);

    sqlite3_bind_text(stmt, 1, rdId.c_str(), -1, SQLITE_TRANSIENT);

    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}
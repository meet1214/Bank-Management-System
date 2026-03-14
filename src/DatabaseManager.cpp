#include "DatabaseManager.h"
#include "BankAccount.h"
#include "Logger.h"
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
}

//==========CLOSE FUNCTION===============
void DatabaseManager::close() {
    if (db_) {
        sqlite3_close(db_);
        db_ = nullptr;
    }
}

//==========SAVE ACCOUNTS===================
void DatabaseManager::saveAccounts(const unordered_map<string, BankAccount> &users){

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
        sqlite3_bind_double(stmt,  5,  user.getBalance());
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
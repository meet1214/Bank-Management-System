#include "BankAccount.h"
#include "BankExceptions.h"
#include "Sha256.h"
#include "Logger.h"

#include <algorithm>
#include <chrono>
#include <ctime>
#include "DatabaseManager.h"
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

using namespace std;

// ================= CONSTRUCTOR =================
BankAccount::BankAccount(const std::string& accNo,
                         const std::string& n,
                         const std::string& pinHash,
                         const std::string& salt,
                         double b,
                         const std::string& r,
                         bool locked,
                         const std::string& accType,
                         double depositLim,
                         double withdrawLim,
                         int    dailyTxnLim,
                         double dailyTransferLim)

    : accountNumber(accNo),
      name(n),
      pinHash(pinHash),
      salt(salt),
      balance(b),
      transactionHistory(),
      lastTransactionId(0),
      role(r),
      isLocked(locked),
      accountType(accType),
      depositLimit(depositLim),
      withdrawLimit(withdrawLim),
      dailyTxnLimit(dailyTxnLim),
      dailyTransferLimit(dailyTransferLim),
      dailyTxnCount(0),
      dailyTransferUsed(0.0),
      lastTxnDate("")
{
    if (accountType == "Savings") {
        interestRate = 4.0;
    }
    else if (accountType == "Current") {
        interestRate = 0.0;
    }
    else if (accountType == "Fixed Deposit") {
        interestRate = 7.0;
    }
    else {
        accountType = "Savings";
        interestRate = 4.0;
    }
}

// ================= GETTERS =================
string BankAccount::getName()           const { return name; }
string BankAccount::getAccountNumber()  const { return accountNumber; }
double BankAccount::getBalance()        const { return balance; }
string BankAccount::getPinHash()        const { return pinHash; }
string BankAccount::getSalt()           const { return salt; }
string BankAccount::getAccountType()    const { return accountType; }
double BankAccount::getInterestRate()   const { return interestRate;}

void BankAccount::setPinHash(const std::string& hash,
                              const std::string& newSalt) {
    pinHash = hash;
    salt    = newSalt;
}

//==================HELPER FUNCTIONS=================

// ===========HELPER: EXTRACT DATE FROM DATETIME STRING ===============
static string extractDateFromDateTime(const string& dateTime) {
    return dateTime.substr(0,10);
}
// ================= HELPER: CASE-INSENSITIVE PARTIAL MATCH =================
static bool containsIgnoreCase(const string& text, const string& search) {
    string textLower = text;
    string searchLower = search;
    
    // Convert both to lowercase
    transform(textLower.begin(), textLower.end(), textLower.begin(), ::tolower);
    transform(searchLower.begin(), searchLower.end(), searchLower.begin(), ::tolower);
    
    // Check if search is found in text
    return textLower.find(searchLower) != string::npos;
}

// ================= AUTHENTICATION =================
bool BankAccount::authenticatePin(int enteredPin) const {
    return verifyPin(enteredPin, pinHash, salt);
}

// ================= HELPER: GET CURRENT TIME =================
string BankAccount::getCurrentDateTime() const {
    auto now   = chrono::system_clock::now();
    time_t t   = chrono::system_clock::to_time_t(now);
    tm* lt     = localtime(&t);
    
    ostringstream timestamp;
    timestamp << (lt->tm_year + 1900);
    timestamp << "-";
    timestamp << setw(2) << setfill('0') << (lt->tm_mon + 1);
    timestamp << "-";
    timestamp << setw(2) << setfill('0') << (lt->tm_mday) << " ";
    timestamp << setw(2) << setfill('0') << (lt->tm_hour) << ":";
    timestamp << setw(2) << setfill('0') << (lt->tm_min) << ":";
    timestamp << setw(2) << setfill('0') << (lt->tm_sec);

    return timestamp.str();
}

// ================= HELPER: GET TODAY'S DATE (YYYY-MM-DD) =================
static string getTodayDate() {
    auto now = chrono::system_clock::now();
    time_t t = chrono::system_clock::to_time_t(now);
    tm* lt   = localtime(&t);
    ostringstream oss;
    oss << (1900 + lt->tm_year) << "-"
        << setw(2) << setfill('0') << (1 + lt->tm_mon) << "-"
        << setw(2) << setfill('0') << lt->tm_mday;
    return oss.str();
}

// ================= HELPER: RESET DAILY COUNTERS IF NEW DAY =================
void BankAccount::resetDailyCountersIfNeeded() {
    string today = getTodayDate();
    if (lastTxnDate != today) {
        dailyTxnCount    = 0;
        dailyTransferUsed = 0.0;
        lastTxnDate      = today;
    }
}

// ================= HELPER: PARSE TIME TO TIMESTAMP =================
static time_t parseDateTime(const string& dateTime) {
    struct tm t = {};
    
    t.tm_year  = stoi(dateTime.substr(0, 4)) - 1900;
    t.tm_mon   = stoi(dateTime.substr(5,2)) - 1;
    t.tm_mday  = stoi(dateTime.substr(8, 2));
    t.tm_hour  = stoi(dateTime.substr(11, 2));
    t.tm_min   = stoi(dateTime.substr(14, 2));
    t.tm_sec   = stoi(dateTime.substr(17, 2));
    t.tm_isdst = -1;
    
    return mktime(&t);
}

// ================= DEPOSIT =================
void BankAccount::depositMoney(double amount) {

    resetDailyCountersIfNeeded();

    // Limit checks
    if (amount > depositLimit) {
        throw DailyLimitExceededException("Deposit denied for " +
            accountNumber + ": exceeds limit of Rs." + to_string(depositLimit));
    }

    if (dailyTxnCount >= dailyTxnLimit) {
        throw DailyLimitExceededException("Deposit denied for " + 
            accountNumber + ": daily limit of " + to_string(dailyTxnLimit) + " reached."); 
    }

    balance += amount;
    ++lastTransactionId;
    ++dailyTxnCount;

    Transaction t;
    t.transactionId = lastTransactionId;
    t.dateTime      = getCurrentDateTime();
    t.type          = "Deposit";
    t.amount        = amount;
    t.balance       = balance;

    transactionHistory.push_back(t);
    saveTransactionToFile(t);
    Logger::getInstance().info("Deposit of Rs." + to_string(amount) + " on account " + accountNumber);
}

//=================CREDIT LOAN AMOUNT============
bool BankAccount::creditAmount(double amount, const std::string& type){
    resetDailyCountersIfNeeded();

    balance += amount;
    ++lastTransactionId;

    Transaction t;
    t.transactionId = lastTransactionId;
    t.dateTime      = getCurrentDateTime();
    t.type          = type;
    t.amount        = amount;
    t.balance       = balance;

    transactionHistory.push_back(t);
    saveTransactionToFile(t);
    Logger::getInstance().info("Loan amount of Rs." + to_string(amount) + " is credited on account " + accountNumber);
    return true;
}

//===============DEBIT LOAN AMOUNT==========================
bool BankAccount::debitAmount(double amount, const std::string& type){
    resetDailyCountersIfNeeded();

    balance -=amount;
    ++lastTransactionId;

    Transaction t;
    t.transactionId = lastTransactionId;
    t.dateTime      = getCurrentDateTime();
    t.type          = type;
    t.amount        = -amount;
    t.balance       = balance;

    transactionHistory.push_back(t);
    saveTransactionToFile(t);
    Logger::getInstance().info("An EMI of Rs." + to_string(amount) + " is debited from account " + accountNumber);
    return true;    
}

// ================= WITHDRAW =================
bool BankAccount::withdrawMoney(double amount) {

    resetDailyCountersIfNeeded();

    if (amount > withdrawLimit) {
        Logger::getInstance().warn("Withdrawal denied for " + accountNumber + 
            ": exceeds limit of Rs." + to_string(withdrawLimit)); 
        return false;
    }
    if (dailyTxnCount >= dailyTxnLimit) {
        Logger::getInstance().warn("Withdrawal denied for " + accountNumber + 
            ": daily limit of " + to_string(dailyTxnLimit) + " reached."); 
        return false;
    }
    if (amount > balance) {
        throw InsufficientFundsException("Insufficient balance in " + accountNumber);
    }

    balance -= amount;
    ++lastTransactionId;
    ++dailyTxnCount;

    Transaction t;
    t.transactionId = lastTransactionId;
    t.dateTime      = getCurrentDateTime();
    t.type          = "Withdraw";
    t.amount        = -amount;
    t.balance       = balance;

    transactionHistory.push_back(t);
    saveTransactionToFile(t);
    Logger::getInstance().info("Withdrawal of Rs." + to_string(amount) + " on account " + accountNumber);
    return true;
}

// ================= SAVE TRANSACTION TO FILE =================
void BankAccount::saveTransactionToFile(const Transaction& t) {
    DatabaseManager::saveTransaction(accountNumber, t);
}

// ================= LOAD TRANSACTIONS =================
void BankAccount::loadTransactionsFromFile() {
    DatabaseManager::loadTransactions(accountNumber,
                                      transactionHistory,
                                      lastTransactionId);
}

// ================= SHOW BALANCE =================
void BankAccount::showBalance() const {
    cout << "Current Balance: Rs."
         << fixed << setprecision(2) << balance << "\n";
}

// ================= TRANSFER =================
bool BankAccount::transferMoney(BankAccount& receiver, double amount) {

    resetDailyCountersIfNeeded();

    if (amount <= 0) {
        Logger::getInstance().warn("Invalid transfer amount." + accountNumber);
        return false;
    }
    if (amount > withdrawLimit) {
        throw DailyLimitExceededException("Transfer denied for " +
            accountNumber + ": exceeds withdrawal limit of Rs." + to_string(withdrawLimit));
    }
    
    if (amount > balance) {
        Logger::getInstance().warn("Insufficient balance in " + accountNumber);
        return false;
    }

    if (receiver.getLockStatus()) {
        throw AccountLockedException("Transfer denied from " + 
            accountNumber + " to frozen account " + receiver.getAccountNumber());
    }

    if (dailyTxnCount >= dailyTxnLimit) {
        Logger::getInstance().warn("Transfer denied for " + accountNumber + ": daily limit of " + 
            to_string(dailyTxnLimit) + " reached."); 
        return false;
    }

    if (dailyTransferUsed + amount > dailyTransferLimit) {
        throw DailyLimitExceededException("Transfer denied for " +
            accountNumber + ": daily transfer limit exceeded.");
    }

    string currentTime = getCurrentDateTime();

    // Deduct from sender
    balance -= amount;
    ++lastTransactionId;
    ++dailyTxnCount;
    dailyTransferUsed += amount;

    Transaction senderTxn;
    senderTxn.transactionId = lastTransactionId;
    senderTxn.dateTime      = currentTime;
    senderTxn.type          = "Transfer To " + receiver.getName() +
                               " (" + receiver.getAccountNumber() + ")";
    senderTxn.amount  = -amount;
    senderTxn.balance = balance;

    transactionHistory.push_back(senderTxn);
    saveTransactionToFile(senderTxn);

    // Credit receiver
    receiver.balance += amount;
    ++receiver.lastTransactionId;

    Transaction receiverTxn;
    receiverTxn.transactionId = receiver.lastTransactionId;
    receiverTxn.dateTime      = currentTime;
    receiverTxn.type          = "Received From " + name +
                                 " (" + accountNumber + ")";
    receiverTxn.amount  = amount;
    receiverTxn.balance = receiver.balance;

    receiver.transactionHistory.push_back(receiverTxn);
    receiver.saveTransactionToFile(receiverTxn);

    Logger::getInstance().info("Transfer Successful to " + receiver.accountNumber);
    return true;
}

// ================= SHOW LIMITS =================
void BankAccount::showLimits() const {
    cout << "\n========== ACCOUNT LIMITS ==========\n";
    cout << fixed << setprecision(2);
    cout << "Deposit limit (per txn):    Rs." << depositLimit       << "\n";
    cout << "Withdrawal limit (per txn): Rs." << withdrawLimit      << "\n";
    cout << "Daily transaction limit:    "    << dailyTxnLimit      << " txns\n";
    cout << "Daily transfer limit:       Rs." << dailyTransferLimit << "\n";
    cout << "\n--- Today's Usage ---\n";
    cout << "Transactions done:    " << dailyTxnCount
         << " / " << dailyTxnLimit << "\n";
    cout << "Transfer amount used: Rs." << dailyTransferUsed
         << " / Rs." << dailyTransferLimit << "\n";
    cout << "=====================================\n";
}

//=============SHOW TRANSACTION HISTORY================
void BankAccount::showTransactionHistory() const {
    cout << "\n================ TRANSACTION HISTORY ================\n\n";

    if (transactionHistory.empty()) {
        cout << "No transactions found.\n";
        return;
    }

    cout << left << setw(6)  << "S.No"
                 << setw(8)  << "TxnID"
                 << setw(25) << "Date & Time"
                 << setw(35) << "Type"
                 << setw(14) << "Amount"
                 << setw(14) << "Balance" << "\n";

    cout << string(102, '-') << "\n";

    int serial = 1;
    for (const auto& entry : transactionHistory) {
        cout << left << setw(6)  << serial++
                     << setw(8)  << entry.transactionId
                     << setw(25) << entry.dateTime
                     << setw(35) << entry.type;

        cout << fixed << setprecision(2)
             << "Rs." << setw(11) << entry.amount
             << "Rs." << setw(11) << entry.balance << "\n";
    }

    cout << string(102, '-') << "\n";
    cout << "Current Balance: Rs."
         << fixed << setprecision(2) << balance << "\n";
}
//===================ADD INTEREST=====================
void BankAccount::addInterest(double amount){
    balance += amount;

    Transaction t;
    ++lastTransactionId;
    t.transactionId = lastTransactionId;
    t.dateTime      = getCurrentDateTime();
    t.type          = "Interest";
    t.amount        = amount;
    t.balance       = balance;

    transactionHistory.push_back(t);
    saveTransactionToFile(t);
    Logger::getInstance().info("Interest of Rs." + 
        to_string(amount) + " added in the account " + accountNumber);

}

//=================CHANGE PIN==========================
bool BankAccount::changePin(int currentPin, int newPin) {
    if (!authenticatePin(currentPin)) {
        Logger::getInstance().warn("Invalid pin for " + accountNumber);
        return false;
    }

    string newSalt = generateSalt();
    string hash = hashPin(to_string(newPin), newSalt);
    setPinHash(hash,newSalt);
    Logger::getInstance().info("Pin changed successfully for " + accountNumber);
    return true;
}

//===============SHOW MINI STATEMENT=======================
void BankAccount::showMiniStatement() const {
    cout << "\n======= MINI STATEMENT (Last 5) =======\n\n";

    if (transactionHistory.empty()) {
        cout << "No transactions found.\n";
        return;
    }

    // print header ONCE here
    cout << left << setw(6)  << "S.No" 
                 << setw(8)  << "TxnID"
                 << setw(25) << "Date & Time"
                 << setw(35) << "Type"
                 << setw(14) << "Amount"
                 << setw(14) << "Balance" << "\n";
    cout << string(102, '-') << "\n";

    int start = max(0, (int)transactionHistory.size() - 5);
    int serial = 1;

    for (int i = start; i < (int)transactionHistory.size(); i++) {
        const Transaction& entry = transactionHistory[i];
        cout << left << setw(6)  << serial++
                     << setw(8)  << entry.transactionId
                     << setw(25) << entry.dateTime
                     << setw(35) << entry.type;

        cout << fixed << setprecision(2)
             << "Rs." << setw(11) << entry.amount
             << "Rs." << setw(11) << entry.balance << "\n";
    }

    cout << string(102, '-') << "\n";
    cout << "Current Balance: Rs." << fixed << setprecision(2) << balance << "\n";
}

//===============SHOW ACCOUNT SUMMARY===================
void BankAccount::showAccountSummary() const {
    double totalDeposited   = 0;
    double totalWithdrawn   = 0;
    double totalTransferred = 0;
    double totalInterest    = 0;

    for (const auto& t : transactionHistory) {
        if (t.type == "Deposit")        totalDeposited  += t.amount;
        else if (t.type == "Withdraw")  totalWithdrawn  += -(t.amount); 
        else if (t.type == "Interest")  totalInterest   += t.amount;
        else if (t.type.find("Transfer To") == 0)  totalTransferred += t.amount;
    }

    cout << "\n========== ACCOUNT SUMMARY ==========\n";
    cout << "Account Number : " << accountNumber << "\n";
    cout << "Account Holder : " << name << "\n";
    cout << "Account Type   : " << accountType << "\n";
    cout << "Interest Rate  : " << interestRate <<"\n";
    cout << "Account Status : " << (isLocked ? "Locked" : "Active") << "\n";
    cout << "Role           : " << role << "\n";
    cout << "-------------------------------------\n";
    cout << fixed << setprecision(2);
    cout << "Current Balance       : Rs." << balance          << "\n";
    cout << "Total Deposited       : Rs." << totalDeposited   << "\n";
    cout << "Total Withdrawn       : Rs." << totalWithdrawn   << "\n";
    cout << "Total Transferred     : Rs." << totalTransferred << "\n";
    cout << "Interest Earned       : Rs." << totalInterest    << "\n";
    cout << "-------------------------------------\n";
    cout << "Total Transactions    : " << transactionHistory.size() << "\n";
    cout << "Last Transaction Date : " << lastTxnDate << "\n";
}
void BankAccount::searchTransactionsByDate(const string& startDate, const string& endDate) const {
    
    cout << "Searching transaction between " << startDate << " to " << endDate << "\n";
    
    vector<Transaction> results;
    
    for (const Transaction& t : transactionHistory) {
        string transactionDate = extractDateFromDateTime(t.dateTime);
        if (transactionDate >= startDate && transactionDate <= endDate) {
            results.push_back(t);
        }
    }
    
    if (results.empty()) {
        cout << "No transactions found between " << startDate << " and " << endDate << "\n";
    }
    else {
        cout << left << setw(6)  << "S.No"
                     << setw(8)  << "TxnID"
                     << setw(25) << "Date & Time"
                     << setw(35) << "Type"
                     << setw(14) << "Amount"
                     << setw(14) << "Balance" << "\n";
        
        cout << string(102, '-') << "\n";
        
        int serial = 1;
        for(const auto& entry : results) {
            cout << left << setw(6)  << serial++
                         << setw(8)  << entry.transactionId
                         << setw(25) << entry.dateTime
                         << setw(35) << entry.type;
            
            cout << fixed << setprecision(2)
                 << "Rs." << setw(11) << entry.amount
                 << "Rs." << setw(11) << entry.balance << "\n";
        }
        
        cout << string(102, '-') << "\n";
        cout << "Found " << results.size() << " transaction(s) between " << startDate << " and " << endDate << "\n";
    }
}

void BankAccount::searchTransactionsByType(const string& type) const {
    
    cout << "\nSearching transactions of type: " << type << "\n\n";
    
    vector<Transaction> results;
    
    for (const Transaction& t : transactionHistory) {
        bool match = false;
        
        
        if (type == "Transfer") {
            if (containsIgnoreCase(t.type, "Transfer To") || 
                containsIgnoreCase(t.type, "Received From")) {
                match = true;
            }
        }
        
        else {
            if (containsIgnoreCase(t.type, type)) {
                match = true;
            }
        }
        
        if (match) {
            results.push_back(t);
        }
    }
    
    
    if (results.empty()) {
        cout << "No transactions found for type: " << type << "\n";
    }
    else {
        cout << left << setw(6)  << "S.No"
                     << setw(8)  << "TxnID"
                     << setw(25) << "Date & Time"
                     << setw(35) << "Type"
                     << setw(14) << "Amount"
                     << setw(14) << "Balance" << "\n";
        
        cout << string(102, '-') << "\n";
        
        int serial = 1;
        for(const auto& entry : results) {
            cout << left << setw(6)  << serial++
                         << setw(8)  << entry.transactionId
                         << setw(25) << entry.dateTime
                         << setw(35) << entry.type;
            
            cout << fixed << setprecision(2)
                 << "Rs." << setw(11) << entry.amount
                 << "Rs." << setw(11) << entry.balance << "\n";
        }
        
        cout << string(102, '-') << "\n";
        cout << "Found " << results.size() << " transaction(s) of type: " << type << "\n";
    }
}

//================= CHECK FOR SUSPICIOUS ACTIVITY =================
void BankAccount::checkForSuspiciousActivity() const {
    
    cout << "\n========== FRAUD DETECTION REPORT ==========\n\n";
    
    if (transactionHistory.empty()) {
        cout << "No transactions to analyze.\n";
        return;
    }
    
    vector<Transaction> suspiciousTransactions;
    
    // Analyze each transaction
    for (size_t i = 0; i < transactionHistory.size(); i++) {
        Transaction currentTxn = transactionHistory[i];
        bool isSuspicious = false;
        string reason = "";
        
        // RULE 1: Large Transaction (> 70% of balance BEFORE transaction)
        double balanceBeforeTxn;
        if (i == 0) {
            balanceBeforeTxn = currentTxn.balance - currentTxn.amount;
        } else {
            balanceBeforeTxn = transactionHistory[i-1].balance;
        }
        
        if (balanceBeforeTxn > 0) {
            double percentOfBalance = (abs(currentTxn.amount) / balanceBeforeTxn) * 100;
            
            if (percentOfBalance > 70 && abs(currentTxn.amount) > 10000) {
                isSuspicious = true;
                reason += "Large transaction (" + to_string((int)percentOfBalance) + 
                          "% of balance); ";
            }
        }
        
        // RULE 2: Rapid Transaction (< 60 seconds from previous)
        if (i > 0) {
            time_t currentTime = parseDateTime(currentTxn.dateTime);
            time_t previousTime = parseDateTime(transactionHistory[i-1].dateTime);
            
            double timeDiff = difftime(currentTime, previousTime);
            
            if (timeDiff < 60 && timeDiff >= 0) {
                isSuspicious = true;
                reason += "Rapid transaction (" + to_string((int)timeDiff) + 
                          " seconds after previous); ";
            }
        }
        
        // RULE 3: Multiple transactions in short time (3+ within 5 minutes)
        if (i >= 2) {
            time_t currentTime = parseDateTime(currentTxn.dateTime);
            time_t twoTxnAgo = parseDateTime(transactionHistory[i-2].dateTime);
            
            double timeDiff = difftime(currentTime, twoTxnAgo);
            
            if (timeDiff < 300) {  // 300 seconds = 5 minutes
                isSuspicious = true;
                reason += "Multiple rapid transactions (3 in " + 
                          to_string((int)(timeDiff/60)) + " minutes); ";
            }
        }
        
        // RULE 4: Transaction near limit (within 5% of limit)
        if (currentTxn.type == "Deposit") {
            double percentOfLimit = (currentTxn.amount / depositLimit) * 100;
            if (percentOfLimit > 95) {
                isSuspicious = true;
                reason += "Near deposit limit (" + to_string((int)percentOfLimit) + 
                          "% of limit); ";
            }
        }
        else if (currentTxn.type == "Withdraw") {
            double percentOfLimit = (abs(currentTxn.amount) / withdrawLimit) * 100;
            if (percentOfLimit > 95) {
                isSuspicious = true;
                reason += "Near withdrawal limit (" + to_string((int)percentOfLimit) + 
                          "% of limit); ";
            }
        }
        
        if (isSuspicious) {
            currentTxn.flaggedSuspicious = true;
            currentTxn.suspiciousReason = reason;
            suspiciousTransactions.push_back(currentTxn);
        }
    }
    
    // Display results
    if (suspiciousTransactions.empty()) {
        cout << "✓ No suspicious activity detected.\n";
        cout << "All " << transactionHistory.size() << " transactions appear normal.\n";
    }
    else {
        cout << "WARNING: " << suspiciousTransactions.size() 
             << " suspicious transaction(s) detected!\n\n";
        
        cout << left << setw(6)  << "TxnID"
                     << setw(25) << "Date & Time"
                     << setw(20) << "Type"
                     << setw(12) << "Amount"
                     << "Reason" << "\n";
        cout << string(100, '-') << "\n";
        
        for (const auto& txn : suspiciousTransactions) {
            cout << left << setw(6)  << txn.transactionId
                         << setw(25) << txn.dateTime
                         << setw(20) << txn.type.substr(0, 18);
            
            cout << fixed << setprecision(2)
                 << "Rs." << setw(9) << abs(txn.amount)
                 << txn.suspiciousReason << "\n";
        }
        
        cout << string(100, '-') << "\n";
        cout << "\n These transactions have been flagged for review.\n";
        cout << "Please verify these activities with the account holder.\n";
    }
    
    cout << "\n============================================\n";
}

//================= SHOW SORTED TRANSACTIONS BY AMOUNT =================
void BankAccount::showSortedTransactionsByAmount() const{
    cout <<"\n============ TRANSACTION SORTED BY AMOUNT==============\n";

    if(transactionHistory.empty()){
        cout << "No transaction found.\n";
        return;
    }

    vector<Transaction> sortedTxns = transactionHistory;

    sort(sortedTxns.begin(),sortedTxns.end(),
    [](const Transaction& a, Transaction& b){
        return a.amount < b.amount;
    });

    cout << left << setw(6)  << "S.No"
                 << setw(8)  << "TxnID"
                 << setw(25) << "Date & Time"
                 << setw(35) << "Type"
                 << setw(14) << "Amount"
                 << setw(14) << "Balance" << "\n";
    
    cout << string(102, '-') << "\n";
    
    int serial = 1;
    for(const auto& entry : sortedTxns) {
        cout << left << setw(6)  << serial++
                     << setw(8)  << entry.transactionId
                     << setw(25) << entry.dateTime
                     << setw(35) << entry.type;
        
        cout << fixed << setprecision(2)
             << "Rs." << setw(11) << entry.amount
             << "Rs." << setw(11) << entry.balance << "\n";
    }
    
    cout << string(102, '-') << "\n";
    cout << "Total transactions: " << sortedTxns.size() << "\n";
    cout << "\nNote: Negative amounts are withdrawals/transfers out.\n";
}

//================= BINARY SEARCH TRANSACTIONS BY AMOUNT =================
vector<Transaction> BankAccount::binarySearchTransactionsByAmount(double targetAmount) const {
    
    vector<Transaction> results;
    
    if (transactionHistory.empty()) {
        return results;
    }
    
    // Create sorted copy
    vector<Transaction> sortedTxns = transactionHistory;
    sort(sortedTxns.begin(), sortedTxns.end(),
         [](const Transaction& a, const Transaction& b) {
             return a.amount < b.amount;
         });
    
    
    int left = 0;
    int right = sortedTxns.size() - 1;
    int foundIndex = -1;
    
    while (left <= right) {
        int mid = left + (right - left) / 2;
        
        
        double diff = abs(sortedTxns[mid].amount - targetAmount);
        
        if (diff < 0.01) {  
            foundIndex = mid;
            break;
        }
        else if (sortedTxns[mid].amount < targetAmount) {
            left = mid + 1;  
        }
        else {
            right = mid - 1;  
        }
    }
    
    
    if (foundIndex != -1) {
        
        
        results.push_back(sortedTxns[foundIndex]);
        
        
        int leftIdx = foundIndex - 1;
        while (leftIdx >= 0 && abs(sortedTxns[leftIdx].amount - targetAmount) < 0.01) {
            results.insert(results.begin(), sortedTxns[leftIdx]);
            leftIdx--;
        }
        
        
        int rightIdx = foundIndex + 1;
        while (rightIdx < (int)sortedTxns.size() && 
               abs(sortedTxns[rightIdx].amount - targetAmount) < 0.01) {
            results.push_back(sortedTxns[rightIdx]);
            rightIdx++;
        }
    }
    
    return results;
}

//================= INTERACTIVE AMOUNT SEARCH =================
void BankAccount::searchTransactionByAmountInteractive() const {
    
    cout << "\n===== SEARCH TRANSACTION BY AMOUNT =====\n\n";
    
    
    cout << "Here are your transactions sorted by amount:\n";
    showSortedTransactionsByAmount();
    
    cout << "\nEnter the amount to search for: Rs.";
    double targetAmount;
    cin >> targetAmount;
    cin.ignore(1000, '\n');
    
    cout << "\nSearching for transactions with amount Rs." 
         << fixed << setprecision(2) << targetAmount << "...\n";
    
    
    vector<Transaction> results = binarySearchTransactionsByAmount(targetAmount);
    
    if (results.empty()) {
        cout << "\n No transactions found with amount Rs." << targetAmount << "\n";
        cout << "Binary search completed - amount not found in transaction history.\n";
    }
    else {
        cout << "\n Found " << results.size() << " transaction(s) with amount Rs." 
             << targetAmount << "\n\n";
        
        cout << left << setw(8)  << "TxnID"
                     << setw(25) << "Date & Time"
                     << setw(35) << "Type"
                     << setw(14) << "Amount"
                     << setw(14) << "Balance" << "\n";
        
        cout << string(96, '-') << "\n";
        
        for(const auto& txn : results) {
            cout << left << setw(8)  << txn.transactionId
                         << setw(25) << txn.dateTime
                         << setw(35) << txn.type;
            
            cout << fixed << setprecision(2)
                 << "Rs." << setw(11) << txn.amount
                 << "Rs." << setw(11) << txn.balance << "\n";
        }
        
        cout << string(96, '-') << "\n";
        cout << "Binary search successful!\n";
    }
}

//======================MONTHLY STATEMENT========================
void BankAccount::showMonthlyStatement(int month, int year) const {

    vector<Transaction> monthTxns;
    for(const auto& t:transactionHistory) {
        string date  =  extractDateFromDateTime(t.dateTime);
        int txnYear  =  stoi(date.substr(0,4));
        int txnMonth =  stoi(date.substr(5,2));
        if(txnYear == year && txnMonth == month) {
            monthTxns.push_back(t);
        }

    }

    if(monthTxns.empty()){
        cout << "No transaction found.\n";
        return;
    }

    double openingBalance = monthTxns.front().balance - monthTxns.front().amount;

    double totalDeposited    = 0;
    double totalWithdrawn    = 0;
    double totalTransferred  = 0;

    for(const auto& t: monthTxns){
        if(t.type == "Deposit"){
            totalDeposited += t.amount;
        }
        else if (t.type == "Withdraw") {
            totalWithdrawn += -(t.amount);
        }
        else if (t.type.find("Transfer To") == 0) {
            totalTransferred += -(t.amount);
        }
    }

    cout << "\n========== MONTHLY STATEMENT ==========\n";
    cout << "Account : " << accountNumber << "\n";
    cout << "Name    : " << name << "\n";
    cout << "Period  : " << month << "/" << year << "\n";
    cout << "Opening Balance: Rs." << fixed << setprecision(2) << openingBalance << "\n";
    cout << string(102, '-') << "\n";

    int serial = 1;
    for (const auto& entry : monthTxns) {
        cout << left << setw(6)  << serial++
                     << setw(8)  << entry.transactionId
                     << setw(25) << entry.dateTime
                     << setw(35) << entry.type;

        cout << fixed << setprecision(2)
             << "Rs." << setw(11) << entry.amount
             << "Rs." << setw(11) << entry.balance << "\n";
    }

    cout << string(102, '-') << "\n";
    cout << "Closing Balance : Rs." << monthTxns.back().balance << "\n";
    cout << "Total Deposited : Rs." << totalDeposited << "\n";
    cout << "Total Withdrawn : Rs." << totalWithdrawn << "\n";
    cout << "Total Transferred: Rs." << totalTransferred << "\n";

}

//===============SPENDING PATTERN=========================
void BankAccount::showSpendingPatterns(int year) const {

    if (transactionHistory.empty()) {
        cout << "No transactions found.\n";
        return;
    }

    double totalDeposited   = 0;
    double totalWithdrawn   = 0;
    double totalTransferred = 0;
    double totalEMI         = 0;
    double totalInterest    = 0;

    for (const auto& t : transactionHistory) {

        if (year != 0) {
            string date = extractDateFromDateTime(t.dateTime);
            int txnYear = stoi(date.substr(0, 4));
            if (txnYear != year) continue;
        }

        if(t.type == "Deposit"){
            totalDeposited += t.amount;
        }

        else if (t.type == "Withdraw") {
            totalWithdrawn += -(t.amount);
        }

        else if (t.type.find("Transfer To") == 0) {
            totalTransferred += -(t.amount);
        }
        
        else if ((t.type == "EMI Payment" || t.type == "Loan Early Closure")) {
            totalEMI += -(t.amount);
        }
       
        else if (t.type == "Interest") {
            totalInterest += t.amount;
        }
    }

    double totalOutflow = totalWithdrawn + totalTransferred + totalEMI;
    double totalInflow  = totalDeposited + totalInterest;

    string period = (year == 0) ? "All Time" : to_string(year);

    cout << "\n========== SPENDING PATTERNS ==========\n";
    cout << "Account : " << accountNumber << "\n";
    cout << "Period  : " << period << "\n\n";

    cout << left << setw(25) << "Category"
                 << setw(16) << "Total (Rs.)"
                 << setw(12) << "% of Outflow" << "\n";
    cout << string(53, '-') << "\n";

   
    double pctWith  = (totalOutflow > 0) ? (totalWithdrawn  / totalOutflow) * 100 : 0;
    double pctTrans = (totalOutflow > 0) ? (totalTransferred / totalOutflow) * 100 : 0;
    double pctEMI   = (totalOutflow > 0) ? (totalEMI        / totalOutflow) * 100 : 0;
    
    //outflow
    cout << fixed << setprecision(2);
    cout << left << setw(25) << "Withdrawals"    << setw(16) << totalWithdrawn    << pctWith  << "%\n";
    cout << left << setw(25) << "Transfers Out"  << setw(16) << totalTransferred  << pctTrans << "%\n";
    cout << left << setw(25) << "EMI Payments"   << setw(16) << totalEMI          << pctEMI   << "%\n";

    //inflow
    cout << "\n--- Inflow ---\n";
    cout << left << setw(25) << "Deposits"  << "Rs." << totalDeposited  << "\n";
    cout << left << setw(25) << "Interest"  << "Rs." << totalInterest   << "\n";

    cout << string(53, '-') << "\n";
    cout << fixed << setprecision(2);
    cout << "\nTotal Outflow : Rs." << totalOutflow << "\n";
    cout << "Total Inflow  : Rs." << totalInflow   << "\n";
    double net = totalInflow - totalOutflow;
    cout << "Net           : " << (net >= 0 ? "+" : "") << "Rs." << net << "\n";
    cout << "========================================\n";
}

//==============INTEREST SUMMARY===============
void BankAccount::showInterestSummary() const {

    double totalInterest = 0;
    vector<Transaction> interestTxns;

    for (const auto& t : transactionHistory) {
        if (t.type == "Interest") {
            interestTxns.push_back(t);
            totalInterest += t.amount;
        }
    }

    if(interestTxns.empty()) {
        cout << "No interest credited yet.\n";
        return;
    }


    cout << "\n========== INTEREST SUMMARY ==========\n";
    cout << "Account : " << accountNumber << "\n";
    cout << "Name    : " << name << "\n\n";

    // S.No | TxnID | Date & Time | Amount | Balance
    cout << left << setw(6)  << "S.No"
                 << setw(8)  << "TxnID"
                 << setw(25) << "Date & Time"
                 << setw(35) << "Type"
                 << setw(14) << "Amount"
                 << setw(14) << "Balance" << "\n";
    
    cout << string(102, '-') << "\n";

    int serial = 1;
    for(const auto& txn : interestTxns) {
            cout << left << setw(6)  << serial++
                         << setw(8)  << txn.transactionId
                         << setw(25) << txn.dateTime
                         << setw(35) << txn.type;
            
            cout << fixed << setprecision(2)
                 << "Rs." << setw(11) << txn.amount
                 << "Rs." << setw(11) << txn.balance << "\n";
        }

    // Step 6 — print total
    cout << string(102, '-') << "\n";
    cout << "Total Interest Earned: Rs." 
         << fixed << setprecision(2) << totalInterest << "\n";
    cout << "======================================\n";
}
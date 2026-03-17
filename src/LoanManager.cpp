#include "LoanManager.h"
#include "BankAccount.h"
#include "Loan.h"
#include "DatabaseManager.h"

#include <cmath>
#include <iomanip>
#include <iostream>


using namespace std;

// ================= PORTABLE DATE HELPER =================
// Parses "YYYY-MM-DD" into a tm struct without using strptime
static struct tm parseDate(const string& date) {
    struct tm t = {};
    // date must be exactly "YYYY-MM-DD"
    if (date.size() != 10 || date[4] != '-' || date[7] != '-')
        return t;
    try {
        t.tm_year = stoi(date.substr(0, 4)) - 1900;
        t.tm_mon  = stoi(date.substr(5, 2)) - 1;
        t.tm_mday = stoi(date.substr(8, 2));
        t.tm_isdst = -1;
    } catch (...) {}
    return t;
}

//==========STATIC MEMBER INITIALIZATION=================
int LoanManager::lastLoanSequenceNumber = 0;

//==========CONSTRUCTOR=================
//interest rates
LoanManager::LoanManager(){
    interestRates["Personal"]            = 12.0;
    interestRates["Home"]                = 8.5;
    interestRates["Auto"]                = 10.0;
    interestRates["Education"]           = 9.0;
    foreclosurePenaltyRates["Personal"]  = 4.0;
    foreclosurePenaltyRates["Home"]      = 2.0;
    foreclosurePenaltyRates["Auto"]      = 3.0;
    foreclosurePenaltyRates["Education"] = 1.0;
    loadLoans();
}

//==========DESTRUCTOR=================
LoanManager::~LoanManager() {
    saveLoans();
}

//==========HELPER METHODS=================
string LoanManager::generateLoanNumber() {
    ++lastLoanSequenceNumber;
    string seq = to_string(lastLoanSequenceNumber);
    // Pad to 8 digits: "LOAN00000001"
    return "LOAN" + string(8 - seq.length(), '0') + seq;
}

double LoanManager::getMonthlyInterestRate(double annualRate) {
    return annualRate / 12.0 / 100.0;
}

int LoanManager::getDaysBetweenDates(const string& startDate, const string& endDate) {
    struct tm startTm = parseDate(startDate);
    struct tm endTm   = parseDate(endDate);

    time_t startEpoch = mktime(&startTm);
    time_t endEpoch   = mktime(&endTm);

    if (startEpoch == -1 || endEpoch == -1) return 0;

    double diffSeconds = difftime(endEpoch, startEpoch);
    return static_cast<int>(diffSeconds / (24 * 60 * 60));
}

string LoanManager::addMonthsToDate(const string& date, int monthsToAdd) {
    struct tm dateTm = parseDate(date);

    dateTm.tm_mon += monthsToAdd;
    dateTm.tm_year += dateTm.tm_mon / 12;
    dateTm.tm_mon  %= 12;
    if (dateTm.tm_mon < 0) {
        dateTm.tm_year -= 1;
        dateTm.tm_mon  += 12;
    }
    dateTm.tm_isdst = -1;

    time_t newEpoch = mktime(&dateTm);
    if (newEpoch == -1) return date;

    char buffer[11];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d", &dateTm);
    return string(buffer);
}

string LoanManager::getTodayDate() {
    time_t now = time(0);
    struct tm* timeinfo = localtime(&now);
    char buffer[11];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d", timeinfo);
    return string(buffer);
}

Loan* LoanManager::getLoanById(const std::string& loanId){
    auto it = loans.find(loanId);
    if (it == loans.end()) return nullptr;
    return &it->second;
}

//============CALCULATE EMI===============
double LoanManager::calculateEMI(double principal, double annualRate, int months) const {
    if (annualRate == 0 || annualRate < 0.01) {
        return principal / months;
    }
    
    double r = getMonthlyInterestRate(annualRate);
    double power = pow(1 + r, months);
    double emi = (principal * r * power) / (power - 1);
    
    return emi;
}

//==================CHECK ELIGIBILITY FOR LOAN=========================
bool LoanManager::checkEligibility(const BankAccount& account, const std::string& loanType, double amount) const {
    
    // CHECK MINIMUM BALANCE
    if (account.getBalance() < 10000) {
        return false;
    }
    
    // CHECK LOAN TYPE LIMIT
    if (loanType == "Personal" && amount > PERSONAL_LOAN_MAX) {
        return false;
    }
    if (loanType == "Home" && amount > HOME_LOAN_MAX) {
        return false;
    }
    if (loanType == "Auto" && amount > AUTO_LOAN_MAX) {
        return false;
    }
    if (loanType == "Education" && amount > EDUCATION_LOAN_MAX) {
        return false;
    }
    
    // ESTIMATE MONTHLY INCOME
    double totalDeposited = 0.0;

    vector<Transaction> history = account.getTransactionHistory();

    if (history.empty()) return false;

    string firstDate = history.front().dateTime.substr(0, 10);
    string lastDate  = history.back().dateTime.substr(0, 10);

    int months = getDaysBetweenDates(firstDate, lastDate)/30;

    if (months < 1) months = 1;

    double monthlyIncome = totalDeposited / months;

    // ELIGIBILITY MULTIPLIER PER LOAN TYPE
    double maxEligible = 0;

    if      (loanType == "Personal")  maxEligible = monthlyIncome * 50;
    else if (loanType == "Home")      maxEligible = monthlyIncome * 300;
    else if (loanType == "Auto")      maxEligible = monthlyIncome * 100;
    else if (loanType == "Education") maxEligible = monthlyIncome * 150;
    else return false;  // Unknown loan type
    
    if (amount > maxEligible) {
        return false;
    }

    // CHECK FOR EXISTING ACTIVE LOANS 
    int active_count = 0;
    
    for (const auto& [loanId, loan] : loans) {
        if (loan.accountNumber == account.getAccountNumber()) {
            if (loan.status == "ACTIVE") {
                active_count++;
            }
        }
    }
    
    if (active_count >= 2) {
        return false;
    }
    
    // IF ALL CHECKS PASS
    return true;
}

//======================APPLY FOR LOAN===================
string LoanManager::applyForLoan(BankAccount& account, const std::string& loanType,double amount, int tenureMonths) {
    if (interestRates.find(loanType) == interestRates.end()) {
        std::cout << "Invalid choice!\n";
        return "";
    }

    if(!checkEligibility(account,loanType,amount)){
        std::cout << "Not eligible for this loan.\n";
        return "";
    }

    double interestRate = interestRates[loanType];
    
    Loan loan;
    loan.loanId = generateLoanNumber();
    loan.accountNumber = account.getAccountNumber();
    loan.loanType = loanType;
    loan.principalAmount = amount;
    loan.tenureInMonths = tenureMonths;
    loan.interestRate = interestRate;
    loan.emiAmount = calculateEMI(amount,interestRate,tenureMonths);
    loan.outstandingAmount = amount;
    loan.status = "PENDING";
    loan.applicationDate = getTodayDate();
    loan.disbursementDate = "";
    loan.nextEmiDate = "";
    loan.paymentsMade = 0;

    loans.emplace(loan.loanId,loan);
    saveLoans();
    return loan.loanId;
}

//================APPROVE LOAN========================
bool LoanManager::approveLoan(const std::string& loanId) {
    auto it = loans.find(loanId);

    if(it == loans.end()) return false;

    if (it->second.status != "PENDING") {
        cout << "Loan cannot be approved. Current status: " << it->second.status << "\n";
        return false;
    }

    it->second.status = "APPROVED";
    saveLoans();
    return true;

}

//===============REJECT LOAN========================
bool LoanManager::rejectLoan(const std::string& loanId, const std::string& reason) {
    auto it = loans.find(loanId);

    if(it == loans.end()) return false;

    if (it->second.status != "PENDING") {
        cout << "Loan cannot be rejected. Current status: " << it->second.status << "\n";
        return false;
    }

    it->second.status = "REJECTED";
    it->second.rejectionReason = reason;
    saveLoans();
    return true;

}

//===================DISBURSE LOAN========================
bool LoanManager::disburseLoan(const std::string& loanId, BankAccount& account) {
    auto it = loans.find(loanId);

    if(it == loans.end()) return false;

    if(it->second.status != "APPROVED") return false;

    if(!account.creditAmount(it->second.principalAmount,"Loan Disbursement")){
        return false;
    }

    it->second.status = "ACTIVE";
    it->second.disbursementDate = getTodayDate();
    it->second.nextEmiDate = addMonthsToDate(getTodayDate(), 1);

    saveLoans();
    return true;

}

//===================MAKE EMI PAYMENT=====================
bool LoanManager::makeEMIPayment(const std::string& loanId, BankAccount& account) {
    auto it = loans.find(loanId);

    if(it == loans.end()) return false;

    if(it->second.status != "ACTIVE"){ 
        cout << "Cannot pay EMI as the loan is not active";
        return false;
    }

    double interestPaid = it->second.outstandingAmount * getMonthlyInterestRate(it->second.interestRate);
    double principalPaid = it->second.emiAmount - interestPaid;

    if(it->second.emiAmount > account.getBalance()){
        cout << "Insufficient Balance!\n";
        it->second.outstandingAmount += 0.05 * it->second.outstandingAmount;
        LoanPayment failed;
        failed.paymentNumber     = it->second.paymentsMade + 1;
        failed.paymentDate       = getTodayDate();
        failed.emiAmount         = it->second.emiAmount;
        failed.principalPaid     = 0;
        failed.interestPaid      = 0;
        failed.outstandingBalance = it->second.outstandingAmount;
        failed.remarks           = "FAILED - Insufficient balance. 5% penalty applied.";
        it->second.paymentHistory.push_back(failed);
        DatabaseManager::saveLoanPayment(loanId, failed);
        saveLoans();
        return false;
    }

    account.debitAmount(it->second.emiAmount,"EMI Payment");

    it->second.outstandingAmount -= principalPaid;
    it->second.paymentsMade++;
    it->second.nextEmiDate = addMonthsToDate(getTodayDate(), 1);
    
    LoanPayment payment;
    payment.paymentNumber     = it->second.paymentsMade;
    payment.paymentDate       = getTodayDate();
    payment.emiAmount         = it->second.emiAmount;
    payment.principalPaid     = principalPaid;
    payment.interestPaid      = interestPaid;
    payment.outstandingBalance = it->second.outstandingAmount;
    payment.remarks           = "SUCCESS";
    it->second.paymentHistory.push_back(payment);
    
    if(it->second.paymentsMade == it->second.tenureInMonths){
        it->second.status = "PAID";
        it->second.nextEmiDate = "";
    }
    DatabaseManager::saveLoanPayment(loanId, payment);
    saveLoans();
    return true;
}

//==============CALCULATE EARLY CLOSURE AMOUNT========================
double LoanManager::calculateEarlyClosureAmount(const std::string& loanId) const{
    auto it = loans.find(loanId);

    if(it == loans.end()) return 0;

    if(it->second.status != "ACTIVE") return 0;

    auto rateIt = foreclosurePenaltyRates.find(it->second.loanType);
    if (rateIt == foreclosurePenaltyRates.end()) return 0;

    double penalty_rate = rateIt->second;

    double penalty = it->second.outstandingAmount * (penalty_rate/100);

    return it->second.outstandingAmount + penalty;
}

//========================CLOSE ON EARLY==========================
bool LoanManager::closeLoanEarly(const std::string& loanId, BankAccount& account) {
    auto it = loans.find(loanId);

    if(it == loans.end()) return false;

    if(it->second.status != "ACTIVE") return false;

    double due = calculateEarlyClosureAmount(loanId);

    if(account.getBalance() < due) return false;

    account.debitAmount(due,"Loan Early Closure");

    LoanPayment payment;
    payment.paymentNumber     = it->second.paymentsMade;
    payment.paymentDate       = getTodayDate();
    payment.emiAmount         = it->second.emiAmount;
    payment.principalPaid     = it->second.principalAmount;
    payment.interestPaid      = 0.0;
    payment.outstandingBalance = it->second.outstandingAmount;
    payment.remarks           = "EARLY CLOSURE";
    it->second.paymentHistory.push_back(payment);

    it->second.status          = "CLOSED";
    it->second.outstandingAmount = 0.0;
    it->second.nextEmiDate     = "";

    DatabaseManager::saveLoanPayment(loanId, payment);
    saveLoans();
    return true;
    
}
//===============PROCESS EMI AUTO DEBITS=============================
void LoanManager::processEMIAutoDebits(BankAccount& account,
                                        AccountManager& manager) {
    string today = getTodayDate();

    for (auto& [loanId, loan] : loans) {

        if (loan.accountNumber != account.getAccountNumber()) continue;

        if (loan.status != "ACTIVE") continue;

        if (today < loan.nextEmiDate) continue;

        while (today >= loan.nextEmiDate &&
               loan.status == "ACTIVE") {

            cout << "\n[AUTO-DEBIT] Processing EMI for loan "
                 << loanId << "...\n";

            bool success = makeEMIPayment(loanId, account);

            if (!success) {
                cout << "[AUTO-DEBIT] EMI failed for " << loanId
                     << " — insufficient balance.\n";
                break;  
            }
        }
    }
}

//===================VIEW OPERATIONS==============================

//=====================LOAN DETAILS VIEW=========================
void LoanManager::viewLoanDetails(const std::string& loanId) const {
    auto it = loans.find(loanId);
    if (it == loans.end()) {
        cout << "Loan not found.\n";
        return;
    }
    cout << "\n========== LOAN DETAILS ==========\n";
    cout << "Loan ID          : " << loanId << "\n";
    cout << "Account number   : " << it->second.accountNumber << "\n";
    cout << "Loan Type        : " << it->second.loanType << "\n";
    cout << "Principal Amount : Rs." << fixed << setprecision(2) << it->second.principalAmount << "\n";
    cout << "Interest Rate    : " << it->second.interestRate << "%\n";
    cout << "Tenure           : " << it->second.tenureInMonths << "months\n";
    cout << "EMI Amount       : Rs." << it->second.emiAmount << "\n";
    cout << "Status           : " << it->second.status <<"\n";
    cout << "Application Date : " << it->second.applicationDate <<"\n";

    if(it->second.status == "ACTIVE"){
        cout << "\n--------Active Loan Info----------\n";
        cout << fixed << setprecision(2);
        cout << "Disbursement Date : " << it->second.disbursementDate << "\n";
        cout << "Outstanding       : Rs." << it->second.outstandingAmount << "\n";
        cout << "Next EMI Date     : " << it->second.nextEmiDate << "\n";   
        cout << "Payments Made     : " << it->second.paymentsMade << " / " << it->second.tenureInMonths << "\n";
    }

    if(it->second.status == "REJECTED"){
         cout << "Rejection Reason : " << it->second.rejectionReason << "\n";
    }
    cout << "====================================\n";
}

//=================SHOW PAYMENT HISTORY=====================
void LoanManager::viewPaymentHistory(const std::string& loanId) const {
    
    auto it = loans.find(loanId);
    if (it == loans.end()) {
        cout << "Loan not found.\n";
        return;
    }

    if (it->second.paymentHistory.empty()) {
        cout << "No payments made yet.\n";
        return;
    }

    cout << "\n================ PAYMENT HISTORY ================\n\n";
    cout << left << setw(6)  << "S.No"
                 << setw(10) << "Pay No"
                 << setw(15) << "Date"
                 << setw(12) << "EMI"
                 << setw(12) << "Principal"
                 << setw(12) << "Interest"
                 << setw(14) << "Outstanding"
                 << setw(20) << "Remarks" << "\n";
    cout << string(101, '-') << "\n";

    int serial = 1;
    for (const auto& entry : it->second.paymentHistory) {
        cout << left << setw(6)  << serial++
                     << setw(10) << entry.paymentNumber
                     << setw(15) << entry.paymentDate
                     << "Rs." << setw(9) << fixed << setprecision(2) << entry.emiAmount
                     << "Rs." << setw(9) << fixed << setprecision(2) << entry.principalPaid
                     << setw(12) << entry.interestPaid
                     << "Rs." << setw(9) << fixed << setprecision(2) << entry.outstandingBalance
                     << setw(20) << entry.remarks << "\n";
    }

    cout << string(101, '-') << "\n";
}

//==================VIEW USER LOANS==============================
void LoanManager::viewUserLoans(const std::string& accountNumber) const {
    
    cout << "\n================== YOUR LOANS ================\n\n";
    cout << left << setw(15) << "Loan ID"
             << setw(12) << "Type"
             << setw(16) << "Principal"
             << setw(16) << "EMI"
             << setw(12) << "Status" << "\n";
    cout << string(71, '-') << "\n";

    bool found = false;
    for (const auto& [loanId, loan] : loans) {
        if (loan.accountNumber == accountNumber) {
            found = true;
            cout << left << setw(15) << loan.loanId
             << setw(12) << loan.loanType
             << "Rs." << setw(13) << fixed << setprecision(2) << loan.principalAmount
             << "Rs." << setw(13) << fixed << setprecision(2) << loan.emiAmount
             << setw(12) << loan.status << "\n";
        }
    }
    if(!found) {
        cout << "No loans found on this account.\n";
    }

    cout << string(71, '-') << "\n";
}

//=======================VIEW ALL LOANS====================
void LoanManager::viewAllLoans() const {
    if (loans.empty()) {
        cout << "No loans found.\n";
        return;
    }

    cout << "\n=================== ALL LOANS ================\n\n";
    cout << left << setw(15) << "Loan ID"
             << setw(12) << "Type"
             << setw(16) << "Principal"
             << setw(16) << "EMI"
             << setw(12) << "Status" << "\n";
    cout << string(71, '-') << "\n";

    for (const auto& [loanId, loan] : loans){
        cout << left << setw(15) << loan.loanId
             << setw(12) << loan.loanType
             << "Rs." << setw(13) << fixed << setprecision(2) << loan.principalAmount
             << "Rs." << setw(13) << fixed << setprecision(2) << loan.emiAmount
             << setw(12) << loan.status << "\n";
    }    
    cout << string(71, '-') << "\n";
}

//================VIEW PENDING LOANS====================
void LoanManager::viewPendingLoans() const {
    
    cout << "\n================== PENDING LOANS ================\n\n";
    cout << left << setw(15) << "Loan ID"
             << setw(12) << "Type"
             << setw(16) << "Principal"
             << setw(16) << "EMI"
             << setw(12) << "Status" << "\n";
    cout << string(71, '-') << "\n";

    bool found = false;
    for (const auto& [loanId, loan] : loans) {
        if (loan.status == "PENDING") {
            found = true;
            cout << left << setw(15) << loan.loanId
             << setw(12) << loan.loanType
             << "Rs." << setw(13) << fixed << setprecision(2) << loan.principalAmount
             << "Rs." << setw(13) << fixed << setprecision(2) << loan.emiAmount
             << setw(12) << loan.status << "\n";
        }
    }
    if(!found) {
        cout << "No pending loans found on this account.\n";
    }

    cout << string(71, '-') << "\n";
}

//================SAVE AND LOAD FILES=================================
void LoanManager::saveLoans() const {
    DatabaseManager::saveLoans(loans);
}

void LoanManager::loadLoans() {
    DatabaseManager::loadLoans(loans, lastLoanSequenceNumber);
}

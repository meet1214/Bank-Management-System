#include "RDManager.h"
#include "DatabaseManager.h"


#include <iostream>
#include <iomanip>
#include <ctime>

using namespace std;

//==========STATIC MEMBER==========
int RDManager::lastRDSequenceNumber = 0;

//==========CONSTRUCTOR==========
RDManager::RDManager() {
    loadRDs();
}

//==========DESTRUCTOR==========
RDManager::~RDManager() {
    // empty — saving handled explicitly
}
// ================= PORTABLE DATE HELPER =================
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

//==========GENERATE RD NUMBER==========
string RDManager::generateRDNumber() {
    ++lastRDSequenceNumber;
    string seq = to_string(lastRDSequenceNumber);
    return "RD" + string(8 - seq.length(), '0') + seq;
}

//==========GET TODAY DATE==========
string RDManager::getTodayDate() {
    time_t now = time(0);
    struct tm* timeinfo = localtime(&now);
    char buffer[11];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d", timeinfo);
    return string(buffer);
}

//==========ADD MONTHS TO DATE==========
string RDManager::addMonthsToDate(const string& date, int months) {
    struct tm dateTm = parseDate(date);

    dateTm.tm_mon += months;
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

//==========CALCULATE MATURITY AMOUNT==========
double RDManager::calculateMaturityAmount(double monthly,
                                           int tenure, double rate) {
    double simpleDeposit = monthly * tenure;
    double interest = monthly * tenure * (tenure + 1) / 2.0 * (rate / 1200.0);
    return simpleDeposit + interest;
}

//==========OPEN RD==========
string RDManager::openRD(BankAccount& account, double monthlyAmount,
                          int tenureMonths, double interestRate) {

    if (account.getBalance() < monthlyAmount) {
        cout << "Insufficient balance to open RD.\n";
        return "";
    }

    RecurringDeposit rd;
    rd.rdId           = generateRDNumber();
    rd.accountNumber  = account.getAccountNumber();
    rd.monthlyAmount  = monthlyAmount;
    rd.tenureMonths   = tenureMonths;
    rd.interestRate   = interestRate;
    rd.startDate      = getTodayDate();
    rd.nextDebitDate  = getTodayDate();  // first debit today
    rd.totalDeposited = 0;
    rd.monthsPaid     = 0;
    rd.maturityAmount = calculateMaturityAmount(monthlyAmount,
                                                tenureMonths, interestRate);
    rd.status         = "ACTIVE";


    rds.push_back(rd);
    DatabaseManager::saveRD(rd);

    cout << "RD opened successfully!\n";
    cout << "RD ID          : " << rd.rdId << "\n";
    cout << "Monthly Amount : Rs." << fixed << setprecision(2)
         << monthlyAmount << "\n";
    cout << "Tenure         : " << tenureMonths << " months\n";
    cout << "Interest Rate  : " << interestRate << "%\n";
    cout << "Maturity Amount: Rs." << rd.maturityAmount << "\n";

    return rd.rdId;
}

//==========PROCESS AUTO DEBITS==========
void RDManager::processAutoDebits(BankAccount& account) {
    string today = getTodayDate();

    for (auto& rd : rds) {
        // skip if not this account
        if (rd.accountNumber != account.getAccountNumber()) continue;

        // skip if not active
        if (rd.status != "ACTIVE") continue;

        // skip if not due yet
        if (today < rd.nextDebitDate) continue;

        // check balance
        if (account.getBalance() < rd.monthlyAmount) {
            cout << "RD " << rd.rdId
                 << ": Insufficient balance for auto-debit. Skipping.\n";
            continue;
        }

        // debit from account
        account.debitAmount(rd.monthlyAmount, "RD Auto-Debit " + rd.rdId);

        // update RD
        rd.totalDeposited += rd.monthlyAmount;
        rd.monthsPaid++;
        rd.nextDebitDate = addMonthsToDate(today, 1);

        cout << "RD " << rd.rdId << ": Auto-debited Rs."
             << fixed << setprecision(2) << rd.monthlyAmount
             << " (" << rd.monthsPaid << "/" << rd.tenureMonths << ")\n";

        // check maturity
        if (rd.monthsPaid == rd.tenureMonths) {
            account.creditAmount(rd.maturityAmount, "RD Maturity " + rd.rdId);
            rd.status = "COMPLETED";
            cout << "RD " << rd.rdId << ": Matured! Rs."
                 << rd.maturityAmount << " credited.\n";
        }

        DatabaseManager::updateRD(rd);
    }
}

//==========VIEW USER RDs==========
void RDManager::viewUserRDs(const string& accountNumber) const {

    cout << "\n================== YOUR RDs ================\n\n";
    cout << left << setw(15) << "RD ID"
             << setw(12) << "Monthly"
             << setw(16) << "Tenure"
             << setw(16) << "PAID"
             << setw(12) << "Maturity Amount"
             << setw(12) << "Status"<<"\n";
    cout << string(83, '-') << "\n";

    bool found = false;
    for (const auto& rd:rds) {
        if (rd.accountNumber == accountNumber) {
            found = true;
            cout << left << setw(15) << rd.rdId
             << "Rs." << setw(12) << fixed << setprecision(2) << rd.monthlyAmount
             << setw(13)<< rd.tenureMonths
             << setw(16) << rd.monthsPaid
             << "Rs." << setw(12) << fixed << setprecision(2) << rd.maturityAmount
             << setw(12) << rd.status << "\n";
        }
    }
    if(!found) {
        cout << "No RDs found on this account.\n";
    }

    cout << string(71, '-') << "\n";
}

//==========VIEW RD DETAILS==========
void RDManager::viewRDDetails(const string& rdId) const {

    const RecurringDeposit* found = nullptr;
    for (const auto& rd : rds) {
        if (rd.rdId == rdId) {
            found = &rd;
            break;
        }
    }


    if (!found) {
        cout << "RD not found.\n";
        return;
    }


    cout << "\n========== RD DETAILS ==========\n";
    cout << "RD ID           : " << found->rdId           << "\n";
    cout << "Account Number  : " << found->accountNumber  << "\n";
    cout << "Monthly Amount  : " << found->monthlyAmount  << "\n";
    cout << "Tenure Months   : " << found->tenureMonths   << "\n";
    cout << "Interest Rate   : " << found->interestRate   << "\n";
    cout << "Start Date      : " << found->startDate      << "\n";
    cout << "Next Debit Date : " << found->nextDebitDate  << "\n";
    cout << "Total Deposited : " << found->totalDeposited << "\n";
    cout << "Months Paid     : " << found->monthsPaid     << "\n";
    cout << "Maturity Amount : " << found->maturityAmount << "\n";
    cout << "Status          : " << found->status          << "\n";

    cout << "=================================\n";
}

//==========CANCEL RD==========
bool RDManager::cancelRD(const string& rdId, BankAccount& account) {
    for (auto& rd : rds) {
        if (rd.rdId != rdId) continue;
        if (rd.status != "ACTIVE") {
            cout << "RD is not active.\n";
            return false;
        }
        // credit back what was deposited (no interest on cancellation)
        account.creditAmount(rd.totalDeposited, "RD Cancellation " + rdId);
        rd.status = "CANCELLED";
        DatabaseManager::updateRD(rd);
        cout << "RD cancelled. Rs." << fixed << setprecision(2)
             << rd.totalDeposited << " refunded.\n";
        return true;
    }
    cout << "RD not found.\n";
    return false;
}

//==========SAVE RDs==========
void RDManager::saveRDs() const {
    for (const auto& rd : rds)
        DatabaseManager::saveRD(rd);
}

//==========LOAD RDs==========
void RDManager::loadRDs() {
    DatabaseManager::loadRDs(rds);
}
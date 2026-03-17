#include "StandingInstructionManager.h"
#include "DatabaseManager.h"
#include "StandingInstruction.h"

#include <iostream>
#include <iomanip>
#include <sstream>
#include <ctime>

using namespace std;

//==========STATIC MEMBER==========
int StandingInstructionManager::lastSISequenceNumber = 0;

//==========CONSTRUCTOR==========
StandingInstructionManager::StandingInstructionManager() {
    loadSIs();
}

//==========DESTRUCTOR==========
StandingInstructionManager::~StandingInstructionManager() {
    // empty — saving handled explicitly
}

//==========PORTABLE DATE HELPER==========
static struct tm parseDate(const string& date) {
    struct tm t = {};
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

//==========GENERATE SI NUMBER==========
string StandingInstructionManager::generateSINumber() {
    ++lastSISequenceNumber;
    string seq = to_string(lastSISequenceNumber);
    return "SI" + string(8 - seq.length(), '0') + seq;
}

//==========GET TODAY DATE==========
string StandingInstructionManager::getTodayDate() {
    time_t now = time(0);
    struct tm* timeinfo = localtime(&now);
    char buffer[11];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d", timeinfo);
    return string(buffer);
}

//==========ADD MONTHS TO DATE==========
string StandingInstructionManager::addMonthsToDate(const string& date,
                                                     int months) {
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

//==========BUILD NEXT EXECUTION DATE==========
string StandingInstructionManager::buildNextExecutionDate(int executionDay) {
    time_t now = time(0);
    struct tm* lt = localtime(&now);

    int today = lt->tm_mday;
    int month = lt->tm_mon + 1;
    int year  = lt->tm_year + 1900;

    // if execution day already passed this month, schedule for next month
    if (today > executionDay) {
        month++;
        if (month > 12) { month = 1; year++; }
    }

    ostringstream oss;
    oss << year << "-"
        << setw(2) << setfill('0') << month << "-"
        << setw(2) << setfill('0') << executionDay;
    return oss.str();
}

//==========CREATE SI==========
string StandingInstructionManager::createSI(
        const string& accountNumber,
        const string& receiverAccountNumber,
        double amount, int executionDay,
        const string& description) {

    // validate execution day
    if (executionDay < 1 || executionDay > 28) {
        cout << "Execution day must be between 1 and 28.\n";
        return "";
    }

    StandingInstruction si;
    si.siId                  = generateSINumber();
    si.accountNumber         = accountNumber;
    si.receiverAccountNumber = receiverAccountNumber;
    si.amount                = amount;
    si.executionDay          = executionDay;
    si.nextExecutionDate     = buildNextExecutionDate(executionDay);
    si.description           = description;
    si.status                = "ACTIVE";

    sis.push_back(si);
    DatabaseManager::saveSI(si);

    cout << "Standing Instruction created!\n";
    cout << "SI ID           : " << si.siId << "\n";
    cout << "Amount          : Rs." << fixed << setprecision(2)
         << amount << "\n";
    cout << "Execution Day   : " << executionDay << " of every month\n";
    cout << "Next Execution  : " << si.nextExecutionDate << "\n";
    cout << "Description     : " << description << "\n";

    return si.siId;
}

//==========PROCESS AUTO EXECUTIONS==========
void StandingInstructionManager::processAutoExecutions(
        BankAccount& account, AccountManager& manager) {

    string today = getTodayDate();

    for (auto& si : sis) {
        if (si.accountNumber != account.getAccountNumber()) continue;
        if (si.status != "ACTIVE") continue;
        if (today < si.nextExecutionDate) continue;

        // get receiver account
        BankAccount* receiver =
            manager.getAccountByAccountNumber(si.receiverAccountNumber);

        if (!receiver) {
            cout << "[SI] " << si.siId
                 << ": Receiver account not found. Skipping.\n";
            si.nextExecutionDate = addMonthsToDate(today, 1);
            DatabaseManager::updateSI(si);
            continue;
        }

        // execute transfer
        bool success = account.transferMoney(*receiver, si.amount);

        if (success) {
            cout << "[SI] " << si.siId << ": Rs."
                 << fixed << setprecision(2) << si.amount
                 << " transferred to " << si.receiverAccountNumber
                 << " (" << si.description << ")\n";
            manager.save();
        } else {
            cout << "[SI] " << si.siId
                 << ": Transfer failed — insufficient balance.\n";
        }

        // advance to next month regardless of success
        si.nextExecutionDate = addMonthsToDate(si.nextExecutionDate, 1);
        DatabaseManager::updateSI(si);
    }
}

//==========VIEW USER SIs==========
void StandingInstructionManager::viewUserSIs(
        const string& accountNumber) const {

    cout << "\n================== YOUR SIs ================\n\n";
    cout << left << setw(15) << "SI ID"
             << setw(12) << "Receiver"
             << setw(12) << "Amount"
             << setw(16) << "Day"
             << setw(12) << "Next Date"
             << setw(17) << "Description"
             << setw(12) << "Status"<<"\n";
    cout << string(80, '-') << "\n";

    bool found = false;
    for (const auto& si:sis) {
        if (si.accountNumber == accountNumber) {
            found = true;
            cout << left << setw(15) << si.siId
             <<setw(12) << si.receiverAccountNumber
             << "Rs." << setw(12) << fixed << setprecision(2) << si.amount
             << setw(16)<< si.executionDay
             << setw(12) << si.nextExecutionDate
             << setw(17) << si.description
             << setw(12) << si.status << "\n";
        }
    }
    if(!found) {
        cout << "No SIs found on this account.\n";
    }

    cout << string(80, '-') << "\n";
}

//==========VIEW SI DETAILS==========
void StandingInstructionManager::viewSIDetails(
        const string& siId) const {
    // find by siId in vector
    // print all fields
    // write this yourself — same pattern as viewRDDetails

    const StandingInstruction* found = nullptr;
    for (const auto& si:sis) {
        if (si.siId == siId) {
            found = &si;
            break;
        }
    }


    if (!found) {
        cout << "SI not found.\n";
        return;
    }

    cout << "\n=================== SI DETAILS ===================\n";
    cout << "SI ID                   : " << found->siId                   << "\n";
    cout << "Account Number          : " << found->accountNumber          << "\n";
    cout << "Receiver Account Number : " << found->receiverAccountNumber  << "\n";
    cout << "Amount                  : " << found->amount                 << "\n";
    cout << "Execution Day           : " << found->executionDay           << "\n";
    cout << "Next Execution Date     : " << found->nextExecutionDate      << "\n";
    cout << "Description             : " << found->description            << "\n";
    cout << "Status                  : " << found->status                 << "\n";

    cout << "====================================================\n";
}

//==========CANCEL SI==========
bool StandingInstructionManager::cancelSI(const string& siId,
                                           const string& accountNumber) {
    for (auto& si : sis) {
        if (si.siId != siId) continue;
        if (si.accountNumber != accountNumber) {
            cout << "Unauthorized.\n";
            return false;
        }
        if (si.status != "ACTIVE") {
            cout << "SI is not active.\n";
            return false;
        }
        si.status = "CANCELLED";
        DatabaseManager::updateSI(si);
        cout << "Standing instruction " << siId
             << " cancelled successfully.\n";
        return true;
    }
    cout << "SI not found.\n";
    return false;
}

//==========SAVE SIs==========
void StandingInstructionManager::saveSIs() const {
    for (const auto& si : sis)
        DatabaseManager::saveSI(si);
}

//==========LOAD SIs==========
void StandingInstructionManager::loadSIs() {
    DatabaseManager::loadSIs(sis);
}
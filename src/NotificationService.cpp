#include "NotificationService.h"
#include <ctime>


using namespace std;

string NotificationService::getTodayDate() {
    time_t now = time(0);
    struct tm* t = localtime(&now);
    char buffer[11];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d", t);
    return string(buffer);
}

int NotificationService::daysBetween(const string& date1, const string& date2) {
    struct tm t = {};

    t.tm_year  = stoi(date1.substr(0, 4)) - 1900;
    t.tm_mon   = stoi(date1.substr(5,2)) - 1;
    t.tm_mday  = stoi(date1.substr(8, 2));
    t.tm_isdst = -1;

    struct tm t1 = {};
    t1.tm_year  = stoi(date2.substr(0, 4)) - 1900;
    t1.tm_mon   = stoi(date2.substr(5,2)) - 1;
    t1.tm_mday  = stoi(date2.substr(8, 2));
    t1.tm_isdst = -1;   

    time_t epoch1 = mktime(&t);
    time_t epoch2 = mktime(&t1);
    return static_cast<int>(difftime(epoch2, epoch1) / 86400);
}

vector<string> NotificationService::getNotifications(
    const BankAccount& account,
    const LoanManager& loanManager,
    const RDManager& rdManager) {

    vector<string> notifications;
    string today = getTodayDate();
    string accNo = account.getAccountNumber();

    // Check loans
    for (const auto& [loanId, loan] : loanManager.getLoans()) {
        if (loan.accountNumber != accNo) continue;
        if (loan.status != "ACTIVE") continue;
        
        int days = daysBetween(today, loan.nextEmiDate);
        if (days <= 3) {
            notifications.push_back("[!] EMI due for " + loanId +
                " on " + loan.nextEmiDate +
                " — Rs." + to_string(loan.emiAmount));
        }
    }

    // Check RDs
    for (const auto& rd : rdManager.getRDs()) {
        if (rd.accountNumber != accNo) continue;
        if (rd.status != "ACTIVE") continue;

        int days = daysBetween(today, rd.nextDebitDate);
        if (days <= 3) {
            notifications.push_back("[!] RD debit due for " + rd.rdId +
                " on " + rd.nextDebitDate +
                " — Rs." + to_string(rd.monthlyAmount));
        }
    }

    return notifications;
}
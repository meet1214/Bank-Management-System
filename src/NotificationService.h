#ifndef NOTIFICATIONSERVICE_H
#define NOTIFICATIONSERVICE_H

#include <string>
#include <vector>
#include "BankAccount.h"
#include "LoanManager.h"
#include "RDManager.h"

using namespace std;

class NotificationService {
public:
    static vector<string> getNotifications(
        const BankAccount& account,
        const LoanManager& loanManager,
        const RDManager& rdManager
    );

private:
    static string getTodayDate();
    static int daysBetween(const string& date1, const string& date2);
};

#endif
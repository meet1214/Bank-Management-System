#ifndef RD_H
#define RD_H

#include <string>
#include <vector>

struct RecurringDeposit {
    std::string rdId;
    std::string accountNumber;
    double      monthlyAmount;
    int         tenureMonths;
    double      interestRate;
    std::string startDate;
    std::string nextDebitDate;
    double      totalDeposited;
    int         monthsPaid;
    double      maturityAmount;
    std::string status;  // ACTIVE, COMPLETED, CANCELLED
};
struct RD {
    std::vector<RecurringDeposit> rdHistory;
};
#endif
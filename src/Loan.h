#ifndef LOAN_H
#define LOAN_H

#include <string>
#include <vector>

struct LoanPayment {
    int paymentNumber;
    std::string paymentDate;
    double emiAmount;
    double principalPaid;
    double interestPaid;
    double outstandingBalance;
    std::string remarks;
};

struct Loan {
    std::string loanId;
    std::string accountNumber;
    std::string loanType;
    double principalAmount;
    double interestRate;
    int tenureInMonths;
    double emiAmount;
    double outstandingAmount;
    std::string status;
    std::string rejectionReason;
    std::string applicationDate;
    std::string disbursementDate;
    std::string nextEmiDate;
    int paymentsMade;
    std::vector<LoanPayment> paymentHistory;
};

#endif
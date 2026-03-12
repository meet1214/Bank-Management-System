#ifndef LOANMANAGER_H
#define LOANMANAGER_H

#include "Loan.h"
#include "BankAccount.h"
#include "LoanFileManager.h"
#include <unordered_map>
#include <string>
#include <vector>
#include <cmath>
#include <ctime>

class LoanManager {
private:    
    // Member Variables
    std::unordered_map<std::string, Loan> loans;
    static int lastLoanSequenceNumber;
    std::unordered_map<std::string, double> interestRates;
    std::unordered_map<std::string, double> foreclosurePenaltyRates;

    // Loan limits
    static const int PERSONAL_LOAN_MAX = 500000;
    static const int HOME_LOAN_MAX = 5000000;
    static const int AUTO_LOAN_MAX = 1000000;
    static const int EDUCATION_LOAN_MAX = 2000000;
    
    // Helper Methods
    static std::string generateLoanNumber();
    static double getMonthlyInterestRate(double annualRate);
    static int getDaysBetweenDates(const std::string& startDate, const std::string& endDate);
    static std::string addMonthsToDate(const std::string& date, int months);
    static std::string getTodayDate();

public:
    // Constructor & Destructor
    LoanManager();
    ~LoanManager();

    // EMI Calculation
    double calculateEMI(double principal, double annualRate, int months) const;
    std::vector<LoanPayment> generateAmortizationSchedule(const std::string& loanId) const;
    
    // Loan Application
    std::string applyForLoan(BankAccount& account, const std::string& loanType,
                    double amount, int tenureMonths);
    bool checkEligibility(const BankAccount& account, const std::string& loanType, 
                          double amount) const;
    
    // Admin Operations
    bool approveLoan(const std::string& loanId);
    bool rejectLoan(const std::string& loanId, const std::string& reason);
    bool disburseLoan(const std::string& loanId, BankAccount& account);
    
    // Payment Operations
    bool makeEMIPayment(const std::string& loanId, BankAccount& account);
    double calculateEarlyClosureAmount(const std::string& loanId) const;
    bool closeLoanEarly(const std::string& loanId, BankAccount& account);
    
    // View Operations
    void viewLoanDetails(const std::string& loanId) const;
    void viewPaymentHistory(const std::string& loanId) const;
    void viewUserLoans(const std::string& accountNumber) const;
    void viewAllLoans() const;
    void viewPendingLoans() const;
    
    // Getter
    Loan* getLoanById(const std::string& loanId);
    
    // File Operations
    void saveLoans() const;
    void loadLoans();
};

#endif
#ifndef RDMANAGER_H
#define RDMANAGER_H

#include "RD.h"
#include "BankAccount.h"
#include <vector>
#include <string>
using namespace std;

class RDManager {
private:
    vector<RecurringDeposit> rds;
    static int lastRDSequenceNumber;

    static string generateRDNumber();
    static string getTodayDate();
    static string addMonthsToDate(const string& date, int months);

public:
    RDManager();
    ~RDManager();

    static double calculateMaturityAmount(double monthly, int tenure, double rate);
    // User operations
    string openRD(BankAccount& account, double monthlyAmount,
                  int tenureMonths, double interestRate);
    void processAutoDebits(BankAccount& account);  // call on every login
    void viewUserRDs(const string& accountNumber) const;
    void viewRDDetails(const string& rdId) const;
    bool cancelRD(const string& rdId, BankAccount& account);

    // File operations
    void saveRDs() const;
    void loadRDs();
};

#endif
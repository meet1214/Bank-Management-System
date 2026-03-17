#ifndef STANDINGINSTRUCTIONMANAGER_H
#define STANDINGINSTRUCTIONMANAGER_H

#include "StandingInstruction.h"
#include "BankAccount.h"
#include "AccountManager.h"
#include <vector>
#include <string>

using namespace std;

class StandingInstructionManager {
private:
    vector<StandingInstruction> sis;
    static int lastSISequenceNumber;

    static string generateSINumber();
    static string getTodayDate();
    static string addMonthsToDate(const string& date, int months);
    static string buildNextExecutionDate(int executionDay);

public:
    StandingInstructionManager();
    ~StandingInstructionManager();

    // User operations
    string createSI(const string& accountNumber,
                    const string& receiverAccountNumber,
                    double amount, int executionDay,
                    const string& description);
    void processAutoExecutions(BankAccount& account,
                               AccountManager& manager);
    void viewUserSIs(const string& accountNumber) const;
    void viewSIDetails(const string& siId) const;
    bool cancelSI(const string& siId,
                  const string& accountNumber);

    // Storage
    void saveSIs() const;
    void loadSIs();
};

#endif
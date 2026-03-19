#ifndef REPORTGENERATOR_H
#define REPORTGENERATOR_H

#include <iostream>
#include <vector>
#include <string>
#include <iomanip>
#include "Loan.h"
#include "RD.h"
#include "BankAccount.h"

using namespace std;

template<typename T>
struct ReportTrait;

template<>
struct ReportTrait<Transaction> {
    static string getTitle() { return "Transaction Report"; }
    static string getHeader() {
        return "TxnID    Date & Time              Type                                Amount        Balance";
    }
    static string getSeparator() { return string(102, '-'); }
    static void printRow(const Transaction& t) {
        cout << left << setw(8)  << t.transactionId
                     << setw(25) << t.dateTime
                     << setw(35) << t.type
             << fixed << setprecision(2)
             << "Rs." << setw(11) << t.amount
             << "Rs." << setw(11) << t.balance << "\n";
    }
};

template<>
struct ReportTrait<Loan> {
    static string getTitle() { return "Loan Report"; }
    static string getHeader() {
        return "LoanID          Type        Principal        EMI          Status";
    }
    static string getSeparator() { return string(71, '-'); }
    static void printRow(const Loan& l) {
        cout << left << setw(15) << l.loanId
                     << setw(12) << l.loanType
             << "Rs." << setw(13) << fixed << setprecision(2) << l.principalAmount
             << "Rs." << setw(13) << fixed << setprecision(2) << l.emiAmount
                     << setw(12) << l.status << "\n";
    }
};

template<>
struct ReportTrait<RecurringDeposit> {
    static string getTitle() { return "Recurring Deposit Report"; }
    static string getHeader() {
        return "RD ID          Monthly      Tenure    Paid    Maturity Amount Status";
    }
    static string getSeparator() { return string(83, '-'); }
    static void printRow(const RecurringDeposit& rd) {
        cout << left << setw(15) << rd.rdId
             << "Rs." << setw(12) << fixed << setprecision(2) << rd.monthlyAmount
                      << setw(13) << rd.tenureMonths
                      << setw(16) << rd.monthsPaid
             << "Rs." << setw(12) << fixed << setprecision(2) << rd.maturityAmount
                      << setw(12) << rd.status << "\n";
    }
};

// ===== THE TEMPLATE FUNCTION =====
template<typename T>
void generateReport(const vector<T>& items, const string& title = "") {

    string reportTitle = title.empty() ? ReportTrait<T>::getTitle() : title;

    cout << "\n========== " << reportTitle << " ==========\n\n";

    if (items.empty()) {
        cout << "No records found.\n";
        cout << ReportTrait<T>::getSeparator() << "\n";
        return;
    }

    cout << ReportTrait<T>::getHeader() << "\n";
    cout << ReportTrait<T>::getSeparator() << "\n";

    for (const auto& item : items) {
        ReportTrait<T>::printRow(item);
    }

    cout << ReportTrait<T>::getSeparator() << "\n";
    cout << "Total records: " << items.size() << "\n";
}

#endif
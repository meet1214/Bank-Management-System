#include "LoanFileManager.h"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <iostream>
#include <string>
using namespace std;

void LoanFileManager::saveLoans(const unordered_map<string, Loan>& loans) {
    
    filesystem::create_directories("data/loans");
    ofstream file("data/loans/loans.txt");
    
    if (!file) {
        cout << "Error: Could not save loans.\n";
        return;
    }
    
    // Save main loan data
    for (const auto& [loanId, loan] : loans) {
        file << loan.loanId            << "|"
             << loan.accountNumber     << "|"
             << loan.loanType          << "|"
             << loan.principalAmount   << "|"
             << loan.interestRate      << "|"
             << loan.tenureInMonths    << "|"
             << loan.emiAmount         << "|"
             << loan.outstandingAmount << "|" 
             << loan.status            << "|"
             << loan.rejectionReason   << "|"
             << loan.applicationDate   << "|"
             << loan.disbursementDate  << "|"
             << loan.nextEmiDate       << "|"
             << loan.paymentsMade      << "\n";
    }
    
    file.close();
    
    // Save payment histories in separate files
    filesystem::create_directories("data/loans/payments");
    
    for (const auto& [loanId, loan] : loans) {
        if (loan.paymentHistory.empty()) continue;  // Skip if no payments
        
        ofstream pFile("data/loans/payments/" + loanId + ".txt");
        
        if (!pFile) continue;
        
        for (const auto& p : loan.paymentHistory) {
            pFile << p.paymentNumber      << "|"
                  << p.paymentDate        << "|"
                  << p.emiAmount          << "|"
                  << p.principalPaid      << "|"
                  << p.interestPaid       << "|"
                  << p.outstandingBalance << "|" 
                  << p.remarks << "\n";
        }
        
        pFile.close();
    }
}

void LoanFileManager::loadLoans(unordered_map<string, Loan>& loans, int& lastSequenceNumber) {
    
    loans.clear();
    lastSequenceNumber = 0;
    
    filesystem::create_directories("data/loans");
    
    ifstream file("data/loans/loans.txt");
    if (!file) return;  // No loans file yet
    
    string line;
    while (getline(file, line)) {
        if (line.empty()) continue;
        
        stringstream ss(line);
        string loanId, accountNumber, loanType, status,rejectionReason, applicationDate, disbursementDate, nextEmiDate;
        string principalStr, rateStr, tenureStr, emiStr, outstandingStr, paymentsStr;
        
        getline(ss, loanId,           '|');
        getline(ss, accountNumber,    '|');
        getline(ss, loanType,         '|');
        getline(ss, principalStr,     '|');
        getline(ss, rateStr,          '|');
        getline(ss, tenureStr,        '|');
        getline(ss, emiStr,           '|');
        getline(ss, outstandingStr,   '|');
        getline(ss, status,           '|');
        getline(ss, rejectionReason,  '|');
        getline(ss, applicationDate,  '|');
        getline(ss, disbursementDate, '|');
        getline(ss, nextEmiDate,      '|');
        getline(ss, paymentsStr,      '|');
        
        try {
            Loan loan;
            loan.loanId            = loanId;
            loan.accountNumber     = accountNumber;
            loan.loanType          = loanType;
            loan.principalAmount   = stod(principalStr);
            loan.interestRate      = stod(rateStr);
            loan.tenureInMonths    = stoi(tenureStr);
            loan.emiAmount         = stod(emiStr);
            loan.outstandingAmount = stod(outstandingStr);
            loan.status            = status;
            loan.rejectionReason   = rejectionReason;
            loan.applicationDate   = applicationDate;
            loan.disbursementDate  = disbursementDate;
            loan.nextEmiDate       = nextEmiDate;
            loan.paymentsMade      = stoi(paymentsStr);
            
            // Load payment history for this loan
            string paymentFile = "data/loans/payments/" + loanId + ".txt";
            ifstream pFile(paymentFile);
            
            if (pFile) {
                string pLine;
                while (getline(pFile, pLine)) {
                    if (pLine.empty()) continue;
                    
                    stringstream pss(pLine);
                    string payNumStr, payDate, emiAmtStr, pPrincipalStr, interestStr, balanceStr,remarksStr;
                    
                    getline(pss, payNumStr,     '|');
                    getline(pss, payDate,       '|');
                    getline(pss, emiAmtStr,     '|');
                    getline(pss, principalStr,  '|');
                    getline(pss, interestStr,   '|');
                    getline(pss, balanceStr,    '|');
                    getline(pss, remarksStr,    '|');
                    
                    LoanPayment payment;
                    payment.paymentNumber      = stoi(payNumStr);
                    payment.paymentDate        = payDate;
                    payment.emiAmount          = stod(emiAmtStr);
                    payment.principalPaid      = stod(pPrincipalStr);
                    payment.interestPaid       = stod(interestStr);
                    payment.outstandingBalance = stod(balanceStr);
                    payment.remarks            = remarksStr;
                    
                    loan.paymentHistory.push_back(payment);
                }
            }
            
            // Track highest loan sequence number
            if (loanId.length() > 4 && loanId.substr(0, 4) == "LOAN") {
                string seqPart = loanId.substr(4);
                try {
                    int seq = stoi(seqPart);
                    if (seq > lastSequenceNumber) {
                        lastSequenceNumber = seq;
                    }
                } catch (...) {
                    // Invalid format, skip
                }
            }
            
            loans.emplace(loanId, loan);
            
        } catch (...) {
            cout << "Skipping corrupted loan entry.\n";
        }
    }
}

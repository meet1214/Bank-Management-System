#ifndef LOANFILEMANAGER_H
#define LOANFILEMANAGER_H

#include "Loan.h"
#include <unordered_map>
#include <string>

class LoanFileManager {
public:
    static void saveLoans(const std::unordered_map<std::string, Loan>& loans);
    static void loadLoans(std::unordered_map<std::string, Loan>& loans, int& lastSequenceNumber);
};

#endif
#pragma once
#include "BankAccount.h"
#include <unordered_map>
#include <string>

class FileManager {
public:
    static void saveAccounts(
        const std::unordered_map<std::string, BankAccount>& users);

    static void loadAccounts(
        std::unordered_map<std::string, BankAccount>& users,
        long long& lastSequenceNumber,
        const std::string& branchCode);
};
#include "AccountManager.h"
#include "BankAccount.h"
#include "FileManager.h"

#include <filesystem>
#include <functional>
#include <iostream>

using namespace std;

// ================= GENERATE ACCOUNT NUMBER =================
string AccountManager::generateAccountNumber() {

  lastSequenceNumber++;

  string sequence = to_string(lastSequenceNumber);

  while (sequence.length() < 8)
    sequence = "0" + sequence;

  return branchCode + sequence;
}

// ================= CREATE ACCOUNT =================
bool AccountManager::createAccount(const string &name, int pin) {

  string accNo = generateAccountNumber();

  size_t hashValue = hash<string>{}(to_string(pin));

  auto result = users.emplace(
      accNo, BankAccount(accNo, name, hashValue, 0.0, "user", false));

  if (!result.second) {
    cout << "Account number collision occurred.\n";
    return false;
  }

  save();

  cout << "Account created successfully!\n";
  cout << "Your Account Number is: " << accNo << "\n";

  return true;
}

// ================= LOGIN =================
string AccountManager::loginAccount(const string &accNo, int pin) {

  auto it = users.find(accNo);

  if (it == users.end()) {
    cout << "Account not found.\n";
    return "";
  }

  BankAccount &user = it->second;

  if (user.getLockStatus()) {
    cout << "Account is locked.\n";
    return "";
  }

  if (!user.authenticatePin(pin)) {
    cout << "Invalid PIN.\n";
    return "";
  }

  return accNo; // return account ID instead of pointer
}

//==================SHOW ADMIN MENU=================
void AccountManager::showAdminMenu() {
  cout << "Welcome Admin.\n";

  bool adminLoggedIn = true;

  while (adminLoggedIn) {

    cout << "\n===== ADMIN MENU =====\n";
    cout << "1. View All Accounts\n";
    cout << "2. Freeze Account\n";
    cout << "3. Unfreeze Account\n";
    cout << "4. Delete Account\n";
    cout << "5. Show Total Bank Balance\n";
    cout << "6. Logout\n";
    cout << "Enter choice: ";

    int adminChoice;
    cin >> adminChoice;

    if (cin.fail()) {
      cin.clear();
      cin.ignore(1000, '\n');
      cout << "Invalid input!\n";
      continue;
    }

    switch (adminChoice) {

    case 1:
      viewAllAccounts();
      break;

    case 2: {
      string acc;
      cout << "Enter account number: ";
      getline(cin >> ws, acc);
      freezeAccount(acc);
      break;
    }

    case 3: {
      string acc;
      cout << "Enter account number: ";
      getline(cin >> ws, acc);
      unfreezeAccount(acc);
      break;
    }

    case 4: {
      string acc;
      cout << "Enter account number: ";
      getline(cin >> ws, acc);
      deleteAccount(acc);
      break;
    }

    case 5:
      showTotalBankBalance();
      break;

    case 6:
      cout << "Admin logging out...\n";
      adminLoggedIn = false;
      break;

    default:
      cout << "Invalid option.\n";
    }
  }
}

// ================= SAVE TO FILE =================
void AccountManager::save() const { FileManager::saveAccounts(users); }

// ================= LOAD FROM FILE =================
void AccountManager::load() {
  FileManager::loadAccounts(users, lastSequenceNumber, branchCode);

  ensureAdminExists();
}
//======ENSURE ADMIN EXISTENCE=============
void AccountManager::ensureAdminExists() {

  for (const auto &[accNo, user] : users) {
    if (user.getRole() == "admin")
      return;
  }

  size_t defaultAdminHash = std::hash<std::string>{}(std::to_string(1234));

  users.emplace("ADMIN00000001",
                BankAccount("ADMIN00000001", "SystemAdmin", defaultAdminHash,
                            0.0, "admin", false));

  save();
}

// ================= GET ACCOUNT =================
BankAccount *AccountManager::getAccountByAccountNumber(const string &accNo) {

  auto it = users.find(accNo);

  if (it == users.end())
    return nullptr;

  return &it->second;
}

// ================= VIEW ALL ACCOUNTS =================
void AccountManager::viewAllAccounts() const {

  cout << "\n----- All User Accounts -----\n";

  for (const auto &[accNo, user] : users) {

    if (user.getRole() == "admin")
      continue;

    cout << "Account No: " << user.getAccountNumber() << "\n";
    cout << "Name: " << user.getName() << "\n";
    cout << "Balance: ₹" << user.getBalance() << "\n";
    cout << "Status: " << (user.getLockStatus() ? "Locked" : "Active")
         << "\n-----------------------------\n";
  }
}

// ================= FREEZE ACCOUNT =================
void AccountManager::freezeAccount(const string &accNo) {

  auto it = users.find(accNo);

  if (it == users.end()) {
    cout << "Account not found.\n";
    return;
  }

  if (it->second.getRole() == "admin") {
    cout << "Cannot freeze admin account.\n";
    return;
  }

  it->second.setLockStatus(true);
  save();

  cout << "Account frozen successfully.\n";
}

// ================= UNFREEZE ACCOUNT =================
void AccountManager::unfreezeAccount(const string &accNo) {

  auto it = users.find(accNo);

  if (it == users.end()) {
    cout << "Account not found.\n";
    return;
  }

  it->second.setLockStatus(false);
  save();

  cout << "Account unfrozen successfully.\n";
}

// ================= DELETE ACCOUNT =================
void AccountManager::deleteAccount(const string &accNo) {

  auto it = users.find(accNo);

  if (it == users.end()) {
    cout << "Account not found.\n";
    return;
  }

  if (it->second.getRole() == "admin") {
    cout << "Cannot delete admin account.\n";
    return;
  }

  // Remove transaction file
  string filePath = "data/transactions/" + accNo + ".txt";
  std::filesystem::remove(filePath);

  users.erase(it);
  save();

  cout << "Account deleted successfully.\n";
}

// ================= TOTAL BANK BALANCE =================
void AccountManager::showTotalBankBalance() const {

  double total = 0;

  for (const auto &[accNo, user] : users) {
    if (user.getRole() == "user")
      total += user.getBalance();
  }

  cout << "Total Bank Balance: ₹" << total << "\n";
}
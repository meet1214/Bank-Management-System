#include "AccountManager.h"
#include "BankAccount.h"
#include "BankExceptions.h"
#include "Config.h"
#include "DatabaseManager.h"
#include "InputValidator.h"
#include "LoanManager.h"
#include "RDManager.h"
#include "StandingInstructionManager.h"

#include <chrono>
#include <iomanip>
#include <iostream>
#include <string>
#include <thread>

using namespace std;

void loanAdminMenu(LoanManager &loanManager, AccountManager &manager) {
  bool loadMenu = true;
  while (loadMenu) {
    cout << "\n===== LOAN SECTION =====\n";
    cout << "1. View all loans\n";
    cout << "2. View Pending Loans\n";
    cout << "3. Approve Loan\n";
    cout << "4. Reject Loan\n";
    cout << "5. Disburse Loan\n";
    cout << "6. Back\n";
    int subChoice = InputValidator::getInt("Enter your choice: ");

    switch (subChoice) {
    case 1:
      loanManager.viewAllLoans();
      break;

    case 2:
      loanManager.viewPendingLoans();
      break;

    case 3: {
      string loanId = InputValidator::getString("Enter loan ID: ");
      loanManager.approveLoan(loanId);
      break;
    }
    case 4: {
      string loanId = InputValidator::getString("Enter loan ID: ");
      string reason = InputValidator::getString("Enter rejection reason: ");
      loanManager.rejectLoan(loanId, reason);
      break;
    }

    case 5: {
      string loanId = InputValidator::getString("Enter loan ID: ");
      Loan *loan = loanManager.getLoanById(loanId);
      if (!loan) {
        cout << "Loan not found.\n";
        break;
      }
      BankAccount *acc = manager.getAccountByAccountNumber(loan->accountNumber);
      if (!acc) {
        cout << "Account not found.\n";
        break;
      }
      loanManager.disburseLoan(loanId, *acc);
      break;
    }

    case 6:
      loadMenu = false;
      break;
    }
  }
}

void loanUserMenu(LoanManager &loanManager, BankAccount &currentUser,
                  const string &currentUserId, AccountManager &manager) {
  bool loadMenu = true;
  while (loadMenu) {
    cout << "\n===== LOAN SERVICES =====\n";
    cout << "1. Apply for Loan\n";
    cout << "2. View My Loans\n";
    cout << "3. Make EMI Payment\n";
    cout << "4. View Payment History\n";
    cout << "5. Early Closure\n";
    cout << "6. Back\n";
    int subChoice = InputValidator::getInt("Enter your choice: ");

    switch (subChoice) {
    case 1: {
      string loanType;
      cout << "1. Personal\n";
      cout << "2. Home\n";
      cout << "3. Auto\n";
      cout << "4. Education\n";
      int choice =
          InputValidator::getInt("Enter the type of loan required(1-4): ");
      switch (choice) {
      case 1:
        loanType = "Personal";
        break;
      case 2:
        loanType = "Home";
        break;
      case 3:
        loanType = "Auto";
        break;
      case 4:
        loanType = "Education";
        break;

      default:
        cout << "Invalid choice.\n";
        break;
      }

      double loanAmount =
          InputValidator::getPositiveDouble("Enter the loan amount required: ");

      if (loanManager.checkEligibility(currentUser, loanType, loanAmount)) {
        int tenure = InputValidator::getInt("Enter tenure in months: ");
        string loanId =
            loanManager.applyForLoan(currentUser, loanType, loanAmount, tenure);
        if (!loanId.empty())
          cout << "Loan applied successfully! Loan ID: " << loanId << "\n";
      }
      break;
    }
    case 2:
      loanManager.viewUserLoans(currentUserId);
      break;

    case 3: {
      loanManager.viewUserLoans(currentUserId);
      string loanId = InputValidator::getString("Enter Loan ID: ");
      try {
          loanManager.makeEMIPayment(loanId, currentUser);
      } catch (const LoanException& e) {
          cout << "EMI payment failed: " << e.what() << "\n";
      }
      break;
    }
    case 4: {
      loanManager.viewUserLoans(currentUserId);
      string loanId = InputValidator::getString("Enter Loan ID: ");
      loanManager.viewPaymentHistory(loanId);
      break;
    }

    case 5: {
      loanManager.viewUserLoans(currentUserId);
      string loanId = InputValidator::getString("Enter Loan ID: ");
      double amount = loanManager.calculateEarlyClosureAmount(loanId);
      cout << "Early closure amount: Rs." << fixed << setprecision(2) << amount
           << "\n";
      string confirm =
          InputValidator::getString("Confirm early closure? (yes/no): ");
      if (confirm == "yes") {
        try {
            loanManager.closeLoanEarly(loanId, currentUser);
        } catch (const LoanException& e) {
            cout << "EMI payment failed: " << e.what() << "\n";
        }
      }
      break;
    }
    case 6:
      loadMenu = false;
      break;
    }
  }
}

void rdMenu(RDManager &rdManager, BankAccount &currentUser,
            const string &currentUserId, AccountManager &manager) {

  bool rdMenu = true;
  while (rdMenu) {
    cout << "\n===== RD SERVICES =====\n";
    cout << "1. Open New RD\n";
    cout << "2. View My RDs\n";
    cout << "3. View RD Details\n";
    cout << "4. Cancel RD\n";
    cout << "5. Back\n";
    int subChoice = InputValidator::getInt("Enter choice: ");
    switch (subChoice) {
    case 1: {
      double amount =
          InputValidator::getPositiveDouble("Monthly deposit amount: Rs.");
      int tenure = InputValidator::getInt("Tenure in months: ");
      double rate = InputValidator::getPositiveDouble("Interest rate (%): ");
      rdManager.openRD(currentUser, amount, tenure, rate);
      manager.save();
      break;
    }
    case 2:
      rdManager.viewUserRDs(currentUserId);
      break;
    case 3: {
      string rdId = InputValidator::getString("Enter RD ID: ");
      rdManager.viewRDDetails(rdId);
      break;
    }
    case 4: {
      rdManager.viewUserRDs(currentUserId);
      string rdId = InputValidator::getString("Enter RD ID to cancel: ");
      rdManager.cancelRD(rdId, currentUser);
      manager.save();
      break;
    }
    case 5:
      rdMenu = false;
      break;
    }
  }
}

void siMenu(StandingInstructionManager &siManager,
            const string &currentUserId) {

  bool siMenu = true;
  while (siMenu) {
    cout << "\n===== STANDING INSTRUCTIONS =====\n";
    cout << "1. Create Standing Instruction\n";
    cout << "2. View My Standing Instructions\n";
    cout << "3. View SI Details\n";
    cout << "4. Cancel Standing Instruction\n";
    cout << "5. Back\n";
    int subChoice = InputValidator::getInt("Enter choice: ");
    switch (subChoice) {
    case 1: {
      string receiverAcc =
          InputValidator::getString("Receiver account number: ");
      double amount = InputValidator::getPositiveDouble("Amount: Rs.");
      int day = InputValidator::getInt("Execution day (1-28): ");
      string desc = InputValidator::getString("Description: ");
      siManager.createSI(currentUserId, receiverAcc, amount, day, desc);
      break;
    }
    case 2:
      siManager.viewUserSIs(currentUserId);
      break;
    case 3: {
      string siId = InputValidator::getString("Enter SI ID: ");
      siManager.viewSIDetails(siId);
      break;
    }
    case 4: {
      siManager.viewUserSIs(currentUserId);
      string siId = InputValidator::getString("Enter SI ID to cancel: ");
      siManager.cancelSI(siId, currentUserId);
      break;
    }
    case 5:
      siMenu = false;
      break;
    }
  }
}

void adminMenu(AccountManager &manager, LoanManager &loanManager,
               const string &currentUserId, const string &sessionToken) {
  bool adminLoggedIn = true;
  while (adminLoggedIn) {

    cout << "\n===== ADMIN MENU =====\n";
    cout << "1. View All Accounts\n";
    cout << "2. View All Accounts(Sorted)\n";
    cout << "3. Freeze Account\n";
    cout << "4. Unfreeze Account\n";
    cout << "5. Delete Account\n";
    cout << "6. Show Total Bank Balance\n";
    cout << "7. Set Account Limits\n";
    cout << "8. Apply Interest\n";
    cout << "9. Loan Section\n";
    cout << "10. Check for fraud\n";
    cout << "11. Logout\n";

    int adminChoice = InputValidator::getInt("Enter your choice: ");

    switch (adminChoice) {

    case 1:
      manager.viewAllAccounts();
      break;
    case 2: {
      cout << "\n===== SORT ACCOUNTS BY =====\n";
      cout << "1. Account Number\n";
      cout << "2. Name (A-Z)\n";
      cout << "3. Balance (High to Low)\n";
      cout << "4. Balance (Low to High)\n";

      int sortChoice = InputValidator::getInt("Enter choice: ");

      string sortBy;
      switch (sortChoice) {
      case 1:
        sortBy = "account";
        break;
      case 2:
        sortBy = "name";
        break;
      case 3:
        sortBy = "balance_high";
        break;
      case 4:
        sortBy = "balance_low";
        break;
      default:
        cout << "Invalid choice.\n";
        break;
      }

      if (sortChoice >= 1 && sortChoice <= 4) {
        manager.viewAllAccountsSorted(sortBy);
      }
      break;
    }

    case 3: {
      string acc =
          InputValidator::getString("Enter account number to freeze: ");
      manager.freezeAccount(acc);
      DatabaseManager::logAudit(currentUserId, sessionToken, "ADMIN_FREEZE",
                                acc, "SUCCESS");
      break;
    }

    case 4: {
      string acc =
          InputValidator::getString("Enter account number to unfreeze: ");
      manager.unfreezeAccount(acc);
      DatabaseManager::logAudit(currentUserId, sessionToken, "ADMIN_UNFREEZE",
                                acc, "SUCCESS");
      break;
    }

    case 5: {
      string acc =
          InputValidator::getString("Enter account number to be deleted: ");
      manager.deleteAccount(acc);
      break;
    }

    case 6:
      manager.showTotalBankBalance();
      break;

    case 7: {
      string acc = InputValidator::getString(
          "Enter account number to set transaction limits: ");
      manager.setAccountLimits(acc);
      break;
    }

    case 8: {
      cout << "\nApplying interest to all accounts based on their "
              "account type...\n";
      cout << "- Savings Account: 4% p.a.\n";
      cout << "- Current Account: 0% p.a. (no interest)\n";
      cout << "- Fixed Deposit: 7% p.a.\n\n";

      char confirm =
          InputValidator::getChar("Proceed with interest application? (y/n): ");

      if (confirm == 'y' || confirm == 'Y') {
        manager.applyInterestToAll();
        cout << "Interest applied successfully!\n";
      } else {
        cout << "Interest application cancelled.\n";
      }
      break;
    }

    case 9: {
      loanAdminMenu(loanManager, manager);
    } break;
    case 10: {
      string acc = InputValidator::getString(
          "Enter the account number to check for fraud: ");
      BankAccount *account = manager.getAccountByAccountNumber(acc);
      if (account) {
        account->checkForSuspiciousActivity();
      } else {
        cout << "Account not found.\n";
      }
      break;
    }

    case 11:
      cout << "Admin logging out...\n";
      adminLoggedIn = false;
      break;

    default:
      cout << "Invalid option.\n";
    }
  }
}

void userMenu(BankAccount &currentUser, const string &currentUserId,
              AccountManager &manager, LoanManager &loanManager,
              RDManager &rdManager, StandingInstructionManager &siManager,
              const string &sessionToken) {

  currentUser.loadTransactionsFromFile();

  rdManager.processAutoDebits(currentUser);
  loanManager.processEMIAutoDebits(currentUser, manager);
  siManager.processAutoExecutions(currentUser, manager);
  manager.save();

  bool loggedIn = true;

  while (loggedIn) {

    cout << "\n===== ACCOUNT MENU =====\n";
    cout << "1. Deposit\n";
    cout << "2. Withdraw\n";
    cout << "3. Transfer\n";
    cout << "4. Show Balance\n";
    cout << "5. Show Transaction History\n";
    cout << "6. Search Transactions by date\n";
    cout << "7. Search Transactions by type\n";
    cout << "8. Search Transactions by Amount(Binary Search)\n";
    cout << "9. View My Limits\n";
    cout << "10. Show Mini Statement\n";
    cout << "11. Show Account Summary\n";
    cout << "12. Change your current pin\n";
    cout << "13. Loan Services\n";
    cout << "14. Check for suspicious activity\n";
    cout << "15. Show Monthly Statement\n";
    cout << "16. Spending Patterns\n";
    cout << "17. Show Interest Summary\n";
    cout << "18. RD Services\n";
    cout << "19. Standing Instructions\n";
    cout << "20. Export Transactions in CSV\n";
    cout << "21. Logout\n";

    int userChoice = InputValidator::getInt("Enter choice: ");

    switch (userChoice) {

    case 1: {
      double amount =
          InputValidator::getPositiveDouble("Enter amount to deposit: ");
      try {
        currentUser.depositMoney(amount);
        manager.save();
        cout << "Deposit successful.\n";
        DatabaseManager::logAudit(currentUserId, sessionToken, "DEPOSIT",
                                  "Amount: " + to_string(amount), "SUCCESS");
      } catch (const BankException &e) {
        cout << "Transaction failed: " << e.what() << "\n";
        DatabaseManager::logAudit(currentUserId, sessionToken, "DEPOSIT",
                                  e.what(), "FAILED");
      }
      break;
    }

    case 2: {
      double amount =
          InputValidator::getPositiveDouble("Enter amount to withdraw: ");
      try {
        currentUser.withdrawMoney(amount);
        manager.save();
        cout << "Withdrawal successful.\n";
        DatabaseManager::logAudit(currentUserId, sessionToken, "WITHDRAWAL",
                                  "Amount: " + to_string(amount), "SUCCESS");
      } catch (const BankException &e) {
        cout << "Transaction failed: " << e.what() << "\n";
        DatabaseManager::logAudit(currentUserId, sessionToken, "WITHDRAW",
                                  e.what(), "FAILED");
      }
      break;
    }

    case 3: {
      string receiverAcc =
          InputValidator::getString("Enter receiver account number: ");

      if (receiverAcc == currentUser.getAccountNumber()) {
        cout << "Cannot transfer to same account.\n";
        break;
      }

      BankAccount *receiver = manager.getAccountByAccountNumber(receiverAcc);

      if (!receiver) {
        cout << "Receiver not found.\n";
        break;
      }

      double amount =
          InputValidator::getPositiveDouble("Enter amount to transfer: ");
      try {
        currentUser.transferMoney(*receiver, amount);
        manager.save();
        DatabaseManager::logAudit(
            currentUserId, sessionToken, "TRANSFER",
            "To: " + receiverAcc + " Amount: " + to_string(amount), "SUCCESS");
      }catch (const BankException &e) {
        cout << "Transaction failed: " << e.what() << "\n";
        DatabaseManager::logAudit(currentUserId, sessionToken, "TRANSFER",
                                  e.what(), "FAILED");
      }
      break;
    }

    case 4:
      currentUser.showBalance();
      break;

    case 5:
      currentUser.showTransactionHistory();
      break;

    case 6: {
      string startDate =
          InputValidator::getString("Enter the start date ([YYYY-MM-DD]): ");
      string endDate =
          InputValidator::getString("Enter the end date ([YYYY-MM-DD]): ");
      currentUser.searchTransactionsByDate(startDate, endDate);
      break;
    }

    case 7: {
      cout << "\n===== SELECT TRANSACTION TYPE =====\n";
      cout << "1. Deposit\n";
      cout << "2. Withdraw\n";
      cout << "3. Transfer (Sent/Received)\n";
      cout << "4. Interest\n";
      cout << "5. All Transactions\n";

      int typeChoice = InputValidator::getInt("Enter your choice: ");

      string searchType;
      switch (typeChoice) {
      case 1:
        searchType = "Deposit";
        break;
      case 2:
        searchType = "Withdraw";
        break;
      case 3:
        searchType = "Transfer";
        break;
      case 4:
        searchType = "Interest";
        break;
      case 5:
        currentUser.showTransactionHistory();
        break;
      default:
        cout << "Invalid choice.\n";
        break;
      }

      if (typeChoice >= 1 && typeChoice <= 4) {
        currentUser.searchTransactionsByType(searchType);
      }
      break;
    }

    case 8:
      currentUser.searchTransactionByAmountInteractive();
      break;

    case 9:
      currentUser.showLimits();
      break;

    case 10:
      currentUser.showMiniStatement();
      break;

    case 11:
      currentUser.showAccountSummary();
      break;

    case 12: {
      string pinStr = InputValidator::getPin("Enter your current pin: ");
      int currentPin = stoi(pinStr);

      string newPinStr = InputValidator::getPin("Enter your new pin: ");
      int newPin = stoi(newPinStr);

      string confirmationPinStr =
          InputValidator::getPin("Please confirm your new pin: ");
      int confirmationPin = stoi(confirmationPinStr);

      if (newPin != confirmationPin) {
        cout << "PINs do not match. Please try again.\n";
        break;
      }

      if (currentUser.changePin(currentPin, newPin)) {
        cout << "The pin has been changed for "
             << currentUser.getAccountNumber() << "\n";
        manager.save();
        DatabaseManager::logAudit(currentUserId, sessionToken, "PIN_CHANGE", "",
                                  "SUCCESS");
      } else {
        cout << "Failed to change the pin for "
             << currentUser.getAccountNumber();
      }
      break;
    }
    case 13: {
      loanUserMenu(loanManager, currentUser, currentUserId, manager);
    } break;

    case 14:
      currentUser.checkForSuspiciousActivity();
      break;

    case 15: {
      int month = InputValidator::getInt("Enter month (1-12): ");
      int year = InputValidator::getInt("Enter year (e.g. 2026): ");
      currentUser.showMonthlyStatement(month, year);
      break;
    }
    case 16: {
      cout << "1. All Time\n";
      cout << "2. Specific Year\n";
      int choice = InputValidator::getInt("Enter choice: ");
      if (choice == 1) {
        currentUser.showSpendingPatterns();
      } else {
        int year = InputValidator::getInt("Enter year (e.g. 2026): ");
        currentUser.showSpendingPatterns(year);
      }
      break;
    }
    case 17:
      currentUser.showInterestSummary();
      break;
    case 18: {
      rdMenu(rdManager, currentUser, currentUserId, manager);
    } break;
    case 19: {
      siMenu(siManager, currentUserId);
    } break;

    case 20: {
        try {
            currentUser.exportToCSV();
        } catch (const DatabaseException& e) {
            cout << "Export failed: " << e.what() << "\n";
        }
        break;
    }
    case 21: {
      DatabaseManager::deleteSession(sessionToken);
      DatabaseManager::logAudit(currentUserId, sessionToken, "LOGOUT", "",
                                "SUCCESS");
      cout << "Logging out...\n";
      loggedIn = false;
      break;
    }
    default:
      cout << "Invalid option.\n";
    }
  }
}

int main() {

  Config::getInstance().load("config.ini");

  AccountManager manager;

  try {
    manager.load();
  } catch (const DatabaseException& e) {
      cout << "Fatal error: " << e.what() << "\n";
      return 1;
  }
  
  LoanManager loanManager;
  RDManager rdManager;
  StandingInstructionManager siManager;

  while (true) {

    cout << "\n===== MAIN MENU =====\n";
    cout << "1. Create Account\n";
    cout << "2. Login\n";
    cout << "3. Exit\n";

    int choice = InputValidator::getInt("Enter your choice: ");

    switch (choice) {

    // ================= CREATE ACCOUNT =================
    case 1: {
      string name = InputValidator::getString("Enter Name: ");

      // Choose account type
      cout << "\n===== SELECT ACCOUNT TYPE =====\n";
      cout << "1. Savings Account (4% interest p.a.)\n";
      cout << "2. Current Account (0% interest, for business)\n";
      cout << "3. Fixed Deposit (7% interest p.a.)\n";

      int typeChoice = InputValidator::getInt("Enter choice: ");

      string accType;
      switch (typeChoice) {
      case 1:
        accType = "Savings";
        break;
      case 2:
        accType = "Current";
        break;
      case 3:
        accType = "Fixed Deposit";
        break;
      default:
        cout << "Invalid choice. Defaulting to Savings Account.\n";
        accType = "Savings";
      }

      string pinStr = InputValidator::getPin("Create a pin: ");
      int pin = stoi(pinStr);

      manager.createAccount(name, pin, accType);
      break;
    }

    // ================= LOGIN =================
    case 2: {
      string accNo = InputValidator::getString("Enter Your Account Number: ");

      string currentUserId = "";
      string sessionToken = "";

      const int MAX_ATTEMPTS = Config::getInstance().getInt("max_login_attempts", 3);
      const int COOLDOWN_SECONDS = Config::getInstance().getInt("cooldown_seconds", 10);

      int attempts = 0;

      // ===== PIN ATTEMPT LOOP =====
      if (DatabaseManager::checkRateLimit(accNo)) {
        cout << "Too many failed attempts. Please try again later.\n";
        break;
      }
      if (!manager.getAccountByAccountNumber(accNo)) {
        cout << "Account not found.\n";
        break;
      }

      while (attempts < MAX_ATTEMPTS) {
        string pinStr = InputValidator::getPin("Enter PIN: ");
        int pin = stoi(pinStr);
        currentUserId = manager.loginAccount(accNo, pin);
        if (!currentUserId.empty()) {
          sessionToken = DatabaseManager::createSession(currentUserId);
          DatabaseManager::logAudit(currentUserId, sessionToken, "LOGIN", "",
                                    "SUCCESS");
          cout << "Login successful!\n";
          break;
        }
        DatabaseManager::recordFailedAttempt(accNo);
        DatabaseManager::logAudit(accNo, "", "LOGIN", "Failed PIN attempt",
                                  "FAILED");
        attempts++;
        cout << "Incorrect PIN. Attempts left: " << (MAX_ATTEMPTS - attempts)
             << "\n";
      }

      // ===== COOLDOWN =====
      if (currentUserId.empty()) {

        cout << "\nToo many failed attempts.\n";
        cout << "Retrying allowed in " << COOLDOWN_SECONDS << " seconds.\n";

        for (int i = COOLDOWN_SECONDS; i > 0; --i) {
          cout << "Retry in " << i << " seconds...\r" << flush;
          this_thread::sleep_for(chrono::seconds(1));
        }

        cout << "\nReturning to main menu.\n";
        break;
      }

      // Fetch logged-in account safely
      BankAccount *currentUser =
          manager.getAccountByAccountNumber(currentUserId);

      if (!currentUser) {
        cout << "Unexpected error. Please login again.\n";
        break;
      }

      // ================= ROLE CHECK =================
      if (currentUser->getRole() == "admin") {
        cout << "Welcome Admin.\n";
        adminMenu(manager, loanManager, currentUserId, sessionToken);
      }

      // ================= USER MENU =================
      else {
        userMenu(*currentUser, currentUserId, manager, loanManager, rdManager,
                 siManager, sessionToken);
      }

      break;
    }

    // ================= EXIT =================
    case 3:
      manager.save();
      rdManager.saveRDs();
      loanManager.saveLoans();
      siManager.saveSIs();
      DatabaseManager::close();
      cout << "Thank you for banking with us.\n";
      return 0;

    default:
      cout << "Invalid choice.\n";
    }
  }

  return 0;
}
#include "AccountManager.h"
#include "BankAccount.h"
#include "InputValidator.h"
#include "LoanManager.h"

#include <chrono>
#include <iomanip>
#include <iostream>
#include <string>
#include <thread>

using namespace std;

int main() {

  AccountManager manager;
  manager.load();
  LoanManager loanManager;

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

      const int MAX_ATTEMPTS = 3;
      const int COOLDOWN_SECONDS = 10;

      int attempts = 0;

      // ===== PIN ATTEMPT LOOP =====
      while (attempts < MAX_ATTEMPTS) {
        string pinStr = InputValidator::getPin("Enter PIN: ");
        int pin = stoi(pinStr);
        currentUserId = manager.loginAccount(accNo, pin);
        if (!currentUserId.empty()) {
          cout << "Login successful!\n";
          break;
        }
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
            break;
          }

          case 4: {
            string acc =
                InputValidator::getString("Enter account number to unfreeze: ");
            manager.unfreezeAccount(acc);
            break;
          }

          case 5: {
            string acc = InputValidator::getString(
                "Enter account number to be deleted: ");
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

            char confirm = InputValidator::getChar("Proceed with interest application? (y/n): ");
            
            if (confirm == 'y' || confirm == 'Y') {
              manager.applyInterestToAll();
              cout << "Interest applied successfully!\n";
            } else {
              cout << "Interest application cancelled.\n";
            }
            break;
          }
          case 9:
          {
            bool loadMenu = true;
            while(loadMenu){
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
                
                case 3:
                {
                  string loanId = InputValidator::getString("Enter loan ID: ");
                  loanManager.approveLoan(loanId);
                  break;
                }
                case 4:
                {
                  string loanId = InputValidator::getString("Enter loan ID: ");
                  string reason = InputValidator::getString("Enter rejection reason: ");
                  loanManager.rejectLoan(loanId, reason);
                  break;
                }

                case 5:
                {
                  string loanId = InputValidator::getString("Enter loan ID: ");
                  Loan* loan = loanManager.getLoanById(loanId);
                  if (!loan) { cout << "Loan not found.\n"; break; }
                  BankAccount* acc = manager.getAccountByAccountNumber(loan->accountNumber);
                  if (!acc) { cout << "Account not found.\n"; break; }
                  loanManager.disburseLoan(loanId, *acc);
                  break;
                }

                case 6:
                  loadMenu = false;
                  break;
              }
              break;
            }
          }

          case 10:
          {
            string acc = InputValidator::getString("Enter the account number to check for fraud: ");
            BankAccount* account = manager.getAccountByAccountNumber(acc);
            if(account){
              account->checkForSuspiciousActivity();
            }
            else{
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

      // ================= USER MENU =================
      else {

        currentUser->loadTransactionsFromFile();

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
          cout << "15. Logout\n";

          int userChoice = InputValidator::getInt("Enter choice: ");

          switch (userChoice) {

          case 1: {
            double amount =
                InputValidator::getPositiveDouble("Enter amount to deposit: ");

            currentUser->depositMoney(amount);
            manager.save();
            cout << "Deposit successful.\n";
            break;
          }

          case 2: {
            double amount =
                InputValidator::getPositiveDouble("Enter amount to withdraw: ");

            if (currentUser->withdrawMoney(amount)) {
              manager.save();
              cout << "Withdrawal successful.\n";
            }

            break;
          }

          case 3: {
            string receiverAcc =
                InputValidator::getString("Enter receiver account number: ");

            if (receiverAcc == currentUser->getAccountNumber()) {
              cout << "Cannot transfer to same account.\n";
              break;
            }

            BankAccount *receiver =
                manager.getAccountByAccountNumber(receiverAcc);

            if (!receiver) {
              cout << "Receiver not found.\n";
              break;
            }

            double amount =
                InputValidator::getPositiveDouble("Enter amount to transfer: ");

            if (currentUser->transferMoney(*receiver, amount)) {

              manager.save();
            }

            break;
          }

          case 4:
            currentUser->showBalance();
            break;

          case 5:
            currentUser->showTransactionHistory();
            break;

          case 6: {
            string startDate = InputValidator::getString(
                "Enter the start date ([YYYY-MM-DD]): ");
            string endDate = InputValidator::getString(
                "Enter the end date ([YYYY-MM-DD]): ");
            currentUser->searchTransactionsByDate(startDate, endDate);
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
              currentUser->showTransactionHistory();
              break;
            default:
              cout << "Invalid choice.\n";
              break;
            }

            if (typeChoice >= 1 && typeChoice <= 4) {
              currentUser->searchTransactionsByType(searchType);
            }
            break;
          }

          case 8:
            currentUser->searchTransactionByAmountInteractive();
            break;

          case 9:
            currentUser->showLimits();
            break;

          case 10:
            currentUser->showMiniStatement();
            break;

          case 11:
            currentUser->showAccountSummary();
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

            if (currentUser->changePin(currentPin, newPin)) {
              cout << "The pin has been changed for "
                   << currentUser->getAccountNumber() << "\n";
              manager.save();
            } else {
              cout << "Failed to change the pin for "
                   << currentUser->getAccountNumber();
            }
            break;
          }
          case 13:
          {
            bool loadMenu = true;
            while(loadMenu){
              cout << "\n===== LOAN SERVICES =====\n";
              cout << "1. Apply for Loan\n";
              cout << "2. View My Loans\n";
              cout << "3. Make EMI Payment\n";
              cout << "4. View Payment History\n";
              cout << "5. Early Closure\n";
              cout << "6. Back\n";
              int subChoice = InputValidator::getInt("Enter your choice: ");

              switch (subChoice) {
                case 1:
                {
                  string loanType;
                  int choice = InputValidator::getInt("Enter the type of loan required(1-4): ");
                  switch(choice) {
                      case 1: loanType = "Personal"; break;
                      case 2: loanType = "Home";     break;
                      case 3: loanType = "Auto";     break;
                      case 4: loanType = "Education";break;
                      default: cout << "Invalid choice.\n"; break;
                  }
                  
                  double loanAmount = InputValidator::getPositiveDouble("Enter the loan amount required: ");

                  if(loanManager.checkEligibility(*currentUser, loanType, loanAmount)) {
                    int tenure = InputValidator::getInt("Enter tenure in months: ");
                    string loanId = loanManager.applyForLoan(*currentUser, loanType, loanAmount, tenure);
                    if (!loanId.empty())
                        cout << "Loan applied successfully! Loan ID: " << loanId << "\n";
                  }
                  break; 
                }
                case 2:
                  loanManager.viewUserLoans(currentUserId);
                  break;
                
                case 3: 
                {
                  loanManager.viewUserLoans(currentUserId); 
                  string loanId = InputValidator::getString("Enter Loan ID: ");
                  loanManager.makeEMIPayment(loanId, *currentUser);
                  break;
                }
                case 4:
                {
                  loanManager.viewUserLoans(currentUserId);
                  string loanId = InputValidator::getString("Enter Loan ID: ");
                  loanManager.viewPaymentHistory(loanId);
                  break;
                }

                case 5:
                {
                  loanManager.viewUserLoans(currentUserId);
                  string loanId = InputValidator::getString("Enter Loan ID: ");
                  double amount = loanManager.calculateEarlyClosureAmount(loanId);
                  cout << "Early closure amount: Rs." << fixed << setprecision(2) << amount << "\n";
                  string confirm = InputValidator::getString("Confirm early closure? (yes/no): ");
                  if (confirm == "yes") {
                      loanManager.closeLoanEarly(loanId, *currentUser);
                  }
                  break;
                }
                case 6:
                  loadMenu = false;
                  break;
              }
              break;
            }
          }
          
          case 14:
            currentUser->checkForSuspiciousActivity();
            break;

          case 15:
            cout << "Logging out...\n";
            loggedIn = false;
            break;

          default:
            cout << "Invalid option.\n";
          }
        }
      }

      break;
    }

    // ================= EXIT =================
    case 3:
      manager.save();
      cout << "Thank you for banking with us.\n";
      return 0;

    default:
      cout << "Invalid choice.\n";
    }
  }

  return 0;
}
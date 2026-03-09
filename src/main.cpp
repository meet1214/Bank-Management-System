#include "AccountManager.h"
#include "BankAccount.h"
#include "InputValidator.h"

#include <chrono>
#include <iostream>
#include <string>
#include <thread>

using namespace std;

int main() {

  AccountManager manager;
  manager.load();

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
          cout << "9. Check for fraud\n";
          cout << "10. Logout\n";

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

          case 10:
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
          cout << "8. View My Limits\n";
          cout << "9. Show Mini Statement\n";
          cout << "10. Show Account Summary\n";
          cout << "11. Change your current pin\n";
          cout << "12. Check for suspicious activity\n";
          cout << "13. Logout\n";

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
            currentUser->showLimits();
            break;

          case 9:
            currentUser->showMiniStatement();
            break;

          case 10:
            currentUser->showAccountSummary();
            break;

          case 11: {
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
          
          case 12:
            currentUser->checkForSuspiciousActivity();
            break;

          case 13:
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
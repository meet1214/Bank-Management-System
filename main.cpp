#include "AccountManager.h"
#include "Utils.h"

#include <chrono>
#include <iostream>
#include <thread>

using namespace std;

int main() {

    AccountManager manager;
    manager.loadFromFile();

    while (true) {

        cout << "\n===== MAIN MENU =====\n";
        cout << "1. Create Account\n";
        cout << "2. Login\n";
        cout << "3. Exit\n";
        cout << "Enter choice: ";

        int choice;
        cin >> choice;

        if (cin.fail()) {
            cin.clear();
            cin.ignore(1000, '\n');
            cout << "Invalid input!\n";
            continue;
        }

        switch (choice) {

        // ================= CREATE ACCOUNT =================
        case 1: {

            string name;
            int pin;

            cout << "Enter Name: ";
            getline(cin >> ws, name);

            cout << "Create 4-digit PIN: ";
            cin >> pin;

            if (cin.fail() || pin < 1000 || pin > 9999) {
                cin.clear();
                cin.ignore(1000, '\n');
                cout << "Invalid PIN. Must be 4 digits.\n";
                break;
            }

            manager.createAccount(name, pin);
            break;
        }

        // ================= LOGIN =================
        case 2: {

            string accNo;
            cout << "Enter Your Account Number: ";
            getline(cin >> ws, accNo);

            string currentUserId = "";

            const int MAX_ATTEMPTS = 3;
            const int COOLDOWN_SECONDS = 10;

            int attempts = 0;

            // ===== PIN ATTEMPT LOOP =====
            while (attempts < MAX_ATTEMPTS) {

                cout << "Enter PIN: ";
                string pinStr = getMaskedInput();

                if (pinStr.length() != 4) {
                    cout << "PIN must be exactly 4 digits.\n";
                    continue;
                }

                int pin;

                try {
                    pin = stoi(pinStr);
                }
                catch (...) {
                    cout << "Invalid PIN format.\n";
                    continue;
                }

                currentUserId = manager.loginAccount(accNo, pin);

                if (!currentUserId.empty()) {
                    cout << "Login successful!\n";
                    break;
                }

                attempts++;
                cout << "Incorrect PIN. Attempts left: "
                     << (MAX_ATTEMPTS - attempts) << "\n";
            }

            // ===== COOLDOWN =====
            if (currentUserId.empty()) {

                cout << "\nToo many failed attempts.\n";
                cout << "Retrying allowed in "
                     << COOLDOWN_SECONDS
                     << " seconds.\n";

                for (int i = COOLDOWN_SECONDS; i > 0; --i) {
                    cout << "Retry in " << i
                         << " seconds...\r" << flush;
                    this_thread::sleep_for(chrono::seconds(1));
                }

                cout << "\nReturning to main menu.\n";
                break;
            }

            // Fetch logged-in account safely
            BankAccount* currentUser =
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
                        manager.viewAllAccounts();
                        break;

                    case 2: {
                        string acc;
                        cout << "Enter account number: ";
                        getline(cin >> ws, acc);
                        manager.freezeAccount(acc);
                        break;
                    }

                    case 3: {
                        string acc;
                        cout << "Enter account number: ";
                        getline(cin >> ws, acc);
                        manager.unfreezeAccount(acc);
                        break;
                    }

                    case 4: {
                        string acc;
                        cout << "Enter account number: ";
                        getline(cin >> ws, acc);
                        manager.deleteAccount(acc);
                        break;
                    }

                    case 5:
                        manager.showTotalBankBalance();
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
                    cout << "6. Logout\n";
                    cout << "Enter choice: ";

                    int userChoice;
                    cin >> userChoice;

                    if (cin.fail()) {
                        cin.clear();
                        cin.ignore(1000, '\n');
                        cout << "Invalid input!\n";
                        continue;
                    }

                    switch (userChoice) {

                    case 1: {
                        double amount;
                        cout << "Enter amount to deposit: ";
                        cin >> amount;

                        if (cin.fail() || amount <= 0) {
                            cin.clear();
                            cin.ignore(1000, '\n');
                            cout << "Invalid amount.\n";
                            break;
                        }

                        currentUser->depositMoney(amount);
                        manager.saveToFile();
                        cout << "Deposit successful.\n";
                        break;
                    }

                    case 2: {
                        double amount;
                        cout << "Enter amount to withdraw: ";
                        cin >> amount;

                        if (cin.fail() || amount <= 0) {
                            cin.clear();
                            cin.ignore(1000, '\n');
                            cout << "Invalid amount.\n";
                            break;
                        }

                        if (currentUser->withdrawMoney(amount)) {
                            manager.saveToFile();
                            cout << "Withdrawal successful.\n";
                        }

                        break;
                    }

                    case 3: {
                        string receiverAcc;
                        double amount;

                        cout << "Enter receiver account number: ";
                        getline(cin >> ws, receiverAcc);

                        if (receiverAcc ==
                            currentUser->getAccountNumber()) {
                            cout << "Cannot transfer to same account.\n";
                            break;
                        }

                        BankAccount* receiver =
                            manager.getAccountByAccountNumber(receiverAcc);

                        if (!receiver) {
                            cout << "Receiver not found.\n";
                            break;
                        }

                        cout << "Enter amount to transfer: ";
                        cin >> amount;

                        if (cin.fail() || amount <= 0) {
                            cin.clear();
                            cin.ignore(1000, '\n');
                            cout << "Invalid amount.\n";
                            break;
                        }

                        if (currentUser->transferMoney(
                                *receiver, amount)) {

                            manager.saveToFile();
                            cout << "Transfer successful.\n";
                        }

                        break;
                    }

                    case 4:
                        currentUser->showBalance();
                        break;

                    case 5:
                        currentUser->showTransactionHistory();
                        break;

                    case 6:
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
            manager.saveToFile();
            cout << "Thank you for banking with us.\n";
            return 0;

        default:
            cout << "Invalid choice.\n";
        }
    }

    return 0;
}
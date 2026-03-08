#include "InputValidator.h"
#include <algorithm>
#include <iostream>
#include "Utils.h"
#include <cctype>

using namespace std;
//===========Integer Failure=========
int InputValidator::getInt(const std::string &prompt) {
    int value;
    while (true) {
        cout << prompt;
        cin >> value;
        if(cin.fail()) {
            cin.clear();
            cin.ignore(1000, '\n');
            cout << "Invalid input! Try again.\n";
        } else {
            cin.ignore(1000, '\n');
            return value;
        }
    }
}

//===========Double Failure============
double InputValidator::getDouble(const std::string &prompt) {
    double value;
    while (true) {
        cout << prompt;
        cin >> value;
        if(cin.fail()) {
            cin.clear();
            cin.ignore(1000, '\n');
            cout << "Invalid input! Try again.\n";
        } else {
            cin.ignore(1000, '\n');
            return value;
        }
    }
}

//===========Positive Double Failure=============
double InputValidator::getPositiveDouble(const std::string &prompt) {
    double value;
    while (true) {
        cout << prompt;
        cin >> value;
        if(cin.fail()) {
            cin.clear();
            cin.ignore(1000, '\n');
            cout << "Invalid input! Try again.\n";
        } 
        else if(value <= 0){
            cin.clear();
            cin.ignore(1000, '\n');
            cout << "Please enter a positive number! Try again.\n";
        }
        
        else {
            cin.ignore(1000, '\n');
            return value;
        }
    }
}

//===============String Failure=====================
string InputValidator::getString(const std::string &prompt) {
    string value;
    while (true) {
        cout << prompt;
        getline(cin >> ws, value);
        if(value.empty()){
            cout << "Input cannot be empty! Try Again.\n";
        } else {
            return value;
        }
    }
}

//===================Pin Failure====================
string InputValidator::getPin(const std::string &prompt) {
    while (true) {
        cout << prompt;
        string input = getMaskedInput();

        if(input.length() != 4){
            cout << "Enter a valid 4 digit pin.\n";
        }
        else if (!all_of(input.begin(),input.end(),::isdigit)) {
            cout << "Please enter a digit only.\n";
        }
        else {
            return input;
        }
    }
}
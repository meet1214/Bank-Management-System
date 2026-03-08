#include "Utils.h"
#include <iostream>
#include <termios.h>
#include <unistd.h>

std::string getMaskedInput() {
    std::string input;
    char ch;

    struct termios oldt, newt;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ECHO);  // Disable echo
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);

    while (true) {
        ch = std::cin.get();

        if (ch == '\n')
            break;

        if (ch == 127) { // Backspace
            if (!input.empty()) {
                input.pop_back();
                std::cout << "\b \b";
            }
        } else {
            input += ch;
            std::cout << '*';
        }
    }

    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);  // Restore settings
    std::cout << std::endl;

    return input;
}
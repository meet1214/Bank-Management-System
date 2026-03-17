#ifndef STANDINGINSTRUCTION_H
#define STANDINGINSTRUCTION_H

#include <string>

struct StandingInstruction {
    std::string siId;
    std::string accountNumber;
    std::string receiverAccountNumber;
    double      amount;
    int         executionDay;
    std::string nextExecutionDate;
    std::string description;
    std::string status;
};

#endif
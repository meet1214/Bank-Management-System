#include <gtest/gtest.h>
#include "LoanManager.h"

TEST(LoanManagerTest, CalculateEMI_PersonalLoan) {
    LoanManager lm;
    double emi = lm.calculateEMI(100000, 12.0, 12);
    EXPECT_NEAR(emi, 8884.88, 1.0);
}

TEST(LoanManagerTest, CalculateEMI_HomeLoan) {
    LoanManager lm;
    double emi = lm.calculateEMI(5000000, 8.5, 240);
    EXPECT_NEAR(emi, 43391.16, 1.0);
}

TEST(LoanManagerTest, CalculateEMI_ZeroInterest) {
    LoanManager lm;
    double emi = lm.calculateEMI(12000, 0.0, 12);
    EXPECT_NEAR(emi, 1000.0, 0.01);
}

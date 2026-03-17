#include <gtest/gtest.h>
#include "RDManager.h"

TEST(RDManagerTest, CalculateMaturity_BasicRD) {
    RDManager rm;
    double maturity = rm.calculateMaturityAmount(1000,12,6.0);
    EXPECT_NEAR(maturity, 12390, 1.0);
}

TEST(RDManagerTest, CalculateMaturity_HigherAmount) {
    RDManager rm;
    double maturity = rm.calculateMaturityAmount(5000,24,7.0);
    EXPECT_NEAR(maturity, 128750, 1.0);
}

TEST(RDManagerTest, CalculateMaturity_ZeroInterest) {
    RDManager rm;
    double maturity = rm.calculateMaturityAmount(2000,6,0.0);
    EXPECT_NEAR(maturity, 12000, 0.01);
}

#include <gtest/gtest.h>
#include "Sha256.h"

#include "BankAccount.h"
#include "BankExceptions.h"

using namespace std;

TEST(SecurityTest, SamePinSameSaltGivesSameHash) {
    string hash1 = hashPin("1234", "TESTSALT");
    string hash2 = hashPin("1234", "TESTSALT");
    EXPECT_EQ(hash1, hash2);
}

TEST(SecurityTest, DifferentPinsGiveDifferentHashes) {
    string hash1 = hashPin("1201", "TESTSALT");
    string hash2 = hashPin("1214", "TESTSALT");
    EXPECT_NE(hash1, hash2);
}

TEST(SecurityTest, VerifyPin_CorrectPin_ReturnsTrue) {
    string salt = "TESTSALT";
    string hash = hashPin("1234", salt);
    EXPECT_TRUE(verifyPin(1234, hash, salt));
}

TEST(SecurityTest, VerifyPin_WrongPin_ReturnsFalse) {
    string salt = "TESTSALT";
    string hash = hashPin("1234", salt);
    EXPECT_FALSE(verifyPin(1214, hash, salt));
}

TEST(SecurityTest, SamePinDifferentSaltGivesDifferentHashes) {
    string hash1 = hashPin("1234", "salt1");
    string hash2 = hashPin("1234", "salt2");
    EXPECT_NE(hash1, hash2);
}

TEST(SecurityTest, WithdrawMoreThanBalance_ThrowsInsufficientFunds) {
    BankAccount acc("TEST001", "Test User", "hash", "salt", 
                    1000.0, "user", false, "Savings");
    EXPECT_THROW(acc.withdrawMoney(5000), InsufficientFundsException);
}

TEST(SecurityTest, DepositExceedsDailyLimit_ThrowsDailyLimitExceeded) {
    BankAccount acc("TEST001", "Test User", "hash", "salt",
                    0.0, "user", false, "Savings",
                    100000.0, 50000.0, 2, 200000.0);
    acc.depositMoney(1000);
    acc.depositMoney(1000);
    EXPECT_THROW(acc.depositMoney(1000), DailyLimitExceededException);
}
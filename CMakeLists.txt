cmake_minimum_required(VERSION 3.16)
project(BankManagementSystem)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SOURCES
    src/main.cpp
    src/AccountManager.cpp
    src/BankAccount.cpp
    src/DatabaseManager.cpp
    src/InputValidator.cpp
    src/LoanManager.cpp
    src/Logger.cpp
    src/RDManager.cpp
    src/StandingInstructionManager.cpp
    src/Utils.cpp
    src/NotificationService.cpp
    src/BackgroundWorker.cpp
)

add_executable(bank ${SOURCES})

find_package(SQLite3 REQUIRED)
target_link_libraries(bank PRIVATE ${SQLite3_LIBRARIES})
target_include_directories(bank PRIVATE src ${SQLite3_INCLUDE_DIRS})

# ===== TESTS =====
enable_testing()
find_package(GTest REQUIRED)

add_executable(bank_tests
    tests/test_main.cpp
    tests/test_loan.cpp
    tests/test_rd.cpp
    tests/test_security.cpp
    tests/test_background.cpp
    src/LoanManager.cpp
    src/RDManager.cpp
    src/BankAccount.cpp
    src/DatabaseManager.cpp
    src/AccountManager.cpp
    src/Logger.cpp
    src/InputValidator.cpp
    src/StandingInstructionManager.cpp
    src/Utils.cpp
    src/BackgroundWorker.cpp
)

target_include_directories(bank_tests PRIVATE src)
target_link_libraries(bank_tests PRIVATE
    GTest::GTest
    GTest::Main
    ${SQLite3_LIBRARIES}
)

include(GoogleTest)
gtest_discover_tests(bank_tests)
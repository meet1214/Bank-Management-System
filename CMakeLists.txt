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
)

add_executable(bank ${SOURCES})

find_package(SQLite3 REQUIRED)
target_link_libraries(bank PRIVATE ${SQLite3_LIBRARIES})
target_include_directories(bank PRIVATE src ${SQLite3_INCLUDE_DIRS})
target_include_directories(bank PRIVATE src)


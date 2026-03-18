# Bank Management System

A feature-rich, terminal-based banking application built in C++17, backed by SQLite3. Designed with real-world financial concepts including loans, recurring deposits, standing instructions, session management, fraud detection, and a full exception hierarchy.

---

## Features

### User Features
- **Account Management** — Create Savings, Current, or Fixed Deposit accounts with salted SHA-256 PIN hashing
- **Transactions** — Deposit, withdraw, and transfer funds with per-transaction and daily limits
- **Transaction History** — Full history, mini statement, monthly statement, and interest summary
- **Search & Analytics** — Search by date range, transaction type, or amount (binary search); spending pattern analysis
- **CSV Export** — Export full transaction history to a `.csv` file openable in Excel or Google Sheets
- **Loan Services** — Apply for Personal, Home, Auto, or Education loans; make EMI payments; early closure with penalty
- **Recurring Deposits (RD)** — Open RDs with configurable tenure and interest rate; auto-debit on login
- **Standing Instructions (SI)** — Schedule recurring monthly transfers to any account
- **Fraud Detection** — Rule-based suspicious activity detection (large transactions, rapid succession, near-limit activity)
- **PIN Management** — Change PIN securely with current PIN verification and masked input

### Admin Features
- **Account Operations** — View, freeze, unfreeze, and delete accounts
- **Sorted Account View** — Sort by account number, name, or balance
- **Interest Application** — Apply type-based interest to all accounts (rates configured in `config.ini`)
- **Loan Administration** — Approve, reject, and disburse loans
- **Fraud Review** — Run suspicious activity reports on any account
- **Transaction Limits** — Set per-account deposit, withdrawal, and daily limits

### Security Features
- Salted SHA-256 PIN hashing (custom implementation, no external crypto library)
- Session tokens with configurable expiry stored in SQLite
- Rate limiting — blocks login after 5 failed attempts within 60 seconds
- Full audit log for every sensitive action (login, logout, deposit, transfer, PIN change, freeze)
- Masked PIN input using POSIX `termios`
- Custom exception hierarchy for safe, predictable error handling

---

## Tech Stack

| Component | Technology |
|-----------|-----------|
| Language | C++17 |
| Database | SQLite3 |
| Build System | CMake 3.16+ |
| Testing | Google Test |
| Hashing | Custom SHA-256 with per-account salt |
| Platform | Linux / macOS |

---

## Project Structure

```
Bank-Management-System/
├── CMakeLists.txt                          # CMake build system
├── config.ini                              # Externalized configuration
├── README.md
├── tests/
│   ├── test_main.cpp                       # Google Test entry point
│   ├── test_loan.cpp                       # EMI calculation tests
│   ├── test_rd.cpp                         # RD maturity tests
│   └── test_security.cpp                   # PIN hashing and exception tests
└── src/
    ├── main.cpp                            # Entry point + menu functions
    ├── AccountManager.cpp/h                # User & admin account operations
    ├── BankAccount.cpp/h                   # Core account logic, transactions, analytics
    ├── BankExceptions.h                    # Custom exception hierarchy
    ├── Config.h                            # INI-style config file parser (singleton)
    ├── DatabaseManager.cpp/h               # All SQLite operations
    ├── LoanManager.cpp/h                   # Loan lifecycle management
    ├── RDManager.cpp/h                     # Recurring deposit management
    ├── StandingInstructionManager.cpp/h    # Scheduled monthly transfer management
    ├── InputValidator.cpp/h                # Safe input parsing
    ├── Logger.cpp/h                        # Singleton logger → data/bank.log
    ├── Sha256.h                            # Self-contained SHA-256 + salt implementation
    ├── Utils.cpp/h                         # Masked PIN input (termios)
    ├── Loan.h                              # Loan and LoanPayment structs
    ├── RD.h                                # RecurringDeposit struct
    └── StandingInstruction.h               # StandingInstruction struct
```

---

## Configuration

All key parameters are externalized in `config.ini` — no recompilation needed to change rates or limits:

```ini
[bank]
branch_code=BNGL
db_path=data/bank.db

[security]
max_login_attempts=3
cooldown_seconds=10

[interest]
savings_rate=4.0
fd_rate=7.0
current_rate=0.0

[loans]
personal_rate=12.0
home_rate=8.5
auto_rate=10.0
education_rate=9.0
personal_penalty=4.0
home_penalty=2.0
auto_penalty=3.0
education_penalty=1.0
```

---

## Exception Hierarchy

The project uses a custom exception hierarchy for safe, predictable error handling:

```
std::exception
└── BankException
    ├── InsufficientFundsException
    ├── AccountLockedException
    ├── DailyLimitExceededException
    ├── LoanException
    └── DatabaseException
```

All transaction operations throw typed exceptions that are caught at the menu level and logged to the audit trail.

---

## Database Schema

The application uses a single SQLite database (`data/bank.db`) with 9 tables:

| Table | Purpose |
|-------|---------|
| `accounts` | Account credentials, balance, limits, type |
| `transactions` | Full transaction history per account |
| `loans` | Loan records with full lifecycle state |
| `loan_payments` | EMI payment history per loan |
| `recurring_deposits` | RD records with auto-debit tracking |
| `standing_instructions` | Scheduled monthly transfer records |
| `sessions` | Active session tokens with expiry |
| `audit_log` | Immutable audit trail for all sensitive actions |
| `login_attempts` | Failed login attempts for rate limiting |

---

## Prerequisites

**Linux (Fedora/RHEL):**
```bash
sudo dnf install cmake sqlite-devel gcc-c++ gtest-devel
```

**Linux (Ubuntu/Debian):**
```bash
sudo apt install cmake libsqlite3-dev g++ libgtest-dev
```

**macOS:**
```bash
brew install cmake sqlite3 googletest
```

---

## Build & Run

```bash
# Clone the repository
git clone https://github.com/meet1214/Bank-Management-System.git
cd Bank-Management-System

# Configure
cmake -B build

# Build
cmake --build build

# Run
./build/bank
```

---

## Running Tests

```bash
cmake -B build
cmake --build build
./build/bank_tests
```

Expected output:
```
[==========] Running 13 tests from 3 test suites.
[  PASSED  ] 13 tests.
```

Test coverage:
- `LoanManagerTest` — EMI calculation for personal, home, and zero-interest loans
- `RDManagerTest` — RD maturity amount calculation
- `SecurityTest` — PIN hashing, verification, and exception throwing

---

## Default Admin Credentials

| Field | Value |
|-------|-------|
| Account Number | `ADMIN00000001` |
| PIN | `1234` |

> Change the admin PIN immediately after first login.

---

## Key Design Decisions

**Why SQLite over flat files?**
The previous version used pipe-delimited `.txt` files. SQLite provides ACID transactions, foreign key constraints with cascade deletes, and indexed queries — essential for a system where transaction history integrity is critical.

**Why custom SHA-256 over a library?**
The SHA-256 implementation in `Sha256.h` is self-contained with no external dependencies. Each account gets a unique random salt, so identical PINs produce different hashes. This keeps the build simple while maintaining real security.

**Why a config file?**
Interest rates, loan rates, and security parameters are business decisions that change over time. Hardcoding them forces recompilation for every change. `config.ini` externalizes these so they can be updated without touching source code.

**Why a custom exception hierarchy?**
`return false` silently swallows errors — the caller has to remember to check the return value. Typed exceptions make failure modes explicit, force the caller to handle them, and carry enough context to log meaningful audit entries.

**Why session tokens?**
Rather than trusting the account number string as identity throughout a session, a random 32-character hex token is issued on login and deleted on logout. Every sensitive operation logs the token to the audit table, creating a tamper-evident trail.

---

## Account Types & Interest Rates

| Account Type | Default Rate | Use Case |
|-------------|-------------|---------|
| Savings | 4.0% p.a. | General purpose |
| Current | 0.0% p.a. | Business transactions |
| Fixed Deposit | 7.0% p.a. | Long-term savings |

Rates are configurable in `config.ini`.

---

## Loan Products

| Loan Type | Default Rate | Max Amount | Foreclosure Penalty |
|-----------|------------|-----------|-------------------|
| Personal | 12.0% p.a. | Rs. 10,00,000 | 4% |
| Home | 8.5% p.a. | Rs. 1,00,00,000 | 2% |
| Auto | 10.0% p.a. | Rs. 20,00,000 | 3% |
| Education | 9.0% p.a. | Rs. 50,00,000 | 1% |

EMI is calculated using the standard reducing balance formula. Failed EMI payments incur a 5% penalty on the outstanding amount. All rates configurable in `config.ini`.

---

## Author

**Meet Brijeshkumar Patel**
[GitHub](https://github.com/meet1214) · [Repository](https://github.com/meet1214/Bank-Management-System)
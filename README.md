# 🏦 Bank Management System

> A production-grade, terminal-based banking application written in **C++17**, backed by **SQLite3** — built from scratch with real-world financial engineering: multithreading, ACID transactions, cryptographic security, fraud detection, and 16 passing unit tests.

---

## Why This Project Stands Out

Most student banking projects store data in flat files, use global variables, and have a single monolithic `main()`. This one doesn't.

| What most projects do | What this project does |
|-----------------------|------------------------|
| Flat-file `.txt` storage | 9-table SQLite3 schema with FK constraints |
| Plaintext passwords | Salted SHA-256 hashing (custom, no external lib) |
| No session concept | 32-char hex session tokens with expiry + audit log |
| Single-threaded | Dedicated `BackgroundWorker` class on its own thread |
| `return false` error handling | Full typed exception hierarchy |
| Hardcoded config | INI-style config parser (header-only singleton) |
| One big `main()` | 17 modular source files, each independently testable |

---

## Features

### 👤 User Features
- **Account Management** — Create Savings, Current, or Fixed Deposit accounts
- **Transactions** — Deposit, withdraw, and transfer with per-transaction and daily limits
- **Transaction History** — Full history, mini statement, monthly statement, and interest summary
- **Search & Analytics** — Search by date range, type, or amount (binary search); spending pattern analysis
- **CSV Export** — Export transaction history to `.csv`, openable in Excel or Google Sheets
- **Loan Services** — Apply for Personal, Home, Auto, or Education loans; EMI payments; early closure with penalty
- **Recurring Deposits (RD)** — Open RDs with configurable tenure and rate; auto-debit triggered on login
- **Standing Instructions (SI)** — Schedule recurring monthly transfers to any account
- **Fraud Detection** — Rule-based suspicious activity detection: large transactions, rapid succession, near-limit activity
- **Notifications** — In-app notification service for account events and alerts
- **PIN Management** — Change PIN with current PIN verification and masked terminal input

### 🔧 Admin Features
- **Account Operations** — View, freeze, unfreeze, and delete accounts
- **Sorted Views** — Sort accounts by number, name, or balance
- **Interest Application** — Apply type-based interest to all accounts (rates from `config.ini`)
- **Loan Administration** — Approve, reject, and disburse loans
- **Fraud Review** — Run suspicious activity reports on any account
- **Limit Controls** — Set per-account deposit, withdrawal, and daily limits

### 🔒 Security Features
- Salted SHA-256 PIN hashing — custom implementation in `Sha256.h`, zero external crypto dependencies
- Session tokens — 32-character random hex tokens issued on login, invalidated on logout
- Rate limiting — blocks login after 3 failed attempts within 10 seconds (configurable)
- Immutable audit log — every sensitive action (login, deposit, transfer, PIN change, freeze) recorded with token traceability
- Masked PIN input — via POSIX `termios`, PIN never echoes to terminal
- Typed exception hierarchy — all failures are explicit, logged, and handled at menu level

---

## Architecture

### Module Map

```
Bank-Management-System/
├── CMakeLists.txt                          # CMake 3.16+ build system
├── config.ini                              # Externalized rates, limits, security params
├── tests/
│   ├── test_main.cpp                       # Google Test entry point
│   ├── test_loan.cpp                       # EMI calculation tests
│   ├── test_rd.cpp                         # RD maturity tests
│   ├── test_security.cpp                   # PIN hashing + exception tests
│   └── test_background.cpp                 # Background worker thread lifecycle tests
└── src/
    ├── main.cpp                            # Entry point — 7 focused menu functions
    ├── BankAccount.cpp/h                   # Core account logic, transactions, analytics
    ├── AccountManager.cpp/h                # User & admin operations
    ├── DatabaseManager.cpp/h               # All SQLite3 queries and schema management
    ├── LoanManager.cpp/h                   # Full loan lifecycle (apply → EMI → closure)
    ├── RDManager.cpp/h                     # Recurring deposit management + auto-debit
    ├── StandingInstructionManager.cpp/h    # Scheduled monthly transfer management
    ├── BackgroundWorker.cpp/h              # std::thread session cleanup worker
    ├── NotificationService.cpp/h           # In-app account event notifications
    ├── Logger.cpp/h                        # Singleton file logger → data/bank.log
    ├── InputValidator.cpp/h                # Safe input parsing + boundary checking
    ├── Config.h                            # Header-only INI parser (singleton)
    ├── Sha256.h                            # Self-contained SHA-256 + salt (header-only)
    ├── Utils.cpp/h                         # Masked PIN input via POSIX termios
    ├── BankExceptions.h                    # Typed exception hierarchy
    ├── Loan.h                              # Loan + LoanPayment structs
    ├── RD.h                                # RecurringDeposit struct
    └── StandingInstruction.h               # StandingInstruction struct
```

### Concurrency Design

A dedicated `BackgroundWorker` thread runs independently of the main application thread:

```
Main Thread                        BackgroundWorker Thread
────────────────────               ──────────────────────────────
User logs in                       Sleeps on condition_variable
Session token issued          ←→   Wakes every 30 seconds
User performs operations           Queries sessions table
User logs out                      Deletes expired tokens
Stop flag set (atomic)             Receives stop signal, exits cleanly
                                   Thread joins — no leaks
```

Synchronization: `std::mutex` + `std::condition_variable` for sleep/wake, `std::atomic<bool>` stop flag for clean shutdown — no busy-waiting, no data races.

### Exception Hierarchy

```
std::exception
└── BankException
    ├── InsufficientFundsException
    ├── AccountLockedException
    ├── DailyLimitExceededException
    ├── LoanException
    └── DatabaseException
```

Every transaction operation throws typed exceptions. Caught at the menu level and logged to the audit trail with the active session token. No silent `return false` — every failure mode is explicit and traceable.

### Database Schema (9 Tables)

```
accounts ──────────────────────────────┐
    │                                   │
    ├── transactions                    │
    ├── loans ──── loan_payments        │
    ├── recurring_deposits              │
    ├── standing_instructions           │
    ├── sessions ───────────────────────┘ (token → account FK)
    ├── audit_log                         (immutable, append-only)
    └── login_attempts                    (rate limiting)
```

| Table | Key Design |
|-------|-----------|
| `accounts` | Salted hash stored, never raw PIN; type enum (SAVINGS / CURRENT / FD) |
| `transactions` | Indexed by account + date; binary search on amount |
| `loans` | Full lifecycle state: PENDING → APPROVED → DISBURSED → CLOSED |
| `loan_payments` | Per-EMI payment history with penalty tracking |
| `recurring_deposits` | Auto-debit flag + last-debit date for idempotency |
| `standing_instructions` | Monthly schedule with next-execution date |
| `sessions` | Token + expiry; cleaned by BackgroundWorker every 30s |
| `audit_log` | Append-only; no UPDATE/DELETE permitted on this table |
| `login_attempts` | Timestamp-windowed rate limiting |

---

## Tech Stack

| Component | Technology |
|-----------|------------|
| Language | C++17 |
| Database | SQLite3 |
| Build System | CMake 3.16+ |
| Testing | Google Test |
| Concurrency | std::thread · std::mutex · std::condition_variable · std::atomic |
| Hashing | Custom SHA-256 + per-account salt (header-only, zero dependencies) |
| Platform | Linux · macOS |

---

## Build & Run

**Ubuntu/Debian:**
```bash
sudo apt install cmake libsqlite3-dev g++ libgtest-dev
```

**Fedora/RHEL:**
```bash
sudo dnf install cmake sqlite-devel gcc-c++ gtest-devel
```

**macOS:**
```bash
brew install cmake sqlite3 googletest
```

```bash
git clone https://github.com/meet1214/Bank-Management-System.git
cd Bank-Management-System
cmake -B build
cmake --build build
./build/bank
```

---

## Running Tests

```bash
cmake -B build && cmake --build build
./build/bank_tests
```

```
[==========] Running 16 tests from 4 test suites.
[  PASSED  ] 16 tests.
```

| Test Suite | What It Covers |
|------------|---------------|
| `LoanManagerTest` | EMI calculation — personal, home, zero-interest edge cases |
| `RDManagerTest` | RD maturity amounts across multiple tenures and rates |
| `SecurityTest` | SHA-256 correctness, salt uniqueness, exception type verification |
| `BackgroundWorkerTest` | Thread start/stop lifecycle, clean shutdown via atomic flag |

---

## Configuration

All business parameters externalized in `config.ini` — change a rate, restart, done. No recompilation needed.

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

## Financial Products

### Account Types

| Type | Interest | Use Case |
|------|---------|---------|
| Savings | 4.0% p.a. | General purpose |
| Current | 0.0% p.a. | Business transactions |
| Fixed Deposit | 7.0% p.a. | Long-term savings |

### Loan Products

| Type | Rate | Max Amount | Foreclosure Penalty |
|------|------|-----------|-------------------|
| Personal | 12.0% p.a. | ₹10,00,000 | 4% |
| Home | 8.5% p.a. | ₹1,00,00,000 | 2% |
| Auto | 10.0% p.a. | ₹20,00,000 | 3% |
| Education | 9.0% p.a. | ₹50,00,000 | 1% |

EMI uses the **reducing balance formula**. Failed payments trigger a 5% penalty on outstanding principal. All rates configurable via `config.ini`.

---

## Key Engineering Decisions

**Why SQLite over flat files?**
The original implementation used pipe-delimited `.txt` files. An `INSERT OR REPLACE` bug silently triggered `ON DELETE CASCADE`, wiping the entire transaction history on every save. Migrating to SQLite gave ACID transactions, proper foreign key enforcement, and indexed queries — essential for a system where data integrity is non-negotiable. The bug was fixed with a safe upsert pattern.

**Why a dedicated `BackgroundWorker` class?**
Session cleanup cannot block the main thread. `BackgroundWorker` encapsulates a `std::thread` that sleeps on a `std::condition_variable` and wakes every 30 seconds to purge expired tokens. It shuts down cleanly via a `std::atomic<bool>` flag — no busy-waiting, no forced termination, no thread leaks.

**Why custom SHA-256 instead of OpenSSL?**
`Sha256.h` is a self-contained, header-only implementation with zero external dependencies. Each account gets a unique random salt at creation time — identical PINs produce different hashes. This keeps the CMake build simple (no crypto library linkage) while providing real cryptographic security.

**Why session tokens instead of account numbers?**
Trusting the account number string as identity is a classic IDOR vulnerability. A random 32-char hex token issued at login and deleted at logout means every audit log entry is tied to a specific authenticated session — tamper-evident and fully traceable.

**Why a typed exception hierarchy?**
`return false` forces the caller to remember to check. Typed exceptions make failure modes impossible to ignore, carry structured context for audit logging, and let the menu layer catch exactly what it needs without swallowing unrelated errors.

**Why externalize config?**
Interest rates are operational decisions, not code. Embedding them forces a recompile-and-redeploy for every rate adjustment. The `Config` singleton parses `config.ini` at startup — change a value, restart, done.

**Why refactor `main()` into 7 menu functions?**
A 500-line `main()` is untestable and unreadable. Breaking it into focused menu functions lets each path be tested in isolation and makes the call graph immediately clear.

---

## Default Admin Credentials

| Field | Value |
|-------|-------|
| Account Number | `ADMIN00000001` |
| PIN | `1234` |

> **Note:** This is a demonstration project. Change the admin PIN immediately after first login. Do not use real financial data.

---

## Author

**Meet Brijeshkumar Patel**

[GitHub](https://github.com/meet1214) · [Repository](https://github.com/meet1214/Bank-Management-System) · [LinkedIn](https://www.linkedin.com/in/meet-brijeshkumar-patel-bb95a2249) · meepatel086@gmail.com
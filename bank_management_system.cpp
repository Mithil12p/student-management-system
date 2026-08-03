/*
 * ============================================================
 *  BANK MANAGEMENT APPLICATION
 *  Language      : C++ (Object-Oriented)
 *  Storage       : Binary file (accounts.dat) via fstream
 *  Features      : Create Account, Deposit, Withdraw,
 *                  Balance Inquiry, Update, Close Account,
 *                  PIN-based authentication, Mini Statement
 * ============================================================
 *
 *  Compile :  g++ -std=c++17 -O2 -o bank bank_management_system.cpp
 *  Run     :  ./bank          (Linux/Mac)
 *             bank.exe        (Windows)
 * ============================================================
 */

#include <iostream>
#include <fstream>
#include <iomanip>
#include <cstring>
#include <limits>
#include <vector>
#include <ctime>

using namespace std;

const char* FILENAME = "accounts.dat";
const int   TXN_LOG_LIMIT = 5;   // how many recent transactions we keep per account

// ------------------------------------------------------------
//  Small struct to keep a short transaction history inline
//  inside each account record (fixed size -> keeps records
//  fixed-length so random access by position still works).
// ------------------------------------------------------------
struct Transaction {
    char type[12];   // "DEPOSIT", "WITHDRAW", "OPEN", "" (empty slot)
    double amount;
    char date[24];   // "DD-MM-YYYY HH:MM"
};

// ------------------------------------------------------------
//  Account class
//  Encapsulates all data + behaviour for a single bank account.
//  Kept as a POD-friendly class (fixed-size members only) so
//  it can be written/read directly as a binary block.
// ------------------------------------------------------------
class Account {
private:
    int    accountNumber;
    char   holderName[50];
    char   pin[7];          // 4-6 digit PIN stored as a string (not hashed - see README note)
    double balance;
    bool   active;           // false = closed account
    Transaction history[TXN_LOG_LIMIT];
    int    historyCount;     // how many of the slots above are used (0..TXN_LOG_LIMIT, ring buffer)

public:
    // ---- constructors ----
    Account() : accountNumber(0), balance(0.0), active(false), historyCount(0) {
        holderName[0] = '\0';
        pin[0] = '\0';
        memset(history, 0, sizeof(history));
    }

    // ---- setup a brand-new account ----
    void create(int accNo, const char* name, const char* pinCode, double openingDeposit) {
        accountNumber = accNo;
        strncpy(holderName, name, sizeof(holderName) - 1);
        holderName[sizeof(holderName) - 1] = '\0';
        strncpy(pin, pinCode, sizeof(pin) - 1);
        pin[sizeof(pin) - 1] = '\0';
        balance = openingDeposit;
        active = true;
        historyCount = 0;
        logTransaction("OPEN", openingDeposit);
    }

    // ---- getters ----
    int    getAccountNumber() const { return accountNumber; }
    const char* getHolderName() const { return holderName; }
    double getBalance() const { return balance; }
    bool   isActive() const { return active; }

    // ---- PIN check ----
    bool verifyPin(const char* enteredPin) const {
        return strcmp(pin, enteredPin) == 0;
    }

    void changePin(const char* newPin) {
        strncpy(pin, newPin, sizeof(pin) - 1);
        pin[sizeof(pin) - 1] = '\0';
    }

    // ---- core banking operations ----
    bool deposit(double amount) {
        if (amount <= 0) return false;
        balance += amount;
        logTransaction("DEPOSIT", amount);
        return true;
    }

    bool withdraw(double amount) {
        if (amount <= 0) return false;
        if (amount > balance) return false;   // insufficient funds
        balance -= amount;
        logTransaction("WITHDRAW", amount);
        return true;
    }

    void close() {
        active = false;
    }

    // ---- transaction history (ring buffer: keeps the last N) ----
    void logTransaction(const char* type, double amount) {
        time_t now = time(nullptr);
        tm* ltm = localtime(&now);

        Transaction t{};
        strncpy(t.type, type, sizeof(t.type) - 1);
        t.amount = amount;
        snprintf(t.date, sizeof(t.date), "%02d-%02d-%04d %02d:%02d",
                 ltm->tm_mday, 1 + ltm->tm_mon, 1900 + ltm->tm_year,
                 ltm->tm_hour, ltm->tm_min);

        // Shift older entries left if the buffer is full, then append.
        if (historyCount < TXN_LOG_LIMIT) {
            history[historyCount++] = t;
        } else {
            for (int i = 1; i < TXN_LOG_LIMIT; i++) history[i - 1] = history[i];
            history[TXN_LOG_LIMIT - 1] = t;
        }
    }

    void printMiniStatement() const {
        cout << "\n--- Last " << historyCount << " Transaction(s) for A/C " << accountNumber << " ---\n";
        cout << left << setw(12) << "Type" << setw(20) << "Date/Time" << "Amount\n";
        cout << string(45, '-') << "\n";
        for (int i = 0; i < historyCount; i++) {
            cout << left << setw(12) << history[i].type
                 << setw(20) << history[i].date
                 << fixed << setprecision(2) << history[i].amount << "\n";
        }
    }

    void printSummary() const {
        cout << left
             << setw(12) << accountNumber
             << setw(22) << holderName
             << setw(12) << fixed << setprecision(2) << balance
             << (active ? "ACTIVE" : "CLOSED") << "\n";
    }
};

// ------------------------------------------------------------
//  Bank class
//  Manages the collection of accounts and all file I/O.
//  This is the "manager" object the menu talks to; it never
//  exposes raw file offsets to the caller.
// ------------------------------------------------------------
class Bank {
public:
    // -------- helpers to read from console safely --------
    static void clearInputBuffer() {
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
    }

    static int readInt(const string& prompt) {
        int value;
        while (true) {
            cout << prompt;
            cin >> value;
            if (cin.fail()) {
                cout << "Invalid input. Please enter a valid number.\n";
                clearInputBuffer();
            } else {
                clearInputBuffer();
                return value;
            }
        }
    }

    static double readDouble(const string& prompt) {
        double value;
        while (true) {
            cout << prompt;
            cin >> value;
            if (cin.fail()) {
                cout << "Invalid input. Please enter a valid amount.\n";
                clearInputBuffer();
            } else {
                clearInputBuffer();
                return value;
            }
        }
    }

    static void readLine(const string& prompt, char* buffer, int size) {
        cout << prompt;
        cin.getline(buffer, size);
        while (strlen(buffer) == 0) {
            cin.getline(buffer, size);
        }
    }

    // Reads a PIN, allowing only digits, length 4-6
    static void readPin(const string& prompt, char* buffer, int size) {
        while (true) {
            cout << prompt;
            cin.getline(buffer, size);
            int len = strlen(buffer);
            bool valid = (len >= 4 && len <= 6);
            for (int i = 0; i < len && valid; i++)
                if (!isdigit(static_cast<unsigned char>(buffer[i]))) valid = false;

            if (valid) return;
            cout << "PIN must be 4-6 digits only. Try again.\n";
        }
    }

    // -------- account number generator --------
    static int nextAccountNumber() {
        ifstream fin(FILENAME, ios::binary);
        int maxAcc = 1000; // accounts start at 1001
        Account a;
        while (fin.read(reinterpret_cast<char*>(&a), sizeof(Account))) {
            if (a.getAccountNumber() > maxAcc) maxAcc = a.getAccountNumber();
        }
        fin.close();
        return maxAcc + 1;
    }

    // -------- find an account's byte position by account number --------
    static bool findAccount(int accNo, Account& result, streampos& pos) {
        ifstream fin(FILENAME, ios::binary);
        if (!fin) return false;

        Account a;
        while (fin.read(reinterpret_cast<char*>(&a), sizeof(Account))) {
            if (a.getAccountNumber() == accNo && a.isActive()) {
                result = a;
                pos = fin.tellg();
                pos -= static_cast<streamoff>(sizeof(Account));
                fin.close();
                return true;
            }
        }
        fin.close();
        return false;
    }

    // -------- save a record back to its position in the file --------
    static void saveAccount(const Account& acc, streampos pos) {
        fstream fs(FILENAME, ios::binary | ios::in | ios::out);
        fs.seekp(pos);
        fs.write(reinterpret_cast<const char*>(&acc), sizeof(Account));
        fs.close();
    }

    // -------- append a brand-new record --------
    static void appendAccount(const Account& acc) {
        ofstream fout(FILENAME, ios::binary | ios::app);
        fout.write(reinterpret_cast<const char*>(&acc), sizeof(Account));
        fout.close();
    }

    // ============================================================
    //  MENU OPERATIONS
    // ============================================================

    static void openAccount() {
        cout << "\n----- Open New Account -----\n";

        char name[50], pinCode[7];
        readLine("Enter Full Name        : ", name, sizeof(name));
        readPin ("Set a 4-6 digit PIN    : ", pinCode, sizeof(pinCode));
        double opening = readDouble("Enter Opening Deposit  : ");

        if (opening < 0) {
            cout << "Opening deposit cannot be negative. Operation cancelled.\n";
            return;
        }

        Account acc;
        int accNo = nextAccountNumber();
        acc.create(accNo, name, pinCode, opening);
        appendAccount(acc);

        cout << "\nAccount created successfully!\n";
        cout << "Your Account Number is: " << accNo << " (keep this safe)\n";
    }

    // Shared login step used by deposit/withdraw/balance/statement/close
    static bool authenticate(Account& acc, streampos& pos) {
        int accNo = readInt("Enter Account Number: ");
        if (!findAccount(accNo, acc, pos)) {
            cout << "No active account found with that number.\n";
            return false;
        }

        char enteredPin[7];
        readPin("Enter PIN: ", enteredPin, sizeof(enteredPin));

        if (!acc.verifyPin(enteredPin)) {
            cout << "Incorrect PIN. Access denied.\n";
            return false;
        }
        return true;
    }

    static void depositMoney() {
        cout << "\n----- Deposit -----\n";
        Account acc; streampos pos;
        if (!authenticate(acc, pos)) return;

        double amount = readDouble("Enter amount to deposit: ");
        if (acc.deposit(amount)) {
            saveAccount(acc, pos);
            cout << "Deposit successful. New balance: " << fixed << setprecision(2)
                 << acc.getBalance() << "\n";
        } else {
            cout << "Deposit failed. Amount must be greater than zero.\n";
        }
    }

    static void withdrawMoney() {
        cout << "\n----- Withdraw -----\n";
        Account acc; streampos pos;
        if (!authenticate(acc, pos)) return;

        double amount = readDouble("Enter amount to withdraw: ");
        if (acc.withdraw(amount)) {
            saveAccount(acc, pos);
            cout << "Withdrawal successful. New balance: " << fixed << setprecision(2)
                 << acc.getBalance() << "\n";
        } else {
            cout << "Withdrawal failed. Check the amount or available balance.\n";
        }
    }

    static void checkBalance() {
        cout << "\n----- Balance Inquiry -----\n";
        Account acc; streampos pos;
        if (!authenticate(acc, pos)) return;

        cout << "Account Holder : " << acc.getHolderName() << "\n";
        cout << "Current Balance: " << fixed << setprecision(2) << acc.getBalance() << "\n";
    }

    static void miniStatement() {
        cout << "\n----- Mini Statement -----\n";
        Account acc; streampos pos;
        if (!authenticate(acc, pos)) return;
        acc.printMiniStatement();
    }

    static void changePin() {
        cout << "\n----- Change PIN -----\n";
        Account acc; streampos pos;
        if (!authenticate(acc, pos)) return;

        char newPin[7];
        readPin("Enter new 4-6 digit PIN: ", newPin, sizeof(newPin));
        acc.changePin(newPin);
        saveAccount(acc, pos);
        cout << "PIN changed successfully.\n";
    }

    static void closeAccount() {
        cout << "\n----- Close Account -----\n";
        Account acc; streampos pos;
        if (!authenticate(acc, pos)) return;

        if (acc.getBalance() > 0) {
            cout << "Please withdraw the remaining balance ("
                 << fixed << setprecision(2) << acc.getBalance()
                 << ") before closing the account.\n";
            return;
        }

        char confirm;
        cout << "Are you sure you want to close this account? (y/n): ";
        cin >> confirm;
        clearInputBuffer();

        if (confirm == 'y' || confirm == 'Y') {
            acc.close();
            saveAccount(acc, pos);
            cout << "Account closed successfully.\n";
        } else {
            cout << "Close operation cancelled.\n";
        }
    }

    static void displayAllAccounts() {
        ifstream fin(FILENAME, ios::binary);
        if (!fin) {
            cout << "\nNo accounts found yet.\n";
            return;
        }

        Account a;
        bool found = false;
        cout << "\n----- All Accounts -----\n";
        while (fin.read(reinterpret_cast<char*>(&a), sizeof(Account))) {
            if (a.isActive()) {
                if (!found) {
                    cout << left << setw(12) << "AccNo" << setw(22) << "Name"
                         << setw(12) << "Balance" << "Status\n";
                    cout << string(55, '-') << "\n";
                    found = true;
                }
                a.printSummary();
            }
        }
        fin.close();
        if (!found) cout << "No active accounts found.\n";
    }
};

// ------------------------------------------------------------
//  Menu display
// ------------------------------------------------------------
void showMenu() {
    cout << "\n============================================\n";
    cout << "         BANK MANAGEMENT APPLICATION\n";
    cout << "============================================\n";
    cout << " 1. Open New Account\n";
    cout << " 2. Deposit\n";
    cout << " 3. Withdraw\n";
    cout << " 4. Balance Inquiry\n";
    cout << " 5. Mini Statement (last transactions)\n";
    cout << " 6. Change PIN\n";
    cout << " 7. Close Account\n";
    cout << " 8. Display All Accounts (admin view)\n";
    cout << " 9. Exit\n";
    cout << "============================================\n";
}

// ------------------------------------------------------------
//  main()
// ------------------------------------------------------------
int main() {
    int choice;

    cout << "Welcome to the Bank Management Application!\n";
    cout << "Data is stored persistently in '" << FILENAME << "'.\n";

    do {
        showMenu();
        choice = Bank::readInt("Enter your choice (1-9): ");

        switch (choice) {
            case 1: Bank::openAccount();        break;
            case 2: Bank::depositMoney();       break;
            case 3: Bank::withdrawMoney();      break;
            case 4: Bank::checkBalance();       break;
            case 5: Bank::miniStatement();      break;
            case 6: Bank::changePin();          break;
            case 7: Bank::closeAccount();       break;
            case 8: Bank::displayAllAccounts(); break;
            case 9: cout << "\nThank you for banking with us. Goodbye!\n"; break;
            default: cout << "Invalid choice! Please enter a number between 1 and 9.\n";
        }

    } while (choice != 9);

    return 0;
}

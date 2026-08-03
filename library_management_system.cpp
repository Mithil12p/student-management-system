/*
 * ============================================================
 *  LIBRARY MANAGEMENT SYSTEM
 *  Language      : C++ (Object-Oriented)
 *  Storage       : Binary files via fstream
 *                  - books.dat   -> all book records
 *                  - members.dat -> all member records
 *                  - issues.dat  -> all issue/return records
 *  Features      : Add Book, Add Member, Issue Book, Return Book,
 *                  Search by Title / Author, Display Books/Members,
 *                  View a Member's currently issued books
 * ============================================================
 *
 *  Compile :  g++ -std=c++17 -O2 -o library library_management_system.cpp
 *  Run     :  ./library          (Linux/Mac)
 *             library.exe        (Windows)
 * ============================================================
 */

#include <iostream>
#include <fstream>
#include <iomanip>
#include <cstring>
#include <limits>
#include <ctime>

using namespace std;

const char* BOOKS_FILE   = "books.dat";
const char* MEMBERS_FILE = "members.dat";
const char* ISSUES_FILE  = "issues.dat";

const int LOAN_PERIOD_DAYS = 14;   // how many days a book can be kept before it's due

// ------------------------------------------------------------
//  Book class
//  Fixed-size members only, so records can be written/read
//  directly as binary blocks and updated in place by seeking.
// ------------------------------------------------------------
class Book {
private:
    int  bookId;
    char title[100];
    char author[60];
    int  totalCopies;
    int  availableCopies;
    bool active;   // false = removed from catalogue

public:
    Book() : bookId(0), totalCopies(0), availableCopies(0), active(false) {
        title[0] = '\0';
        author[0] = '\0';
    }

    void create(int id, const char* t, const char* a, int copies) {
        bookId = id;
        strncpy(title, t, sizeof(title) - 1);  title[sizeof(title) - 1] = '\0';
        strncpy(author, a, sizeof(author) - 1); author[sizeof(author) - 1] = '\0';
        totalCopies = copies;
        availableCopies = copies;
        active = true;
    }

    int  getId() const { return bookId; }
    const char* getTitle() const { return title; }
    const char* getAuthor() const { return author; }
    int  getTotalCopies() const { return totalCopies; }
    int  getAvailableCopies() const { return availableCopies; }
    bool isActive() const { return active; }

    bool hasAvailableCopy() const { return availableCopies > 0; }
    void decreaseAvailable() { if (availableCopies > 0) availableCopies--; }
    void increaseAvailable() { if (availableCopies < totalCopies) availableCopies++; }

    void addCopies(int extra) {
        totalCopies += extra;
        availableCopies += extra;
    }

    void deactivate() { active = false; }

    // case-insensitive substring check, used by search
    static bool containsIgnoreCase(const char* haystack, const char* needle) {
        string h(haystack), n(needle);
        for (auto& c : h) c = tolower(static_cast<unsigned char>(c));
        for (auto& c : n) c = tolower(static_cast<unsigned char>(c));
        return h.find(n) != string::npos;
    }

    bool titleMatches(const char* query)  const { return containsIgnoreCase(title, query); }
    bool authorMatches(const char* query) const { return containsIgnoreCase(author, query); }

    void printRow() const {
        cout << left
             << setw(6)  << bookId
             << setw(35) << title
             << setw(25) << author
             << setw(10) << availableCopies
             << setw(6)  << totalCopies << "\n";
    }
};

// ------------------------------------------------------------
//  Member class
// ------------------------------------------------------------
class Member {
private:
    int  memberId;
    char name[50];
    char contact[20];
    bool active;

public:
    Member() : memberId(0), active(false) {
        name[0] = '\0';
        contact[0] = '\0';
    }

    void create(int id, const char* n, const char* c) {
        memberId = id;
        strncpy(name, n, sizeof(name) - 1);       name[sizeof(name) - 1] = '\0';
        strncpy(contact, c, sizeof(contact) - 1);  contact[sizeof(contact) - 1] = '\0';
        active = true;
    }

    int  getId() const { return memberId; }
    const char* getName() const { return name; }
    const char* getContact() const { return contact; }
    bool isActive() const { return active; }

    void deactivate() { active = false; }

    void printRow() const {
        cout << left
             << setw(6)  << memberId
             << setw(30) << name
             << setw(15) << contact << "\n";
    }
};

// ------------------------------------------------------------
//  IssueRecord class
//  Represents one "book borrowed by a member" transaction.
// ------------------------------------------------------------
class IssueRecord {
private:
    int  issueId;
    int  bookId;
    int  memberId;
    char issueDate[20];
    char dueDate[20];
    char returnDate[20];   // empty string until the book is returned
    bool returned;

public:
    IssueRecord() : issueId(0), bookId(0), memberId(0), returned(false) {
        issueDate[0] = '\0';
        dueDate[0] = '\0';
        returnDate[0] = '\0';
    }

    static void formatDate(char* buffer, int size, time_t t) {
        tm* ltm = localtime(&t);
        snprintf(buffer, size, "%02d-%02d-%04d",
                 ltm->tm_mday, 1 + ltm->tm_mon, 1900 + ltm->tm_year);
    }

    void create(int id, int bId, int mId) {
        issueId = id;
        bookId = bId;
        memberId = mId;
        time_t now = time(nullptr);
        formatDate(issueDate, sizeof(issueDate), now);
        formatDate(dueDate, sizeof(dueDate), now + LOAN_PERIOD_DAYS * 24 * 3600);
        returnDate[0] = '\0';
        returned = false;
    }

    void markReturned() {
        time_t now = time(nullptr);
        formatDate(returnDate, sizeof(returnDate), now);
        returned = true;
    }

    int  getIssueId() const { return issueId; }
    int  getBookId() const { return bookId; }
    int  getMemberId() const { return memberId; }
    bool isReturned() const { return returned; }
    const char* getIssueDate() const { return issueDate; }
    const char* getDueDate() const { return dueDate; }
    const char* getReturnDate() const { return returnDate; }

    void printRow(const char* bookTitle, const char* memberName) const {
        cout << left
             << setw(6)  << issueId
             << setw(28) << bookTitle
             << setw(20) << memberName
             << setw(12) << issueDate
             << setw(12) << dueDate
             << setw(12) << (returned ? returnDate : "-- OUT --") << "\n";
    }
};

// ------------------------------------------------------------
//  Library class
//  The manager: owns all file I/O and every menu operation.
// ------------------------------------------------------------
class Library {
public:
    // -------- console input helpers --------
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

    static void readLine(const string& prompt, char* buffer, int size) {
        cout << prompt;
        cin.getline(buffer, size);
        while (strlen(buffer) == 0) {
            cin.getline(buffer, size);
        }
    }

    // -------- ID generators --------
    static int nextBookId() {
        ifstream fin(BOOKS_FILE, ios::binary);
        int maxId = 100;
        Book b;
        while (fin.read(reinterpret_cast<char*>(&b), sizeof(Book)))
            if (b.getId() > maxId) maxId = b.getId();
        fin.close();
        return maxId + 1;
    }

    static int nextMemberId() {
        ifstream fin(MEMBERS_FILE, ios::binary);
        int maxId = 200;
        Member m;
        while (fin.read(reinterpret_cast<char*>(&m), sizeof(Member)))
            if (m.getId() > maxId) maxId = m.getId();
        fin.close();
        return maxId + 1;
    }

    static int nextIssueId() {
        ifstream fin(ISSUES_FILE, ios::binary);
        int maxId = 1000;
        IssueRecord r;
        while (fin.read(reinterpret_cast<char*>(&r), sizeof(IssueRecord)))
            if (r.getIssueId() > maxId) maxId = r.getIssueId();
        fin.close();
        return maxId + 1;
    }

    // -------- generic find-by-id + position (templated over record type) --------
    template <typename T>
    static bool findById(const char* filename, int id, T& result, streampos& pos,
                          bool requireActive = true) {
        ifstream fin(filename, ios::binary);
        if (!fin) return false;

        T rec;
        while (fin.read(reinterpret_cast<char*>(&rec), sizeof(T))) {
            bool activeOk = !requireActive || rec.isActive();
            if (rec.getId() == id && activeOk) {
                result = rec;
                pos = fin.tellg();
                pos -= static_cast<streamoff>(sizeof(T));
                fin.close();
                return true;
            }
        }
        fin.close();
        return false;
    }

    template <typename T>
    static void saveAt(const char* filename, const T& rec, streampos pos) {
        fstream fs(filename, ios::binary | ios::in | ios::out);
        fs.seekp(pos);
        fs.write(reinterpret_cast<const char*>(&rec), sizeof(T));
        fs.close();
    }

    template <typename T>
    static void append(const char* filename, const T& rec) {
        ofstream fout(filename, ios::binary | ios::app);
        fout.write(reinterpret_cast<const char*>(&rec), sizeof(T));
        fout.close();
    }

    // ============================================================
    //  MENU OPERATIONS
    // ============================================================

    static void addBook() {
        cout << "\n----- Add New Book -----\n";
        char title[100], author[60];
        readLine("Enter Book Title  : ", title, sizeof(title));
        readLine("Enter Author Name : ", author, sizeof(author));
        int copies = readInt("Enter Number of Copies: ");
        if (copies <= 0) {
            cout << "Number of copies must be at least 1. Operation cancelled.\n";
            return;
        }

        Book b;
        int id = nextBookId();
        b.create(id, title, author, copies);
        append(BOOKS_FILE, b);

        cout << "Book added successfully! Book ID: " << id << "\n";
    }

    static void addMember() {
        cout << "\n----- Add New Member -----\n";
        char name[50], contact[20];
        readLine("Enter Member Name   : ", name, sizeof(name));
        readLine("Enter Contact Number: ", contact, sizeof(contact));

        Member m;
        int id = nextMemberId();
        m.create(id, name, contact);
        append(MEMBERS_FILE, m);

        cout << "Member registered successfully! Member ID: " << id << "\n";
    }

    static void displayAllBooks() {
        ifstream fin(BOOKS_FILE, ios::binary);
        if (!fin) { cout << "\nNo books in the catalogue yet.\n"; return; }

        Book b;
        bool found = false;
        cout << "\n----- Book Catalogue -----\n";
        while (fin.read(reinterpret_cast<char*>(&b), sizeof(Book))) {
            if (b.isActive()) {
                if (!found) {
                    cout << left << setw(6) << "ID" << setw(35) << "Title"
                         << setw(25) << "Author" << setw(10) << "Available"
                         << setw(6) << "Total" << "\n";
                    cout << string(82, '-') << "\n";
                    found = true;
                }
                b.printRow();
            }
        }
        fin.close();
        if (!found) cout << "No books found.\n";
    }

    static void displayAllMembers() {
        ifstream fin(MEMBERS_FILE, ios::binary);
        if (!fin) { cout << "\nNo members registered yet.\n"; return; }

        Member m;
        bool found = false;
        cout << "\n----- Registered Members -----\n";
        while (fin.read(reinterpret_cast<char*>(&m), sizeof(Member))) {
            if (m.isActive()) {
                if (!found) {
                    cout << left << setw(6) << "ID" << setw(30) << "Name"
                         << setw(15) << "Contact" << "\n";
                    cout << string(51, '-') << "\n";
                    found = true;
                }
                m.printRow();
            }
        }
        fin.close();
        if (!found) cout << "No members found.\n";
    }

    static void searchByTitle() {
        char query[100];
        readLine("\nEnter title (or part of it) to search: ", query, sizeof(query));

        ifstream fin(BOOKS_FILE, ios::binary);
        if (!fin) { cout << "No books in the catalogue yet.\n"; return; }

        Book b;
        bool found = false;
        while (fin.read(reinterpret_cast<char*>(&b), sizeof(Book))) {
            if (b.isActive() && b.titleMatches(query)) {
                if (!found) {
                    cout << "\nSearch Results:\n";
                    cout << left << setw(6) << "ID" << setw(35) << "Title"
                         << setw(25) << "Author" << setw(10) << "Available"
                         << setw(6) << "Total" << "\n";
                    cout << string(82, '-') << "\n";
                    found = true;
                }
                b.printRow();
            }
        }
        fin.close();
        if (!found) cout << "No books found matching \"" << query << "\".\n";
    }

    static void searchByAuthor() {
        char query[60];
        readLine("\nEnter author name (or part of it) to search: ", query, sizeof(query));

        ifstream fin(BOOKS_FILE, ios::binary);
        if (!fin) { cout << "No books in the catalogue yet.\n"; return; }

        Book b;
        bool found = false;
        while (fin.read(reinterpret_cast<char*>(&b), sizeof(Book))) {
            if (b.isActive() && b.authorMatches(query)) {
                if (!found) {
                    cout << "\nSearch Results:\n";
                    cout << left << setw(6) << "ID" << setw(35) << "Title"
                         << setw(25) << "Author" << setw(10) << "Available"
                         << setw(6) << "Total" << "\n";
                    cout << string(82, '-') << "\n";
                    found = true;
                }
                b.printRow();
            }
        }
        fin.close();
        if (!found) cout << "No books found by author \"" << query << "\".\n";
    }

    static void issueBook() {
        cout << "\n----- Issue Book -----\n";
        int bookId = readInt("Enter Book ID  : ");
        int memberId = readInt("Enter Member ID: ");

        Book b; streampos bPos;
        if (!findById(BOOKS_FILE, bookId, b, bPos)) {
            cout << "No such book found (ID " << bookId << ").\n";
            return;
        }
        Member m; streampos mPos;
        if (!findById(MEMBERS_FILE, memberId, m, mPos)) {
            cout << "No such member found (ID " << memberId << ").\n";
            return;
        }
        if (!b.hasAvailableCopy()) {
            cout << "Sorry, \"" << b.getTitle() << "\" has no available copies right now.\n";
            return;
        }

        b.decreaseAvailable();
        saveAt(BOOKS_FILE, b, bPos);

        IssueRecord r;
        int issueId = nextIssueId();
        r.create(issueId, bookId, memberId);
        append(ISSUES_FILE, r);

        cout << "Book issued successfully!\n";
        cout << "Issue ID: " << issueId << " | Due Date: " << r.getDueDate() << "\n";
    }

    static void returnBook() {
        cout << "\n----- Return Book -----\n";
        int issueId = readInt("Enter Issue ID (shown when the book was issued): ");

        IssueRecord r; streampos rPos;
        if (!findIssueById(issueId, r, rPos)) {
            cout << "No such active issue record found (ID " << issueId << ").\n";
            return;
        }
        if (r.isReturned()) {
            cout << "This book was already returned on " << r.getReturnDate() << ".\n";
            return;
        }

        r.markReturned();
        saveAt(ISSUES_FILE, r, rPos);

        Book b; streampos bPos;
        if (findById(BOOKS_FILE, r.getBookId(), b, bPos)) {
            b.increaseAvailable();
            saveAt(BOOKS_FILE, b, bPos);
            cout << "\"" << b.getTitle() << "\" returned successfully. Thank you!\n";
        } else {
            cout << "Return recorded, but the original book record could not be found.\n";
        }
    }

    // Finds an issue record by its issue ID (regardless of returned status)
    static bool findIssueById(int issueId, IssueRecord& result, streampos& pos) {
        ifstream fin(ISSUES_FILE, ios::binary);
        if (!fin) return false;

        IssueRecord r;
        while (fin.read(reinterpret_cast<char*>(&r), sizeof(IssueRecord))) {
            if (r.getIssueId() == issueId) {
                result = r;
                pos = fin.tellg();
                pos -= static_cast<streamoff>(sizeof(IssueRecord));
                fin.close();
                return true;
            }
        }
        fin.close();
        return false;
    }

    // Looks up a book's title / a member's name by ID for display purposes
    static string bookTitleById(int id) {
        Book b; streampos pos;
        if (findById(BOOKS_FILE, id, b, pos, false)) return b.getTitle();
        return "(unknown book)";
    }

    static string memberNameById(int id) {
        Member m; streampos pos;
        if (findById(MEMBERS_FILE, id, m, pos, false)) return m.getName();
        return "(unknown member)";
    }

    static void viewIssuedBooks() {
        cout << "\n----- View Issued Books -----\n";
        cout << "1. View all currently issued (not yet returned) books\n";
        cout << "2. View issue history for a specific member\n";
        int choice = readInt("Enter choice: ");

        int filterMemberId = -1;
        if (choice == 2) filterMemberId = readInt("Enter Member ID: ");

        ifstream fin(ISSUES_FILE, ios::binary);
        if (!fin) { cout << "No issue records found.\n"; return; }

        IssueRecord r;
        bool found = false;
        while (fin.read(reinterpret_cast<char*>(&r), sizeof(IssueRecord))) {
            bool matchesFilter = (choice == 1) ? !r.isReturned()
                                                : (r.getMemberId() == filterMemberId);
            if (matchesFilter) {
                if (!found) {
                    cout << "\n" << left
                         << setw(6)  << "ID"
                         << setw(28) << "Book Title"
                         << setw(20) << "Member"
                         << setw(12) << "Issued"
                         << setw(12) << "Due"
                         << setw(12) << "Returned" << "\n";
                    cout << string(90, '-') << "\n";
                    found = true;
                }
                r.printRow(bookTitleById(r.getBookId()).c_str(),
                           memberNameById(r.getMemberId()).c_str());
            }
        }
        fin.close();
        if (!found) cout << "No matching issue records found.\n";
    }
};

// ------------------------------------------------------------
//  Menu display
// ------------------------------------------------------------
void showMenu() {
    cout << "\n============================================\n";
    cout << "         LIBRARY MANAGEMENT SYSTEM\n";
    cout << "============================================\n";
    cout << " 1. Add Book\n";
    cout << " 2. Add Member\n";
    cout << " 3. Issue Book\n";
    cout << " 4. Return Book\n";
    cout << " 5. Search Book by Title\n";
    cout << " 6. Search Book by Author\n";
    cout << " 7. Display All Books\n";
    cout << " 8. Display All Members\n";
    cout << " 9. View Issued Books\n";
    cout << "10. Exit\n";
    cout << "============================================\n";
}

// ------------------------------------------------------------
//  main()
// ------------------------------------------------------------
int main() {
    int choice;

    cout << "Welcome to the Library Management System!\n";
    cout << "Data is stored persistently in '" << BOOKS_FILE << "', '"
         << MEMBERS_FILE << "', and '" << ISSUES_FILE << "'.\n";

    do {
        showMenu();
        choice = Library::readInt("Enter your choice (1-10): ");

        switch (choice) {
            case 1:  Library::addBook();          break;
            case 2:  Library::addMember();        break;
            case 3:  Library::issueBook();        break;
            case 4:  Library::returnBook();       break;
            case 5:  Library::searchByTitle();    break;
            case 6:  Library::searchByAuthor();   break;
            case 7:  Library::displayAllBooks();  break;
            case 8:  Library::displayAllMembers();break;
            case 9:  Library::viewIssuedBooks();  break;
            case 10: cout << "\nGoodbye!\n";      break;
            default: cout << "Invalid choice! Please enter a number between 1 and 10.\n";
        }

    } while (choice != 10);

    return 0;
}

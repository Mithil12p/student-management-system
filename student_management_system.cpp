/*
 * ============================================================
 *  STUDENT MANAGEMENT SYSTEM
 *  Language      : C++
 *  Storage       : Binary file (students.dat) via fstream
 *  Features      : Add, Update, Delete, Display (all/one),
 *                  Search, persistent storage, menu-driven UI
 * ============================================================
 *
 *  Compile :  g++ -std=c++17 -O2 -o sms student_management_system.cpp
 *  Run     :  ./sms          (Linux/Mac)
 *             sms.exe        (Windows)
 * ============================================================
 */

#include <iostream>
#include <fstream>
#include <iomanip>
#include <cstring>
#include <limits>
#include <vector>

using namespace std;

// ------------------------------------------------------------
//  Student record structure
//  Fixed-size fields are used (char arrays) so that every
//  record occupies exactly the same number of bytes in the
//  binary file. This makes random-access update/delete easy
//  (we can seek directly to a record using its index).
// ------------------------------------------------------------
struct Student {
    int  id;
    char name[50];
    int  age;
    char course[30];
    float marks;
    bool active;   // "soft delete" flag -> true = valid record, false = deleted
};

const char* FILENAME = "students.dat";

// ------------------------------------------------------------
//  Utility: clear bad input state & flush the input buffer
// ------------------------------------------------------------
void clearInputBuffer() {
    cin.clear();
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
}

// ------------------------------------------------------------
//  Utility: safely read an integer from the user
// ------------------------------------------------------------
int readInt(const string& prompt) {
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

// ------------------------------------------------------------
//  Utility: safely read a float from the user
// ------------------------------------------------------------
float readFloat(const string& prompt) {
    float value;
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

// ------------------------------------------------------------
//  Utility: read a full line of text (for names, courses)
// ------------------------------------------------------------
void readLine(const string& prompt, char* buffer, int size) {
    cout << prompt;
    cin.getline(buffer, size);
    // If the previous cin >> left a newline in the buffer,
    // getline would return instantly with an empty string.
    // We guard against that by re-reading if the buffer is empty.
    while (strlen(buffer) == 0) {
        cin.getline(buffer, size);
    }
}

// ------------------------------------------------------------
//  Check whether a given student ID already exists
//  (only among active/non-deleted records)
// ------------------------------------------------------------
bool idExists(int id) {
    ifstream fin(FILENAME, ios::binary);
    if (!fin) return false;

    Student s;
    while (fin.read(reinterpret_cast<char*>(&s), sizeof(Student))) {
        if (s.active && s.id == id) {
            fin.close();
            return true;
        }
    }
    fin.close();
    return false;
}

// ------------------------------------------------------------
//  ADD a new student record (appends to the file)
// ------------------------------------------------------------
void addStudent() {
    Student s;
    cout << "\n----- Add New Student -----\n";

    s.id = readInt("Enter Student ID   : ");
    if (idExists(s.id)) {
        cout << "A student with ID " << s.id << " already exists! Operation cancelled.\n";
        return;
    }

    readLine("Enter Student Name : ", s.name, sizeof(s.name));
    s.age = readInt("Enter Age          : ");
    readLine("Enter Course       : ", s.course, sizeof(s.course));
    s.marks = readFloat("Enter Marks (0-100): ");
    s.active = true;

    ofstream fout(FILENAME, ios::binary | ios::app);
    if (!fout) {
        cout << "Error: could not open file for writing.\n";
        return;
    }
    fout.write(reinterpret_cast<char*>(&s), sizeof(Student));
    fout.close();

    cout << "Student record added successfully!\n";
}

// ------------------------------------------------------------
//  Print a single record in a formatted row
// ------------------------------------------------------------
void printHeader() {
    cout << left
         << setw(8)  << "ID"
         << setw(22) << "Name"
         << setw(6)  << "Age"
         << setw(16) << "Course"
         << setw(8)  << "Marks" << "\n";
    cout << string(60, '-') << "\n";
}

void printRecord(const Student& s) {
    cout << left
         << setw(8)  << s.id
         << setw(22) << s.name
         << setw(6)  << s.age
         << setw(16) << s.course
         << setw(8)  << fixed << setprecision(2) << s.marks << "\n";
}

// ------------------------------------------------------------
//  DISPLAY all active students
// ------------------------------------------------------------
void displayAll() {
    ifstream fin(FILENAME, ios::binary);
    if (!fin) {
        cout << "\nNo records found. (File does not exist yet)\n";
        return;
    }

    Student s;
    bool found = false;
    cout << "\n----- All Student Records -----\n";

    while (fin.read(reinterpret_cast<char*>(&s), sizeof(Student))) {
        if (s.active) {
            if (!found) { printHeader(); found = true; }
            printRecord(s);
        }
    }
    fin.close();

    if (!found) cout << "No records found.\n";
}

// ------------------------------------------------------------
//  SEARCH / DISPLAY a single student by ID
// ------------------------------------------------------------
bool findStudentById(int id, Student& result, streampos& pos) {
    ifstream fin(FILENAME, ios::binary);
    if (!fin) return false;

    Student s;
    while (fin.read(reinterpret_cast<char*>(&s), sizeof(Student))) {
        if (s.active && s.id == id) {
            result = s;
            pos = fin.tellg();
            pos -= static_cast<streamoff>(sizeof(Student));
            fin.close();
            return true;
        }
    }
    fin.close();
    return false;
}

void searchStudent() {
    int id = readInt("\nEnter Student ID to search: ");
    Student s;
    streampos pos;

    if (findStudentById(id, s, pos)) {
        cout << "\nStudent Found:\n";
        printHeader();
        printRecord(s);
    } else {
        cout << "No student found with ID " << id << ".\n";
    }
}

// ------------------------------------------------------------
//  UPDATE an existing student record
// ------------------------------------------------------------
void updateStudent() {
    int id = readInt("\nEnter Student ID to update: ");
    Student s;
    streampos pos;

    if (!findStudentById(id, s, pos)) {
        cout << "No student found with ID " << id << ".\n";
        return;
    }

    cout << "Current details:\n";
    printHeader();
    printRecord(s);

    cout << "\nEnter new details (this will overwrite the old record):\n";
    readLine("Enter Student Name : ", s.name, sizeof(s.name));
    s.age = readInt("Enter Age          : ");
    readLine("Enter Course       : ", s.course, sizeof(s.course));
    s.marks = readFloat("Enter Marks (0-100): ");

    fstream fs(FILENAME, ios::binary | ios::in | ios::out);
    if (!fs) {
        cout << "Error: could not open file for updating.\n";
        return;
    }
    fs.seekp(pos);
    fs.write(reinterpret_cast<char*>(&s), sizeof(Student));
    fs.close();

    cout << "Student record updated successfully!\n";
}

// ------------------------------------------------------------
//  DELETE a student record (soft delete: mark active = false)
// ------------------------------------------------------------
void deleteStudent() {
    int id = readInt("\nEnter Student ID to delete: ");
    Student s;
    streampos pos;

    if (!findStudentById(id, s, pos)) {
        cout << "No student found with ID " << id << ".\n";
        return;
    }

    cout << "Record to delete:\n";
    printHeader();
    printRecord(s);

    char confirm;
    cout << "Are you sure you want to delete this record? (y/n): ";
    cin >> confirm;
    clearInputBuffer();

    if (confirm == 'y' || confirm == 'Y') {
        s.active = false;
        fstream fs(FILENAME, ios::binary | ios::in | ios::out);
        if (!fs) {
            cout << "Error: could not open file for deleting.\n";
            return;
        }
        fs.seekp(pos);
        fs.write(reinterpret_cast<char*>(&s), sizeof(Student));
        fs.close();
        cout << "Student record deleted successfully!\n";
    } else {
        cout << "Delete operation cancelled.\n";
    }
}

// ------------------------------------------------------------
//  Compact the file: physically removes soft-deleted records
//  and rewrites the file with only active records.
//  (Optional maintenance utility - keeps file size small)
// ------------------------------------------------------------
void compactFile() {
    ifstream fin(FILENAME, ios::binary);
    if (!fin) {
        cout << "No file to compact.\n";
        return;
    }

    vector<Student> activeRecords;
    Student s;
    while (fin.read(reinterpret_cast<char*>(&s), sizeof(Student))) {
        if (s.active) activeRecords.push_back(s);
    }
    fin.close();

    ofstream fout(FILENAME, ios::binary | ios::trunc);
    for (auto& rec : activeRecords) {
        fout.write(reinterpret_cast<char*>(&rec), sizeof(Student));
    }
    fout.close();

    cout << "File compacted. Total active records: " << activeRecords.size() << "\n";
}

// ------------------------------------------------------------
//  Display the menu and get the user's choice
// ------------------------------------------------------------
void showMenu() {
    cout << "\n============================================\n";
    cout << "        STUDENT MANAGEMENT SYSTEM\n";
    cout << "============================================\n";
    cout << " 1. Add Student\n";
    cout << " 2. Display All Students\n";
    cout << " 3. Search Student by ID\n";
    cout << " 4. Update Student\n";
    cout << " 5. Delete Student\n";
    cout << " 6. Compact Data File (remove deleted records)\n";
    cout << " 7. Exit\n";
    cout << "============================================\n";
}

// ------------------------------------------------------------
//  main() - program entry point / menu loop
// ------------------------------------------------------------
int main() {
    int choice;

    cout << "Welcome to the Student Management System!\n";
    cout << "Data is stored persistently in '" << FILENAME << "'.\n";

    do {
        showMenu();
        choice = readInt("Enter your choice (1-7): ");

        switch (choice) {
            case 1: addStudent();      break;
            case 2: displayAll();      break;
            case 3: searchStudent();   break;
            case 4: updateStudent();   break;
            case 5: deleteStudent();   break;
            case 6: compactFile();     break;
            case 7: cout << "\nExiting... Thank you for using the system!\n"; break;
            default: cout << "Invalid choice! Please enter a number between 1 and 7.\n";
        }

    } while (choice != 7);

    return 0;
}

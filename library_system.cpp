#include <iostream>
#include <vector>
#include <map>
#include <string>
#include <memory>
#include <algorithm>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <regex>
#include <sys/stat.h>

// User roles for RBAC enforcement
enum class SystemRole {
    LIBRARIAN,
    MEMBER,
    NONE
};

class Book {
private:
    std::string isbn;
    std::string title;
    std::string author;
    bool available;

public:
    Book(std::string isbn, std::string title, std::string author, bool avail = true)
        : isbn(isbn), title(title), author(author), available(avail) {}

    std::string getISBN() const { return isbn; }
    std::string getTitle() const { return title; }
    std::string getAuthor() const { return author; }
    bool isAvailable() const { return available; }

    void setAvailable(bool status) { available = status; }

    void display() const {
        std::cout << std::left << std::setw(15) << isbn
                  << std::setw(28) << title
                  << std::setw(20) << author
                  << std::setw(12) << (available ? "Available" : "Checked Out") << "\n";
    }

    std::string toCSV() const {
        return isbn + "," + title + "," + author + "," + (available ? "1" : "0");
    }

    static Book fromCSV(const std::string& line) {
        std::stringstream ss(line);
        std::string isbn, title, author, availStr;
        std::getline(ss, isbn, ',');
        std::getline(ss, title, ',');
        std::getline(ss, author, ',');
        std::getline(ss, availStr, ',');
        return Book(isbn, title, author, availStr == "1");
    }
};

class User {
protected:
    int userId;
    std::string name;
    std::vector<std::string> checkedOutISBNs;

public:
    User(int id, std::string name) : userId(id), name(name) {}
    virtual ~User() {}

    int getUserId() const { return userId; }
    std::string getName() const { return name; }
    const std::vector<std::string>& getCheckedOutISBNs() const { return checkedOutISBNs; }

    void addBook(const std::string& isbn) {
        checkedOutISBNs.push_back(isbn);
    }

    void removeBook(const std::string& isbn) {
        checkedOutISBNs.erase(
            std::remove(checkedOutISBNs.begin(), checkedOutISBNs.end(), isbn),
            checkedOutISBNs.end()
        );
    }

    bool hasBook(const std::string& isbn) const {
        return std::find(checkedOutISBNs.begin(), checkedOutISBNs.end(), isbn) != checkedOutISBNs.end();
    }

    virtual int getMaxBooks() const = 0;
    virtual double calculateFine(int daysLate) const = 0;
    virtual std::string getUserType() const = 0;

    virtual void display() const {
        std::cout << "ID: " << userId << " | Name: " << name << " | Type: " << getUserType() 
                  << " | Books Borrowed: " << checkedOutISBNs.size() << "/" << getMaxBooks() << "\n";
    }

    std::string checkedOutToString() const {
        std::string res = "";
        for (size_t i = 0; i < checkedOutISBNs.size(); ++i) {
            res += checkedOutISBNs[i];
            if (i + 1 < checkedOutISBNs.size()) res += ";";
        }
        return res;
    }
};

class StudentUser : public User {
public:
    StudentUser(int id, std::string name) : User(id, name) {}
    StudentUser(int id, std::string name, std::vector<std::string> books) : User(id, name) {
        checkedOutISBNs = books;
    }

    int getMaxBooks() const override { return 3; }
    double calculateFine(int daysLate) const override {
        return daysLate > 0 ? daysLate * 1.50 : 0.0;
    }
    std::string getUserType() const override { return "Student"; }
};

class FacultyUser : public User {
public:
    FacultyUser(int id, std::string name) : User(id, name) {}
    FacultyUser(int id, std::string name, std::vector<std::string> books) : User(id, name) {
        checkedOutISBNs = books;
    }

    int getMaxBooks() const override { return 6; }
    double calculateFine(int daysLate) const override {
        return daysLate > 0 ? daysLate * 0.50 : 0.0;
    }
    std::string getUserType() const override { return "Faculty"; }
};

class SecureLibraryManager {
private:
    std::map<std::string, Book> books;
    std::map<int, std::shared_ptr<User>> users;
    const std::string bookFile = "secure_books.csv";
    const std::string userFile = "secure_users.csv";

    void setSecureFilePermissions(const std::string& filename) {
#ifdef _WIN32
        // Windows ACL enforcement handled at environment layer
#else
        chmod(filename.c_str(), S_IRUSR | S_IWUSR);
#endif
    }

    void loadData() {
        try {
            std::ifstream bFile(bookFile);
            if (bFile.is_open()) {
                std::string line;
                while (std::getline(bFile, line)) {
                    if (!line.empty()) {
                        Book b = Book::fromCSV(line);
                        books[b.getISBN()] = b;
                    }
                }
                bFile.close();
            }

            std::ifstream uFile(userFile);
            if (uFile.is_open()) {
                std::string line;
                while (std::getline(uFile, line)) {
                    if (!line.empty()) {
                        std::stringstream ss(line);
                        std::string idStr, name, type, bookListStr;
                        std::getline(ss, idStr, ',');
                        std::getline(ss, name, ',');
                        std::getline(ss, type, ',');
                        std::getline(ss, bookListStr, ',');

                        int id = std::stoi(idStr);
                        std::vector<std::string> borrowedBooks;
                        if (!bookListStr.empty()) {
                            std::stringstream bss(bookListStr);
                            std::string isbn;
                            while (std::getline(bss, isbn, ';')) {
                                borrowedBooks.push_back(isbn);
                            }
                        }

                        if (type == "Student") {
                            users[id] = std::make_shared<StudentUser>(id, name, borrowedBooks);
                        } else {
                            users[id] = std::make_shared<FacultyUser>(id, name, borrowedBooks);
                        }
                    }
                }
                uFile.close();
            }
        } catch (const std::exception& e) {
            std::cerr << "[Security Warning] Data load corruption handled. Initialized safe state.\n";
        }
    }

    void saveData() const {
        try {
            std::ofstream bFile(bookFile);
            for (const auto& pair : books) {
                bFile << pair.second.toCSV() << "\n";
            }
            bFile.close();
            const_cast<SecureLibraryManager*>(this)->setSecureFilePermissions(bookFile);

            std::ofstream uFile(userFile);
            for (const auto& pair : users) {
                auto u = pair.second;
                uFile << u->getUserId() << "," << u->getName() << "," << u->getUserType() << "," << u->checkedOutToString() << "\n";
            }
            uFile.close();
            const_cast<SecureLibraryManager*>(this)->setSecureFilePermissions(userFile);
        } catch (const std::exception& e) {
            std::cerr << "[Security Error] Failed to secure persist state.\n";
        }
    }

    bool validateISBNFormat(const std::string& isbn) const {
        // Allow-list: Alphanumeric and hyphens only (Max 20 chars)
        const std::regex pattern("^[a-zA-Z0-9\\-]{5,20}$");
        return std::regex_match(isbn, pattern);
    }

    bool validateTextField(const std::string& field) const {
        // Allow-list: Alphanumeric, spaces, and basic punctuation (Max 50 chars)
        const std::regex pattern("^[a-zA-Z0-9\\s\\.,\\-_]{2,50}$");
        return std::regex_match(field, pattern);
    }

public:
    SecureLibraryManager() {
        loadData();
    }

    ~SecureLibraryManager() {
        saveData();
    }

    void addBook() {
        std::string isbn, title, author;
        std::cout << "\nEnter ISBN (alphanumeric/hyphens): ";
        std::cin >> isbn;
        if (!validateISBNFormat(isbn)) {
            std::cout << "[Security Alert] Invalid ISBN format allow-list check failed.\n";
            return;
        }

        if (books.find(isbn) != books.end()) {
            std::cout << "Error: Book conflict. ISBN already exists.\n";
            return;
        }

        std::cout << "Enter Title: ";
        std::cin.ignore();
        std::getline(std::cin, title);
        if (!validateTextField(title)) {
            std::cout << "[Security Alert] Invalid title format detected.\n";
            return;
        }

        std::cout << "Enter Author: ";
        std::getline(std::cin, author);
        if (!validateTextField(author)) {
            std::cout << "[Security Alert] Invalid author format detected.\n";
            return;
        }

        books[isbn] = Book(isbn, title, author, true);
        saveData();
        std::cout << "Book securely added to inventory.\n";
    }

    void registerUser() {
        int id, typeChoice;
        std::string name;
        std::cout << "\nEnter User ID (positive integer): ";
        while (!(std::cin >> id) || id <= 0 || users.find(id) != users.end()) {
            std::cout << "Invalid or already registered ID. Enter valid unique ID: ";
            std::cin.clear();
            std::cin.ignore(10000, '\n');
        }

        std::cout << "Enter Name: ";
        std::cin.ignore();
        std::getline(std::cin, name);
        if (!validateTextField(name)) {
            std::cout << "[Security Alert] Invalid name formatting restrictions applied.\n";
            return;
        }

        std::cout << "Select User Type (1. Student, 2. Faculty): ";
        while (!(std::cin >> typeChoice) || (typeChoice != 1 && typeChoice != 2)) {
            std::cout << "Invalid selection. Enter 1 or 2: ";
            std::cin.clear();
            std::cin.ignore(10000, '\n');
        }

        if (typeChoice == 1) {
            users[id] = std::make_shared<StudentUser>(id, name);
        } else {
            users[id] = std::make_shared<FacultyUser>(id, name);
        }

        saveData();
        std::cout << "Member registered securely.\n";
    }

    void searchBooks() const {
        if (books.empty()) {
            std::cout << "Inventory is empty.\n";
            return;
        }

        int choice;
        std::cout << "\nSearch by:\n1. ISBN\n2. Title\n3. Author\nEnter choice: ";
        std::cin >> choice;

        std::string query;
        std::cout << "Enter search keyword: ";
        std::cin.ignore();
        std::getline(std::cin, query);

        std::cout << "\n=========================================================================\n";
        std::cout << std::left << std::setw(15) << "ISBN"
                  << std::setw(28) << "Title"
                  << std::setw(20) << "Author"
                  << std::setw(12) << "Status" << "\n";
        std::cout << "=========================================================================\n";

        bool found = false;
        for (const auto& pair : books) {
            const Book& b = pair.second;
            if ((choice == 1 && b.getISBN() == query) ||
                (choice == 2 && b.getTitle().find(query) != std::string::npos) ||
                (choice == 3 && b.getAuthor().find(query) != std::string::npos)) {
                b.display();
                found = true;
            }
        }

        if (!found) {
            std::cout << "No matching entries found.\n";
        }
        std::cout << "=========================================================================\n";
    }

    void issueBook() {
        int userId;
        std::string isbn;

        std::cout << "\nEnter User ID: ";
        std::cin >> userId;
        if (users.find(userId) == users.end()) {
            std::cout << "User record not found.\n";
            return;
        }

        std::cout << "Enter Book ISBN: ";
        std::cin >> isbn;
        if (!validateISBNFormat(isbn) || books.find(isbn) == books.end()) {
            std::cout << "Book record not found or format invalid.\n";
            return;
        }

        auto user = users[userId];
        Book& book = books[isbn];

        if (!book.isAvailable()) {
            std::cout << "Error: Book is already checked out.\n";
            return;
        }

        if (user->getCheckedOutISBNs().size() >= static_cast<size_t>(user->getMaxBooks())) {
            std::cout << "Borrowing limit reached for this profile type (" << user->getMaxBooks() << ").\n";
            return;
        }

        book.setAvailable(false);
        user->addBook(isbn);
        saveData();
        std::cout << "Book issued securely to " << user->getName() << "!\n";
    }

    void returnBook() {
        int userId;
        std::string isbn;
        int daysLate;

        std::cout << "\nEnter User ID: ";
        std::cin >> userId;
        if (users.find(userId) == users.end()) {
            std::cout << "User record not found.\n";
            return;
        }

        std::cout << "Enter Book ISBN to return: ";
        std::cin >> isbn;
        if (!validateISBNFormat(isbn)) {
            std::cout << "Invalid ISBN formatting.\n";
            return;
        }

        auto user = users[userId];
        if (!user->hasBook(isbn)) {
            std::cout << "Mismatch: Record indicates user did not check out this specific book.\n";
            return;
        }

        std::cout << "Enter days late (0 if on time): ";
        while (!(std::cin >> daysLate) || daysLate < 0 || daysLate > 365) {
            std::cout << "Invalid bounds. Enter valid positive day count: ";
            std::cin.clear();
            std::cin.ignore(10000, '\n');
        }

        double fine = user->calculateFine(daysLate);

        books[isbn].setAvailable(true);
        user->removeBook(isbn);
        saveData();

        std::cout << "Return transaction processed successfully.\n";
        if (fine > 0.0) {
            std::cout << "Late fine assessed: $" << std::fixed << std::setprecision(2) << fine << "\n";
        } else {
            std::cout << "No late fees incurred.\n";
        }
    }

    void displayInventory() const {
        if (books.empty()) {
            std::cout << "\nInventory is empty.\n";
            return;
        }
        std::cout << "\n=========================================================================\n";
        std::cout << std::left << std::setw(15) << "ISBN"
                  << std::setw(28) << "Title"
                  << std::setw(20) << "Author"
                  << std::setw(12) << "Status" << "\n";
        std::cout << "=========================================================================\n";
        for (const auto& pair : books) {
            pair.second.display();
        }
        std::cout << "=========================================================================\n";
    }

    void displayUsers() const {
        if (users.empty()) {
            std::cout << "\nNo registered profiles.\n";
            return;
        }
        std::cout << "\n=== Secure Member Registry ===\n";
        for (const auto& pair : users) {
            pair.second->display();
        }
        std::cout << "==============================\n";
    }
};

SystemRole authenticateUser() {
    std::string user, pass;
    std::cout << "=== Library Portal Authentication ===\n";
    std::cout << "Username: ";
    std::cin >> user;
    std::cout << "Password: ";
    std::cin >> pass;

    // Hardcoded credentials for simulation context (managed via environment vaults in production)
    if (user == "librarian_admin" && pass == "LibAdminSecure#2026!") {
        std::cout << "[Access Granted] Logged in with LIBRARIAN privileges.\n";
        return SystemRole::LIBRARIAN;
    } else if (user == "library_member" && pass == "MemberPass#2026!") {
        std::cout << "[Access Granted] Logged in with MEMBER privileges.\n";
        return SystemRole::MEMBER;
    }

    std::cout << "[Access Denied] Authentication failed.\n";
    return SystemRole::NONE;
}

int main() {
    SystemRole role = authenticateUser();
    if (role == SystemRole::NONE) {
        return 1;
    }

    SecureLibraryManager library;
    int choice;

    do {
        std::cout << "\n=== Secure Library Book Management System ===\n";
        if (role == SystemRole::LIBRARIAN) {
            std::cout << "1. Add Book to Inventory\n";
            std::cout << "2. Register Member Profile\n";
            std::cout << "3. Search Books\n";
            std::cout << "4. Issue Book\n";
            std::cout << "5. Return Book & Calculate Fines\n";
            std::cout << "6. View Inventory\n";
            std::cout << "7. View Members\n";
            std::cout << "8. Exit\n";
            std::cout << "Enter choice (1-8): ";
        } else {
            std::cout << "3. Search Books\n";
            std::cout << "6. View Inventory\n";
            std::cout << "8. Exit\n";
            std::cout << "Enter choice (3, 6, or 8): ";
        }

        if (!(std::cin >> choice)) {
            std::cout << "Invalid input format. Enter a valid menu option.\n";
            std::cin.clear();
            std::cin.ignore(10000, '\n');
            continue;
        }

        if (role == SystemRole::MEMBER) {
            if (choice != 3 && choice != 6 && choice != 8) {
                std::cout << "[Authorization Error] Insufficient privileges for requested action.\n";
                continue;
            }
        }

        switch (choice) {
            case 1: library.addBook(); break;
            case 2: library.registerUser(); break;
            case 3: library.searchBooks(); break;
            case 4: library.issueBook(); break;
            case 5: library.returnBook(); break;
            case 6: library.displayInventory(); break;
            case 7: library.displayUsers(); break;
            case 8: std::cout << "Saving data securely. Exiting system. Goodbye!\n"; break;
            default: std::cout << "Invalid option selected.\n";
        }
    } while (choice != 8);

    return 0;
}

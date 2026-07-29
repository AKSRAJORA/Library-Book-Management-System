# Library Book Management System (CLI)

An enterprise-grade, secure C++ command-line application implementing a **Library Book Management System** with rigorous defensive programming controls, Role-Based Access Control (RBAC), regular expression input validation, and secure file persistence.

## 🚀 Key Security & Architectural Features

- **Role-Based Access Control (RBAC):** Gated access enforcing least privilege across `Librarian` (full administrative management) and `Member` (read/search restricted) roles.
- **Strict Input Validation & Allow-Listing:** Employs regular expressions (`std::regex`) to validate ISBN patterns, human names, titles, and author entries, preventing malformed inputs and injection risks.
- **Secure File Persistence & POSIX Permissions:** Automatically serializes records to `secure_books.csv` and `secure_users.csv`, enforcing hardened file permissions (`chmod 600` - owner-only read/write).
- **Object-Oriented Architecture:** Leverages abstract base classes (`User`) and polymorphism across `StudentUser` and `FacultyUser` subclasses for customized borrowing limits and fine calculations.
- **Safe Error Masking:** Encapsulates data streams and parsers in robust try-catch blocks to prevent internal stack disclosures.

---

## 📂 Project Structure

```text
├── secure_library.cpp    # Main application source code with security refactoring
├── secure_books.csv      # Persistent inventory storage (auto-generated with secure permissions)
├── secure_users.csv      # Persistent member storage (auto-generated with secure permissions)
└── README.md             # Project documentation

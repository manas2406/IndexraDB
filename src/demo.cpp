#include <iostream>
#include "../include/database.hpp"
#include "../include/join.hpp"
#include "../include/fileio.hpp"

using namespace std;

// Helper to print a table view
void print_table(Table* t) {
    if (t) cout << t->view();
}

int main() {
    cout << "=== Antigravity DB API Demo ===\n\n";

    Database db;

    // 1. Create Tables
    cout << "[1] Creating Tables...\n";
    db.createTable("students", {"id", "name", "major"});
    db.createTable("grades", {"student_id", "course", "grade"});
    
    // 2. Insert Data
    cout << "[2] Inserting Data...\n";
    Table* students = db.getTable("students");
    students->insert({{"id", "101"}, {"name", "Alice"}, {"major", "CS"}});
    students->insert({{"id", "102"}, {"name", "Bob"}, {"major", "Math"}});
    students->insert({{"id", "103"}, {"name", "Charlie"}, {"major", "CS"}});

    Table* grades = db.getTable("grades");
    grades->insert({{"student_id", "101"}, {"course", "CS101"}, {"grade", "A"}});
    grades->insert({{"student_id", "102"}, {"course", "CS101"}, {"grade", "B"}});
    grades->insert({{"student_id", "101"}, {"course", "Math202"}, {"grade", "A-"}});

    cout << "--- Students Table ---\n";
    print_table(students);
    cout << "--- Grades Table ---\n";
    print_table(grades);

    // 3. Search (B+ Tree)
    cout << "\n[3] B+ Tree Search (Major = CS)...\n";
    vector<int> cs_students = students->search("major", "CS");
    for (int idx : cs_students) {
        cout << "Found: " << students->rows[idx]["name"] << " (ID: " << students->rows[idx]["id"] << ")\n";
    }

    // 4. Join
    cout << "\n[4] Joining Students and Grades on ID...\n";
    // Join students (id) with grades (student_id)
    vector<map<string, string>> transcript = joinTables(db, "students", "grades", "id", "student_id");
    
    cout << "--- Transcript ---\n";
    for (const auto& row : transcript) {
        cout << row.at("name") << " | " << row.at("course") << " | " << row.at("grade") << "\n";
    }

    // 5. Persistence
    cout << "\n[5] Saving Database to 'demo_data'...\n";
    saveDatabase(db, "demo_data");

    cout << "[6] Loading into new Database instance...\n";
    Database db2;
    loadDatabase(db2, "demo_data");
    
    cout << "--- Loaded Students Table ---\n";
    print_table(db2.getTable("students"));

    return 0;
}

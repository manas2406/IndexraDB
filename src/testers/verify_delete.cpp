#include <iostream>
#include <cassert>
#include "../../include/database.hpp"
#include "../../include/nlq/executor.hpp"
#include "../../include/nlq/intent.hpp"

using namespace std;

int main() {
    cout << "=== Verifying DELETE Logic ===\n";
    Database db;
    vector<string> fields = {"id", "name"};
    db.createTable("student", fields);
    Table* t = db.getTable("student");
    
    // Insert 2 records
    map<string, string> r1; r1["id"] = "1"; r1["name"] = "alice";
    t->insert(r1);
    
    map<string, string> r2; r2["id"] = "2"; r2["name"] = "bob";
    t->insert(r2);
    
    cout << "Initial Count: " << t->rows.size() << "\n";
    
    // Test 1: Delete Where id=1
    NLQ::Query q1;
    q1.action = "DELETE";
    q1.table = "student";
    NLQ::Condition c; c.field = "id"; c.op = "="; c.value = "1";
    q1.conditions.push_back(c);
    
    cout << "Executing: delete where id=1\n";
    NLQ::executeQuery(q1, db);
    
    // Check if row 0 (id 1) is deleted
    // note: rows are preserved indices.
    if (!t->rows[0].count("__deleted")) {
        cout << "FAILED: Row 0 should be deleted.\n";
        return 1;
    }
    if (t->rows[1].count("__deleted")) {
        cout << "FAILED: Row 1 should NOT be deleted.\n";
        return 1;
    }
    cout << "Test 1 Passed.\n";
    
    // Test 2: Delete All
    NLQ::Query q2;
    q2.action = "DELETE";
    q2.table = "student";
    // empty conditions
    
    cout << "Executing: delete all\n";
    NLQ::executeQuery(q2, db);
    
    // Check all deleted
    if (!t->rows[1].count("__deleted")) {
        cout << "FAILED: Row 1 should be deleted now.\n";
        return 1;
    }
    cout << "Test 2 Passed.\n";
    
    cout << "=== Verification Successful ===\n";
    return 0;
}

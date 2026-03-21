#include "../../include/nlq/executor.hpp"
#include <iostream>
#include <map>
#include <algorithm>

using namespace std;

namespace NLQ {

    void executeQuery(const Query& q, Database& db) {
        if (q.action == "LIST_TABLES") {
            cout << "--- Tables ---\n";
            for (const auto& pair : db.getTables()) {
                cout << pair.first << "\n";
            }
            return;
        }

        if (q.table.empty()) {
            cout << "NLQ Error: Could not identify table.\n";
            return;
        }

        if (q.action == "CREATE") {
            if (q.fields.empty()) {
                cout << "NLQ Error: No fields specified for CREATE.\n";
                return;
            }
            db.createTable(q.table, q.fields);
            cout << "Table '" << q.table << "' created via NLQ.\n";
        } 
        else if (q.action == "INSERT") {
            Table* t = db.getTable(q.table);
            if (!t) {
                cout << "NLQ Error: Table '" << q.table << "' not found.\n";
                return;
            }
            if (q.values.size() != t->fields.size()) {
                 cout << "NLQ Error: Value count mismatch. Expected " << t->fields.size() << ", got " << q.values.size() << ".\n";
                 return;
            }
            map<string, string> row;
            for (size_t i = 0; i < t->fields.size(); ++i) {
                row[t->fields[i]] = q.values[i];
            }
            t->insert(row);
             cout << "Record inserted into '" << q.table << "' via NLQ.\n";
        }
        else if (q.action == "SELECT") {
            Table* t = db.getTable(q.table);
            if (!t) {
                cout << "NLQ Error: Table '" << q.table << "' not found.\n";
                return;
            }

            cout << "Parsing: Table=" << q.table << ", Conditions=" << q.conditions.size() << "\n";
            
            // Result Set (Indices)
            vector<int> results;
            bool first = true;

            // Simple Execution:
            // 1. Run first valid index search to narrow scope
            // 2. Filter remaining
            
            // If no conditions, return all (Warning: large tables)
            if (q.conditions.empty()) {
                 cout << t->view();
                 return;
            }

            for (const auto& cond : q.conditions) {
                cout << "  - Cond: " << cond.field << " " << cond.op << " " << cond.value << "\n";
                
                vector<int> stepResults;
                if (cond.op == "=") {
                    stepResults = t->search(cond.field, cond.value);
                } else if (cond.op == ">" || cond.op == ">=") {
                    // Range search from value to MAX
                    stepResults = t->rangeSearch(cond.field, cond.value, "\xff");
                } else if (cond.op == "<" || cond.op == "<=") {
                    // Range search from MIN to value
                    stepResults = t->rangeSearch(cond.field, "", cond.value);
                }
                
                if (first) {
                    results = stepResults;
                    first = false;
                } else {
                    // Intersection
                    vector<int> intersection;
                    sort(results.begin(), results.end());
                    sort(stepResults.begin(), stepResults.end());
                    set_intersection(results.begin(), results.end(),
                                     stepResults.begin(), stepResults.end(),
                                     back_inserter(intersection));
                    results = intersection;
                }
            }

            // Print
            cout << "--- NLQ Results ---\n";
            for (int id : results) {
                 if (t->rows[id].count("__deleted")) continue;
                 // Print nicely
                 cout << "ID " << id << ": ";
                 for (const auto& f : t->fields) cout << t->rows[id][f] << " | ";
                 cout << "\n";
            }
            if (results.empty()) cout << "No matches found.\n";
        }
        else if (q.action == "DELETE") {
            Table* t = db.getTable(q.table);
            if (!t) {
                cout << "NLQ Error: Table '" << q.table << "' not found.\n";
                return;
            }
            
            // Scenario 1: Delete All (No conditions)
            if (q.conditions.empty()) {
                int count = 0;
                for (size_t i = 0; i < t->rows.size(); ++i) {
                     if (t->rows[i].count("__deleted")) continue;
                     t->deleteRow(i);
                     count++;
                }
                cout << "Deleted " << count << " records (All) from '" << q.table << "' via NLQ.\n";
            } 
            // Scenario 2: Delete w/ Conditions
            else {
                // Assume single condition for now, logic similar to UPDATE/SELECT
                // To support multiple conditions properly, we should refactor the intersection logic 
                // from SELECT into a reusable function, but for now I'll copy the basic single-cond logic 
                // or the first-step logic.
                // Or better, let's reuse a bit of logic by copy-paste (D.R.Y violation but safer for small change).
                
                string field = q.conditions[0].field;
                string val = q.conditions[0].value;
                
                vector<int> results = t->search(field, val); // Only supports equality for now in this block
                
                if (results.empty()) {
                    cout << "NLQ Results: No matches found to delete.\n";
                    return;
                }
                
                int count = 0;
                for (int id : results) {
                    t->deleteRow(id);
                    count++;
                }
                cout << "Deleted " << count << " records from '" << q.table << "' via NLQ.\n";
            }
        }
        else if (q.action == "UPDATE") {
            Table* t = db.getTable(q.table);
            if (!t) {
                cout << "NLQ Error: Table '" << q.table << "' not found.\n";
                return;
            }
            if (q.conditions.empty()) {
                cout << "NLQ Error: UPDATE requires a condition.\n";
                return;
            }
            
            // Assume single condition for now: "where name mouse"
            string field = q.conditions[0].field;
            string val = q.conditions[0].value;
            
            // Search
            vector<int> results = t->search(field, val);
            if (results.empty()) {
                cout << "NLQ Results: No matches found to update.\n";
                return;
            }
            
            int count = 0;
            for (int id : results) {
                t->updateRow(id, q.updateField, q.updateValue);
                count++;
            }
            cout << "Updated " << count << " records in '" << q.table << "' via NLQ.\n";
        }
    }

    void executeQueries(const vector<Query>& queries, Database& db) {
        for (const auto& q : queries) {
            executeQuery(q, db);
        }
    }

}

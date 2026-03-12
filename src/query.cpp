#include "../include/database.hpp"
#include "../include/query.hpp"
#include "../include/nlq/time.hpp" // Reuse for date calc
#include <iostream>
#include <algorithm>

using namespace std;

// Forward decl of helpers if needed
namespace NLQ {
    // We can't use the old executor exactly because Query struct changed structure (std::vector conds vs single fields)
    // We implementing the new "Smart" routing here.
    
    void executeSmart(Query& q, Database& db) {
        if (q.intent == "CREATE") {
            if (q.table.empty() || q.createFields.empty()) {
                cout << "ML Error: Missing table or fields for CREATE.\n"; return;
            }
            db.createTable(q.table, q.createFields);
            cout << "Table '" << q.table << "' created.\n";
            return;
        }
        
        if (q.intent == "INSERT") {
             if (q.table.empty()) { cout << "ML Error: Missing table.\n"; return; }
             Table* t = db.getTable(q.table);
             if (!t) { cout << "Table not found.\n"; return; }
             
             // Map values positional
             map<string, string> row;
             if (q.insertValues.size() > t->fields.size()) {
                  // Maybe extracted too many? truncate
             }
             for(size_t i=0; i<min(q.insertValues.size(), t->fields.size()); ++i) {
                 row[t->fields[i]] = q.insertValues[i];
             }
             t->insert(row);
             cout << "Inserted into " << q.table << ".\n";
             return;
        }

        if (q.intent == "UPDATE") {
        if (q.table.empty() || q.updateField.empty() || q.field.empty()) {
            cout << "UPDATE requires Table, Set Field, and Condition.\n";
            return;
        }
        Table* t = db.getTable(q.table);
        if (!t) { cout << "Table not found.\n"; return; }
        
        // 1. Find rows to update (Equality only for now)
        cout << "Updating " << q.table << ": Set " << q.updateField << " = " << q.updateValue 
             << " WHERE " << q.field << " = " << q.value << "\n";
        
        vector<int> results = t->search(q.field, q.value);
        
        if (results.empty()) {
            cout << "No matching records found to update.\n";
            return;
        }
        
        int count = 0;
        for (int idx : results) {
            t->updateRow(idx, q.updateField, q.updateValue);
            count++;
        }
        cout << count << " records updated.\n";
        return;
    }

    if (q.intent == "SELECT") {
            if (q.table.empty()) { cout << "ML Error: Missing table.\n"; return; }
            Table* t = db.getTable(q.table);
            if (!t) { cout << "Table not found.\n"; return; }
            
            cout << "Executing SELECT on " << q.table << " where " << q.field << " " << q.op << " " << q.value << "\n";

            // Time Filter Logic
            if (!q.timeRange.empty()) {
                string dateVal;
                if (q.timeRange == "7_days") dateVal = NLQ::getOffsetDate(7);
                else if (q.timeRange == "30_days") dateVal = NLQ::getOffsetDate(30);
                else if (q.timeRange == "1_day") dateVal = NLQ::getOffsetDate(1);
                
                // Add implicit condition
                // Assuming table has "date" field
                q.useIndex = true; // Use index on date?
                // For simplicity, just use rangeSearch on date if field matches "date"
                // Or intersect.
                // Current logic handles single primary filter.
                // If query is "orders last week", field is empty.
                if (q.field.empty()) {
                    q.field = "date";
                    q.op = ">=";
                    q.value = dateVal;
                }
            }

            // Route to B+ Tree
            vector<int> results;
            if (!q.field.empty() && !q.value.empty()) {
                 // Check if indexed? VultureDB indexes ALL fields by default.
                 // So yes, use index.
                 if (q.op == ">" || q.op == ">=") {
                     results = t->rangeSearch(q.field, q.value, "\xff");
                 } else if (q.op == "<" || q.op == "<=") {
                     results = t->rangeSearch(q.field, "", q.value);
                 } else if (q.op == "=") {
                     results = t->search(q.field, q.value);
                 }
            } else {
                // Full Scan / View All
                // We just grab all IDs? Viewing uses string return.
                // Let's just print view()
                cout << t->view();
                return;
            }

            // Print Results
            cout << "--- Results ---\n";
            for (int id : results) {
                 if (t->rows[id].count("__deleted")) continue;
                 cout << "ID " << id << ": ";
                 for (const auto& f : t->fields) cout << t->rows[id][f] << " | ";
                 cout << "\n";
            }
            if (results.empty()) cout << "No matches.\n";
        }
    }
}

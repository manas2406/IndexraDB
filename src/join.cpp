#include "../include/join.hpp"
#include <iostream>

vector<map<string, string>> joinTables(Database& db, string tableName1, string tableName2, string joinField, string joinField2) {
    Table* t1 = db.getTable(tableName1);
    Table* t2 = db.getTable(tableName2);
    
    if (!t1 || !t2) return {}; // One table missing

    // Default join field 2 to same as 1 if empty
    if (joinField2.empty()) joinField2 = joinField;

    vector<map<string, string>> result;

    // Naive Nested Loop with Index Lookup (O(N * log M))
    // Iterate t1 (Outer)
    for (const auto& row1 : t1->rows) {
        if (row1.count("__deleted")) continue;
        if (row1.find(joinField) == row1.end()) continue;

        string val = row1.at(joinField);

        // Search in t2 (Inner)
        // Check if t2 has index on joinField2
        vector<int> matchIndices = t2->search(joinField2, val);

        for (int idx : matchIndices) {
            if (t2->rows[idx].count("__deleted")) continue;
            
            // Merge
            map<string, string> merged;
            // Prefix keys to avoid collision
            for (const auto& pair : row1) {
                if (pair.first == "__deleted") continue;
                merged[pair.first] = pair.second; // Or t1.name + "." + pair.first
            }
            for (const auto& pair : t2->rows[idx]) {
                if (pair.first == "__deleted") continue;
                // Simple overwrite if collision, or prefix
                // Requirement says "Combine rows". 
                // Let's just merge. If collision, T2 wins (standard map behavior).
                // Or better, let's prefix if collision to be safe.
                if (merged.count(pair.first)) {
                    merged[t2->name + "." + pair.first] = pair.second;
                } else {
                    merged[pair.first] = pair.second;
                }
            }
            result.push_back(merged);
        }
    }

    return result;
}

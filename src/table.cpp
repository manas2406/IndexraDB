#include "../include/table.hpp"
#include <sstream>
#include <iomanip>

Table::Table(string name, vector<string> fields) : name(name), fields(fields) {
    // Create index for every field by default
    for (const auto& field : fields) {
        indexes[field] = new BPlusTree<string, int>();
    }
}

Table::~Table() {
    for (auto& pair : indexes) {
        delete pair.second;
    }
}

void Table::insert(map<string, string> row) {
    int id = rows.size(); // Index in the vector
    rows.push_back(row);
    
    // Update indexes
    for (const auto& pair : row) {
        string field = pair.first;
        string value = pair.second;
        if (indexes.find(field) != indexes.end()) {
            indexes[field]->insert(value, id);
        }
    }
}

void Table::deleteRow(int index) {
    if (index < 0 || index >= rows.size()) return;
    
    // Soft delete check
    if (rows[index].count("__deleted")) return;

    // Mark as deleted
    rows[index]["__deleted"] = "true";

    // Update indexes (Remove from B+ Trees)
    for (const auto& pair : rows[index]) {
        string field = pair.first;
        string value = pair.second;
        if (field == "__deleted") continue;

        if (indexes.find(field) != indexes.end()) {
            indexes[field]->remove(value, index);
        }
    }
}

void Table::updateRow(int index, string field, string newValue) {
    if (index < 0 || index >= rows.size()) return;
    if (rows[index].count("__deleted")) return;

    string oldValue = rows[index][field];
    
    // Update storage
    rows[index][field] = newValue;

    // Update Index
    if (indexes.find(field) != indexes.end()) {
        indexes[field]->remove(oldValue, index);
        indexes[field]->insert(newValue, index);
    }
}

vector<int> Table::search(string field, string value) {
    if (indexes.find(field) != indexes.end()) {
        return indexes[field]->search(value);
    }
    return {};
}

vector<int> Table::rangeSearch(string field, string start, string end) {
    if (indexes.find(field) != indexes.end()) {
        return indexes[field]->rangeSearch(start, end);
    }
    return {};
}

string Table::view() {
    stringstream ss;
    // Header
    ss << "ID | ";
    for (const auto& f : fields) ss << f << " | ";
    ss << "\n----------------------------------------\n";
    
    // Use B+ Tree of the first field to iterate in order? 
    // Or just iterate vector? Iterating vector is O(N) but easiest for "Dump".
    // Prompt said "View table (sorted output using B+ tree order)".
    // So we should pick the primary key or first field to sort by. 
    // Let's use the first field defined in `fields`.
    
    if (fields.empty()) return "Empty Schema\n";
    string sortField = fields[0];
    
    // Get all IDs from the index of the first field (Sorted by that field)
    // To do this, we need a full scan of the leaf nodes of the B+ Tree.
    // Our B+ Tree hasn't exposed a "getAll" easily, but `rangeSearch` from "" to "~" (ASCII max) handles it.
    
    vector<int> sortedIndices = rangeSearch(sortField, "", "\xff"); 
    
    // Note: This only shows rows that have a value for the first field. 
    // If we want to show ALL rows regardless, we might need a different approach.
    // For now, assuming all rows have values for all fields.

    for (int idx : sortedIndices) {
        if (rows[idx].count("__deleted")) continue;
        ss << idx << " | ";
        for (const auto& f : fields) {
            ss << rows[idx][f] << " | ";
        }
        ss << "\n";
    }
    return ss.str();
}

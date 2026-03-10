#ifndef TABLE_HPP
#define TABLE_HPP

#include <string>
#include <vector>
#include <map>
#include "bptree.hpp"

using namespace std;

class Table {
public:
    string name;
    vector<string> fields;
    vector<map<string, string>> rows;
    
    // We store pointers to trees because they are non-copyable/heavy or just for map storage ease
    // Key = FieldName, Value = B+ Tree indexing that field
    map<string, BPlusTree<string, int>*> indexes;

    // Helper to track deleted rows (Soft Delete)
    // We could store a bool in the row map, e.g., "__deleted" -> "true"
    
    Table(string name, vector<string> fields);
    ~Table(); // Cleanup trees

    void insert(map<string, string> row);
    void deleteRow(int index);
    void updateRow(int index, string field, string newValue);
    
    // Returns list of Row Indices
    vector<int> search(string field, string value);
    vector<int> rangeSearch(string field, string start, string end);
    
    // Helper to print table
    string view();
};

#endif

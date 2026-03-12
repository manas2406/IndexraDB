#include "../include/fileio.hpp"
#include <fstream>
#include <sstream>
#include <filesystem>
#include <iostream>

using namespace std;
namespace fs = std::filesystem;

void saveDatabase(Database& db, string directory) {
    if (!fs::exists(directory)) {
        fs::create_directories(directory);
    }

    // Save Schema
    ofstream schemaFile(directory + "/schema.txt");
    for (const auto& pair : db.getTables()) {
        Table* t = pair.second;
        schemaFile << t->name;
        for (const auto& f : t->fields) {
            schemaFile << " " << f;
        }
        schemaFile << "\n";
    }
    schemaFile.close();

    // Save Data
    for (const auto& pair : db.getTables()) {
        Table* t = pair.second;
        ofstream dataFile(directory + "/" + t->name + ".db");
        
        for (const auto& row : t->rows) {
            if (row.count("__deleted")) continue;
            
            // Format: field1:val1|field2:val2|...
            bool first = true;
            for (const auto& f : t->fields) {
                if (!first) dataFile << "|";
                dataFile << f << ":" << row.at(f);
                first = false;
            }
            dataFile << "\n";
        }
        dataFile.close();
    }
    cout << "Database saved to " << directory << endl;
}

void loadDatabase(Database& db, string directory) {
    if (!fs::exists(directory + "/schema.txt")) {
        cout << "No save found." << endl;
        return;
    }

    // Load Schema
    ifstream schemaFile(directory + "/schema.txt");
    string line;
    while (getline(schemaFile, line)) {
        stringstream ss(line);
        string tableName, field;
        vector<string> fields;
        ss >> tableName;
        while (ss >> field) fields.push_back(field);
        
        db.createTable(tableName, fields);
    }
    schemaFile.close();

    // Load Data
    for (const string& tableName : db.listTables()) {
        ifstream dataFile(directory + "/" + tableName + ".db");
        if (!dataFile.is_open()) continue;

        string rowLine;
        while (getline(dataFile, rowLine)) {
            stringstream ss(rowLine);
            string segment;
            map<string, string> row;
            
            // Split by '|'
            while (getline(ss, segment, '|')) {
                size_t delimPos = segment.find(':');
                if (delimPos != string::npos) {
                    string key = segment.substr(0, delimPos);
                    string val = segment.substr(delimPos + 1);
                    row[key] = val;
                }
            }
            
            if (!row.empty()) {
                db.getTable(tableName)->insert(row);
            }
        }
        dataFile.close();
    }
    cout << "Database loaded from " << directory << endl;
}

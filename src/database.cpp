#include "../include/database.hpp"

Database::~Database() {
    for (auto& pair : tables) {
        delete pair.second;
    }
}

void Database::createTable(string name, vector<string> fields) {
    if (tables.find(name) != tables.end()) {
        // Table exists, maybe error or return? 
        // For simplicity, we do nothing or overwrite. Let's do nothing.
        return;
    }
    tables[name] = new Table(name, fields);
}

void Database::dropTable(string name) {
    if (tables.find(name) != tables.end()) {
        delete tables[name];
        tables.erase(name);
    }
}

Table* Database::getTable(string name) {
    if (tables.find(name) != tables.end()) {
        return tables[name];
    }
    return nullptr;
}

vector<string> Database::listTables() {
    vector<string> names;
    for (const auto& pair : tables) {
        names.push_back(pair.first);
    }
    return names;
}

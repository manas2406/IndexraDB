#ifndef DATABASE_HPP
#define DATABASE_HPP

#include "table.hpp"
#include <map>
#include <string>
#include <vector>

using namespace std;

class Database {
private:
    map<string, Table*> tables;

public:
    ~Database();
    
    void createTable(string name, vector<string> fields);
    void dropTable(string name);
    Table* getTable(string name);
    vector<string> listTables();
    
    // File I/O will be handled by separate module or helper, 
    // but Database needs to expose data for saving.
    const map<string, Table*>& getTables() const { return tables; }
};

#endif

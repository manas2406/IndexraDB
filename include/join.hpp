#ifndef JOIN_HPP
#define JOIN_HPP

#include "database.hpp"
#include <string>
#include <vector>
#include <map>

using namespace std;

// Returns a list of merged rows (column -> value).
// result columns will be prefixed if collision occurs or just flat. 
// For simplicity in this terminal app:
// Result = "Table1.Col" | "Table2.Col" ...
vector<map<string, string>> joinTables(Database& db, string tableName1, string tableName2, string joinField, string joinField2 = "");

#endif

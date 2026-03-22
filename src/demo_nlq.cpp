#include <iostream>
#include "../include/database.hpp"
#include "../include/nlq/parser.hpp"
#include "../include/nlq/executor.hpp"

using namespace std;

int main() {
    cout << "=== Antigravity DB NLQ Demo ===\n\n";

    Database db;

    // 1. Create Table
    string q1 = "create table products with fields id name price";
    cout << "[Query] " << q1 << "\n";
    NLQ::executeQuery(NLQ::parseQuery(q1), db);
    cout << "\n";

    // 2. Insert Data
    string q2 = "add to products 1 Laptop 900";
    cout << "[Query] " << q2 << "\n";
    NLQ::executeQuery(NLQ::parseQuery(q2), db);

    string q3 = "insert into products values 2 Mouse 050"; // 050 < 900
    cout << "[Query] " << q3 << "\n";
    NLQ::executeQuery(NLQ::parseQuery(q3), db);
    cout << "\n";

    // 3. Select Data
    string q4 = "show me products above 100";
    cout << "[Query] " << q4 << "\n";
    NLQ::executeQuery(NLQ::parseQuery(q4), db);

    return 0;
}

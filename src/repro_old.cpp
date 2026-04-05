#include <iostream>
#include <vector>
#include <string>
#include "../include/nlq/parser.hpp"
#include "../include/nlq/intent.hpp"

using namespace std;

void printQuery(const NLQ::Query& q) {
    cout << "Action: " << q.action << "\n";
    cout << "Table: " << q.table << "\n";
    cout << "Conditions: " << q.conditions.size() << "\n";
    for(const auto& c : q.conditions) {
        cout << "  - " << c.field << " " << c.op << " " << c.value << "\n";
    }
}

int main() {
    cout << "--- CASE 1: UPDATE ---\n";
    string q1 = "update items set price 100 where name mouse";
    NLQ::Query p1 = NLQ::parseQuery(q1);
    printQuery(p1);

    cout << "\n--- CASE 2: SELECT ---\n";
    string q2 = "show me items where name mouse";
    NLQ::Query p2 = NLQ::parseQuery(q2);
    printQuery(p2);
    
    return 0;
}

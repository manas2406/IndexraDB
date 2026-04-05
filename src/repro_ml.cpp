#include <iostream>
#include <vector>
#include <string>
#include "../include/nlq_ml/pipeline.hpp"
#include "../include/nlq_ml/intent.hpp"
#include "../include/nlq_ml/slots.hpp"
#include "../include/nlq_ml/normalize.hpp"

// Mock linkage if needed or just compile the necessary files.
// We need pipeline.cpp, intent.cpp, slots.cpp, normalize.cpp, embed.cpp (maybe)

using namespace std;

void printQuery(const Query& q) {
    cout << "Intent: " << q.intent << "\n";
    cout << "Table: " << q.table << "\n";
    cout << "Field: " << q.field << "\n";
    cout << "Value: " << q.value << "\n";
    cout << "Op: " << q.op << "\n";
    cout << "UpdateField: " << q.updateField << "\n";
    cout << "UpdateValue: " << q.updateValue << "\n";
}

int main() {
    cout << "--- CASE 1: UPDATE ---\n";
    string q1 = "update items set price 100 where name mouse";
    Query p1 = NLQ_ML::processParams(q1);
    printQuery(p1);

    cout << "\n--- CASE 2: SELECT ---\n";
    string q2 = "show me items where name mouse";
    Query p2 = NLQ_ML::processParams(q2);
    printQuery(p2);

    cout << "\n--- CASE 3: SELECT IS ---\n";
    string q3 = "show me items where name is mouse";
    Query p3 = NLQ_ML::processParams(q3);
    printQuery(p3);

    return 0;
}

#ifndef NLQ_INTENT_HPP
#define NLQ_INTENT_HPP

#include <string>
#include <vector>

using namespace std;

namespace NLQ {

    struct Condition {
        string field;
        string op; // ">", "<", "=", ">=", "<="
        string value;
    };

    struct Query {
        string action; // "SELECT", "CREATE", "INSERT"
        string table;
        vector<Condition> conditions; // For SELECT
        
        vector<string> fields; // For CREATE
        vector<string> values; // For INSERT
        
        // For UPDATE
        string updateField;
        string updateValue;

        string sortField;
        bool isRange = false;
    };

}

#endif

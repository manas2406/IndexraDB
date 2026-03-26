#include "../../include/nlq_ml/slots.hpp"
#include <sstream>
#include <regex>
#include <iostream>

using namespace std;

namespace NLQ_ML {

    static vector<string> tokenize(const string& str) {
        vector<string> tokens;
        stringstream ss(str);
        string word;
        while(ss >> word) tokens.push_back(word);
        return tokens;
    }

    bool isNumber(const string& s) {
        return !s.empty() && std::all_of(s.begin(), s.end(), ::isdigit);
    }
    
    bool isOperator(const string& s) {
        return s == ">" || s == "<" || s == "=" || s == ">=" || s == "<=";
    }

    std::vector<Slot> SlotModel::runTokens(const std::string& text, const std::string& intent) {
        vector<Slot> slots;
        vector<string> tokens = tokenize(text);
        
        // 1. Extract Time
        // "last week" -> 7_days
        // "yesterday" -> 1_day
        // We do this via string search first as tokens might be split
        if (text.find("last week") != string::npos || text.find("7 days") != string::npos) slots.push_back({"7_days", "TIME"});
        else if (text.find("last month") != string::npos || text.find("30 days") != string::npos) slots.push_back({"30_days", "TIME"});
        else if (text.find("yesterday") != string::npos) slots.push_back({"1_day", "TIME"});

        // 2. Token Scanning
        for (size_t i = 0; i < tokens.size(); ++i) {
            string t = tokens[i];
            
            // OPERATOR
            if (isOperator(t)) {
                slots.push_back({t, "OPERATOR"});
                
                // Usually VALUE follows OPERATOR
                if (i + 1 < tokens.size()) {
                    slots.push_back({tokens[i+1], "VALUE"});
                    // If preceding was not already tagged, maybe it's field?
                     // (Simple heuristic: assumes field op value)
                    if (i > 0 && !isNumber(tokens[i-1])) { // Don't tag numbers as field
                         string cand = tokens[i-1];
                         if (cand != "orders" && cand != "users" && cand != "products" && cand != "items" && cand != "table") {
                            slots.push_back({cand, "FIELD"});
                         }
                    }
                }
            }
            // CREATE context
            else if (intent == "CREATE") {
                if ((t == "table" || t == "named") && i + 1 < tokens.size()) {
                    if (tokens[i+1] != "with" && tokens[i+1] != "fields")
                        slots.push_back({tokens[i+1], "TABLE"});
                }
                // Fields are usually after "fields" keyword
                // Logic handled by specialized CREATE parser usually, but let's try to tag
                if (t != "fields" && t != "with" && t != "table" && t != "create") {
                     // potentially a field if intent is create
                     // This is risky without strict BIO.
                }
            }
            // INSERT context
            else if (intent == "INSERT") {
                 if ((t == "into" || t == "to") && i + 1 < tokens.size()) {
                      slots.push_back({tokens[i+1], "TABLE"});
                 }
                 if (isNumber(t)) slots.push_back({t, "VALUE"});
                 // String values? Hard to distiguish from table name without schema.
            }
            // SELECT context
            else {
                // Table markers
                if ((t == "from" || t == "in" || t == "table") && i + 1 < tokens.size()) {
                    if (tokens[i+1] != "last") // avoid "from last week"
                       slots.push_back({tokens[i+1], "TABLE"});
                }
                // Implicit: "orders" is usually a table
                if (t == "orders" || t == "users" || t == "products" || t == "items") slots.push_back({t, "TABLE"});
                
                // If we found a value but no field, tag "price" or "amount" as inferred? 
                // That's Semantic Phase. Here we just tag what we see.
            }
        }
        return slots;
    }
}

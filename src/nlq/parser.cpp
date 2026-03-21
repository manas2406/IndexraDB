#include "../../include/nlq/parser.hpp"
#include "../../include/nlq/time.hpp"
#include <sstream>
#include <algorithm>
#include <iostream>
#include <vector>
#include <cmath>
#include <map>

using namespace std;

namespace NLQ {

    // --- Levenshtein (Fuzzy Logic) ---
    int levenshtein(const string& s1, const string& s2) {
        const size_t m = s1.length();
        const size_t n = s2.length();
        if (m == 0) return n;
        if (n == 0) return m;

        vector<vector<int>> dp(m + 1, vector<int>(n + 1));

        for (size_t i = 0; i <= m; ++i) dp[i][0] = i;
        for (size_t j = 0; j <= n; ++j) dp[0][j] = j;

        for (size_t i = 1; i <= m; ++i) {
            for (size_t j = 1; j <= n; ++j) {
                int cost = (s1[i - 1] == s2[j - 1]) ? 0 : 1;
                dp[i][j] = min({ dp[i - 1][j] + 1, dp[i][j - 1] + 1, dp[i - 1][j - 1] + cost });
            }
        }
        return dp[m][n];
    }

    bool matches(const string& word, const string& target) {
        if (word == target) return true;
        int dist = levenshtein(word, target);
        if (target.length() <= 3) return dist == 0;
        if (target.length() <= 6) return dist <= 1;
        return dist <= 2;
    }

    vector<string> tokenize(string s) {
         vector<string> tokens;
         stringstream ss(s);
         string word;
         while (ss >> word) tokens.push_back(word);
         return tokens;
    }

    // --- Intent Classification ---
    string detectIntent(const vector<string>& tokens) {
        map<string, int> scores;
        scores["CREATE"] = 0;
        scores["INSERT"] = 0;
        scores["UPDATE"] = 0;
        scores["SELECT"] = 0;

        for (const string& tok : tokens) {
            // Create Qualifiers
            if (matches(tok, "create") || matches(tok, "make") || matches(tok, "construct") || matches(tok, "build") || matches(tok, "define") || matches(tok, "new")) scores["CREATE"] += 2;
            if (matches(tok, "want") || matches(tok, "need")) scores["CREATE"] += 1; // "I want a table"
            
            // Insert Qualifiers
            if (matches(tok, "insert") || matches(tok, "add") || matches(tok, "put") || matches(tok, "append") || matches(tok, "push")) scores["INSERT"] += 2;
            if (matches(tok, "into") || matches(tok, "values")) scores["INSERT"] += 1;

            // Select Qualifiers
            if (matches(tok, "select") || matches(tok, "show") || matches(tok, "find") || matches(tok, "get") || matches(tok, "list") || matches(tok, "search") || matches(tok, "give")) scores["SELECT"] += 2;
            if (matches(tok, "from") || matches(tok, "where") || matches(tok, "search") || matches(tok, "query")) scores["SELECT"] += 1;

            // Update Qualifiers
            if (matches(tok, "update") || matches(tok, "mod") || matches(tok, "change") || matches(tok, "set") || matches(tok, "edit")) scores["UPDATE"] += 2;
        }

        // Context Logic: "table" keyword boosts CREATE if no other strong action
        bool hasTable = false;
        for (const auto& t : tokens) if (matches(t, "table")) hasTable = true;
        if (hasTable && scores["INSERT"] == 0 && scores["SELECT"] == 0 && scores["UPDATE"] == 0) scores["CREATE"] += 1;

        string bestAction = "SELECT"; // Default
        int maxScore = 0;
        for (auto const& [action, score] : scores) {
            if (score > maxScore) {
                maxScore = score;
                bestAction = action;
            }
        }
        return bestAction;
    }

    // --- Entity Extraction ---
    // Extract table name based on context markers
    string extractTable(const vector<string>& tokens, string action) {
        // Markers that precede a table name
        vector<string> markers;
        if (action == "CREATE") markers = {"named", "called", "table", "structure"};
        else if (action == "INSERT") markers = {"to", "into", "table", "in"};
        else if (action == "UPDATE") markers = {"update", "table"};
        else markers = {"from", "in", "table", "find", "show", "of"}; 

        for (size_t i = 0; i < tokens.size(); ++i) {
            for (const string& m : markers) {
                if (matches(tokens[i], m) && i + 1 < tokens.size()) {
                    size_t k = i + 1;
                    while (k < tokens.size()) {
                        string candidate = tokens[k];
                        bool skip = false;
                        
                        // Skip common stopwords/fillers
                        if (matches(candidate, "me") || matches(candidate, "all") || matches(candidate, "us")) skip = true;
                        if (matches(candidate, "with") || matches(candidate, "name") || matches(candidate, "named") || matches(candidate, "called")) skip = true;
                        if (matches(candidate, "new") || matches(candidate, "table")) skip = true;
                        if (matches(candidate, "a") || matches(candidate, "an") || matches(candidate, "the")) skip = true; 

                        // Skip if candidate is another marker (e.g. "table from")
                        for(const string& m2 : markers) if (matches(candidate, m2)) skip = true;

                        if (!skip) return candidate;
                        k++;
                    }
                }
            }
        }
        // Fallback: Check known entities
        for (const auto& tok : tokens) {
            if (matches(tok, "users")) return "users";
            if (matches(tok, "orders")) return "orders";
            if (matches(tok, "products")) return "products";
            if (matches(tok, "students")) return "students";
            if (matches(tok, "items")) return "items";
        }
        return "";
    }

    Query parseQuery(string input) {
        Query q;
        transform(input.begin(), input.end(), input.begin(), ::tolower);
        vector<string> tokens = tokenize(input);
        if (tokens.empty()) return q;

        q.action = detectIntent(tokens);
        q.table = extractTable(tokens, q.action);

        // --- Payload Extraction (Fields, Values, Conditions) ---
        
        if (q.action == "CREATE") {
            bool capturing = false;
            for (const string& tok : tokens) {
                bool isTrigger = matches(tok, "fields") || matches(tok, "columns") || matches(tok, "with");
                if (capturing && !isTrigger) q.fields.push_back(tok);
                if (isTrigger) capturing = true;
            }
        }
        else if (q.action == "INSERT") {
             bool capturing = false;
             for (size_t i = 0; i < tokens.size(); ++i) {
                 bool isTrigger = matches(tokens[i], "values") || matches(tokens[i], q.table);
                 if (capturing && !matches(tokens[i], "values")) q.values.push_back(tokens[i]);
                 if (isTrigger) capturing = true;
             }
             // If we captured the table name itself by accident (if table name was trigger), remove it
             // (Logic above handles this mostly, but "values" check is explicit)
        }
        else if (q.action == "UPDATE") {
             // update items set price 100 where name mouse
             bool capturingSet = false;
             bool capturingWhere = false;
             
             for (size_t i = 0; i < tokens.size(); ++i) {
                 string t = tokens[i];
                 if (matches(t, "set")) { capturingSet = true; capturingWhere = false; continue; }
                 if (matches(t, "where")) { capturingWhere = true; capturingSet = false; continue; }
                 
                 if (capturingSet) {
                     // price 100
                     // Heuristic: if digit, it's value. prev is field.
                     if (isdigit(t[0])) {
                         q.updateValue = t;
                         if (i > 0) q.updateField = tokens[i-1];
                     }
                 }
                 if (capturingWhere) {
                     // name mouse
                     // Heuristic: if i+1 is value?
                     // Similar to SELECT loop but simpler
                     // Just grab field and value
                         if (i + 1 < tokens.size()) {
                             string field = t;
                             string value = tokens[i+1];
                             string op = "=";
                             q.conditions.push_back({field, op, value});
                             break; // Assume 1 condition for now
                         }
                 }
             }
        }
        else if (q.action == "SELECT") {
            // Conditions
             // 1. Comparison
            vector<string> ops = {"above", "more", "greater", "below", "less", "cheaper", "is", "equals", "where", "known"};
            for (size_t i = 0; i < tokens.size(); ++i) {
                 for (const string& op : ops) {
                     if (matches(tokens[i], op)) {
                         string field = (i > 0) ? tokens[i-1] : "";
                         string val = (i + 1 < tokens.size()) ? tokens[i+1] : "";
                         
                         // Fix "cheaper than X"
                         if (op == "cheaper" && i + 2 < tokens.size()) val = tokens[i+2];

                         // Handle "where field val" -> op is where. field is previous token? 
                         // No, "where name mouse". tokens[i] is where. field=tokens[i-1] is items? No.
                         // If op is "where", logic changes.
                         if (op == "where") {
                            if (i + 2 < tokens.size()) {
                                field = tokens[i+1];
                                val = tokens[i+2];
                            }
                         }

                         // Cleanup field inference
                         if (field == q.table || field == "me" || field == "all" || matches(field, "products") || matches(field, "items")) {
                             // If explicit "where name mouse", field is 'name', not 'items'.
                             // Only default to price if field is truly generic
                             field = "price";
                         }
                         
                         // Check valid field name?
                         if (op == "where" && i + 2 < tokens.size()) {
                             field = tokens[i+1]; // definitive override
                         }

                         string dbOp = ">";
                         if (op == "below" || op == "less" || op == "cheaper") dbOp = "<";
                         if (op == "is" || op == "equals" || op == "where") dbOp = "=";

                         if (!val.empty() && !field.empty()) q.conditions.push_back({field, dbOp, val});
                     }
                 }
            }

            // 2. Time
            if (input.find("last week") != string::npos) q.conditions.push_back({"date", ">=", getOffsetDate(7)});
            else if (input.find("last month") != string::npos) q.conditions.push_back({"date", ">=", getOffsetDate(30)});
            else if (input.find("yesterday") != string::npos) q.conditions.push_back({"date", "=", getOffsetDate(1)});
        }

        return q;
    }
}

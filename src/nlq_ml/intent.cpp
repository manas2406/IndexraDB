#include "../../include/nlq_ml/intent.hpp"
#include <map>
#include <vector>
#include <sstream>

using namespace std;

namespace NLQ_ML {
    
    // Simple tokenizer
    static vector<string> tokenize(const string& str) {
        vector<string> tokens;
        stringstream ss(str);
        string word;
        while(ss >> word) tokens.push_back(word);
        return tokens;
    }

    string IntentModel::run(const string& text) {
        map<string, int> logits;
        logits["SELECT"] = 0;
        logits["CREATE"] = 0;
        logits["INSERT"] = 0;
        logits["JOIN"] = 0;

        vector<string> tokens = tokenize(text);
        
        for (const auto& t : tokens) {
            // SELECT features
            if (t == "show" || t == "find" || t == "get" || t == "list" || t == "search" || t == "select") logits["SELECT"] += 3;
            if (t == "from" || t == "where" || t == "which" || t == "who") logits["SELECT"] += 1;
            if (t == ">" || t == "<" || t == "=") logits["SELECT"] += 2; // comparison implies filter/select

            // CREATE features
            if (t == "create" || t == "make" || t == "new" || t == "construct") logits["CREATE"] += 3;
            if (t == "fields" || t == "columns") logits["CREATE"] += 2;

            // INSERT features
            if (t == "insert" || t == "add" || t == "put" || t == "push") logits["INSERT"] += 3;
            if (t == "into" || t == "value" || t == "values") logits["INSERT"] += 2;
            
            // JOIN features
            if (t == "join" || t == "combine" || t == "merge") logits["JOIN"] += 5; // Strong signal
            
            // UPDATE features
            if (t == "update" || t == "change" || t == "modify" || t == "set") logits["UPDATE"] += 5;
        }

        // Softmax argmax
        string bestClass = "SELECT";
        int maxLogit = 0;
        for (auto const& [label, score] : logits) {
            if (score > maxLogit) {
                maxLogit = score;
                bestClass = label;
            }
        }
        return bestClass;
    }
}

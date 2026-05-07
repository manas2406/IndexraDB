#include "../../include/nlq_ml/pipeline.hpp"
#include "../../include/nlq_ml/normalize.hpp"
#include <iostream>
#include <algorithm>
#include <sstream>
#include <cstdio>
#include <memory>
#include <stdexcept>
#include <array>

using namespace std;

// Forward declare httpPost from llm_bridge.cpp
string httpPost(const string& url, const string& jsonPayload);
string escapeJSON(const string& s);

namespace NLQ_ML {

    // --- Minimal JSON helpers for parsing ML response ---
    
    static string extractJSONString(const string& json, const string& key) {
        string search = "\"" + key + "\":";
        size_t pos = json.find(search);
        if (pos == string::npos) return "";
        
        pos = json.find("\"", pos + search.length());
        if (pos == string::npos) return "";
        size_t start = pos + 1;
        size_t end = json.find("\"", start);
        if (end == string::npos) return "";
        return json.substr(start, end - start);
    }

    // Parse slots array: [{"text": "...", "label": "..."}, ...]
    struct MLSlot {
        string text;
        string label;
    };

    static vector<MLSlot> parseSlots(const string& json) {
        vector<MLSlot> slots;
        string search = "\"slots\":";
        size_t pos = json.find(search);
        if (pos == string::npos) return slots;
        
        size_t arrayStart = json.find("[", pos);
        size_t arrayEnd = json.find("]", arrayStart);
        if (arrayStart == string::npos || arrayEnd == string::npos) return slots;
        
        string content = json.substr(arrayStart + 1, arrayEnd - arrayStart - 1);
        
        // Iterate through objects
        size_t objStart = 0;
        while ((objStart = content.find("{", objStart)) != string::npos) {
            size_t objEnd = content.find("}", objStart);
            if (objEnd == string::npos) break;
            string objStr = content.substr(objStart, objEnd - objStart + 1);
            
            MLSlot s;
            s.text = extractJSONString(objStr, "text");
            s.label = extractJSONString(objStr, "label");
            if (!s.text.empty() && !s.label.empty()) {
                slots.push_back(s);
            }
            objStart = objEnd + 1;
        }
        return slots;
    }

    Query processParams(string userQuery) {
        Query q;
        
        // Phase 1: Normalize (still in C++)
        string cleanText = normalizeText(userQuery);
        
        // Phase 2: Call Python ML Server
        string payload = "{\"query\": \"" + escapeJSON(cleanText) + "\"}";
        
        const char* envUrl = getenv("LLM_SERVER_URL");
        string url = envUrl ? string(envUrl) + "/nlq_ml" : "http://localhost:8000/nlq_ml";
        
        string response;
        try {
            response = httpPost(url, payload);
        } catch (const exception& e) {
            cout << "[ML] Server error: " << e.what() << "\n";
            cout << "[ML] Falling back to heuristic mode.\n";
            // Fallback: simple keyword intent detection
            if (cleanText.find("create") != string::npos) q.intent = "CREATE";
            else if (cleanText.find("insert") != string::npos || cleanText.find("add") != string::npos) q.intent = "INSERT";
            else if (cleanText.find("update") != string::npos || cleanText.find("modify") != string::npos) q.intent = "UPDATE";
            else if (cleanText.find("delete") != string::npos || cleanText.find("remove") != string::npos) q.intent = "DELETE";
            else if (cleanText.find("join") != string::npos || cleanText.find("merge") != string::npos) q.intent = "JOIN";
            else q.intent = "SELECT";
            return q;
        }
        
        if (response.empty() || response.find("\"error\"") != string::npos) {
            cout << "[ML] Empty or error response from ML server.\n";
            cout << "[ML] Response: " << response << "\n";
            return q;
        }
        
        // Phase 3: Parse response
        q.intent = extractJSONString(response, "intent");
        
        vector<MLSlot> slots = parseSlots(response);
        
        // Debug output
        cout << "[ML] Intent: " << q.intent << "\n";
        for (const auto& s : slots) {
            cout << "[ML] Slot: " << s.text << " [" << s.label << "]\n";
        }
        
        // Phase 4: Map slots to Query fields
        for (const auto& s : slots) {
            if (s.label == "TABLE" && q.table.empty()) q.table = s.text;
            if (s.label == "FIELD" && q.field.empty()) q.field = s.text;
            if (s.label == "OPERATOR") q.op = s.text;
            if (s.label == "VALUE" && q.value.empty()) q.value = s.text;
            if (s.label == "TIME") q.timeRange = s.text;
        }
        
        // Phase 5: Semantic Resolution / Post-Processing
        
        // Normalize operators from natural language to symbols
        if (q.op == "above" || q.op == "more than" || q.op == "greater than" || 
            q.op == "over" || q.op == "exceeding" || q.op == "higher than") q.op = ">";
        if (q.op == "below" || q.op == "less than" || q.op == "under" || 
            q.op == "cheaper than" || q.op == "lower than" || q.op == "fewer than") q.op = "<";
        if (q.op == "equal to" || q.op == "equals" || q.op == "is") q.op = "=";
        if (q.op == "at least") q.op = ">=";
        if (q.op == "at most" || q.op == "not exceeding") q.op = "<=";
        
        // If we have a value and operator but no field, assume "price"
        if (q.field.empty() && !q.value.empty() && !q.op.empty()) {
            q.field = "price";
        }
        
        // CREATE: extract fields from slots
        if (q.intent == "CREATE") {
            for (const auto& s : slots) {
                if (s.label == "FIELD") {
                    // The slot model may return "name age grade" as one token
                    // Split by spaces
                    stringstream ss(s.text);
                    string word;
                    while (ss >> word) {
                        q.createFields.push_back(word);
                    }
                }
            }
            // Fallback: parse from text if no fields found
            if (q.createFields.empty()) {
                stringstream ss(cleanText);
                string word;
                bool capturing = false;
                while (ss >> word) {
                    if (word == "fields" || word == "with") {
                        capturing = true;
                        continue;
                    }
                    if (capturing) q.createFields.push_back(word);
                }
            }
        }
        
        // INSERT: extract values from slots
        if (q.intent == "INSERT") {
            for (const auto& s : slots) {
                if (s.label == "VALUE") {
                    // May be "1 laptop 900" — split
                    stringstream ss(s.text);
                    string word;
                    while (ss >> word) {
                        q.insertValues.push_back(word);
                    }
                }
            }
            // Fallback
            if (q.insertValues.empty()) {
                stringstream ss(cleanText);
                string word;
                bool capturing = false;
                while (ss >> word) {
                    if (capturing && word != "values") q.insertValues.push_back(word);
                    if (word == "values" || word == q.table) capturing = true;
                }
            }
        }
        
        // UPDATE: Map second field/value to updateField/updateValue
        if (q.intent == "UPDATE") {
            // Slots may give: TABLE=products, FIELD=price, VALUE=60, FIELD=name, VALUE=mouse
            // We need: updateField=price, updateValue=60, field=name (WHERE), value=mouse (WHERE)
            
            vector<string> fieldSlots, valueSlots;
            for (const auto& s : slots) {
                if (s.label == "FIELD") {
                    stringstream ss(s.text);
                    string w;
                    while (ss >> w) fieldSlots.push_back(w);
                }
                if (s.label == "VALUE") {
                    stringstream ss(s.text);
                    string w;
                    while (ss >> w) valueSlots.push_back(w);
                }
            }
            
            if (fieldSlots.size() >= 2 && valueSlots.size() >= 2) {
                q.updateField = fieldSlots[0];
                q.updateValue = valueSlots[0];
                q.field = fieldSlots[1];
                q.value = valueSlots[1];
            } else if (fieldSlots.size() == 1 && valueSlots.size() >= 1) {
                q.updateField = fieldSlots[0];
                q.updateValue = valueSlots[0];
            }
        }
        
        // Time range normalization
        if (!q.timeRange.empty()) {
            string tr = q.timeRange;
            if (tr.find("last week") != string::npos || tr.find("7 days") != string::npos) q.timeRange = "7_days";
            else if (tr.find("last month") != string::npos || tr.find("30 days") != string::npos) q.timeRange = "30_days";
            else if (tr.find("yesterday") != string::npos) q.timeRange = "1_day";
        }

        return q;
    }
}

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <sstream>
#include <cstdio>
#include <memory>
#include <stdexcept>
#include <algorithm>
#include <array>
#include "../include/database.hpp"
#include "../include/nlq/intent.hpp"

using namespace std;

// --- JSON Helper ---
// Very minimal JSON string escaper
string escapeJSON(const string& s) {
    ostringstream o;
    for (char c : s) {
        if (c == '"') o << "\\\"";
        else if (c == '\\') o << "\\\\";
        else if (c == '\b') o << "\\b";
        else if (c == '\f') o << "\\f";
        else if (c == '\n') o << "\\n";
        else if (c == '\r') o << "\\r";
        else if (c == '\t') o << "\\t";
        else if (c >= 0 && c <= 0x1f) {} // ignore control chars
        else o << c;
    }
    return o.str();
}

// --- Schema Export ---
string exportSchemaJSON(const Database& db) {
    ostringstream oss;
    oss << "{";
    bool firstTable = true;
    for (const auto& pair : db.getTables()) {
        if (!firstTable) oss << ", ";
        firstTable = false;
        
        Table* t = pair.second;
        oss << "\"" << escapeJSON(t->name) << "\": [";
        for (size_t i = 0; i < t->fields.size(); ++i) {
            if (i > 0) oss << ", ";
            oss << "\"" << escapeJSON(t->fields[i]) << "\"";
        }
        oss << "]";
    }
    oss << "}";
    return oss.str();
}

// --- HTTP Request (via curl) ---
string httpPost(const string& url, const string& jsonPayload) {
    string cmd = "curl -s -X POST " + url + " -H \"Content-Type: application/json\" -d '" + jsonPayload + "'";
    
    // Safety: escape single quotes in payload for shell command
    // Actually, passing complex JSON in shell cmd is risky. 
    // Better to use -d @- and pipe it, or just risk it for this demo if we escape quotes.
    // Let's do the quote escape for simple demo safety.
    string safePayload = jsonPayload;
    // Replace ' with '"'"' (bash escape)
    size_t pos = 0;
    while ((pos = safePayload.find("'", pos)) != string::npos) {
        safePayload.replace(pos, 1, "'\"'\"'");
        pos += 5;
    }
    
    cmd = "curl -s -X POST " + url + " -H \"Content-Type: application/json\" -d '" + safePayload + "'";

    array<char, 128> buffer;
    string result;
    unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd.c_str(), "r"), pclose);
    if (!pipe) {
        throw runtime_error("popen() failed!");
    }
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    return result;
}

// --- Minimal JSON Parser (LLM Response) ---
// We expect a flat JSON object effectively.
// Format: {"intent": "...", "table": "...", "conditions": [...]}
// This parser is fragile but sufficient for the contract.

string extractString(const string& json, const string& key) {
    string search = "\"" + key + "\":";
    size_t pos = json.find(search);
    if (pos == string::npos) return "";
    
    pos = json.find("\"", pos + search.length()); 
    if (pos == string::npos) return "";
    size_t start = pos + 1;
    size_t end = json.find("\"", start);
    return json.substr(start, end - start);
}

// Helper to find array objects
vector<map<string, string>> extractArrayObjects(const string& json, const string& key) {
    vector<map<string, string>> list;
    string search = "\"" + key + "\":";
    size_t pos = json.find(search);
    if (pos == string::npos) return list;
    
    size_t arrayStart = json.find("[", pos);
    size_t arrayEnd = json.find("]", arrayStart);
    if (arrayStart == string::npos || arrayEnd == string::npos) return list;
    
    string content = json.substr(arrayStart + 1, arrayEnd - arrayStart - 1);
    
    // Naive split by "}"
    size_t objStart = 0;
    while ((objStart = content.find("{", objStart)) != string::npos) {
        size_t objEnd = content.find("}", objStart);
        string objStr = content.substr(objStart, objEnd - objStart + 1);
        
        map<string, string> obj;
        obj["field"] = extractString(objStr, "field");
        obj["op"] = extractString(objStr, "op");
        obj["value"] = extractString(objStr, "value");
        if(obj["field"].empty()) { // maybe it's using different keys or flat?
             // fallback? No, enforce contract.
        }
        list.push_back(obj);
        objStart = objEnd + 1;
    }
    return list;
}

// Helper to extract simple string array ["a", "b"]
vector<string> extractStringArray(const string& json, const string& key) {
    vector<string> list;
    string search = "\"" + key + "\":";
    size_t pos = json.find(search);
    if (pos == string::npos) return list;
    
    size_t arrayStart = json.find("[", pos);
    size_t arrayEnd = json.find("]", arrayStart);
    if (arrayStart == string::npos || arrayEnd == string::npos) return list;
    
    string content = json.substr(arrayStart + 1, arrayEnd - arrayStart - 1);
    
    // Naive split by comma, ignoring quotes
    bool inQuote = false;
    string current;
    for (char c : content) {
        if (c == '"') {
            inQuote = !inQuote;
        } else if (c == ',' && !inQuote) {
             if (!current.empty()) list.push_back(current);
             current = "";
        } else {
             // crude trim
             if (c != ' ' && c != '\n' && c != '\r' && c != '\t') {
                 current += c;
             }
        }
    }
    if (!current.empty()) list.push_back(current);
    
    return list;
}

// Helper to extract top-level array of objects from a JSON string
vector<string> extractJsonObjectArray(const string& json) {
    vector<string> objects;
    size_t arrayStart = json.find("[");
    size_t arrayEnd = json.rfind("]");
    
    if (arrayStart == string::npos || arrayEnd == string::npos) {
        // Not an array, maybe a single object?
        size_t objStart = json.find("{");
        size_t objEnd = json.rfind("}");
        if (objStart != string::npos && objEnd != string::npos) {
             objects.push_back(json.substr(objStart, objEnd - objStart + 1));
        }
        return objects;
    }

    // It is an array, iterate through objects
    // This is a naive parser that assumes objects are balanced and separated by commas
    size_t pos = arrayStart + 1;
    int braceCount = 0;
    size_t objStart = string::npos;
    
    while (pos < arrayEnd) {
        char c = json[pos];
        if (c == '{') {
            if (braceCount == 0) objStart = pos;
            braceCount++;
        } else if (c == '}') {
            braceCount--;
            if (braceCount == 0 && objStart != string::npos) {
                objects.push_back(json.substr(objStart, pos - objStart + 1));
                objStart = string::npos;
            }
        }
        pos++;
    }
    return objects;
}

NLQ::Query parseSingleJSONToQuery(const string& json) {
    NLQ::Query q;
    q.action = extractString(json, "intent");
    q.table = extractString(json, "table");
    
    // Conditions
    auto conds = extractArrayObjects(json, "conditions");
    for (auto& c : conds) {
        NLQ::Condition nc;
        nc.field = c["field"];
        nc.op = c["op"];
        nc.value = c["value"];
        q.conditions.push_back(nc);
    }
    
    // Update fields
    q.updateField = extractString(json, "updateField");
    q.updateValue = extractString(json, "updateValue");
    
    // Create fields
    q.fields = extractStringArray(json, "fields");
    
    // Insert values (often parsed from implicit 'value' or args)
    // NOTE: The original code didn't fully implement 'values' extraction from JSON for 'INSERT'
    // but the prompt returns 'value' or 'values'? 
    // The previous error "Table 'student' not found" suggests the CREATE happen correctly?
    // Wait, the prompt returns `values`? Or `value`?
    // The previous code had `q.values = extractStringArray(json, "values")` ? 
    // Looking at the original file provided in Step 16:
    // It did NOT have q.values extraction! 
    // And `INSERT` uses `q.values`. 
    // I need to fix this too. The Prompt says "value", but usually for INSERT it might be a list?
    // The user query was "insert 1 vaidik 12".
    // I will add extraction for "value" (as array) just in case the LLM puts it there.
    // Or I'll stick to what was there if it was working for single inserts.
    // Wait, previous file content for `parseJSONToQuery` ended at line 191 and DID NOT extract `values`.
    // But `executor.cpp` uses `q.values` in line 38.
    // So `parseJSONToQuery` was ALREADY BUGGY for INSERT if it didn't extract values?
    // Ah, I see `q.action` etc.
    // Let's look at `executor.cpp`: `if (q.values.size() != t->fields.size())`
    // So `q.values` MUST be populated.
    // I will checking the `planner_prompt.txt`... it says "value".
    // I will try to extract "value" as a string, and if it looks like an array, parse it.
    // For now, I'll add `q.values = extractStringArray(json, "value");` assuming the LLM puts `["1", "vaidik", "12"]` in "value".
    
    q.values = extractStringArray(json, "value");
    // Also try "values"
    if (q.values.empty()) {
        q.values = extractStringArray(json, "values");
    }

    return q;
}

vector<NLQ::Query> parseJSONToQueries(const string& json) {
    vector<NLQ::Query> queries;
    vector<string> objStrings = extractJsonObjectArray(json);
    
    for (const string& s : objStrings) {
        queries.push_back(parseSingleJSONToQuery(s));
    }
    return queries;
}

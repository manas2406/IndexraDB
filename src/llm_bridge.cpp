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
    
    // Find first non-whitespace character after the colon
    size_t valStart = pos + search.length();
    while (valStart < json.length() && (json[valStart] == ' ' || json[valStart] == '\t' || json[valStart] == '\n' || json[valStart] == '\r')) {
        valStart++;
    }
    if (valStart >= json.length()) return "";
    
    if (json[valStart] == '"') {
        // String value
        size_t start = valStart + 1;
        size_t end = json.find("\"", start);
        if (end == string::npos) return "";
        return json.substr(start, end - start);
    } else {
        // Non-string value (number, bool, null, etc.)
        size_t end = valStart;
        while (end < json.length() && 
               json[end] != ',' && 
               json[end] != '}' && 
               json[end] != ']' && 
               json[end] != ' ' && 
               json[end] != '\n' && 
               json[end] != '\r' && 
               json[end] != '\t') {
            end++;
        }
        return json.substr(valStart, end - valStart);
    }
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
    
    // Insert values
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

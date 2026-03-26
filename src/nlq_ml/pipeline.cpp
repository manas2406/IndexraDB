#include "../../include/nlq_ml/pipeline.hpp"
#include "../../include/nlq_ml/normalize.hpp"
#include "../../include/nlq_ml/intent.hpp"
#include "../../include/nlq_ml/slots.hpp"
#include "../../include/nlq_ml/embed.hpp"
#include <iostream>
#include <algorithm>
#include <sstream>

using namespace std;

namespace NLQ_ML {

    Query processParams(string userQuery) {
        Query q;
        
        // Phase 1: Normalize
        string cleanText = normalizeText(userQuery);
        // cout << "[DEBUG] Normalized: " << cleanText << "\n";

        // Phase 2: Intent
        IntentModel intentModel;
        q.intent = intentModel.run(cleanText);
        // cout << "[DEBUG] Intent: " << q.intent << "\n";

        // Phase 3: Slots
        SlotModel slotModel;
        vector<Slot> slots = slotModel.runTokens(cleanText, q.intent);
        
        for (const auto& s : slots) {
            // cout << "[DEBUG] Slot: " << s.text << " [" << s.label << "]\n";
            if (s.label == "TABLE") q.table = s.text;
            if (s.label == "FIELD") q.field = s.text;
            if (s.label == "OPERATOR") q.op = s.text;
            if (s.label == "VALUE") q.value = s.text;
            if (s.label == "TIME") q.timeRange = s.text;
        }

        // Phase 4: Embeddings (Template Verification)
        // If confidence is low (e.g. no table extracted), we could use cosine sim 
        // to find nearest valid template. For now, we trust Slots.

        // Phase 5: Semantic Resolution / Heuristics
        
        // Heuristic: If we have a value and operator but no field, assume "price" or "amount"
        if (q.field.empty() && !q.value.empty() && !q.op.empty()) {
            q.field = "price"; // Default guess
        }
        
        // CREATE Handling: Extract fields manually if slots failed (since slots assumes NER logic)
        if (q.intent == "CREATE") {
             // Basic fallback for create fields if not tagged
             // (Slot model currently doesn't tag "val", "name" as fields in CREATE context reliably without training)
             stringstream ss(cleanText);
             string word;
             bool capturing = false;
             while(ss >> word) {
                 if (word == "fields" || word == "with") {
                     capturing = true;
                     continue; 
                 }
                 if (capturing) q.createFields.push_back(word);
             }
        }
        
        // UPDATE Handling
        if (q.intent == "UPDATE") {
            // Heuristic: Split by "where" if present
            // Everything before "where" is SET (updateField, updateValue)
            // Everything after "where" is CONDITION (q.field, q.value)
            
            size_t wherePos = cleanText.find("where");
            if (wherePos != string::npos) {
                // We have a WHERE clause
                // Re-run slots on the condition part to be sure? 
                // Or just use the already extracted slots and assign them based on position?
                // Simpler: Use the text position.
                
                // Let's iterate slots.
                // If a slot's value appears BEFORE 'wherePos', it's update info.
                // If AFTER, it's condition info.
                
                // But Slot struct doesn't have position info stored currently. 
                // (My SlotModel implementation returns vector<Slot> without offset).
                
                // Fallback: Parse distinct words from text parts.
                string setPart = cleanText.substr(0, wherePos);
                string wherePart = cleanText.substr(wherePos + 5);
                
                // Parse setPart for field/value
                {
                     // Try to find field and value in setPart
                     // "update products set price 60"
                     // heuristic: last number is value? word before it is field?
                     stringstream ss(setPart);
                     string w;
                     vector<string> words;
                     while(ss >> w) words.push_back(w);
                     
                     // Walk backwards
                     for (int i=words.size()-1; i>=0; --i) {
                        if (all_of(words[i].begin(), words[i].end(), ::isdigit)) {
                            q.updateValue = words[i];
                            if (i>0) q.updateField = words[i-1];
                            break;
                        }
                     }
                }
                
                // Parse wherePart for field/value
                {
                    // "name is mouse" or "name mouse"
                     stringstream ss(wherePart);
                     string w;
                     vector<string> words;
                     while(ss >> w) words.push_back(w);
                     
                     // Try to find field and value
                     // last word value? 
                     if (!words.empty()) {
                         q.value = words.back();
                         if (words.size() > 1) q.field = words[words.size()-2]; // "is" might be gone or present
                         if (q.field == "is" && words.size() > 2) q.field = words[words.size()-3];
                     }
                }
                
            } else {
                // No WHERE clause? potentially dangerous or user forgot.
                // treat as just SET?
            }
        }
        
        // INSERT Handling
        if (q.intent == "INSERT") {
             stringstream ss(cleanText);
             string word;
             bool capturing = false;
             while(ss >> word) {
                 // Capture values after "values" or after table name
                 if (capturing && word != "values") q.insertValues.push_back(word);
                 if (word == "values" || word == q.table) capturing = true;
             }
         }

        return q;
    }
}

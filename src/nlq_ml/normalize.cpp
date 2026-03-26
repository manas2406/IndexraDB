#include "../../include/nlq_ml/normalize.hpp"
#include <algorithm>
#include <regex>
#include <vector>
#include <iostream>

using namespace std;

namespace NLQ_ML {

    // Helper to replace all occurrences
    string replaceAll(string str, const string& from, const string& to) {
        size_t start_pos = 0;
        while((start_pos = str.find(from, start_pos)) != string::npos) {
            str.replace(start_pos, from.length(), to);
            start_pos += to.length();
        }
        return str;
    }

    string normalizeText(string text) {
        // 1. Lowercase
        transform(text.begin(), text.end(), text.begin(), ::tolower);

        // 2. Remove Punctuation (keep > < = though)
        // regex_replace might be slow, but robust.
        // Let's just remove chars that are not alphanumeric or specific symbols
        // Simplification: just removing common punctuation
        text = replaceAll(text, "?", " ");
        text = replaceAll(text, ".", " ");
        text = replaceAll(text, ",", " ");
        text = replaceAll(text, "!", " ");

        // 3. Remove Filler Words
        vector<string> fillers = {"please", "can you", "could you", "kindly", "i want", "tell me", "give me", "fetch", "show me"}; 
        // "show me" might be useful for intent, but "me" is filler. "Show" is intent.
        // Let's match phrases.
        for (const auto& fill : fillers) {
            text = replaceAll(text, fill, "");
        }
        // Remove stand-alone filler words carefully (avoid partial mathc)
        // For simplicity in C++, just basic string replace for now.
        text = replaceAll(text, " me ", " ");
        text = replaceAll(text, " all ", " ");
        text = replaceAll(text, " the ", " ");
        text = replaceAll(text, " a ", " "); // "create a table" -> "create table"

        // 4. Normalize Numbers & Currency
        text = replaceAll(text, "k ", "000 "); // "2k " -> "2000 "
        if (text.back() == 'k') text.replace(text.length()-1, 1, "000"); // "2k" at end
        
        // Remove currency symbols (assuming bytes, simplistic)
        // Only ASCII for now
        text = replaceAll(text, "$", "");
        // rupee symbol is multibyte, careful. 
        // regex replace for non-ascii?
        
        // 5. Unify Comparators
        text = replaceAll(text, "greater than", ">");
        text = replaceAll(text, "more than", ">");
        text = replaceAll(text, "above", ">");
        text = replaceAll(text, "over", ">");
        
        text = replaceAll(text, "less than", "<");
        text = replaceAll(text, "lower than", "<");
        text = replaceAll(text, "below", "<");
        text = replaceAll(text, "cheaper than", "<"); // cheaper already maps to < logic usually
        text = replaceAll(text, "under", "<");

        text = replaceAll(text, "equal to", "=");
        text = replaceAll(text, "equals", "=");
        text = replaceAll(text, " is ", " = "); // "where price is 20" -> "where price = 20"

        // Collapse strict spaces
        regex space_re("\\s+");
        text = regex_replace(text, space_re, " ");
        if (!text.empty() && text.front() == ' ') text.erase(0, 1);
        if (!text.empty() && text.back() == ' ') text.pop_back();

        return text;
    }
}

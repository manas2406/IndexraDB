#ifndef QUERY_HPP
#define QUERY_HPP

#include <string>
#include <vector>

struct Query {
    std::string intent;          // SELECT, RANGE_FILTER, JOIN, CREATE, INSERT
    std::string table;           // extracted table/entity
    
    // For SELECT/FILTER
    std::string field;           // amount, price, grade, etc.
    std::string op;              // >, <, =, >=, <=
    std::string value;           // "2000", "Lucknow"
    std::string updateField;     // For UPDATE SET
    std::string updateValue;     // For UPDATE SET
    std::string timeRange;       // "7_days", "30_days", "1_day"
    bool useIndex = false;       // route to B+ Tree if available

    // For CREATE
    std::vector<std::string> createFields; 
    
    // For INSERT
    std::vector<std::string> insertValues;
};

#endif

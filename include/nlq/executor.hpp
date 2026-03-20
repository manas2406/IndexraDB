#ifndef NLQ_EXECUTOR_HPP
#define NLQ_EXECUTOR_HPP

#include "intent.hpp"
#include "../../include/database.hpp"

namespace NLQ {
    // Executes the query against the database and prints results.
    void executeQuery(const Query& q, Database& db);
    
    // Executes a list of queries sequentially.
    void executeQueries(const std::vector<Query>& queries, Database& db);
}

#endif

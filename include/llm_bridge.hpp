#ifndef LLM_BRIDGE_HPP
#define LLM_BRIDGE_HPP

#include <string>
#include "../include/database.hpp"
#include "../include/nlq/intent.hpp"

std::string exportSchemaJSON(const Database& db);
std::string httpPost(const std::string& url, const std::string& jsonPayload);
NLQ::Query parseJSONToQuery(const std::string& json);
std::vector<NLQ::Query> parseJSONToQueries(const std::string& json);
std::string escapeJSON(const std::string& s);

#endif

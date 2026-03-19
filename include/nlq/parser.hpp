#ifndef NLQ_PARSER_HPP
#define NLQ_PARSER_HPP

#include "intent.hpp"
#include <string>

namespace NLQ {
    Query parseQuery(std::string input);
}

#endif

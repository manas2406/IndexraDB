#ifndef NLQ_ML_PIPELINE_HPP
#define NLQ_ML_PIPELINE_HPP

#include "../query.hpp"
#include <string>

namespace NLQ_ML {
    // Runs the full ML pipeline to produce a Query struct
    Query processParams(std::string userQuery);
}

#endif

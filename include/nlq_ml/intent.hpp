#ifndef NLQ_INTENT_MODEL_HPP
#define NLQ_INTENT_MODEL_HPP

#include <string>

namespace NLQ_ML {
    class IntentModel {
    public:
        // Returns "SELECT", "CREATE", "INSERT", "JOIN", etc.
        std::string run(const std::string& text);
    };
}

#endif

#ifndef NLQ_SLOT_MODEL_HPP
#define NLQ_SLOT_MODEL_HPP

#include <string>
#include <vector>
#include <map>

namespace NLQ_ML {

    struct Slot {
        std::string text;
        std::string label; // TABLE, FIELD, OPERATOR, VALUE, TIME
    };

    class SlotModel {
    public:
        // Returns tag sequences
        std::vector<Slot> runTokens(const std::string& text, const std::string& intent);
    };
}

#endif

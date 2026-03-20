#ifndef NLQ_TIME_HPP
#define NLQ_TIME_HPP

#include <string>

namespace NLQ {
    // Returns YYYY-MM-DD for current time - days offset
    std::string getOffsetDate(int daysAgo);
}

#endif

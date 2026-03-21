#include "../../include/nlq/time.hpp"
#include <ctime>
#include <iomanip>
#include <sstream>

namespace NLQ {

    std::string getOffsetDate(int daysAgo) {
        time_t t = time(nullptr);
        tm* now = localtime(&t);
        
        // Subtract days
        // Note: mktime handles renormalization (e.g., Oct 1 - 1 day = Sep 30)
        now->tm_mday -= daysAgo;
        mktime(now); 
        
        std::stringstream ss;
        ss << (now->tm_year + 1900) << "-"
           << std::setw(2) << std::setfill('0') << (now->tm_mon + 1) << "-"
           << std::setw(2) << std::setfill('0') << now->tm_mday;
           
        return ss.str();
    }
}

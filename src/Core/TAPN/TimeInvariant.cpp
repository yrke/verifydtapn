#include "Core/TAPN/TimeInvariant.hpp"
#include <sstream>

namespace VerifyTAPN::TAPN {
    const TimeInvariant TimeInvariant::LS_INF;

    TimeInvariant TimeInvariant::createFor(const std::string &invariant, std::map<std::string, int> replace) {
        bool strict = invariant.find("<=") == std::string::npos;
        int bound = std::numeric_limits<int>::max();

        int pos = strict ? 1 : 2;
        std::string number = invariant.substr(pos);

        if (invariant.find("inf") == std::string::npos) {
            if (replace.count(number))
                bound = replace.at(number);
            else
                bound = std::atoi(number.c_str());
        }
        if (bound == std::numeric_limits<int>::max()) return LS_INF;
        else return TimeInvariant(strict, bound);
    }

    void TimeInvariant::print(std::ostream &out) const {
        std::string comparison = strictComparison ? "<" : "<=";
        std::string strBound = bound == std::numeric_limits<int>::max() ? "inf" : std::to_string(bound);

        out << comparison <<  " " << strBound;
    }
}

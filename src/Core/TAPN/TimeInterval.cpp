#include "Core/TAPN/TimeInterval.hpp"
#include <boost/algorithm/string.hpp>
#include <vector>

namespace VerifyTAPN::TAPN {
    using namespace boost::algorithm;

    bool str_contains(const std::string& s, char c) {
        return std::any_of(s.begin(), s.end(), [c](auto a) { return a == c; });
    }

    TimeInterval TimeInterval::createFor(const std::string &interval, const std::map<std::string, int> &replace) {
        bool leftStrict = str_contains(interval, '(');
        bool rightStrict = str_contains(interval, ')');

        std::vector<std::string> splitVector;
        split(splitVector, interval, is_any_of(","));

        std::string strLowerBound = splitVector[0].substr(1);
        std::string strUpperBound = splitVector[1].substr(0, splitVector[1].size() - 1);

        int lowerBound;
        if (replace.count(strLowerBound) > 0)
            lowerBound = replace.at(strLowerBound);
        else
            lowerBound = std::atoi(strLowerBound.c_str());

        int upperBound = std::numeric_limits<int>::max();

        if (!iequals(strUpperBound, "inf")) {
            if (replace.count(strUpperBound) > 0)
                upperBound = replace.at(strUpperBound);
            else
                upperBound = std::atoi(strUpperBound.c_str());
        }

        return TimeInterval(leftStrict, lowerBound, upperBound, rightStrict);
    }

    void TimeInterval::print(std::ostream &out) const {
        std::string leftParenthesis = leftStrict ? "(" : "[";
        std::string rightParenthesis = rightStrict ? ")" : "]";

        auto strLowerBound = std::to_string(lowerBound);
        std::string strUpperBound =
                upperBound == std::numeric_limits<int>::max() ? "inf" : std::to_string(upperBound);

        out << leftParenthesis << strLowerBound << "," << strUpperBound << rightParenthesis;
    }

    void TimeInterval::divideBoundsBy(int divider) {
        if (lowerBound != 0) {
            lowerBound = lowerBound / divider;
        }
        if (upperBound != std::numeric_limits<int>::max()) {
            upperBound = upperBound / divider;
        }
    }
}

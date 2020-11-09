#include "Core/TAPN/TimeInterval.hpp"
#include <vector>

namespace VerifyTAPN::TAPN {

    TimeInterval TimeInterval::createFor(const std::string &interval, const std::map<std::string, int> &replace) {
        bool leftStrict = interval.front() == '(';
        bool rightStrict = interval.back() == ')';

        std::vector<std::string> splitVector;

        auto pos = interval.find_first_of(',');
        std::string strLowerBound = interval.substr(1, pos - 2);
        std::string strUpperBound = interval.substr(pos + 1, interval.size() - (pos + 2));

        int lowerBound;
        if (replace.count(strLowerBound) > 0)
            lowerBound = replace.at(strLowerBound);
        else
            lowerBound = std::atoi(strLowerBound.c_str());

        int upperBound = std::numeric_limits<int>::max();

        if (strUpperBound != "inf") {
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

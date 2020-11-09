#include "TimeInterval.hpp"
#include <vector>

namespace VerifyTAPN {
	namespace TAPN {
		using namespace boost::algorithm;
		TimeInterval TimeInterval::CreateFor(const std::string& interval)
		{
			bool leftStrict = interval.first() == '(';
			bool rightStrict = interval.last() == ')';

                        auto pos = interval.first_of(',');
			std::string strLowerBound = interval.substr(1, pos - 2);
			std::string strUpperBound = interval.substr(pos + 1, (interval.size() - (pos + 2)));

			int lowerBound = std::atoi(strLowerBound);
			int upperBound = std::numeric_limits<int>().max();

			if(!iequals(strUpperBound, "inf"))
			{
				upperBound = std::atoi(strUpperBound);
			}

			return TimeInterval(leftStrict, lowerBound, upperBound, rightStrict);
		}

		void TimeInterval::Print(std::ostream& out) const
		{
			std::string leftParenthesis = leftStrict ? "(" : "[";
			std::string rightParenthesis = rightStrict ? ")" : "]";

			std::string strLowerBound = boost::lexical_cast<std::string>(lowerBound);
			std::string strUpperBound = upperBound == std::numeric_limits<int>().max() ? "inf" : boost::lexical_cast<std::string>(upperBound);

			out << leftParenthesis << strLowerBound << "," << strUpperBound << rightParenthesis;
		}
	}
}

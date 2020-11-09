#include "TimeInterval.hpp"
#include "boost/algorithm/string.hpp"
#include <vector>

namespace VerifyTAPN {
	namespace TAPN {
		using namespace boost::algorithm;
		TimeInterval TimeInterval::CreateFor(const std::string& interval)
		{
			bool leftStrict = boost::algorithm::icontains(interval, "(");
			bool rightStrict = boost::algorithm::icontains(interval,")");

			std::vector<std::string> splitVector;
			split(splitVector, interval, is_any_of(","));

			std::string strLowerBound = splitVector[0].substr(1);
			std::string strUpperBound = splitVector[1].substr(0,splitVector[1].size()-1);

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

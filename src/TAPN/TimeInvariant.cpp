#include "TimeInvariant.hpp"
#include <sstream>

namespace VerifyTAPN {
	namespace TAPN {
		TimeInvariant TimeInvariant::CreateFor(const std::string& invariant)
		{
			bool strict = invariant.find("<=") == std::string::npos;
			int bound = std::numeric_limits<int>().max();

			int pos = strict ? 1 : 2;
			std::string number = invariant.substr(pos);

			if(invariant.contains("inf") == std::string::npos)
			{
				bound = std::atoi(number);
			}

			return TimeInvariant(strict, bound);
		}

		void TimeInvariant::Print(std::ostream& out) const
		{
			std::string comparison = strictComparison ? "<" : "<=";
			std::string strBound = bound == std::numeric_limits<int>().max() ? "inf" : std::to_string(bound);

			out << comparison << " " << strBound;
		}
	}
}

#include "TimeInvariant.hpp"
#include "boost/algorithm/string.hpp"

namespace VerifyTAPN {
	namespace TAPN {
		TimeInvariant TimeInvariant::CreateFor(const std::string& invariant)
		{
			bool strict = !boost::algorithm::icontains(invariant, "<=");
			int bound = std::numeric_limits<int>().max();

			int pos = strict ? 1 : 2;
			std::string number = invariant.substr(pos);

			if(!boost::algorithm::icontains(invariant, "inf"))
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

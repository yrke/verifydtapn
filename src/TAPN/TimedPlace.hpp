
#ifndef VERIFYYAPN_TAPN_TIMEDPLACE_HPP_
#define VERIFYYAPN_TAPN_TIMEDPLACE_HPP_


#include "TimeInvariant.hpp"
#include "TimedInputArc.hpp"
#include "OutputArc.hpp"

#include <string>
#include <vector>
#include <iostream>

namespace VerifyTAPN{
	namespace TAPN{
		class TimedPlace {
		public: // static
			static const TimedPlace& Bottom() {
				static TimedPlace bottom;
				return bottom;
			}

			static const int BottomIndex() {
				return -1;
			}

		public: // typedefs
			typedef std::vector< std::shared_ptr<TimedPlace> > Vector;

		public: // construction / destruction
			TimedPlace(const std::string& name, const std::string& id, const TimeInvariant timeInvariant)
			: name(name), id(id), timeInvariant(timeInvariant), postset(), preset(), index(-2), isInfinityPlace(false) { };
			TimedPlace() : name("*BOTTOM*"), timeInvariant(), postset(), preset(), index(-1), isInfinityPlace(false) { };
			virtual ~TimedPlace() { /* empty */ };

		public: // modifiers
			void AddToPreset(const std::shared_ptr<OutputArc>& arc);
			void AddToPostset(const std::shared_ptr<TimedInputArc>& arc);
			inline void MarkInfinityPlace(bool isInfPlace) { isInfinityPlace = isInfPlace; }
			inline void SetIndex(int i) { index = i; };
			inline void SetMaxConstant(int max) { maxConstant = max; }
		public: // inspection
			const std::string& GetName() const;
			const std::string& GetId() const;
			void Print(std::ostream& out) const;
			inline int GetIndex() const { return index; };
			inline const TimedInputArc::WeakPtrVector& GetPostset() const { return postset; }
			inline const bool IsInfinityPlace() const { return isInfinityPlace; }
			inline const int GetMaxConstant() const { return maxConstant; }

		private: // data
			std::string	name;
			std::string id;
			TimeInvariant timeInvariant;
			TimedInputArc::WeakPtrVector postset;
			OutputArc::WeakPtrVector preset;
			int index;
			bool isInfinityPlace;
			int maxConstant;
		};

		inline std::ostream& operator<<(std::ostream& out, const TimedPlace& place)
		{
			place.Print(out);
			return out;
		}

		// TAPAAL does not allow multiple places with the same name,
		// thus it is enough to use the name to determine equality.
		inline bool operator==(TimedPlace const& a, TimedPlace const& b)
		{
			return a.GetName() == b.GetName();
		}
	}
}
#endif /* VERIFYYAPN_TAPN_TIMEDPLACE_HPP_ */

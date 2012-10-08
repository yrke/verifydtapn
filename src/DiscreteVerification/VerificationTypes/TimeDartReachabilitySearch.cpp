/*
 * NonStrictSearch.cpp
 *
 *  Created on: 26/04/2012
 *      Author: MathiasGS
 */

#include "TimeDartReachabilitySearch.hpp"

namespace VerifyTAPN {
namespace DiscreteVerification {

TimeDartReachabilitySearch::TimeDartReachabilitySearch(boost::shared_ptr<TAPN::TimedArcPetriNet>& tapn, NonStrictMarking& initialMarking, AST::Query* query, VerificationOptions options, WaitingList<TimeDart>* waiting_list)
	: pwList(waiting_list), tapn(tapn), initialMarking(initialMarking), query(query), options(options), successorGenerator( *tapn.get() ), allwaysEnabled(), exploredMarkings(0){

	//Find the transitions which don't have input arcs
	for(TimedTransition::Vector::const_iterator iter = tapn->GetTransitions().begin(); iter != tapn->GetTransitions().end(); iter++){
		if((*iter)->GetPreset().size() + (*iter)->GetTransportArcs().size() == 0){
			allwaysEnabled.push_back(iter->get());
		}
	}
}

bool TimeDartReachabilitySearch::Verify(){
	if(addToPW(&initialMarking, 0, INT_MAX)){
		return true;
	}

	//Main loop
	while(pwList.HasWaitingStates()){
		TimeDart& dart = *pwList.GetNextUnexplored();
		exploredMarkings++;

#ifdef DEBUG
		std::cout << "-----------------------------------------------------------------------------\n";
		std::cout << "Marking: " << *(dart.getBase()) << " waiting: " << dart.getWaiting() << " passed: " << dart.getPassed() << std::endl;
#endif

		int passed = dart.getPassed();
		dart.setPassed(dart.getWaiting());
		vector<const TimedTransition*> transitions = getTransitions(dart.getBase());
		for(vector<const TimedTransition*>::const_iterator transition = transitions.begin(); transition != transitions.end(); transition++){
			pair<int,int> calculatedStart = calculateStart(*(*transition), dart.getBase());
			if(calculatedStart.first == -1){	// Transition cannot be enabled in marking
				continue;
			}
			int start = max(dart.getWaiting(), calculatedStart.first);
			int end = min(passed-1, calculatedStart.second);
#ifdef DEBUG
			std::cout << "New end: " << calculatedStart.second << " old end: " << calculateEnd(*(*transition), dart.getBase());
#endif
			if(start <= end){
				if((*transition)->GetPostset().size() == 0){
					NonStrictMarking Mpp(*dart.getBase());
					Mpp.incrementAge(start);
					vector<NonStrictMarking*> next = getPossibleNextMarkings(Mpp, *(*transition));
					for(vector<NonStrictMarking*>::iterator it = next.begin(); it != next.end(); it++){
						if(addToPW(*it, start, INT_MAX)){
							return true;
						}
					}
				}else{
					int stop = min(max(start, calculateStop(*(*transition), dart.getBase())), end);
					for(int n = start; n <= stop; n++){
						NonStrictMarking Mpp(*dart.getBase());
						Mpp.incrementAge(n);
						Mpp.cut();
						NonStrictMarking* Mppp = new NonStrictMarking(Mpp);
						if(pwList.Add(tapn.get(), Mppp, 0, INT_MAX, true)){
							break;
						}
						vector<NonStrictMarking*> next = getPossibleNextMarkings(Mpp, **transition);
						for(vector<NonStrictMarking*>::iterator it = next.begin(); it != next.end(); it++){
							if(addToPW(*it, 0, INT_MAX)){
								return true;
							}
						}
					}
				}
			}
		}
	}

	return false;
}

bool TimeDartReachabilitySearch::isDelayPossible(NonStrictMarking& marking){
	PlaceList& places = marking.places;
	if(places.size() == 0) return true;	//Delay always possible in empty markings

	PlaceList::const_iterator markedPlace_iter = places.begin();
	for(TAPN::TimedPlace::Vector::const_iterator place_iter = tapn->GetPlaces().begin(); place_iter != tapn->GetPlaces().end(); place_iter++){
		int inv = place_iter->get()->GetInvariant().GetBound();
		if(*(place_iter->get()) == *(markedPlace_iter->place)){
			if(markedPlace_iter->MaxTokenAge() > inv-1){
				return false;
			}

			markedPlace_iter++;

			if(markedPlace_iter == places.end())	return true;
		}
	}
	assert(false);	// This happens if there are markings on places not in the TAPN
	return false;
}

vector<NonStrictMarking*> TimeDartReachabilitySearch::getPossibleNextMarkings(NonStrictMarking& marking, const TimedTransition& transition){
	return successorGenerator.generateSuccessors(marking, transition);
}

bool TimeDartReachabilitySearch::addToPW(NonStrictMarking* marking, int w, int p){
	marking->cut();

	unsigned int size = marking->size();

	pwList.SetMaxNumTokensIfGreater(size);

	if(size > options.GetKBound()) {
		return false;
	}

	if(pwList.Add(tapn.get(), marking, w, p)){
		QueryVisitor checker(*marking);
		boost::any context;
		query->Accept(checker, context);
		if(boost::any_cast<bool>(context)) {
			lastMarking = marking;
			return true;
		} else {
			return false;
		}
	}

	return false;
}

bool compare( const TimedTransition* lx, const TimedTransition* rx ) {
	return lx->GetIndex() < rx->GetIndex();
}

vector<const TimedTransition*> TimeDartReachabilitySearch::getTransitions(NonStrictMarking* marking){
	vector<const TimedTransition*> transitions;

	// TODO nicer?
	for(TimedTransition::Vector::const_iterator iter = tapn->GetTransitions().begin(); iter != tapn->GetTransitions().end(); iter++){
		transitions.push_back(iter->get());
	}

	return transitions;

	// Go through places
	for(vector<Place>::const_iterator place_iter = marking->places.begin(); place_iter != marking->places.end(); place_iter++){
		// Normal arcs
		for(TAPN::TimedInputArc::WeakPtrVector::const_iterator arc_iter = place_iter->place->GetInputArcs().begin(); arc_iter != place_iter->place->GetInputArcs().end(); arc_iter++){
			transitions.push_back(&arc_iter->lock()->OutputTransition());
		}
		// Transport arcs
		for(TAPN::TransportArc::WeakPtrVector::const_iterator arc_iter = place_iter->place->GetTransportArcs().begin(); arc_iter != place_iter->place->GetTransportArcs().end(); arc_iter++){
			transitions.push_back(&arc_iter->lock()->Transition());
		}
	}

	std::sort(transitions.begin(), transitions.end(), compare);
	transitions.erase(std::unique(transitions.begin(), transitions.end()), transitions.end());

	transitions.insert(transitions.end(), allwaysEnabled.begin(), allwaysEnabled.end());

	return transitions;
}

pair<int,int> TimeDartReachabilitySearch::calculateStart(const TimedTransition& transition, NonStrictMarking* marking){
	vector<Util::interval > start;
	Util::interval initial(0, INT_MAX);
	start.push_back(initial);

	if(transition.NumberOfInputArcs() + transition.NumberOfTransportArcs() == 0){ //always enabled
		pair<int, int> p(0, INT_MAX);
		return p;
	}

	for(TAPN::TimedInputArc::WeakPtrVector::const_iterator arc = transition.GetPreset().begin(); arc != transition.GetPreset().end(); arc++){
		vector<Util::interval > intervals;
		int range;
		if(arc->lock()->Interval().GetUpperBound() == INT_MAX){
			range = INT_MAX;
		}else{
			range = arc->lock()->Interval().GetUpperBound()-arc->lock()->Interval().GetLowerBound();
		}
		int weight = arc->lock()->GetWeight();

		TokenList tokens = marking->GetTokenList(arc->lock()->InputPlace().GetIndex());
		if(tokens.size() == 0){
			pair<int, int> p(-1, -1);
			return p;
		}

		unsigned int j = 0;
		int numberOfTokensAvailable = tokens.at(j).getCount();
		for(unsigned int  i = 0; i < tokens.size(); i++){
			if(numberOfTokensAvailable < weight){
				for(j=max(i,j); j < tokens.size() && numberOfTokensAvailable < weight; j++){
					numberOfTokensAvailable += tokens.at(j).getCount();
				}
				j--;
			}
			if(numberOfTokensAvailable >= weight && tokens.at(j).getAge() - tokens.at(i).getAge() <= range){ //This span is interesting
				int low = arc->lock()->Interval().GetLowerBound() - tokens.at(i).getAge();
				int heigh = arc->lock()->Interval().GetUpperBound() - tokens.at(j).getAge();

				Util::interval element(low < 0 ? 0 : low,
						arc->lock()->Interval().GetUpperBound() == INT_MAX ? INT_MAX : heigh);
				Util::set_add(intervals, element);
			}
			numberOfTokensAvailable -= tokens.at(i).getCount();
		}

		start = Util::setIntersection(start, intervals);
	}

	// Transport arcs
	for(TAPN::TransportArc::WeakPtrVector::const_iterator arc = transition.GetTransportArcs().begin(); arc != transition.GetTransportArcs().end(); arc++){
			vector<Util::interval > intervals;
			int range;
			if(arc->lock()->Interval().GetUpperBound() == INT_MAX){
				range = INT_MAX;
			}else{
				range = arc->lock()->Interval().GetUpperBound()-arc->lock()->Interval().GetLowerBound();
			}
			int weight = arc->lock()->GetWeight();

			TokenList tokens = marking->GetTokenList(arc->lock()->Source().GetIndex());

			if(tokens.size() == 0){
				pair<int, int> p(-1, -1);
				return p;
			}

			unsigned int j = 0;
			int numberOfTokensAvailable = tokens.at(j).getCount();
			for(unsigned int  i = 0; i < tokens.size(); i++){
				if(numberOfTokensAvailable < weight){
					for(j=max(i,j); j < tokens.size() && numberOfTokensAvailable < weight; j++){
						numberOfTokensAvailable += tokens.at(j).getCount();
					}
					j--;
				}
				if(numberOfTokensAvailable >= weight && tokens.at(j).getAge() - tokens.at(i).getAge() <= range){ //This span is interesting
					Util::interval element(arc->lock()->Interval().GetLowerBound() - tokens.at(i).getAge(),
							arc->lock()->Interval().GetUpperBound() - tokens.at(j).getAge());
					Util::set_add(intervals, element);
				}
				numberOfTokensAvailable -= tokens.at(i).getCount();
			}

			start = Util::setIntersection(start, intervals);
		}

		int invariantPart = INT_MAX;

		for(PlaceList::const_iterator iter = marking->GetPlaceList().begin(); iter != marking->GetPlaceList().end(); iter++){
			if(iter->place->GetInvariant().GetBound() != std::numeric_limits<int>::max() && iter->place->GetInvariant().GetBound()-iter->tokens.back().getAge() < invariantPart){
				invariantPart = iter->place->GetInvariant().GetBound()-iter->tokens.back().getAge();
			}
		}

		vector<Util::interval > invEnd;
		Util::interval initialInv(0, invariantPart);
		invEnd.push_back(initialInv);
		start = Util::setIntersection(start, invEnd);

#if DEBUG
		std::cout << "Intervals in start: " << start.size() << std::endl;
#endif

	if(start.empty()){
		pair<int, int> p(-1,-1);
		return p;
	}else{
		pair<int, int> p(start.front().lower(), start.back().upper());
		return p;
	}
}

int TimeDartReachabilitySearch::calculateEnd(const TimedTransition& transition, NonStrictMarking* marking){

	int part1 = 0;

	PlaceList placeList = marking->GetPlaceList();

	for(PlaceList::const_iterator place_iter = placeList.begin(); place_iter != placeList.end(); place_iter++){
		//The smallest age is the first as the tokens is sorted
		int maxDelay = place_iter->place->GetMaxConstant() - place_iter->tokens.at(0).getAge();
		if(maxDelay > part1){
			part1 = maxDelay;
		}
	}

	//should be maxconstant + 1
	part1++;

	int part2 = INT_MAX;

	for(PlaceList::const_iterator iter = marking->GetPlaceList().begin(); iter != marking->GetPlaceList().end(); iter++){
		if(iter->place->GetInvariant().GetBound() != std::numeric_limits<int>::max() && iter->place->GetInvariant().GetBound()-iter->tokens.back().getAge() < part2){
			part2 = iter->place->GetInvariant().GetBound()-iter->tokens.back().getAge();
		}
	}

	return min(part1, part2);
}

int TimeDartReachabilitySearch::calculateStop(const TimedTransition& transition, NonStrictMarking* marking){
	int value = INT_MAX;
	int MC = 0;

	unsigned int i = 0;
	for(PlaceList::const_iterator iter = marking->GetPlaceList().begin(); iter != marking->GetPlaceList().end(); iter++){
		if(i < transition.GetPreset().size()){	// TODO make this a little nicer
			while(i < transition.GetPreset().size()){
				if(transition.GetPreset().at(i).lock()->InputPlace().GetIndex() > iter->place->GetIndex() ||
				(transition.GetPreset().at(i).lock()->InputPlace().GetIndex() == iter->place->GetIndex() && transition.GetPreset().at(i).lock()->GetWeight() < iter->NumberOfTokens())){
					value = min(value, iter->tokens.front().getAge());
					MC = max(MC, iter->place->GetMaxConstant() - iter->tokens.front().getAge());
					break;
				}
				i++;
			}
		}else{
			value = min(value, iter->tokens.front().getAge());
			MC = max(MC, iter->place->GetMaxConstant() - iter->tokens.front().getAge());
		}
	}

	return MC+1-value;
}

void TimeDartReachabilitySearch::printStats(){
	std::cout << "  discovered markings:\t" << pwList.discoveredMarkings << std::endl;
	std::cout << "  explored markings:\t" << exploredMarkings << std::endl;
	std::cout << "  stored markings:\t" << pwList.Size() << std::endl;
}

TimeDartReachabilitySearch::~TimeDartReachabilitySearch() {
}

} /* namespace DiscreteVerification */
} /* namespace VerifyTAPN */

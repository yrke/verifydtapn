/*
 * NonStrictSearch.cpp
 *
 *  Created on: 26/04/2012
 *      Author: MathiasGS
 */

#include "TimeDartReachabilitySearch.hpp"

namespace VerifyTAPN {
namespace DiscreteVerification {

bool TimeDartReachabilitySearch::Verify(){
	// no need to check if trace is set, just as expensive as setting the generated by
	initialMarking.SetGeneratedBy(NULL);

	if(addToPW(&initialMarking, NULL, INT_MAX)){
		return true;
	}

	//Main loop
	while(pwList->HasWaitingStates()){
		TimeDartBase& dart = *pwList->GetNextUnexplored();
		exploredMarkings++;

		int passed = dart.getPassed();
		dart.setPassed(dart.getWaiting());
		tapn->GetTransitions();
		for(TimedTransition::Vector::const_iterator transition_iter = tapn->GetTransitions().begin();
				transition_iter != tapn->GetTransitions().end(); transition_iter++){
			TimedTransition& transition = **transition_iter;
			pair<int,int> calculatedStart = calculateStart(transition, dart.getBase());
			if(calculatedStart.first == -1){	// Transition cannot be enabled in marking
				continue;
			}
			int start = max(dart.getWaiting(), calculatedStart.first);
			int end = min(passed-1, calculatedStart.second);
			if(start <= end){

				if(transition.hasUntimedPostset()){
					NonStrictMarkingBase Mpp(*dart.getBase());
					Mpp.incrementAge(start);
					vector<NonStrictMarkingBase*> next = getPossibleNextMarkings(Mpp, transition);
					for(vector<NonStrictMarkingBase*>::iterator it = next.begin(); it != next.end(); it++){
                                                TraceDart* traceD = NULL;
                                                if(options.GetTrace() == SOME){
                                                    traceD = ((ReachabilityTraceableDart*)&dart)->trace;
                                                    (*it)->SetGeneratedBy(&transition);
                                                }
						if(addToPW(*it, traceD, start)){
							return true;
						}
					}
				}else{
					int stop = min(max(start, calculateStop(transition, dart.getBase())), end);
					for(int n = start; n <= stop; n++){
						NonStrictMarkingBase Mpp(*dart.getBase());
						Mpp.incrementAge(n);

						vector<NonStrictMarkingBase*> next = getPossibleNextMarkings(Mpp, transition);
						for(vector<NonStrictMarkingBase*>::iterator it = next.begin(); it != next.end(); it++){
                                                    TraceDart* traceD = NULL;
                                                    if(options.GetTrace() == SOME){
                                                        traceD = ((ReachabilityTraceableDart*)&dart)->trace;
                                                        (*it)->SetGeneratedBy(&transition);
                                                    }
                                                        if(addToPW(*it, traceD, n)){
								return true;
							}
						}
					}
				}
			}
		}
                deleteBase(dart.getBase());
	}

	return false;
}

bool TimeDartReachabilitySearch::addToPW(NonStrictMarkingBase* marking, WaitingDart* parent, int upper){
        int start;
        if(options.GetTrace() == SOME){
            start = marking->getYoungest();
        }
	marking->cut();

	unsigned int size = marking->size();

	pwList->SetMaxNumTokensIfGreater(size);

	if(size > options.GetKBound()) {
                delete marking;
		return false;
	}
        int youngest = marking->makeBase(tapn.get());
                                //int youngest, WaitingDart* parent, int upper
	if(pwList->Add(tapn.get(), marking, youngest, parent, upper, start)){
		QueryVisitor<NonStrictMarkingBase> checker(*marking);
		boost::any context;
		query->Accept(checker, context);
		if(boost::any_cast<bool>(context)) {
                        if (options.GetTrace()) {
                            lastMarking = pwList->GetLast();
                        }
			return true;
		} else {
                        deleteBase(marking);
			return false;
		}
	}
        deleteBase(marking);
	return false;
}

void TimeDartReachabilitySearch::printStats(){
	std::cout << "  discovered markings:\t" << pwList->discoveredMarkings << std::endl;
	std::cout << "  explored markings:\t" << exploredMarkings << std::endl;
	std::cout << "  stored markings:\t" << pwList->Size() << std::endl;
}

TimeDartReachabilitySearch::~TimeDartReachabilitySearch() {
}

} /* namespace DiscreteVerification */
} /* namespace VerifyTAPN */

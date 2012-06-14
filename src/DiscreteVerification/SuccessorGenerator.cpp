/*
 * SuccessorGenerator.cpp
 *
 *  Created on: 22/03/2012
 *      Author: MathiasGS
 */

#include "SuccessorGenerator.hpp"

namespace VerifyTAPN {
namespace DiscreteVerification {

SuccessorGenerator::~SuccessorGenerator(){

}

SuccessorGenerator::SuccessorGenerator(TAPN::TimedArcPetriNet& tapn)  : tapn(tapn), allwaysEnabled(){
	//Find the transitions which don't have input arcs
	for(TimedTransition::Vector::const_iterator iter = tapn.GetTransitions().begin(); iter != tapn.GetTransitions().end(); iter++){
		if((*iter)->GetPreset().size() + (*iter)->GetTransportArcs().size() == 0){
			allwaysEnabled.push_back(iter->get());
		}
	}
}

vector< NonStrictMarking > SuccessorGenerator::generateSuccessors(const NonStrictMarking& marking) const{

#if DEBUG
	std::cout << "------------------------------- SuccessorGenerator ---------------------------" << std::endl;
#endif
	vector< NonStrictMarking > result;
	ArcHashMap enabledArcs(tapn.GetInhibitorArcs().size() + tapn.GetInputArcs().size() + tapn.GetTransportArcs().size());
	std::vector<unsigned int> enabledTransitionArcs(tapn.GetTransitions().size(), 0);
	std::vector<const TAPN::TimedTransition* > enabledTransitions;

	for(PlaceList::const_iterator iter = marking.places.begin(); iter < marking.places.end(); iter++){
		for(TAPN::TimedInputArc::WeakPtrVector::const_iterator arc_iter = iter->place->GetInputArcs().begin();
				arc_iter != iter->place->GetInputArcs().end(); arc_iter++){
			processArc(enabledArcs, enabledTransitionArcs, enabledTransitions,
					*iter, arc_iter->lock()->Interval(), arc_iter->lock().get(), arc_iter->lock()->OutputTransition());
		}

		for(TAPN::TransportArc::WeakPtrVector::const_iterator arc_iter = iter->place->GetTransportArcs().begin();
				arc_iter != iter->place->GetTransportArcs().end(); arc_iter++){
			processArc(enabledArcs, enabledTransitionArcs, enabledTransitions,
					*iter, arc_iter->lock()->Interval(), arc_iter->lock().get(),
					arc_iter->lock()->Transition(), arc_iter->lock()->Destination().GetInvariant().GetBound());
		}

		for(TAPN::InhibitorArc::WeakPtrVector::const_iterator arc_iter = iter->place->GetInhibitorArcs().begin();
				arc_iter != iter->place->GetInhibitorArcs().end(); arc_iter++){
			TimeInterval t(false, 0, std::numeric_limits<int>().max(), true);
			processArc(enabledArcs, enabledTransitionArcs, enabledTransitions,
					*iter, t, arc_iter->lock().get(), arc_iter->lock()->OutputTransition(), std::numeric_limits<int>().max(), true);
		}
	}

	enabledTransitions.insert(enabledTransitions.end(), allwaysEnabled.begin(), allwaysEnabled.end());
	generateMarkings(result, marking, enabledTransitions, enabledArcs);
#if DEBUG
	std::cout << "------------------------------- SuccessorGenerator done---------------------------" << std::endl;
#endif
	return result;
}

void SuccessorGenerator::processArc(
		ArcHashMap& enabledArcs,
		std::vector<unsigned int>& enabledTransitionArcs,
		std::vector<const TAPN::TimedTransition* >& enabledTransitions,
		const Place& place,
		const TAPN::TimeInterval& interval,
		const void* arcAddress,
		const TimedTransition& transition,
		int bound,
		bool isInhib
) const{
	bool arcIsEnabled = false;
#if DEBUG
	std::cout << "Bound: " << bound << std::endl;
#endif
	for(TokenList::const_iterator token_iter = place.tokens.begin(); token_iter != place.tokens.end(); token_iter++){
		if(interval.GetLowerBound() <= token_iter->getAge() && token_iter->getAge() <= interval.GetUpperBound() && token_iter->getAge() <= bound){
			enabledArcs[arcAddress].push_back(*token_iter);
			arcIsEnabled = true;
#if DEBUG
			std::cout << "Arc enabled!" << std::endl;
#endif
		}
	}
	if(arcIsEnabled && !isInhib){
		enabledTransitionArcs[transition.GetIndex()]++;
	}
	if(enabledTransitionArcs[transition.GetIndex()] == transition.GetPreset().size() + transition.GetTransportArcs().size() && !isInhib){
#if DEBUG
		std::cout << "Transition pushed: " << transition << std::endl;
#endif
		enabledTransitions.push_back(&transition);
	}
}


TokenList SuccessorGenerator::getPlaceFromMarking(const NonStrictMarking& marking, int placeID) const{
	for(PlaceList::const_iterator iter = marking.GetPlaceList().begin();
			iter != marking.GetPlaceList().end();
			iter++){
		if(iter->place->GetIndex() == placeID) return iter->tokens;
	}
	return TokenList();
}

void SuccessorGenerator::generateMarkings(vector<NonStrictMarking>& result, const NonStrictMarking& init_marking,
		const std::vector< const TimedTransition* >& transitions, ArcHashMap& enabledArcs) const {

	for(std::vector< const TimedTransition* >::const_iterator iter = transitions.begin(); iter != transitions.end(); iter++){
		bool inhibited = false;
		//Check that no inhibitors is enabled;
#if DEBUG
		std::cout << "Transition: " << *(*iter) << "Number of inhibitors: " << iter->GetInhibitorArcs().size() << std::endl;
#endif

		for(TAPN::InhibitorArc::WeakPtrVector::const_iterator inhib_iter = (*iter)->GetInhibitorArcs().begin(); inhib_iter != (*iter)->GetInhibitorArcs().end(); inhib_iter++){
#if DEBUG
			std::cout << "Inhibitor: " << *(inhib_iter->lock()) << " Inhibitor size: " << enabledArcs[inhib_iter->lock().get()].size() << std::endl;
#endif
			// Maybe this could be done more efficiently using ArcHashMap? Dunno exactly how it works
			if(init_marking.NumberOfTokensInPlace(inhib_iter->lock().get()->InputPlace().GetIndex()) >= inhib_iter->lock().get()->GetWeight()){
				inhibited = true;
				break;
			}
		}
#if DEBUG
		std::cout << "No inhibitors activated" << std::endl;
#endif
		if (inhibited) continue;
		NonStrictMarking m(init_marking);
		m.SetGeneratedBy(*iter);
		recursiveGenerateMarking(result, m, *(*iter), enabledArcs, 0);
	}
}


void SuccessorGenerator::recursiveGenerateMarking(vector<NonStrictMarking>& result, NonStrictMarking& init_marking, const TimedTransition& transition, ArcHashMap& enabledArcs, unsigned int index) const{
#if DEBUG
	std::cout << "Transition: " << transition << " Num of input arcs: " << transition.GetPreset().size() << " Num of transport arcs: " << transition.GetTransportArcs().size() << std::endl;
#endif

	// Initialize vectors
	ArcAndTokensVector indicesOfCurrentPermutation;
	for(TimedInputArc::WeakPtrVector::const_iterator iter = transition.GetPreset().begin(); iter != transition.GetPreset().end(); iter++){
		InputArcAndTokens* arcAndTokens = new InputArcAndTokens(*iter, enabledArcs[iter->lock().get()]);
		indicesOfCurrentPermutation.push_back(arcAndTokens);
	}
	// Transport arcs
	for(TransportArc::WeakPtrVector::const_iterator iter = transition.GetTransportArcs().begin(); iter != transition.GetTransportArcs().end(); iter++){
		TransportArcAndTokens* arcAndTokens = new TransportArcAndTokens(*iter, enabledArcs[iter->lock().get()]);
		indicesOfCurrentPermutation.push_back(arcAndTokens);
	}

	// Generate permutations
	bool changedSomething = true;
	while(changedSomething){
		changedSomething = false;
		addMarking(result, init_marking, transition, indicesOfCurrentPermutation);

		//Loop through arc indexes from the back
		for(int arcAndTokenIndex = indicesOfCurrentPermutation.size()-1; arcAndTokenIndex >= 0; arcAndTokenIndex--){
			TokenList& enabledTokens = indicesOfCurrentPermutation.at(arcAndTokenIndex).enabledBy;
			vector<unsigned int >& modificationVector = indicesOfCurrentPermutation.at(arcAndTokenIndex).modificationVector;
			if(incrementModificationVector(modificationVector, enabledTokens)){
				//Reset following vectors
				for(arcAndTokenIndex++; arcAndTokenIndex < indicesOfCurrentPermutation.size(); arcAndTokenIndex++){
					indicesOfCurrentPermutation.at(arcAndTokenIndex).reset();
				}
				changedSomething = true;
			}
		}
	}
}

bool SuccessorGenerator::incrementModificationVector(vector<unsigned int >& modificationVector, TokenList& enabledTokens) const{
	int numOfTokenIndices = enabledTokens.size();
	int currentRemaining = enabledTokens.at(modificationVector.back()).getCount();

	//Loop through modification vector from the back
	for(int i = modificationVector.size()-1; i >= 0; i--){
		if(modificationVector.at(i) < numOfTokenIndices-1){
			modificationVector.at(i)++;
			return true;
		}else{
			i--;
			currentRemaining--;
			if(i < 0) break;
			if(currentRemaining == 0){
				currentRemaining = enabledTokens.at(modificationVector.at(i)).getCount();
			}
		}
	}
	return false;
}

void SuccessorGenerator::addMarking(vector<NonStrictMarking >& result, NonStrictMarking& init_marking, const TimedTransition& transition, ArcAndTokensVector& indicesOfCurrentPermutation) const{
	NonStrictMarking m(init_marking);
	for(ArcAndTokensVector::iterator iter = indicesOfCurrentPermutation.begin(); iter != indicesOfCurrentPermutation.end(); iter++){
		vector<unsigned int>& tokens = iter->modificationVector;

		for(vector< unsigned int >::const_iterator tokenIter = tokens.begin(); tokenIter != tokens.end(); tokenIter++){
			Token t((iter->enabledBy)[*tokenIter].getAge(), 1);
			iter->moveToken(t, m);
		}
	}

	for(OutputArc::WeakPtrVector::const_iterator postsetIter = transition.GetPostset().begin(); postsetIter != transition.GetPostset().end(); postsetIter++){
		Token t(0, postsetIter->lock()->GetWeight());
		m.AddTokenInPlace(postsetIter->lock()->OutputPlace(), t);
	}

	result.push_back(m);
}

void SuccessorGenerator::generatePermultations(vector< NonStrictMarking >& result, NonStrictMarking& init_marking, int placeID, TokenList enabledBy, int tokenToProcess, int remainingToRemove, TimedPlace* destinationPlace) const{
	if(remainingToRemove == 0){
		result.push_back(init_marking);
		return;
	} else if (tokenToProcess >= enabledBy.size()){
		return;
	}

	bool breakouter = false;
	int age = enabledBy[tokenToProcess].getAge();
	for(int i = 0; i <= remainingToRemove; i++){
		NonStrictMarking marking(init_marking);
		for(int j = 0; j < i; j++){
			if(!marking.RemoveToken(placeID, age)){
				breakouter = true;
				break;
			}

			if (destinationPlace) {
			    // Input arc is a transport arc
				Token t(age, 1);
				marking.AddTokenInPlace(*destinationPlace, t);
			}
		}
		if(breakouter) break;
		generatePermultations(result, marking, placeID, enabledBy, tokenToProcess+1, remainingToRemove-i, destinationPlace);
	}
}


} /* namespace DiscreteVerification */
} /* namespace VerifyTAPN */

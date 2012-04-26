/*
 * DiscreteVerification.cpp
 *
 *  Created on: 23 Feb 2012
 *      Author: jakob
 */

#include <iostream>
#include "DiscreteVerification.hpp"
#include "../Core/TAPN/TAPN.hpp"
#include "boost/smart_ptr.hpp"
#include "../Core/QueryParser/AST.hpp"
#include "NonStrictMarking.hpp"
#include "NonStrictDFS.hpp"
#include "NonStrictBFS.hpp"


namespace VerifyTAPN {

namespace DiscreteVerification {

DiscreteVerification::DiscreteVerification() {
	// TODO Auto-generated constructor stub

}

DiscreteVerification::~DiscreteVerification() {
	// TODO Auto-generated destructor stub
}

int DiscreteVerification::run(boost::shared_ptr<TAPN::TimedArcPetriNet>& tapn, std::vector<int> initialPlacement, AST::Query* query, VerificationOptions options){
	if(!(*tapn).IsNonStrict()){
		std::cout << "The supplied network contains strict intervals." << std::endl;
		return -1;
	}

	NonStrictMarking* initialMarking = new NonStrictMarking(*tapn, initialPlacement);

	std::cout << "MC: " << tapn->MaxConstant() << std::endl;
	std::cout << "initialMarking: " << *initialMarking << std::endl;
	std::cout << "size: " << initialMarking->size() << std::endl;
	std::cout << "hash: " << boost::hash_value(initialMarking) << std::endl;

	if(initialMarking->size() > options.GetKBound())
	{
		std::cout << "The specified k-bound is less than the number of tokens in the initial markings.";
		return 1;
	}

	std::cout << options << std::endl;
	// Select search strategy
	NonStrictSearch* strategy;
	if(query->GetQuantifier() == EG || query->GetQuantifier() == AF){
		//Liveness query, force DFS
		strategy = new NonStrictDFS(tapn, *initialMarking, query, options);
	}else{
		switch(options.GetSearchType()){
		case DEPTHFIRST:
			strategy = new NonStrictDFS(tapn, *initialMarking, query, options);
			break;
		case BREADTHFIRST:
		case RANDOM:
		case COVERMOST:
			strategy = new NonStrictBFS(tapn, *initialMarking, query, options);
			break;
		}
	}

	bool result = (query->GetQuantifier() == AG || query->GetQuantifier() == AF)? !strategy->Verify() : strategy->Verify();

	if(query->GetQuantifier() == EG || query->GetQuantifier() == AF){
		std::cout << "Trace (length = "<< strategy->trace.size() <<"): " << std::endl;
		while(!strategy->trace.empty()){
			std::cout << *strategy->trace.top() << std::endl;
			strategy->trace.pop();
		}
	}

	//std::cout << strategy->GetStats() << std::endl;
	std::cout << "Query is " << (result ? "satisfied" : "NOT satisfied") << "." << std::endl;
	/*std::cout << "Max number of tokens found in any reachable marking: ";
	if(strategy->MaxUsedTokens() == options.GetKBound() + 1)
		std::cout << ">" << options.GetKBound() << std::endl;
	else
		std::cout << strategy->MaxUsedTokens() << std::endl;

	try{
		strategy->PrintTraceIfAny(result);
	}catch(const trace_exception& e){
		std::cout << "There was an error generating a trace. This is a bug. Please report this on launchpad and attach your TAPN model and this error message: ";
		std::cout << e.what() << std::endl;
		return 1;
	}*/

	delete strategy;

	return 0;
}

}

}

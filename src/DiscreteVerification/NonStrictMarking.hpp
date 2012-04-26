/*
 * NonStrictMarking.hpp
 *
 *  Created on: 29/02/2012
 *      Author: MathiasGS
 */

#ifndef NONSTRICTMARKING_HPP_
#define NONSTRICTMARKING_HPP_

#include <assert.h>
#include <vector>
#include "boost/functional/hash.hpp"
#include "NonStrictMarking.hpp"
#include <iostream>
#include "../Core/TAPN/TAPN.hpp"

using namespace std;

namespace VerifyTAPN {
namespace DiscreteVerification {

class Place;
class Token;
typedef vector<Token> TokenList;

class Token {
private:
	int age;
	int count;
public:
	Token(int age, int count) : age(age), count(count) {  };
	Token(const Token& t) : age(t.age), count(t.count) {  };

	bool equals(const Token &t) const { return (this->age == t.age && this->count == t.count); };

	void add(int num){ count = count + num; };
	int getCount() const { return count; };
	int getAge() const { return age; };
	void setAge(int i) { age = i; };
	void setCount(int i) { count = i; };
	void remove(int num){ count = count - num; };

	// Ages all tokens by 1
	inline void incrementAge(){
		age++;
	}
	inline void decrementAge(){
		age--;
	}

	friend std::size_t hash_value(Token const& t)
	{
		size_t seed = 0;
		boost::hash_combine(seed, t.getAge());
		boost::hash_combine(seed, t.getCount());
		return seed;
	}
};

class Place {
public:
	const TAPN::TimedPlace* place;
	TokenList tokens;

	Place(const TAPN::TimedPlace* place) : place(place){};
	Place(const Place& p) : place(p.place){
		for(TokenList::const_iterator it = p.tokens.begin(); it != p.tokens.end(); it++){
			tokens.push_back(*it);
		}
	};

	friend std::size_t hash_value(Place const& p)
	{
		std::size_t seed = boost::hash_range(p.tokens.begin(), p.tokens.end());
		boost::hash_combine(seed, p.place->GetIndex());

		return seed;
	}

	int NumberOfTokens() const{
		int count = 0;
		for(TokenList::const_iterator iter = tokens.begin(); iter != tokens.end(); iter++){
			count += iter->getCount();
		}
		return count;
	}

	int MaxTokenAge() const{
		int max = -1;
		for(TokenList::const_iterator iter = tokens.begin(); iter != tokens.end(); iter++){
			if(iter->getAge() > max) max = iter->getAge();
		}
		return max;
	}

	// Ages all tokens by 1
	void incrementAge(){
		for(TokenList::iterator iter = tokens.begin(); iter != tokens.end(); iter++){
			iter->incrementAge();
		}
	}

	void decrementAge(){
		for(TokenList::iterator iter = tokens.begin(); iter != tokens.end(); iter++){
			iter->decrementAge();
		}
	}
};

typedef vector<Place> PlaceList;

class NonStrictMarking {
public:
	NonStrictMarking();
	NonStrictMarking(const TAPN::TimedArcPetriNet& tapn, const std::vector<int>& v);
	NonStrictMarking(const NonStrictMarking& nsm);

public:
	friend std::ostream& operator<<(std::ostream& out, NonStrictMarking& x );
	friend class DiscreteVerification;

	virtual ~NonStrictMarking();

	virtual size_t HashKey() const { return boost::hash_range(places.begin(), places.end()); };

	virtual NonStrictMarking& Clone()
		{
			NonStrictMarking* clone = new NonStrictMarking(*this);
			return *clone;
		};

	public: // inspectors
		//int NumberOfTokensInPlace(const Place& palce) const;
		int NumberOfTokensInPlace(int placeId) const;
		const TokenList& GetTokenList(int placeId);
		const PlaceList& GetPlaceList() const{ return places; }
		unsigned int size();
		const NonStrictMarking* GetParent() const { return parent; }
		const TAPN::TimedTransition* GetGeneratedBy() const { return generatedBy; }


	public: // modifiers
		bool RemoveToken(int placeId, int age);
		bool RemoveToken(Place& place, Token& token);
		void AddTokenInPlace(TAPN::TimedPlace& place, int age);
		void AddTokenInPlace(Place& place, Token& token);
		void incrementAge();	// increment
		void decrementAge();	// decrement
		void RemoveRangeOfTokens(Place& place, TokenList::iterator begin, TokenList::iterator end);
		void SetParent(NonStrictMarking* parent) { this->parent = parent; }
		void SetGeneratedBy(const TAPN::TimedTransition* generatedBy) {
			std::cout << generatedBy << " " << this->generatedBy << " " << *this << std::endl;
			this->generatedBy = generatedBy;
		}

	public:
		bool equals(const NonStrictMarking &m1) const;

	public:
		int children;
		bool inTrace;
		PlaceList places;
		TokenList emptyTokenList;

	private:
		NonStrictMarking* parent;
		const TAPN::TimedTransition* generatedBy;
};

std::ostream& operator<<(std::ostream& out, NonStrictMarking& x);

} /* namespace DiscreteVerification */
} /* namespace VerifyTAPN */

#endif /* NONSTRICTMARKING_HPP_ */

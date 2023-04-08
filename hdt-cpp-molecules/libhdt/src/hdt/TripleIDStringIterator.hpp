/*
 * TripleIDStringIterator.hpp
 *
 *  Created on: 11/08/2012
 *      Author: mck
 */

#ifndef TRIPLEIDSTRINGITERATOR_HPP_
#define TRIPLEIDSTRINGITERATOR_HPP_

#include <HDT.hpp>
#include <Iterator.hpp>

namespace hdt {

class TripleIDStringIterator : public IteratorTripleString {

private:
	Dictionary *dict;
	IteratorTripleID *iterator;
	TripleString result;
public:
	TripleIDStringIterator(Dictionary *dict, IteratorTripleID *iterator);
	virtual ~TripleIDStringIterator();
	bool hasNext();
	TripleString *next();
	bool hasPrevious();
	TripleString *previous();
	bool canGoTo();
	void goToStart();
	size_t estimatedNumResults();
	ResultEstimationType numResultEstimation();
	size_t estimatedNumSubjects();
	size_t estimatedNumPredicates();
	size_t estimatedNumObjects();
	void skip(size_t pos);
};

class VectorIDIteratorTripleString : public IteratorTripleString {
public:
    VectorIDIteratorTripleString(std::vector<TripleID> &vector,Dictionary *dict) : vector(vector), dict(dict),pos(0) { }
	virtual ~VectorIDIteratorTripleString() { }

	 bool hasNext() {
		return pos<vector.size();
	}

	TripleString*next() {
		TripleID tid = vector[pos++];
		dict->tripleIDtoTripleString(tid, result);
		return &result;

	}
	size_t estimatedNumResults(){
		return vector.size();
	}
private:
	std::vector<TripleID> &vector;
    Dictionary *dict;
	size_t pos;
    TripleString result;
};


} /* namespace hdt */
#endif /* TRIPLEIDSTRINGITERATOR_HPP_ */

/*
 * Tutorial01.cpp
 *
 *  Created on: 02/03/2011
 *      Author: mck
 */

#include <HDT.hpp>
#include <HDTManager.hpp>

#include <getopt.h>
#include <string>
#include <iostream>
#include <fstream>
#include <unordered_set>
#include <algorithm>

#include "../src/util/StopWatch.hpp"
#include "../src/triples/TripleIterators.hpp"
#include "../src/triples/TriplesList.hpp"
#include "../src/triples/BitmapTriples.hpp"
#include "../src/hdt/TripleIDStringIterator.hpp"

using namespace hdt;
using namespace std;

hdt::HDT *hdt_file1;
bool verbose = false;
int numHops =2;
string filterPrefixStr;
bool filterPrefix=false;
bool continuousDictionary=true;

ModifiableTriples* triples = new TriplesList();
std::unordered_set<size_t> processedTerms;

bool filterPredicates=false;
std::unordered_set<unsigned int> preds;

void addhop(size_t termID,int currenthop,TripleComponentRole role){

	processedTerms.insert(termID);
	IteratorTripleID *it=NULL;
	// process as a subjectID
	if (role==SUBJECT || termID<=hdt_file1->getDictionary()->getNshared()){
		if (termID<=hdt_file1->getDictionary()->getMaxSubjectID()){
			TripleID patternSubject(termID,0,0);
			if (verbose){
				cout<< "searching termID "<<termID<<":"<<hdt_file1->getDictionary()->idToString(termID,SUBJECT)<< endl;
			}
			it  = hdt_file1->getTriples()->search(patternSubject);
			while (it->hasNext())
			{
				TripleID *triple = it->next();

				// check the predicate filter if needed
				if (!filterPredicates || preds.find(triple->getPredicate())!=preds.end())
				{
					//check the prefix if needed
					if (!filterPrefix || (hdt_file1->getDictionary()->idToString(triple->getObject(),OBJECT).find(filterPrefixStr) != std::string::npos)){
						triples->insert(*triple);
						if ((currenthop+1)<=numHops){ // we could do it in the beginning of the function but it saves time to do it here and avoid to change the context
							if (processedTerms.find(triple->getObject())==processedTerms.end()){
								if (verbose) cout<<"next hop object"<<endl;
								addhop(triple->getObject(),currenthop+1,OBJECT);
							}
						}
					}
				}

			}
		}
	}
	// process as a objectID
	if (role==OBJECT || termID<=hdt_file1->getDictionary()->getNshared()){
		if (termID<=hdt_file1->getDictionary()->getMaxObjectID()){
			TripleID patternObject(0,0,termID);
			if (verbose){
						cout<< "searching termID "<<termID<<":"<<hdt_file1->getDictionary()->idToString(termID,OBJECT)<< endl;
					}
			it = hdt_file1->getTriples()->search(patternObject);
			while (it->hasNext())
			{
				TripleID *triple = it->next();
				// check the predicate filter if needed
				if (!filterPredicates || preds.find(triple->getPredicate())!=preds.end())
				{
					//check the prefix if needed
					if (!filterPrefix || (hdt_file1->getDictionary()->idToString(triple->getObject(),OBJECT).find(filterPrefixStr) != std::string::npos)){
					triples->insert(*triple);
						if ((currenthop+1)<=numHops){ // we could do it in the beginning of the function but it saves time to do it here and avoid to change the context
							if (processedTerms.find(triple->getSubject())==processedTerms.end()){
								if (verbose) cout<<"next hop subject"<<endl;
								addhop(triple->getSubject(),currenthop+1,SUBJECT);
							}
						}
					}
				}
			}
		}
	}
	delete it;
}




void help()
{
	cout << "$ hops [options] <hdtfile> " << endl;
	cout << "\t-h\t\t\tThis help" << endl;

	cout
		<< "\t-v\t\t\tVerbose, show results (warning: will print all triples, use for test only)"
		<< endl;
	cout
		<< "\t-n <numberHops> \tNumber of hops, 2 by default"
		<< endl;
	cout
		<< "\t-t \"[<termURI>]+\" \tList of initial seed terms, e.g.: \"<URI1><URI2><URI3>\""
		<< endl;
	cout
		<< "\t-p <prefix> \tFilter the hops to only those terms starting with the given <prefix>"
		<< endl;
	cout
		<< "\t-f \"[<predicateURI>]+\" \tFilter only those hops with the given predicate terms, e.g.: \"<predicate1><predicate2><predicate3>\""
		<< endl;
	cout
		<< "\t-o <outputFile> \tOutput the solution in the given <outputFile>"
		<< endl;
	cout
		<< "\t-d \tUse the traditional dictionary instead of a continuous mapping (default)"
		<< endl;

	cout << "\t-v\tVerbose output" << endl;
}





int main(int argc, char **argv)
{
	int c;
	string inputFile;
	string outputFileString;
	string termsStr="";
	string predsStr="";


	while ((c = getopt(argc, argv, "hvn:p:o:t:df:")) != -1)
	{
		switch (c)
		{
		case 'h':
			help();
			break;
		case 'n':
			numHops=atoi(optarg);
			break;
		case 'o':
			outputFileString = optarg;
			break;
		case 't':
			termsStr= optarg;
			break;
		case 'f':
			predsStr= optarg;
			break;
		case 'v':
			verbose = true;
			break;
		case 'p':
			filterPrefixStr=optarg;
			filterPrefix = true;
			break;
		case 'd':
			continuousDictionary=false;
			break;
		default:
			cout << "ERROR: Unknown option" << endl;
			help();
			return 1;
		}
	}

	if (argc - optind < 1)
	{
		cout << "ERROR: You must supply an HDT File" << endl
			 << endl;
		help();
		return 1;
	}
	if (termsStr==""){
		cout << "ERROR: Please provide at least 1 initial seed subject" << endl
					 << endl;
				help();
				return 1;
	}

	inputFile = argv[optind];

	ostream *out;
	ofstream outF;

	if(outputFileString!="") {
		outF.open(outputFileString.c_str());
		out = &outF;
	} else {
		out = &cout;
	}


	try
	{
		hdt_file1 = HDTManager::mapIndexedHDT(inputFile.c_str());

		// get terms
		vector<string> terms;
		size_t pos = 0;
		string term="";
		string delimiter = ">";
		// get term string
		while ((pos = termsStr.find(delimiter)) != std::string::npos) {
			term = termsStr.substr(1, pos-1);//remove the '<' and '>'
		    termsStr.erase(0, pos + delimiter.length());

		    //check the prefix if needed
		    if (!filterPrefix || (term.find(filterPrefixStr) != std::string::npos)){
		    	terms.push_back(term);
		    }
		}

		string pred="";
		// get potential predicate filters
		if (predsStr!=""){
			filterPredicates=true;
			while ((pos = predsStr.find(delimiter)) != std::string::npos) {
				pred = predsStr.substr(1, pos-1);//remove the '<' and '>'
				predsStr.erase(0, pos + delimiter.length());
				unsigned int idPred = hdt_file1->getDictionary()->stringToId(pred,PREDICATE);
				if (idPred!=0)
					preds.insert(idPred);
			}
		}
		// do a recursive function to iterate terms 2 hops, and keep the result in a TripleList, then order by PSO and dump.
		if (numHops>=1){
			TripleComponentRole role=SUBJECT;
			for (int i=0;i<terms.size();i++){
				if (verbose)
					cout<<"- Initial term:"<<terms[i]<<"."<<endl;
				size_t termIDSubject = hdt_file1->getDictionary()->stringToId(terms[i],SUBJECT);
				if (verbose)
					cout<<"- Initial termID term:"<<termIDSubject<<endl;
				if (termIDSubject==0){
					termIDSubject = hdt_file1->getDictionary()->stringToId(terms[i],OBJECT);
					role=OBJECT;
					if (verbose)
						cout<<"- Initial termID object:"<<termIDSubject<<endl;
				}
				if (termIDSubject!=0){
					addhop(termIDSubject,1,role);
				}
			}
		}

		//sort PSO and remove duplicates
		TripleComponentOrder order = PSO;
		triples->sort(order,NULL);
		triples->removeDuplicates(NULL);

		// dump output
		IteratorTripleID *it = triples->searchAll();
		while (it->hasNext())
		{
			TripleID *triple = it->next();
			if (!continuousDictionary)
				*out<<*triple<<endl;
			else{
				size_t subject = triple->getSubject();
				size_t predicate = triple->getPredicate();
				size_t object = triple->getObject();
				*out<<subject<<" "<<predicate<<" ";
				if (object>hdt_file1->getDictionary()->getNshared()){
					object=object+(hdt_file1->getDictionary()->getNsubjects()-hdt_file1->getDictionary()->getNshared());
				}
				*out<<object<<endl;
			}
		}
		delete it;

		if (verbose){
			IteratorTripleID *itVerbose = triples->searchAll();
			TripleIDStringIterator* iterator = new TripleIDStringIterator(hdt_file1->getDictionary(), itVerbose);
			while (iterator->hasNext())
			{
				TripleString *triple = iterator->next();
				cout<<*triple<<endl;
			}

		}
		outF.close();


		delete hdt_file1;
	}
	catch (std::exception &e)
	{
		cerr << "ERROR in order!: " << e.what() << endl;
	}
}

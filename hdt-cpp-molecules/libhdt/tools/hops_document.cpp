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
#include <tuple>
#include <unistd.h>

#include "../src/util/StopWatch.hpp"

#include <hdt_document.hpp>

using namespace hdt;
using namespace std;


bool verbose = false;
int numHops =2;
string filterPrefixStr;
bool filterPrefix=false;
bool continuousDictionary=true;



bool filterPredicates=false;
std::tuple<vector<unsigned int>,vector<unsigned int>,vector<vector<std::tuple<unsigned int, unsigned int>>>> solution;






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
	cout
			<< "\t-l <Limit> \tLimit the number of results. Use a number or 'all' for all (default)"
			<< endl;
	cout
			<< "\t-O <Offset> \tOffset of the results, 0 by default"
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
	string limitStr="";
	unsigned int limit=0;
	unsigned int offset=0;
	bool changeLimit=false;
	bool maxLimit=false;


	while ((c = getopt(argc, argv, "hvn:p:o:t:df:l:O:")) != -1)
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
		case 'l':
			changeLimit=true;
			limitStr = optarg;
			if (limitStr=="all"){
				cout<<"ALL"<<endl;
				maxLimit=true;
			}
			else{
				limit=atoi(optarg);
			}
			break;
		case 'O':
			offset=atoi(optarg);
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

		HDTDocument hdt (inputFile);

		if (changeLimit==false || maxLimit==true){
			limit = hdt.getNbTriples();
		}


		// get terms
		vector<string> terms;
		vector<unsigned int> termIDs;
		size_t pos = 0;
		string term="";
		string delimiter = ">";
		// get term string
		while ((pos = termsStr.find(delimiter)) != std::string::npos) {
			term = termsStr.substr(1, pos-1);//remove the '<' and '>'
		    termsStr.erase(0, pos + delimiter.length());
		    //check the prefix if needed
			if (filterPrefixStr=="" || (term.find(filterPrefixStr) != std::string::npos)){
				 terms.push_back(term);
			}
		}
		// convert to ids
		for (int i=0;i<terms.size();i++){
			if (verbose)
				cout<<"- Initial term:"<<terms[i]<<"."<<endl;
			size_t termID = hdt.StringToid(terms[i],SUBJECT);
			if (verbose)
				cout<<"- Initial termID term:"<<termID<<endl;
			if (termID==0){
				termID = hdt.StringToid(terms[i],OBJECT);
				// now convert it to the new id is needed
				if (continuousDictionary){
					termID=termID+(hdt.getNbSubjects()-hdt.getNbShared());
				}

			}
			termIDs.push_back(termID);
		}


		vector<unsigned int> preds;
		string pred="";
		// get potential predicate filters
		if (predsStr!=""){
			filterPredicates=true;
			while ((pos = predsStr.find(delimiter)) != std::string::npos) {
				pred = predsStr.substr(1, pos-1);//remove the '<' and '>'
				predsStr.erase(0, pos + delimiter.length());
				preds.push_back(hdt.StringToid(pred,PREDICATE));
			}
			if (predsStr!="")
				preds.push_back(hdt.StringToid(predsStr,PREDICATE));
		}

		hdt.configureHops(numHops,preds,filterPrefixStr,continuousDictionary);

		//for (int i=1;i<=1000;i++){
		int i=1;
			cout<<"loop "<<i<<endl;
			solution = hdt.computeHopsIDs(termIDs,limit,offset);

			vector<unsigned int> mapping = std::get<0>(solution);
			vector<unsigned int> predicates = std::get<1>(solution);
			vector<vector<std::tuple<unsigned int, unsigned int>>> matrix = std::get<2>(solution);

			cout<<"Mapping:"<<endl;
			for (int i=0;i<mapping.size();i++){
				cout<<(i)<<" --> "<<mapping[i]<<endl;
			}
			cout<<endl<<"Predicates:"<<endl;
			for (int i=0;i<predicates.size();i++){
				cout<<" Predicate "<<predicates[i]<<endl;
			}
			cout<<endl<<"Matrixes:"<<endl;
			for (int i=0;i<matrix.size();i++){
				cout<<"Matrix "<<(i+1)<<endl;
				for (int j=0;j<matrix[i].size();j++){
					unsigned int source = std::get<0>(matrix[i][j]);
					unsigned int target = std::get<1>(matrix[i][j]);
					cout<< source<< " -- "<<target<<endl;
				}
				cout<<endl;
			}
		//	usleep(5000000);
		//}
		hdt.remove();

		//delete hdt;
	}
	catch (std::exception &e)
	{
		cerr << "ERROR in order!: " << e.what() << endl;
	}
}

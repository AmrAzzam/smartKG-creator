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
	cout << "$ types [options] <hdtfile> " << endl;
	cout << "\t-h\t\t\tThis help" << endl;

	cout
		<< "\t-v\t\t\tVerbose, show results (warning: will print all triples, use for test only)"
		<< endl;
	cout
		<< "\t-t \"[<termURI>]+\" \tList of initial seed terms, e.g.: \"<URI1><URI2><URI3>\""
		<< endl;
	cout
		<< "\t-c \"[<classURI>]+\" \tFilter only those classes, e.g.: \"<class1><class2><class3>\""
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
	string classStr="";
	string limitStr="";


	while ((c = getopt(argc, argv, "hvo:t:dc:O:")) != -1)
	{
		switch (c)
		{
		case 'h':
			help();
			break;;
		case 'o':
			outputFileString = optarg;
			break;
		case 't':
			termsStr= optarg;
			break;
		case 'c':
			classStr= optarg;
			break;
		case 'v':
			verbose = true;
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

		HDTDocument hdt (inputFile);
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


		vector<unsigned int> classes;
		string classs="";
		// get potential predicate filters
		if (classStr!=""){
			while ((pos = classStr.find(delimiter)) != std::string::npos) {
				classs = classStr.substr(1, pos-1);//remove the '<' and '>'
				classStr.erase(0, pos + delimiter.length());
				unsigned int classID = hdt.StringToid(classs,OBJECT);
				// now convert it to the new id is needed
				if (continuousDictionary){
					if (classID>hdt.getNbShared())
						classID=classID+(hdt.getNbSubjects()-hdt.getNbShared());
				}
				classes.push_back(classID);
			}
			if (classStr!="")
				classes.push_back(hdt.StringToid(classStr,OBJECT));
		}

		/*************************** call */
		vector<vector<unsigned int>> results;
		results=hdt.filterTypeIDs(termIDs,classes);

		cout<<endl<<"vectors:"<<endl;
		for (int i=0;i<results.size();i++){
			cout<<"Entities "<<(i+1)<<endl;
			for (int j=0;j<results[i].size();j++){
				cout<< "result "<<j<<":"<<results[i][j]<< " -- "<<endl;
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

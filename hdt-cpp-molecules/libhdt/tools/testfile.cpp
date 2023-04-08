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
#include "../src/triples/BitmapTriples.hpp"
#include "../src/hdt/TripleIDStringIterator.hpp"

using namespace hdt;
using namespace std;

hdt::HDT *hdt_file1;
bool verbose = false;
int MAX_OPEN_FILES = 130000;
double PERCENTAGE_INFREQUENT_PREDICATES = 0.01; //0.01 % of the number of triples
double PERCENTAGE_CUT_PREDICATES = 0.1; //0.1 % of the number of triples
unsigned int MAX_BUFFER_DUMP = 100000; //max size in each vector<triples> before dumping to a merge ID.
unsigned int PERCENTAGE_MAX_SIZE_GROUP=5; // max size to allow for a creation of a new group, in % of the total number of triples. E.g. with max=5, it will create a group if the total estimated size is less than 5% the number of triples. Use 100 to allow all
// Uncomment to try a small test
//unsigned int PERCENTAGE_MAX_SIZE_GROUP=40; // max size to allow for a creation of a new group, in % of the total number of triples. E.g. with max=5, it will create a group if the total estimated size is less than 5% the number of triples
int MAX_SIZE_GROUP=0; // the actual max of the size of the group given the percentage and the numbe of triples

double PERCENTAGE_SUBJECT_GROUPS = 0.01; //0.01 % of the number of subjects
int MIN_SUBJECT_GROUP=0;

unsigned int numFamilies = 1;
map<string, unsigned int> families; //give an ID to the different families (temporal structure, just for construction)
vector<vector<unsigned int>> predicatesPerFamily; // the predicates of each family
int max_size_group =0;
vector<vector<unsigned int>> max_groups;
std::unordered_set<string> groups;
map<unsigned int,std::unordered_set<unsigned int>> mergedFamilies; // initial family --> grouped Family(ies)

map<unsigned int,std::unordered_set<unsigned int>> sourceFamilies; // grouped family --> source Families(ies)

map<unsigned int,unsigned int> numTriplesperFamily;	// family--> numberOfTriples. Temporal, just needed for metadata if we want to split by family and export in files

map<unsigned int,unsigned int> estimatedNumTriplesperFamily;	// family--> numberOfTriples. Temporal, just needed for metadata if we want to split by family and export in files

map<unsigned int, vector<unsigned int>> subjectsPerFamilies;  // all different subjects per Family, i.e, IdFamily -- > [subjectIDs,..]

std::unordered_set<string> bannedFamily_largeGroups; // the groups (in string version) that exceed the maximum size limit
std::unordered_set<size_t> bannedFamily_largeGroups_ID; // the groups (in ID version) that exceed the maximum size limit

std::unordered_set<size_t> infrequent_predicates;
std::unordered_set<size_t> cut_predicates; //massive predicates

map<unsigned int,std::unordered_set<unsigned int>> predToMergeFam; //temporal structure, from predicate to the Merged Families
map<unsigned int,map<unsigned int,std::unordered_set<unsigned int>>> familyToMap; //temporal structure, from family to the map with predicate to the Merged Family>

std::unordered_set<size_t> groupedFamily; //set to keep the grouped families

bool includeInfrequentPredicatesDedicatedFile=false;
unsigned int max_estimated_numTriples=0;
unsigned int max_estimated_numTriples_Stats=0;

int countSource=0;

vector <float> StatsTriplesperPredicate; // statistics with the occurrences of each predicates in the global HDT, in percentage over the total number of triples.

std::unordered_set<unsigned int> familiesLargerSubjects; //for the grouping by subject
std::unordered_set<unsigned int> familiesLargerSubjects_fromMerge;
vector<unsigned int> familiesLargerSubjects_total;

map<unsigned int, unsigned int> subjectfamiliesToFinalID; //give an ID of subject family (in grouping), get the final ID (to be correlative)

void help()
{
	cout << "$ getFamilies [options] <hdtfile> " << endl;
	cout << "\t-h\t\t\tThis help" << endl;

	cout
		<< "\t-v\t\t\tVerbose, show results (warning: will print all triples, use for test only)"
		<< endl;
	cout
		<< "\t-e <exportFile>\tExport metadata of families in <exportFile>.json and the groups in <exportFile>_group.json"
		<< endl;
	cout
		<< "\t-s <splitFilePrefix>\tSplit triples by families in <splitFilePrefix.json>"
		<< endl;
	cout
		<< "\t-i\t\t\tInclude infrequent predicates, i.e. less than "<<PERCENTAGE_INFREQUENT_PREDICATES<<" % occurrences (it will create more partitions). False by default"
		<< endl;
	cout
			<< "\t-c\t\t\tCut the massive predicates, i.e. >= than "<<PERCENTAGE_CUT_PREDICATES<<" % occurrences (it will create huge partitions). False by default"
			<< endl;
	cout
			<< "\t-d\t\t\tDump the infrequent predicates in a dedicated JSON file (_infreqPreds.json)"
			<< endl;
	cout
		<< "\t-u\t\t\t Ungroup, i.e. do not group predicates. We group predicates by default"
		<< endl;
	cout
			<< "\t-S\t\t\t Group with a minimum number of subjects (0.01%). We don't apply this filter by default, but it is recommended for very unstructure datasets (e.g. dbpedia)"
			<< endl;
	cout
			<< "\t-m\t\t\t max size to allow for a creation of a new group, in % of the total number of triples. E.g. with -m 5, it will create a group if the total estimated size is less than 5% the number of triples. Use -m 100 to allow all"
			<< endl;
	cout
			<< "\t-G\t\t\texport the group in a separate JSON file"
			<< endl;
	//cout << "\t-v\tVerbose output" << endl;
}



int main(int argc, char **argv)
{
	int c;
	string inputFile;
	string exportFileString;

	if (argc - optind < 1)
	{
		cout << "ERROR: You must supply an HDT File" << endl
			 << endl;
		help();
		return 1;
	}

	inputFile = argv[optind];



	// uncomment to reuse the serializers, but it is problematic with many families.
	RDFSerializer *serializer;
	bool newSerializer=false;

	unsigned int numNew =1;

	hdt::HDT *hdt_file1;
	hdt_file1 = HDTManager::mapIndexedHDT(inputFile.c_str());

	string newSplitFile;
	for (int i=0;i<15000;i++){
		newSplitFile = "nt/" + to_string(i) + ".nt";

	    serializer = RDFSerializer::getSerializer(newSplitFile.c_str(), NTRIPLES);				  // create new serializer

		IteratorTripleID *it1 = hdt_file1->getTriples()->searchAll();
		vector<TripleID> triples; // temp just needed if we want to split by family and export in files
		while (it1->hasNext()){
			TripleID *triple = it1->next();
			triples.push_back(*triple);
		}

		VectorIDIteratorTripleString *it = new VectorIDIteratorTripleString(triples, hdt_file1->getDictionary()); // create iterator of triples in the partition

		serializer->serialize(it, NULL, it->estimatedNumResults());		// serialize the triples in the partition

		//delete serializer; //no needed if serializers are reused
		delete it1;
		triples.clear();
	}


}


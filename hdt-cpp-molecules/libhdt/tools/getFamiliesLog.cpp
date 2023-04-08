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
		<< "\t-l <logFile>\tGet predefined families from <logFile> (format: one family per line with:pred1,pred2,pred3"
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

void computeStatsPredicate(){


	for (size_t i=0;i<hdt_file1->getDictionary()->getNpredicates();i++){
		TripleID tid(0,(i+1),0);
		IteratorTripleID *it = hdt_file1->getTriples()->search(tid);
		StatsTriplesperPredicate.push_back((float)it->estimatedNumResults()/hdt_file1->getTriples()->getNumberOfElements());
		if (verbose) cout<<"Estimation predicate "<<hdt_file1->getDictionary()->idToString(i+1,PREDICATE)<<":"<<it->estimatedNumResults()<<endl;
		if (verbose) cout<<"Percentage predicate "<<(i+1)<<":"<<StatsTriplesperPredicate[i]<<endl;
	}
}

string getFamilyStr(vector<unsigned int>* vec){
	string family="";
	for (int p=0;p<vec->size();p++){
		ostringstream pred;
		pred << (*vec)[p];
		if (family.length() != 0)
			family = family + " " + pred.str();
		else
			family = pred.str();
	}
	return family;
}

void banFamilies(){
	for (int i=0;i<predicatesPerFamily.size();i++){
		//cout<<"MAX_SIZE_GROUP:"<<MAX_SIZE_GROUP<<endl;
		if ((numTriplesperFamily[i+1]+estimatedNumTriplesperFamily[i+1])>MAX_SIZE_GROUP){
			cerr<<"!!!!!! BANNED - family "<<(i+1)<<endl;
			bannedFamily_largeGroups_ID.insert(i+1);
		}
	}
}


void printMaterializedVersions(unsigned int bannedId,ostream* out){


	for (auto itr = sourceFamilies[bannedId].begin(); itr != sourceFamilies[bannedId].end(); ++itr) {
		unsigned int sourceFamily= *itr;
		if (bannedFamily_largeGroups_ID.find(sourceFamily)==bannedFamily_largeGroups_ID.end()){
			if (countSource!=0){
				*out<<", ";
			}
			*out<<sourceFamily;
		}
		else{
			//if banned, do the same for the source
			printMaterializedVersions(sourceFamily,out);
		}
		countSource++;
	}
}
/*
 * Recursive function to groups common predicates in predicatesPerFamily, starting from position @initial.
 * Store the new groups at the end, and returns the number of new groups created
 */
size_t group(size_t initial,size_t initial_size_without_grouping) {
	int newGroups=0;
	int exists=0;
	size_t initial_size = predicatesPerFamily.size();
	/*if (verbose){
		cout<< "group from "<<initial<<endl;
		cout<< "group to "<<initial_size- 1<<endl;
		cout<< "total numFamilies:"<<numFamilies<<endl;
	}*/
	for (int i = initial; i <= (initial_size- 1); i++) // iterate families
	{
		// if (verbose) cout<<"-- from "<<i+1<<endl;
		vector<unsigned int> predicates_i = predicatesPerFamily[i]; // iterate predicates per family
		string family = getFamilyStr(&predicates_i);
			for (int j = 0; j < initial_size; j++) {
				//if (verbose) cout<<"   -- to "<<j+1<<endl;
		//for (int j = (i + 1); j < initial_size; j++) {
			// iterate families
			vector<unsigned int> predicates_j = predicatesPerFamily[j];
			vector<unsigned int> intersect;
			set_intersection(predicates_i.begin(), predicates_i.end(),
					predicates_j.begin(), predicates_j.end(),
					std::inserter(intersect, intersect.begin()));


			if (intersect.size() > 1) {

				string family_intersect = getFamilyStr(&intersect);

					if (families[family_intersect]==0){
						// the target intersect family is a new family


							unsigned int familyID = numFamilies;
							numFamilies++;
							families[family_intersect] = familyID; //insert new family string in the map

							groupedFamily.insert(familyID); // mark the family as a grouped family
							//cout<<"families[family_intersect]:"<<families[family_intersect]<<endl;
							predicatesPerFamily.push_back(intersect); // insert the family in the vector
							newGroups++;

							//insert both merged families into map of grouped families
							mergedFamilies[(i+1)].insert(familyID);
							mergedFamilies[(j+1)].insert(familyID);

					}
					else{
						// the target intersect family already existed

						bool inserted=false;
						if ((i+1)!=families[family_intersect]){
							mergedFamilies[(i+1)].insert(families[family_intersect]);
							sourceFamilies[families[family_intersect]].insert(i+1);
							inserted=true;
						}
						if ((j+1)!=families[family_intersect]){
							mergedFamilies[(j+1)].insert(families[family_intersect]);
							sourceFamilies[families[family_intersect]].insert(j+1);
							inserted=true;
						}
						if (inserted){ // mark the family as a grouped family
							groupedFamily.insert(families[family_intersect]);
						}

						exists++;
				}

			}
		}
	}
	//cout<< "exists:"<<exists<<endl;
	if (newGroups>0){ // if there are new groups, try to group them again
		return newGroups + group(initial_size,initial_size_without_grouping);
	}
	return 0;
}

//void addPredicatesToMerge(unsigned int famID, map<unsigned int, std::unordered_set<unsigned int> >* predToMergeFam) {
// This fills the structure "familyToMap" that gives, for a given family, the correspondance betwee each of the predicates of the family and the families where these predicates need to be merged to
void addPredicatesToMerge(unsigned int famID) {
	map<unsigned int,std::unordered_set<unsigned int>> tempPredToMergeFam; //temporal structure, from predicate to the Merged Families

	//cout<<" add Predicates to Merge from family "<<famID<<endl;
	for (auto itr = mergedFamilies[famID].begin(); itr != mergedFamilies[famID].end(); ++itr) {

	//for (int i = 0; i < mergedFamilies[famID].size(); i++) {
		//for each target merged.
		//cout << "destiny family id: " << mergedFamilies[famID][i] << endl;
		//unsigned int targetFamily= mergedFamilies[famID][i];
		unsigned int targetFamily= *itr;

		//cout<<"    looking in target Family "<<targetFamily<<endl;
		//add link to current target family
		vector<unsigned int> predicates_in_target_family =
						predicatesPerFamily[targetFamily - 1];
				for (int j = 0; j < predicates_in_target_family.size(); j++) {
					//for each predicate of the target
					//cout << "    add predicate " << predicates_in_target_family[j] << " to target Family "<< targetFamily<<endl;
					// insert mapping to know where to dump the predicate
					tempPredToMergeFam[predicates_in_target_family[j]].insert(targetFamily);
				}

		if (mergedFamilies[targetFamily].size()!=0){ //if it goes to a merge family
			if (familyToMap[targetFamily].size()!=0){
			//	cout<<"   now reuse the next steps of the target family"<<endl;
				//already done, reuse
				//tempPredToMergeFam.insert(familyToMap[targetFamily].begin(),familyToMap[targetFamily].end());
			}
			else{

				//cout<<"   cannot reuse, but there could be more steps, catch them"<<endl;
				addPredicatesToMerge(targetFamily); // if needed, familytoMap will be updated

				//tempPredToMergeFam.insert(familyToMap[targetFamily].begin(),familyToMap[targetFamily].end());
			}
			//update the family
			for (std::map<unsigned int,std::unordered_set<unsigned int>>::iterator it=familyToMap[targetFamily].begin(); it!=familyToMap[targetFamily].end(); ++it){
				tempPredToMergeFam[it->first].insert( it->second.begin(),it->second.end());

			/*	for (auto itr = it->second.begin(); itr != it->second.end(); ++itr) {
					tempPredToMergeFam[it->first].insert(*itr);

				}*/

			}

				//map<unsigned int,std::unordered_set<unsigned int>> newPredToMergeFam;
				//newPredToMergeFam = predToMergeFam;
		}
		// else - NO reuse and no more next steps
	}
	familyToMap[famID] = tempPredToMergeFam;

}





// This fills the structure "familyToMap" that gives, for a given family, the correspondance betwee each of the predicates of the family and the families where these predicates need to be merged to
// it considers only direct merges, it doesn't go to the merged family recursively
void addPredicatesToMergeNoRecursive(unsigned int famID) {
	map<unsigned int,std::unordered_set<unsigned int>> tempPredToMergeFam; //temporal structure, from predicate to the Merged Families

	for (auto itr = mergedFamilies[famID].begin(); itr != mergedFamilies[famID].end(); ++itr) {
		unsigned int targetFamily= *itr;
		//add link to current target family
		vector<unsigned int> predicates_in_target_family =
						predicatesPerFamily[targetFamily - 1];
		for (int j = 0; j < predicates_in_target_family.size(); j++) {
			//for each predicate of the target
			// insert mapping to know where to dump the predicate
			tempPredToMergeFam[predicates_in_target_family[j]].insert(targetFamily);
		}
	}
	familyToMap[famID] = tempPredToMergeFam;

}
void dumpInfrequentPredicatesJSON(ostream* out){
	if (includeInfrequentPredicatesDedicatedFile)
		*out << "{" << endl; //output as an object
	*out << "  \"infrequentPredicates\": [ ";
		for (auto itr = infrequent_predicates.begin(); itr != infrequent_predicates.end(); ++itr) {
			*out<< "\""<<hdt_file1->getDictionary()->idToString(*itr,
					PREDICATE)<<"\"" ;
			auto temp = itr;
			temp++;
			if (temp != infrequent_predicates.end())
				*out << ", ";
		}
		*out << " ]" ;
	if (includeInfrequentPredicatesDedicatedFile)
		*out << endl << "}" << endl; //close the object

}
void dumpCutPredicatesJSON(ostream* out){ //massive predicates
	if (cut_predicates.size()>0){ //if the cut predicate option is not activated, the size will be 0 in any case.
		*out << "  \"massivePredicates\": [ ";
		for (auto itr = cut_predicates.begin(); itr != cut_predicates.end(); ++itr) {
			*out<< "\""<<hdt_file1->getDictionary()->idToString(*itr,
					PREDICATE)<<"\"" ;
			auto temp = itr;
			temp++;
			if (temp != cut_predicates.end())
				*out << ", ";
		}
		*out << " ],"<<endl ;
	}

}

void dumpFamiliesJSON(const string& splitFileString, ostream* out, unsigned int start, unsigned int end, bool markGroup, unsigned int initial_size_without_grouping) {
	*out << "{" << endl;

	*out << "  \"numFamilies\": " << (numFamilies - 1) << ", " << endl;
	if (!includeInfrequentPredicatesDedicatedFile){
		// dump the infrequente predicates
		dumpInfrequentPredicatesJSON(out);
		*out<<", "<<endl;
	}
	dumpCutPredicatesJSON(out);
	// TODO: add timestamp here
	*out << "  \"families\": [ " << endl;
	for (int i = start; i < end; i++) // iterate families
			{
		*out << "    { \"index\": " << (i + 1) << ", " << endl;
		*out << "      \"name\": \"" << splitFileString << (i + 1) << ".hdt\""
				<< ", " << endl;

		//check if it is banned
		if (bannedFamily_largeGroups_ID.find(i+1)==bannedFamily_largeGroups_ID.end()){
			//not banned
			*out << "      \"numSubjects\": " << subjectsPerFamilies[i + 1].size()
							<< ", " << endl;
			*out << "      \"numTriples\": " << numTriplesperFamily[i+1] << ", "
					<< endl;
		}
		else{
			if (i<initial_size_without_grouping){
				*out << "      \"originalFamily\": true, "<< endl;
			}
			*out << "      \"numTriples\": " << estimatedNumTriplesperFamily[i+1] << ", "
								<< endl;
			*out << "      \"sourceSet\": [ ";
			countSource=0;
			printMaterializedVersions(i+1,out);
			*out << " ]," << endl;
		}
		//if (markGroup && (initial_size_without_grouping <= i)) {
		if (markGroup && (groupedFamily.find((i+1))!=groupedFamily.end())) {
			*out << "      \"grouped\": true, " << endl;
		}
		*out << "      \"predicateSet\": [ ";
		vector<unsigned int> predicates = predicatesPerFamily[i]; // iterate predicates per family
		for (int j = 0; j < predicates.size(); j++) {
			*out << "\""
					<< hdt_file1->getDictionary()->idToString(predicates[j],
							PREDICATE) << "\"";
			if (j != predicates.size() - 1)
				*out << ", ";
		}
		*out << " ]" << endl;





		if (i != predicatesPerFamily.size() - 1)
			*out << "    }, " << endl;
		else
			*out << "    } " << endl;
	}
	*out << "  ] " << endl;
	*out << "}" << endl;
}

void dumpFamiliesJSONFilterSubjects(const string& splitFileString, ostream* out, bool markGroup) {
	*out << "{" << endl;

	*out << "  \"numFamilies\": " << (numFamilies - 1) << ", " << endl;
	if (!includeInfrequentPredicatesDedicatedFile){
		// dump the infrequente predicates
		dumpInfrequentPredicatesJSON(out);
		*out<<", "<<endl;
	}
	// TODO: add timestamp here
	*out << "  \"families\": [ " << endl;
	for (int i = 0; i < familiesLargerSubjects_total.size(); i++) // iterate families
			{
		*out << "    { \"index\": " << (i + 1) << ", " << endl;
		*out << "      \"name\": \"" << splitFileString << (i + 1) << ".hdt\""
				<< ", " << endl;

		//check if it is banned
		if (bannedFamily_largeGroups_ID.find(familiesLargerSubjects_total[i])==bannedFamily_largeGroups_ID.end()){
			//not banned
			*out << "      \"numSubjects\": " << subjectsPerFamilies[familiesLargerSubjects_total[i]].size()
							<< ", " << endl;
			*out << "      \"numTriples\": " << numTriplesperFamily[familiesLargerSubjects_total[i]] << ", "
					<< endl;
		}
		else{ //banned
			*out << "      \"originalFamily\": true, "<< endl;

			*out << "      \"numTriples\": " << estimatedNumTriplesperFamily[familiesLargerSubjects_total[i]] << ", "
								<< endl;
			/**out << "      \"sourceSet\": [ ";
			countSource=0;
			printMaterializedVersions(i+1,out);
			*out << " ]," << endl;*/
			*out << "      \"noMaterialized\": true, "<< endl;
			// note that the family can be banned also from the beginning in FilterSubjects if it's too big
		}
		//if (markGroup && (initial_size_without_grouping <= i)) {
		if (markGroup && (groupedFamily.find(familiesLargerSubjects_total[i])!=groupedFamily.end())) {
			*out << "      \"grouped\": true, " << endl;
		}
		*out << "      \"predicateSet\": [ ";
		vector<unsigned int> predicates = predicatesPerFamily[familiesLargerSubjects_total[i]-1]; // iterate predicates per family
		for (int j = 0; j < predicates.size(); j++) {
			*out << "\""
					<< hdt_file1->getDictionary()->idToString(predicates[j],
							PREDICATE) << "\"";
			if (j != predicates.size() - 1)
				*out << ", ";
		}
		*out << " ]" << endl;


		if (i != familiesLargerSubjects_total.size() - 1)
			*out << "    }, " << endl;
		else
			*out << "    } " << endl;
	}
	*out << "  ] " << endl;
	*out << "}" << endl;
}

int main(int argc, char **argv)
{
	int c;
	string inputFile;
	string exportFileString;
	string splitFileString;
        string logFileString;
	bool exportMetadata = false;
	bool split = false;
	bool fromLog = false;
	bool includeInfrequentPredicates=false;
	bool cutMassivePredicates=false;
	bool groupPredicates=true;
	int open_files=1;
	int MAX_INFREQUENT_PREDICATES=0;
	int MAX_CUT_PREDICATES=0;
	int MIN_SUBJECT_GROUP=0;
	bool separateGroupExport = false;
	bool groupFilterSubjects=false;

	StopWatch timeTotal;

	while ((c = getopt(argc, argv, "hve:o:s:iuGdm:Scl:")) != -1)
	{
		switch (c)
		{
		case 'h':
			help();
			break;
		case 'e':
			exportFileString = optarg;
			exportMetadata = true;
			break;
		case 's':
			splitFileString = optarg;
			split = true;
			break;
		case 'l':
			logFileString = optarg;
			fromLog = true;
			break;
		case 'v':
			verbose = true;
			break;
		case 'i':
			includeInfrequentPredicates = true;
			break;
		case 'S':
			groupFilterSubjects = true;
			break;
		case 'd':
			includeInfrequentPredicatesDedicatedFile=true;
			break;
		case 'u':
			groupPredicates = false;
			break;
		case 'm':
			PERCENTAGE_MAX_SIZE_GROUP = atoi(optarg);
			break;
		case 'G':
			separateGroupExport = true;
			break;
		case 'c':
			cutMassivePredicates = true;
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

	inputFile = argv[optind];

	try
	{
		hdt_file1 = HDTManager::mapIndexedHDT(inputFile.c_str());

		computeStatsPredicate();

		//cout<<"id:"<<hdt_file1->getDictionary()->stringToId("http://db.uwaterloo.ca/~galuc/wsdbm/User10462",SUBJECT)<<endl;

		MAX_INFREQUENT_PREDICATES=(hdt_file1->getTriples()->getNumberOfElements()*PERCENTAGE_INFREQUENT_PREDICATES)/100;
		MAX_CUT_PREDICATES=(hdt_file1->getTriples()->getNumberOfElements()*PERCENTAGE_CUT_PREDICATES)/100;
		MAX_SIZE_GROUP = (hdt_file1->getTriples()->getNumberOfElements()*PERCENTAGE_MAX_SIZE_GROUP)/100;

		MIN_SUBJECT_GROUP = (hdt_file1->getDictionary()->getNsubjects()*PERCENTAGE_SUBJECT_GROUPS)/100;

		if (verbose)
		{
			cout << endl
				 << "(verbose)- Let's iterate all file and get the different combination of predicates:"
				 << endl;
		}

		int num_infrequentPreds = 0;
		int num_cutPreds = 0;
		// check infrequent predicates
		if (includeInfrequentPredicates==false){
			for (size_t i=1;i<=hdt_file1->getDictionary()->getNpredicates();i++){
				size_t numOccs_predicate = hdt_file1->getTriples()->getNumAppearances(i);
				if (verbose)
					cout<<"numOccs predicate "<<i<<" ("<<hdt_file1->getDictionary()->idToString(i,PREDICATE)<<") : "<<numOccs_predicate<<endl;
				if (numOccs_predicate<MAX_INFREQUENT_PREDICATES){
					num_infrequentPreds++;
					infrequent_predicates.insert(i);
				}
				if (cutMassivePredicates && (numOccs_predicate>=MAX_CUT_PREDICATES) ){
					num_cutPreds++;
					cut_predicates.insert(i);
				}
			}
			cout<<endl<< "Total predicates: "<<hdt_file1->getDictionary()->getNpredicates()<<endl;
			cout<< "Infrequent predicates (less than "<<MAX_INFREQUENT_PREDICATES<<") : "<< num_infrequentPreds << endl<<endl;
			cout<< "Max group size, less than "<<MAX_SIZE_GROUP << endl<<endl;
			if (groupFilterSubjects)
				cout<< "Min subject group size, more than "<<MIN_SUBJECT_GROUP << endl<<endl;
			if (cutMassivePredicates){
				cout<< "Cut predicates (>= "<<MAX_CUT_PREDICATES<<") : "<< num_cutPreds << endl<<endl;
			}

			// export metadata of the partitions in JSON
			if (exportMetadata && includeInfrequentPredicatesDedicatedFile)
			{
				ostream *outPred;
				ofstream outFPred;

				// open export file or use standard output
				if (exportFileString != "")
				{
					string expP = exportFileString+"_infreqPreds.json";
					outFPred.open(expP.c_str());
					outPred = &outFPred;



				}
				else
				{
					outPred = &cout;

				}
				dumpInfrequentPredicatesJSON(outPred);
			}

		}


		unsigned int numTriples = 0;
		unsigned int prevSubject = 1, prevPredicate = 0;

		string line="";
		std::unordered_set<string> familiesLog; //mark the families from Log
		if (fromLog==true){
			std::ifstream infile(logFileString);
			char * pch;
			while (std::getline(infile, line))
			{
				// split the line by predicate and check if every pred is in the dictionary
				bool outDict=false;
				string familyRead ="";
				string originalLine = line;
				pch = strtok ((char*)line.c_str(),",");
				while (pch != NULL && outDict==false)
  				{
					unsigned int id = hdt_file1->getDictionary()->stringToId(pch,PREDICATE);
					cout<<"id:"<<id<<endl;	
					if (id==0){
						outDict = true;
						familyRead=""; //one predicate does not exist = no family would be found in the data
					}
					else{
						ostringstream pred;
						pred << id;
						if (familyRead.length() != 0)
                                                	familyRead = familyRead + " " + pred.str();
                                        	else
                                                	familyRead = pred.str();


					}	
					pch = strtok (NULL, ","); //look for next predicate
				}
				if (outDict==false){ //no problem found
					familiesLog.insert(familyRead);
					cout<<"New family from log:"<<familyRead<<endl;
				}
				else{
					cerr<<"[ERROR] Some predicate was not found in the log family: "<<originalLine<<endl;
				}
				
			}
		}


		vector<unsigned int> predsofFamily;				  //temporal

		map<unsigned int, vector<unsigned int>> familiesPerPredicate; // all different families per predicate, i.e, IdPredicate -- > [familyIDs,..]
		string family = "";


		// serializers if split is enabled, one per family
		// uncomment to reuse the serializers, but it is problematic with many families.
		vector<RDFSerializer *> serializers; // = RDFSerializer::getSerializer(outputFile.c_str(), notation);

		IteratorTripleID *it1 = hdt_file1->getTriples()->searchAll();
		vector<TripleID> triples; // temp just needed if we want to split by family and export in files
		StopWatch timeFirstFamilies;
		while (it1->hasNext())
		{
			TripleID *triple = it1->next();
			numTriples++;

			if (numTriples%10000000==0){
				cout<<numTriples<<" triples"<<endl;
			}
			if (triple->getSubject() != prevSubject)
			{
				if (predsofFamily.size()>0){ // condition to assure that we have something, as maybe all infrequent predicate have been filtered out
					cout<<"check family:"<<family<<"."<<endl;
					if (!fromLog || (familiesLog.find(family)!=familiesLog.end())){ //if we don't look for exising families in a log or we do and the family is in the laog
						cout<<"NEW family:"<<family<<endl;
						unsigned int familyID = families[family];
						if (familyID == 0)
						{ // new family
							familyID = numFamilies;
							families[family] = familyID;
							predicatesPerFamily.push_back(predsofFamily); //store the predicatesIDs of the family

							numFamilies++;

							for (int i = 0; i < predsofFamily.size(); i++)
							{ //store this new family for each predicate
								familiesPerPredicate[predsofFamily[i]].push_back(
									familyID);
							}

							if (groupFilterSubjects){ //just count the triples but do not split in files (very costly)
								numTriplesperFamily[familyID]=triples.size();
								triples.clear();
							}
							else if (split)
							{

								numTriplesperFamily[familyID]=triples.size();
								estimatedNumTriplesperFamily[familyID]=triples.size(); // add the same estimation so far
	
								string newSplitFile = splitFileString + to_string(familyID) + ".nt";									  // open file to dump the triples of this partition
								RDFSerializer *serializer = RDFSerializer::getSerializer(newSplitFile.c_str(), NTRIPLES);				  // create new serializer
								VectorIDIteratorTripleString *it = new VectorIDIteratorTripleString(triples, hdt_file1->getDictionary()); // create iterator of triples in the partition
								serializer->serialize(it, NULL, it->estimatedNumResults());
								// serialize the triples in the partition
								// uncomment to reuse the serializers, but it is problematic with many families.
								if (open_files<MAX_OPEN_FILES){
									serializers.push_back(serializer);																		  // keep the serializer in case there are more triples in this partition
									open_files++;
									 if (open_files%1000==0){
														cout<<open_files<<" open files"<<endl;
												}

								}
								else{
									delete serializer; //no needed if serializers are reused
								}
								triples.clear();																						  // empty the triples in the partitions
							}
	
							if (verbose)
								cout << "NEW family with id "<<familyID<<": " << family << endl;
						} //else we have an existing family
						else if (groupFilterSubjects){ //just count the triples but do not split in files (very costly)
							numTriplesperFamily[familyID] = numTriplesperFamily[familyID] + triples.size();
							triples.clear();
						}
						else if (split)
						{
							numTriplesperFamily[familyID] = numTriplesperFamily[familyID] + triples.size();
							estimatedNumTriplesperFamily[familyID] = estimatedNumTriplesperFamily[familyID] + triples.size(); // add the same estimation so far
							string newSplitFile = splitFileString + to_string(familyID) + ".nt";
							// uncomment to reuse the serializers, but it is problematic with many families.
							RDFSerializer *serializer;
							if (familyID>=MAX_OPEN_FILES){
								serializer = RDFSerializer::getSerializer(newSplitFile.c_str(), NTRIPLES);				  // create new serializer
							}
							else{
								serializer = serializers[familyID - 1];	// retrieve the existing serializer for the partition
							}
							VectorIDIteratorTripleString *it = new VectorIDIteratorTripleString(triples, hdt_file1->getDictionary()); // create iterator of triples in the partition
							serializer->serialize(it, NULL, it->estimatedNumResults());		// serialize the triples in the partition
							if (familyID>=MAX_OPEN_FILES){
								delete serializer; //no needed if serializers are reused
							}
							triples.clear();																						  // empty the triples in the partitions
						}

						subjectsPerFamilies[familyID].push_back(prevSubject);
					}
				}
				predsofFamily.clear();
				family = "";
				prevPredicate = 0;

			}
			bool predbanned=false;
			if (!includeInfrequentPredicates && (infrequent_predicates.find(triple->getPredicate())!=infrequent_predicates.end()))
				predbanned=true;
			else if (cutMassivePredicates && (cut_predicates.find(triple->getPredicate())!=cut_predicates.end())){
				predbanned=true;
			}

			if (!predbanned){
				triples.push_back(*triple); // save triple for the next batch of partitions
				if (triple->getPredicate() != prevPredicate)
				{
					ostringstream pred;
					pred << triple->getPredicate();

					if (family.length() != 0)
						family = family + " " + pred.str();
					else
						family = pred.str();
					predsofFamily.push_back(triple->getPredicate());
				}
			}
			prevSubject = triple->getSubject();
			prevPredicate = triple->getPredicate();
		}
		delete it1;



		//store the last subject
		if (predsofFamily.size()>0){ // condition to assure that we have something, as maybe all infrequent predicate have been filtered out
			cout<<"check family:"<<family<<"."<<endl;
			if (!fromLog || (familiesLog.find(family)!=familiesLog.end())){ //if we don't look for exising families in a log or we do and the family is in the laog
				cout<<"NEW family:"<<family<<endl;
				unsigned int familyID = families[family];
				if (familyID == 0)
				{ // new family
					familyID = numFamilies;
					families[family] = familyID;
					predicatesPerFamily.push_back(predsofFamily); //store the predicatesIDs of the family
	
					numFamilies++;
	
					for (int i = 0; i < predsofFamily.size(); i++)
					{ //store this new family for each predicate
						familiesPerPredicate[predsofFamily[i]].push_back(familyID);
					}
	
					if (groupFilterSubjects){ //just count the triples but do not split in files (very costly)
						numTriplesperFamily[familyID]=triples.size();
						triples.clear();
					}
					else if (split)
					{
						numTriplesperFamily[familyID]=triples.size();
						estimatedNumTriplesperFamily[familyID]=triples.size();
	
						string newSplitFile = splitFileString + to_string(familyID) + ".nt";									  // open file to dump the triples of this partition
						RDFSerializer *serializer = RDFSerializer::getSerializer(newSplitFile.c_str(), NTRIPLES);				  // create new serializer
						VectorIDIteratorTripleString *it = new VectorIDIteratorTripleString(triples, hdt_file1->getDictionary()); // create iterator of triples in the partition
						serializer->serialize(it, NULL, it->estimatedNumResults());												  // serialize the triples in the partition
						delete serializer; //no needed if serializers are reused
						triples.clear();																						  // empty the triples in the partitions
					}
				}//else we have an existing family
				else if (groupFilterSubjects){ //just count the triples but do not split in files (very costly)
					numTriplesperFamily[familyID] = numTriplesperFamily[familyID] + triples.size();
					triples.clear();
				}
				else if (split)
				{
					string newSplitFile = splitFileString + to_string(familyID) + ".nt";
					numTriplesperFamily[familyID] = numTriplesperFamily[familyID] + triples.size();
					estimatedNumTriplesperFamily[familyID] = estimatedNumTriplesperFamily[familyID] + triples.size();
					RDFSerializer *serializer;
					if (familyID>=MAX_OPEN_FILES){
						serializer = RDFSerializer::getSerializer(newSplitFile.c_str(), NTRIPLES);				  // create new serializer
					}
					else{
						serializer = serializers[familyID - 1];	// retrieve the existing serializer for the partition
					}
	
					VectorIDIteratorTripleString *it = new VectorIDIteratorTripleString(triples, hdt_file1->getDictionary()); // create iterator of triples in the partition
					serializer->serialize(it, NULL, it->estimatedNumResults());												  // serialize the triples in the partition
	
					if (familyID>=MAX_OPEN_FILES){
						delete serializer; //no needed if serializers are reused
					}
					triples.clear();																						  // empty the triples in the partitions
				}
				subjectsPerFamilies[familyID].push_back(prevSubject);
			}
		}
			cerr << " Time first families" << timeFirstFamilies<< endl;
		//test for histograms
		cout<<" Number of families:"<<predicatesPerFamily.size()<<endl;
		/*
		cout<<"family;triples;subjects"<<endl;
		for (int i=0;i<predicatesPerFamily.size();i++){
				cout<<i<<";"<<numTriplesperFamily[i+1]<<";"<<subjectsPerFamilies[i+1].size()<<endl;
		}*/


		if (verbose){
			cout<< "deleting serializers"<<endl;
		}
		for (int i=0;i<serializers.size();i++){
			delete serializers[i];
			open_files=0;
		}
		if (verbose){
			cout<< "export Metadata"<<endl;
		}



		if (groupFilterSubjects){ // if we restrict the families with a min number of subjects, then first identify these families
			cout<<"Get the families with at least "<<MIN_SUBJECT_GROUP<<" triples"<<endl;

			//first check the families with more predicates
			for (int i=0;i<predicatesPerFamily.size();i++){
				if (subjectsPerFamilies[i+1].size()>=11399){ //0.01% of number of subjects
						familiesLargerSubjects.insert(i+1);
				}
			  //  cout<<(i+1)<<";"<<numTriplesperFamily[i+1]<<";"<<subjectsPerFamilies[i+1].size()<<";"<<predicatesPerFamily[i].size()<<endl;
			}
			cout<<familiesLargerSubjects.size()<< " families present in >= "<<MIN_SUBJECT_GROUP<<" ("<<PERCENTAGE_SUBJECT_GROUPS<<" percent of the subjects)"<<endl<<endl;

			//numFamilies=familiesLargerSubjects.size();
		}



		size_t initial_size_without_grouping = predicatesPerFamily.size();

		StopWatch timeGroup;
		if (groupPredicates){ //grouping activated



			if (groupFilterSubjects){ // if we group restricting to a minimum number of covered subjects

				// identify the merges from all families to the selected familiesLargerSubjects

				unsigned int numFound=0;
				for (int j=0;j<predicatesPerFamily.size();j++){
					bool found=false;
					for (auto itr = familiesLargerSubjects.begin(); itr != familiesLargerSubjects.end(); ++itr) {
							unsigned int targetFamily= *itr;

					//for (int i=0;i<familiesLargerSubjects.size();i++){

						vector<unsigned int> intersect;
						if (targetFamily!=(j+1)){ //otherwise it is just the same
							set_intersection(predicatesPerFamily[targetFamily-1].begin(), predicatesPerFamily[targetFamily-1].end(),
											predicatesPerFamily[j].begin(), predicatesPerFamily[j].end(),
											std::inserter(intersect, intersect.begin()));
						}
						else{
							// for the identity of the larger families, we keep an identity map family--> family in order to output it.
							mergedFamilies[(j+1)].insert(j+1);
							found=true;
						}
						if (intersect.size()>0){
							found=true;


							string family_intersect = getFamilyStr(&intersect);

							if (families[family_intersect]!=0){
							// the target intersect family exists
								if (familiesLargerSubjects.find(families[family_intersect])!=familiesLargerSubjects.end()){
									// it is already a larger family, merge into that
									unordered_set<unsigned int>::const_iterator got =familiesLargerSubjects.find(families[family_intersect]);
									unsigned int destinationMergeFam =*got;
									mergedFamilies[(j+1)].insert(destinationMergeFam);
									groupedFamily.insert(destinationMergeFam); //mark the family as grouped
									sourceFamilies[destinationMergeFam].insert(j+1);
								}
								else if (familiesLargerSubjects_fromMerge.find(families[family_intersect])!=familiesLargerSubjects_fromMerge.end()){
									// it already exists as a merged family but not an original larger one

									unordered_set<unsigned int>::const_iterator got =familiesLargerSubjects_fromMerge.find(families[family_intersect]);
									unsigned int destinationMergeFam =*got;
									mergedFamilies[(j+1)].insert(destinationMergeFam);
									groupedFamily.insert(destinationMergeFam); //mark the family as grouped
									sourceFamilies[destinationMergeFam].insert(j+1);
								}
								else { // it is a small family that exists --> add it as familiesLargerSubjects_fromMerge
										unsigned int destinationMergeFam = families[family_intersect];
										mergedFamilies[(j+1)].insert(destinationMergeFam);
										groupedFamily.insert(destinationMergeFam); //mark the family as grouped
										sourceFamilies[destinationMergeFam].insert(j+1);
										familiesLargerSubjects_fromMerge.insert(destinationMergeFam);

								}

							}
							else{
								// totally new family
								unsigned int familyID = numFamilies;
								numFamilies++;
								families[family_intersect] = familyID; //insert new family string in the map
								groupedFamily.insert(familyID); // mark the family as a grouped family
								//cout<<"families[family_intersect]:"<<families[family_intersect]<<endl;
								predicatesPerFamily.push_back(intersect); // insert the family in the vector
								//insert both merged families into map of grouped families
								mergedFamilies[(j+1)].insert(familyID);
								mergedFamilies[targetFamily].insert(familyID);
								sourceFamilies[familyID].insert(j+1);
								sourceFamilies[familyID].insert(targetFamily);
								familiesLargerSubjects_fromMerge.insert(familyID);
							}
						}

					}
					if (found)
						numFound++;
					else{
						// if an intersection is not found, we keep an identity map family--> family in order to output it.
						familiesLargerSubjects_fromMerge.insert(j+1);
						mergedFamilies[(j+1)].insert(j+1);

					}
					if ((j%1000)==0)
						cout<<j<<" families processed"<<endl;
				}
				familiesLargerSubjects_total.insert(familiesLargerSubjects_total.end(), familiesLargerSubjects.begin(), familiesLargerSubjects.end());
				//familiesLargerSubjects_total =familiesLargerSubjects;
				familiesLargerSubjects_total.insert(familiesLargerSubjects_total.end(), familiesLargerSubjects_fromMerge.begin(), familiesLargerSubjects_fromMerge.end());
	//			for (int i=0;i<familiesLargerSubjects_fromMerge.size();i++)
		//			familiesLargerSubjects_total.push_back(familiesLargerSubjects_fromMerge[i]);
				numFamilies=familiesLargerSubjects_total.size();


				// create a mapping with the final id
				for (int i = 0; i < familiesLargerSubjects_total.size(); i++){ // iterate families
					subjectfamiliesToFinalID[familiesLargerSubjects_total[i]]=i+1;
				}


				cout<<endl<<"numFound intersects:"<<numFound<<" out of "<<predicatesPerFamily.size()<<" families"<<endl<<endl;
				cout<<endl<<"new Families:"<<familiesLargerSubjects_fromMerge.size()<<endl<<endl;

				initial_size_without_grouping = predicatesPerFamily.size();
				// prepare auxiliary structures, familyToMap
				cout<<"prepare auxiliary structures, familyToMap"<<endl;
				for (int i=0;i<initial_size_without_grouping;i++){
					predToMergeFam.clear();
					addPredicatesToMergeNoRecursive((i+1));
					if (i%1000==0)
						cout<<"Completing the predicates of family:"<<i<<endl;
					}

				if (verbose){
					cout<<"test output of familyToMap"<<endl;
					for (std::map<unsigned int,map<unsigned int,std::unordered_set<unsigned int>>>::iterator it=familyToMap.begin(); it!=familyToMap.end(); ++it){
						std::cout << it->first << " => " ;
						for (std::map<unsigned int,std::unordered_set<unsigned int>>::iterator it2=it->second.begin(); it2!=it->second.end(); ++it2){
						//for (int i=0;i<it->second.size();i++){
							cout<< it2->first<< "( -> ";
							for (auto itr = it2->second.begin(); itr != it2->second.end(); ++itr) {
								cout<< *itr<< " ";
							}
							cout<<")   ; ";
						}
						cout<<endl;
					}
				}

			}
			else{ //normal grouping
				cout<< " Initial groups: "<<initial_size_without_grouping<<endl;

				//grouping
				size_t num_new_groups = group(0,initial_size_without_grouping);

				cout<< "Total potential new groups: "<<num_new_groups<<endl;
				//cout<< "- Max group size: "<<max_size_group<<endl;

				if (verbose){
					for (int i=initial_size_without_grouping;i<initial_size_without_grouping+num_new_groups;i++){
						cout<< "New family  "<<i+1<<": ";
						std::vector<unsigned int>::iterator it;
						for (it = predicatesPerFamily[i].begin(); it != predicatesPerFamily[i].end(); ++it)
						{
						   cout<<*it<<" ";
						}
						cout<<endl;
					}

					for (std::map<unsigned int,std::unordered_set<unsigned int>>::iterator it=mergedFamilies.begin(); it!=mergedFamilies.end(); ++it){
						std::cout << it->first << " => " ;
						for (auto itr = it->second.begin(); itr != it->second.end(); ++itr) {
						//for (int i=0;i<it->second.size();i++){
							cout<< *itr<< " ";
						}
						cout<<endl;
					}
				}

				cerr << " Time grouping" << timeGroup<< endl;

				//iterate all triples again and dump new groups

				// first, prepare the structures to help in the process
				//cout<<"Test families"<<endl;
				for (int i=0;i<initial_size_without_grouping;i++){
					predToMergeFam.clear();
					addPredicatesToMerge((i+1));
					//familyToMap[(i+1)] = predToMergeFam;
					if (i%1000==0)
						cout<<"Completing the predicates of family:"<<i<<endl;
	//				cout<<"previous familyToMap[i] size:"<<familyToMap[i].size()<<endl;
	//				cout<<"current familyToMap[i+1] size:"<<familyToMap[i+1].size()<<endl;
				}

				if (verbose){
				cout<<"test output of familyToMap"<<endl;
				for (std::map<unsigned int,map<unsigned int,std::unordered_set<unsigned int>>>::iterator it=familyToMap.begin(); it!=familyToMap.end(); ++it){
						std::cout << it->first << " => " ;
						for (std::map<unsigned int,std::unordered_set<unsigned int>>::iterator it2=it->second.begin(); it2!=it->second.end(); ++it2){
						//for (int i=0;i<it->second.size();i++){
							cout<< it2->first<< "( -> ";
							for (auto itr = it2->second.begin(); itr != it2->second.end(); ++itr) {

								cout<< *itr<< " ";
							}
							cout<<")   ; ";
						}
						cout<<endl;
					}
				}
			}
			if (groupFilterSubjects){
				initial_size_without_grouping=0; //just to be consistent with the case of the grouping
			}

			bool jumpSubject=false;
			prevSubject=0;
			numTriples=0;
			std::unordered_set<unsigned int> tempfamsWithSubject;
			map<unsigned int,RDFSerializer *> newserializers; //family--> serializer
			map<unsigned int,vector<TripleID>> buffer_dump; //a buffer of triples to dump, from familyID-->set of Triples;

			//do a first pass to compute the exact stats of the number of triples and decide if a family is not materialized

			StopWatch timeFirstPassStats;

			cout<<"do a first pass to compute the exact stats of the number of triples and decide if a family is not materialized"<<endl;
			IteratorTripleID *itFirst = hdt_file1->getTriples()->searchAll();

			map<string,unsigned int> mapFilterInfrequent; //a temporary structure to speed up filtering the infrequent predicate. From a list of preds to a list of preds without the infreq. predicate
			map<string,unsigned int>::iterator itString;
			while (itFirst->hasNext())
			{
				TripleID *triple = itFirst->next();

				numTriples++;
				if (numTriples%100000==0){
					cout<<numTriples<<" triples"<<endl;
				}

				//cout<<"subject "<<triple->getSubject()<<endl;
				if (triple->getSubject() != prevSubject){
					jumpSubject=false;
					predToMergeFam.clear();
					vector<unsigned int> predicates = ((BitmapTriplesSearchIterator*)itFirst)->getPredicates();
					unsigned int famID=0;
					// filter infrequent predicates
					if (!includeInfrequentPredicates){
						string fam = getFamilyStr(&predicates);
						itString = mapFilterInfrequent.find(fam);
						if(itString != mapFilterInfrequent.end()){ //exists in cache, get the filter
							famID=itString->second;
						}
						else{
							vector<unsigned int>::iterator iter = predicates.begin();
							while (iter != predicates.end())
							{
								if (infrequent_predicates.find(*iter)!=infrequent_predicates.end()){
									// erase returns the new iterator
									iter = predicates.erase(iter);
								}
								else if (cut_predicates.find(*iter)!=cut_predicates.end()){
									// erase returns the new iterator
									iter = predicates.erase(iter);
								}
								else
								{
									++iter;
								}
							}
							string family = getFamilyStr(&predicates);
							famID = families[family];
							mapFilterInfrequent[fam]=famID;

						}
					}

					if (mergedFamilies[famID].size()!=0){ //if it goes to a merge family
						if (familyToMap[famID].size()!=0){ //already processed, reuse
							predToMergeFam = familyToMap[famID];
						}
					}
					else{
						jumpSubject=true;
					}
				}
				if (!jumpSubject){
					if (predToMergeFam[triple->getPredicate()].size()!=0){
						for (auto itr = predToMergeFam[triple->getPredicate()].begin(); itr != predToMergeFam[triple->getPredicate()].end(); ++itr) {
							unsigned int familyMergeID = *itr;
							estimatedNumTriplesperFamily[familyMergeID] = estimatedNumTriplesperFamily[familyMergeID] + 1;
						}
					}
				}
				prevSubject = triple->getSubject();
			}
			delete itFirst;

			cerr << " Time first pass stats" << timeFirstPassStats<< endl;

			// now iterate and mark the banned families
			banFamilies();

			// do a second iteration but avoid banned families

			prevSubject=0;

			IteratorTripleID *itSecond = hdt_file1->getTriples()->searchAll();

			numTriples=0;

			StopWatch timeSecondpass;

			while (itSecond->hasNext())
			{
				TripleID *triple = itSecond->next();

				numTriples++;
				if (numTriples%100000==0){
					cout<<numTriples<<" triples"<<endl;
				}

				if (triple->getSubject() != prevSubject){
					jumpSubject=false;
					predToMergeFam.clear();
					vector<unsigned int> predicates = ((BitmapTriplesSearchIterator*)itSecond)->getPredicates();
					unsigned int famID=0;
					// filter infrequent predicates
					if (!includeInfrequentPredicates){
						string fam = getFamilyStr(&predicates);
						itString = mapFilterInfrequent.find(fam);
						if(itString != mapFilterInfrequent.end()){ //exists in cache, get the filter
							famID=itString->second;
						}
						else{
							vector<unsigned int>::iterator iter = predicates.begin();
							while (iter != predicates.end())
							{
								if (infrequent_predicates.find(*iter)!=infrequent_predicates.end()){
									// erase returns the new iterator
									iter = predicates.erase(iter);
								}
								else if (cut_predicates.find(*iter)!=cut_predicates.end()){
									// erase returns the new iterator
									iter = predicates.erase(iter);
								}
								else
								{
									++iter;
								}
							}
							string family = getFamilyStr(&predicates);
							famID = families[family];
							mapFilterInfrequent[fam]=famID;

						}
					}

					if (mergedFamilies[famID].size()!=0){ //if it goes to a merge family
						if (familyToMap[famID].size()!=0){ //already processed, reuse
							predToMergeFam = familyToMap[famID];
						}
					}
					else{
						jumpSubject=true;
					}
					for (auto itr = tempfamsWithSubject.begin(); itr != tempfamsWithSubject.end(); ++itr) {
						//for (int i=0;i<tempfamsWithSubject.size();i++){
						// update subject per family
						subjectsPerFamilies[*itr].push_back(prevSubject);
					}
					tempfamsWithSubject.clear();


				}
				if (!jumpSubject){
					if (predToMergeFam[triple->getPredicate()].size()!=0){
						for (auto itr = predToMergeFam[triple->getPredicate()].begin(); itr != predToMergeFam[triple->getPredicate()].end(); ++itr) {
					//	for (int i=0;i<predToMergeFam[triple->getPredicate()].size();i++){ // look if we need to dump the predicate to a target merged family. if size =0, do nothing
							unsigned int familyMergeID = *itr;
							if (bannedFamily_largeGroups_ID.find(familyMergeID)==bannedFamily_largeGroups_ID.end()){
							//not banned
								//cout<< *triple << " will go to "<<familyMergeID<<endl;
								 //cout<<"familyMergeID:"<<familyMergeID<<endl;
								tempfamsWithSubject.insert(familyMergeID);



								numTriplesperFamily[familyMergeID] = numTriplesperFamily[familyMergeID] + 1;

								buffer_dump[familyMergeID].push_back(*triple);

								if (buffer_dump[familyMergeID].size()>MAX_BUFFER_DUMP){

									string newSplitFile = splitFileString + to_string(familyMergeID) + ".nt";
									if (groupFilterSubjects){ // take into account the final ID, correlative
										//cout<<"look for family familyMergeID:";
										newSplitFile = splitFileString + to_string(subjectfamiliesToFinalID[familyMergeID]) + ".nt";
										//cout<<"found:"<<subjectfamiliesToFinalID[familyMergeID]<<endl;
									}

									//cout<<"newSplitFile would be: "<<newSplitFile<<endl;

									// uncomment to reuse the serializers, but it is problematic with many families.
									RDFSerializer *serializer;
									bool newSerializer=false;
									unsigned int numNew = familyMergeID-initial_size_without_grouping;
									//if (numNew<newserializers.size()){
										//cout<<"search serializer "<<numNew<<endl;
										 if (newserializers.find(numNew)!=newserializers.end()){
										//	cout<<"retrieve existing serializer "<<numNew<<endl;
											serializer = newserializers.find(numNew)->second;	// retrieve the existing serializer for the partition
										}
										 else{
											 serializer = RDFSerializer::getSerializer(newSplitFile.c_str(), NTRIPLES);				  // create new serializer
											 newSerializer=true;
										 }
								/*	}
									else {
										serializer = RDFSerializer::getSerializer(newSplitFile.c_str(), NTRIPLES);				  // create new serializer
										newSerializer=true;
									}*/



									VectorIDIteratorTripleString *it = new VectorIDIteratorTripleString(buffer_dump[familyMergeID], hdt_file1->getDictionary()); // create iterator of triples in the partition
									//cout<<"serialize"<<endl;
									//cout<<"it->estimatedNumResults():"<<it->estimatedNumResults()<<endl;
									serializer->serialize(it, NULL, it->estimatedNumResults());		// serialize the triples in the partition
									//cout<<"end serialize"<<endl;
									if (newSerializer && numNew>=MAX_OPEN_FILES){
										//cout<<"delete serializer"<<endl;
										delete serializer; //no needed if serializers are reused
									}
									else if (newSerializer){
										//cout<< "    saving new serializer at pos "<< newserializers.size()<<endl;
										newserializers[numNew]=serializer;																		  // keep the serializer in case there are more triples in this partition
										open_files++;
									}
									buffer_dump[familyMergeID].clear();
								}
							}

						}
					}


				}
				prevSubject = triple->getSubject();
			}
			delete itSecond;
			// update subject per family, for the last subject

			for (auto itr = tempfamsWithSubject.begin(); itr != tempfamsWithSubject.end(); ++itr) {


			//for (int i=0;i<tempfamsWithSubject.size();i++){
				subjectsPerFamilies[*itr].push_back(prevSubject);
			}
			tempfamsWithSubject.clear();

			cerr << " Time second pass " << timeSecondpass<< endl;

			StopWatch timeDumpBuffer;

			// dump the remaining buffer
			for (std::map<unsigned int,vector<TripleID>>::iterator itr=buffer_dump.begin(); itr!=buffer_dump.end(); ++itr){
				//cout<<"dump the remaining buffer"<<endl;
				unsigned int famMergeID = itr->first;


				RDFSerializer *serializer;
				bool newSerializer=false;
				unsigned int numNew = famMergeID-initial_size_without_grouping;
				string newSplitFile = splitFileString + to_string(famMergeID) + ".nt";
				if (groupFilterSubjects){ // take into account the final ID, correlative
					newSplitFile = splitFileString + to_string(subjectfamiliesToFinalID[famMergeID]) + ".nt";
				}

				 if (newserializers.find(numNew)!=newserializers.end()){

					serializer = newserializers.find(numNew)->second;	// retrieve the existing serializer for the partition
				}
				 else{
					 serializer = RDFSerializer::getSerializer(newSplitFile.c_str(), NTRIPLES);				  // create new serializer
					 newSerializer=true;
				 }



				VectorIDIteratorTripleString *it = new VectorIDIteratorTripleString(itr->second, hdt_file1->getDictionary()); // create iterator of triples in the partition

				serializer->serialize(it, NULL, it->estimatedNumResults());		// serialize the triples in the partition
				//cout<<"end serialize"<<endl;
				if (newSerializer && numNew>=MAX_OPEN_FILES){
					//cout<<"delete serializer"<<endl;
					delete serializer; //no needed if serializers are reused
				}
				else if (newSerializer){
					//cout<< "    saving new serializer at pos "<< newserializers.size()<<endl;
					newserializers[numNew]=serializer;																		  // keep the serializer in case there are more triples in this partition
					open_files++;
				}
				buffer_dump[famMergeID].clear();

			}
			// dump the cache in the serializers (otherwise small files won't be updated)
			for (int i=0;i<newserializers.size();i++){
				delete newserializers[i];
				open_files=0;
			}
			cerr << " Time dump buffer " << timeDumpBuffer<< endl;

		}



		// export metadata of the partitions in JSON
		if (exportMetadata)
		{
			ostream *out;
			ofstream outF;

			ostream *outGroup;
			ofstream outFGroup;

			// open export file or use standard output
			if (exportFileString != "")
			{
				string exp = exportFileString+".json";
				outF.open(exp.c_str());
				out = &outF;



			}
			else
			{
				out = &cout;

			}
			bool markGroup=false;
			if (groupPredicates && !separateGroupExport){
				markGroup =true;
				if (groupFilterSubjects){
					dumpFamiliesJSONFilterSubjects(splitFileString, out,markGroup);
				}else{
				// dump all families in the same JSON and mark with true the ones grouped
					dumpFamiliesJSON(splitFileString, out,0,predicatesPerFamily.size(),markGroup,initial_size_without_grouping);
				}
			}
			else{
				if (groupFilterSubjects){
					cerr<<"unsupported separateGroupExport option with groupFilterSubjects"<<endl;
					exit(1);
				}
				// export in two files
				if (exportFileString!=""){
					string groupFilename = exportFileString+"_group.json";
					outFGroup.open(groupFilename.c_str());
					outGroup = &outFGroup;
				}
				else{
					outGroup = &cout;
				}
				// dump main families
				dumpFamiliesJSON(splitFileString, out,0,initial_size_without_grouping,markGroup,initial_size_without_grouping);
				// dump grouped families
				dumpFamiliesJSON(splitFileString, outGroup,initial_size_without_grouping,predicatesPerFamily.size(),markGroup,initial_size_without_grouping);
			}
		}



		// verbose metadata, not updated for grouped familie
		if (verbose)
		{
			cerr<< "verbose metadata, not updated for grouped families"<<endl;
			cout << endl
				 << endl
				 << "* Print current structures" << endl;
			cout << "- Family ID-> included predicates" << endl;
			for (int i = 0; i < predicatesPerFamily.size(); i++)
			{
				cout << (i + 1) << "-> ";
				vector<unsigned int> predicates = predicatesPerFamily[i];
				for (int j = 0; j < predicates.size(); j++)
				{
					cout
						<< hdt_file1->getDictionary()->idToString(
							   predicates[j], PREDICATE);
					if (j != predicates.size() - 1)
						cout << " , ";
				}
				cout << endl;
			}
			cout << endl;

			cout << "- Family ID: subjects in that family" << endl;
			for (int i = 1; i <= subjectsPerFamilies.size(); i++)
			{
				cout << i << "-> ";
				vector<unsigned int> subjects = subjectsPerFamilies[i];
				for (int j = 0; j < subjects.size(); j++)
				{
					cout
						<< hdt_file1->getDictionary()->idToString(
							   subjects[j], SUBJECT);
					if (j != subjects.size() - 1)
						cout << " , ";
				}
				cout << endl;
			}

			cout << "- Predicate ID: Families including such predicate" << endl;
			for (int i = 1; i <= familiesPerPredicate.size(); i++)
			{
				cout << i << "-> ";
				vector<unsigned int> families = familiesPerPredicate[i];
				for (int j = 0; j < families.size(); j++)
				{
					cout << families[j];
					if (j != families.size() - 1)
						cout << " , ";
				}
				cout << endl;
			}
		}

		cerr << " Time total " << timeTotal<< endl;


		//todo: Sort each structure
		cout << "TODO>>>> Create one HDT per file in the partitions" << endl;


		delete hdt_file1;
	}
	catch (std::exception &e)
	{
		cerr << "ERROR!: " << e.what() << endl;
	}
}

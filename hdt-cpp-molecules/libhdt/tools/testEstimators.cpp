
#include <iomanip>

#include <HDTVersion.hpp>
#include <HDT.hpp>
#include <HDTManager.hpp>
#include <signal.h>

#include <getopt.h>
#include <string.h>
#include <string>
#include <cstdint>
#include <iostream>
#include <fstream>
#include "../src/util/StopWatch.hpp"

using namespace hdt;
using namespace std;

int interruptSignal = 0;

void signalHandler(int sig)
{
	interruptSignal = 1;
}

void help() {
	cout << "$ testEstimators [options] <hdtfile> " << endl;
	cout << "\t-h\t\t\tThis help" << endl;
	cout << "\t-q\t<query>\t\tLaunch query and exit." << endl;
	cout << "\t-s\t\t\tSilent run, only show estimation." << endl;
	cout << "\t-o\t<output>\tSave query output to file." << endl;
}

void iterate(HDT *hdt, char *query, ostream &out, bool measure, uint32_t offset, bool silent) {
	TripleString tripleString;
	tripleString.read(query);

	const char *subj = tripleString.getSubject().c_str();
	const char *pred = tripleString.getPredicate().c_str();
	const char *obj = tripleString.getObject().c_str();
	if(strcmp(subj, "?")==0) {
		subj="";
	}
	if(strcmp(pred, "?")==0) {
		pred="";
	}
	if(strcmp(obj, "?")==0) {
		obj="";
	}

#if 0
	cout << "Subject: |" << subj <<"|"<< endl;
	cout << "Predicate: |" << pred <<"|"<< endl;
	cout << "Object: |" << obj << "|"<<endl;
#endif

	try {
		IteratorTripleString *it = hdt->search(subj, pred, obj);

		cout<<"Total estimated results: "<<it->estimatedNumResults() << endl;
		cout<<"Subject estimated results: "<<it->estimatedNumSubjects() << endl;
		cout<<"Predicate estimated results: "<<it->estimatedNumPredicates() << endl;
		cout<<"Object estimated results: "<<it->estimatedNumObjects() << endl;

	if (!silent){
		StopWatch st;

        // Go to the right offset.
        if(it->canGoTo()) {
            try {
                it->skip(offset);
                offset = 0;
            }
            catch (const runtime_error error) {
                /*invalid offset*/
                interruptSignal = 1;
            }
        }
        else {
            while(offset && it->hasNext()) {
                it->next();
                offset--;
            }
        }

        // Get results.
		size_t numTriples=0;
		while(it->hasNext() && interruptSignal==0) {
			TripleString *ts = it->next();
			if(!measure)
				out << *ts << '\n';
			numTriples++;
		}
		cerr << numTriples << " results in " << st << endl;
		delete it;
	}
		interruptSignal=0;	// Interrupt caught, enable again.
	} catch (std::exception& e) {
		cerr << e.what() << endl;
	}

}

int main(int argc, char **argv) {
	int c;
	string query, inputFile, outputFile;
    stringstream sstream;
    uint32_t offset = 0;
	bool measure = false;
	bool silent = false;

	while( (c = getopt(argc,argv,"hq:o:s"))!=-1) {
		switch(c) {
		case 'h':
			help();
			break;
		case 'q':
			query = optarg;
			break;
		case 'o':
			outputFile = optarg;
			break;
		case 's':
			silent = true;
			break;
		default:
			cerr << "ERROR: Unknown option" << endl;
			help();
			return 1;
		}
	}

	if(argc-optind<1) {
		cerr << "ERROR: You must supply an HDT File" << endl << endl;
		help();
		return 1;
	}

	inputFile = argv[optind];


	try {
		StdoutProgressListener prog;
		HDT *hdt = HDTManager::mapIndexedHDT(inputFile.c_str(), &prog);

		ostream *out;
		ofstream outF;

		if(outputFile!="") {
			outF.open(outputFile.c_str());
			out = &outF;
		} else {
			out = &cout;
		}

		if(query!="") {
			// Supplied query, search and exit.
			iterate(hdt, (char*)query.c_str(), *out, measure, offset,silent);
		} else {
			// No supplied query, show terminal.
			char line[1024*10];

			signal(SIGINT, &signalHandler);
			cerr << "                                                 \r>> ";
			while(cin.getline(line, 1024*10)) {
				if(strcmp(line, "exit")==0|| strcmp(line,"quit")==0) {
					break;
				}
				if(strlen(line)==0 || strcmp(line, "help")==0) {
					cerr << "Please type Triple Search Pattern, using '?' for wildcards. e.g " << endl;
					cerr << "   http://www.somewhere.com/mysubject ? ?" << endl;
					cerr << "Interrupt with Control+C. Type 'exit', 'quit' or Control+D to exit the shell." << endl;
					cerr << ">> ";
					continue;
				}

				iterate(hdt, line, *out, measure, offset,silent);

				cerr << ">> ";
			}
		}

		if(outputFile!="") {
			outF.close();
		}

		delete hdt;
	} catch (std::exception& e) {
		cerr << "ERROR: " << e.what() << endl;
		return 1;
	}
}

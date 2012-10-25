
#include <iostream>
#include <fstream>
#include <cassert>
#include <vector>
#include <string>
#include "Util.h"
#include "Alignments.h"

using namespace std;
using namespace Moses;

void createXML(const string &source, const string &input, const string &target, const string &align, const string &path );

int main(int argc, char **argv)
{
  assert(argc == 2);

  string inPath(argv[1]);

  ifstream inStrme(inPath.c_str());
  ofstream rule((inPath + ".extract").c_str());
  ofstream ruleInv((inPath + ".extract.inv").c_str());

  int setenceId;
  float score;
  string source, target, align, path;
  string *input = NULL;
  int count;

  string inLine;
  
  int step = 0;
  while (!inStrme.eof())
  {
    getline(inStrme, inLine);
    cout << inLine << endl;
    switch (step)
    {
    case 0:
      setenceId = Scan<int>(inLine);
      ++step;
      break;
    case 1:
      score = Scan<float>(inLine);
      ++step;
      break;
    case 2:
      source = inLine;
      ++step;
      break;
    case 3:
      if (input == NULL) {
        input = new string(inLine);
      }
      else {
        assert(inLine == *input);
      }
      ++step;
      break;
    case 4:
      target = inLine;
      ++step;
      break;
    case 5:
      align = inLine;
      ++step;
      break;
    case 6:
      path = inLine + "X";
      ++step;
      break;
    case 7:
      count = Scan<int>(inLine);
      createXML(source, *input, target, align, path);

      step = 0;
      break;

    }


  }

  delete input;
  ruleInv.close();
  rule.close();
  inStrme.close();

}


void createXML(const string &source, const string &input, const string &target, const string &align, const string &path)
{
  vector<string> sourceToks   = Tokenize(source, " ")
                ,inputToks    = Tokenize(input, " ")
                ,targetsToks  = Tokenize(target, " ");
  Alignments alignments(align, sourceToks.size(), targetsToks.size());
  map<int, string> frameInput;
  map<int, int> alignI2S;
  vector< map<string, int> > nonTerms;
  vector<bool> targetBitmap(targetsToks.size(), true);
  vector<bool> inputBitmap;

  // STEP 1: FIND MISMATCHES
  int s = 0, i = 0;
  bool currently_matching = false;
  int start_s = 0, start_i = 0;

  cerr << input << endl << source << endl << target << endl << path << endl;
  for ( size_t p = 0 ; p < path.length() ; p++ ) {
    string action = path.substr(p, 1);

		// beginning of a mismatch
		if ( currently_matching && action != "M" && action != "X" ) {
			start_i            = i;
			start_s            = s;
			currently_matching = 0;
		} // if ( currently_matching
		// end of a mismatch
		else if ( !currently_matching && ( action == "M" || action == "X" ) ) {

			// remove use of affected target words
			for ( int ss = start_s ; ss < s ; ss++ ) {
				const std::map<int, int> &targets = alignments.m_alignS2T[ss];

				std::map<int, int>::const_iterator iter;
				for (iter = targets.begin(); iter != targets.end(); ++iter) {
					size_t tt = iter->first;
					targetBitmap[tt] = 0;
				}

				// also remove enclosed unaligned words?
			} //for ( int ss = start_s ; ss < s ; ss++ ) {

			// are there input words that need to be inserted ?
			cerr << start_i << "<" << i << "?" << endl;
			if (start_i < i ) {

				// take note of input words to be inserted
				string insertion = "";
				for (size_t ii = start_i ; ii < i ; ii++ ) {
					insertion += inputToks[ii] + " ";
				}

				// find position for inserted input words

				// find first removed target word
				int start_t = 1000;
				for ( int ss = start_s ; ss < s ; ss++ ) {
					const std::map<int, int> &targets = alignments.m_alignS2T[ss];

					std::map<int, int>::const_iterator iter;
					for (iter = targets.begin(); iter != targets.end(); ++iter) {
						size_t tt = iter->first;
						if (tt < start_t) {
							start_t = tt;
						}
					}

					// end of sentence? add to end
					if ( start_t == 1000 && i > inputToks.size() - 1 ) {
						start_t = targetsToks.size() - 1;
					}

					// backtrack to previous words if unaligned
					if ( start_t == 1000 ) {
						start_t = -1;
						for ( int ss = s - 1 ; start_t == -1 && ss >= 0 ; ss-- ) {
							const std::map<int, int> &targets = alignments.m_alignS2T[ss];

							std::map<int, int>::const_iterator iter;
							for (iter = targets.begin(); iter != targets.end(); ++iter) {
								size_t tt = iter->first;
								if (tt > start_t) {
									start_t = tt;
								}
							}
						}
					} // if ( start_t == 1000 ) {

					frameInput[start_t] += insertion;
					map<string, int> nt;
					nt["start_t"] = start_t;
					nt["start_i"] = start_i;
					nonTerms.push_back(nt);
				}

				currently_matching = 1;

			} // if (start_i < i ) {

	    cerr << action << " " << s << " " << i
	    		<< "(" << start_s << " " << start_i << ")"
	    		<< currently_matching;

	    if ( action != "I" ) {
	    	cerr << " ->";

	    	const std::map<int, int> &targets = alignments.m_alignS2T[s];

				std::map<int, int>::const_iterator iter;
				for (iter = targets.begin(); iter != targets.end(); ++iter) {
					size_t tt = iter->first;
					cerr << " " << tt;
				}
	    }
	    cerr << endl;

	    if (action != "I")
	    	s++;
	    if (action != "D") {
	    	i++;
	    	alignI2S[i] = s;
	    }

	    if (action == "M") {
	    	inputBitmap.push_back(1);
	    }
	    else if (action == "I" || action == "S") {
	    	inputBitmap.push_back(0);
	    }
		} // else if ( !currently_matching
  } // for ( size_t p = 0

	cerr << target << endl;
	for (size_t i = 0; i < targetBitmap.size(); ++i)
		cerr << targetBitmap[i];
	cerr << endl;

	for (map<int, string>::const_iterator iter = frameInput.begin(); iter != frameInput.end(); ++iter) {
		cerr << iter->first << ":" <<iter->second << endl;
	}

	// STEP 2: BUILD RULE AND FRAME

	// hierarchical rule
	string rule_s     = "";
	int rule_pos_s = 0;
	map<int, int> ruleAlignS;

	for (size_t i = 0 ; i < inputBitmap.size() ; ++i ) {
		if ( inputBitmap[i] ) {
			rule_s += inputToks[i] + " ";
			ruleAlignS[ alignI2S[i] ] = rule_pos_s++;
		}

		for (size_t j = 0; j < nonTerms.size(); ++j) {
			map<string, int> &nt = nonTerms[j];
			if (i == nt["start_i"]) {
				rule_s += "[X][X]";
				nt["rule_pos_s"] = rule_pos_s++;
			}
		}
	}

	string rule_t     = "";
	int rule_pos_t = 0;
	map<int, int> ruleAlignT;

	for (size_t t = -1 ; t < targetBitmap.size(); t++ ) {
		if (t >= 0 && targetBitmap[t]) {
			rule_t += targetsToks[t] + " ";
			ruleAlignT[t] = rule_pos_t++;
		}

		for (size_t i = 0; i < nonTerms.size(); ++i) {
			map<string, int> &nt = nonTerms[i];

			if (t == nt["start_t"]) {
				rule_t += "[X][X] ";
				nt["rule_pos_t"] = rule_pos_t++;
			}
		}
	}

	string ruleAlignment;

	for (map<int, int>::const_iterator iter = ruleAlignT.begin(); iter != ruleAlignT.end(); ++iter) {
		int s = iter->first;
		std::map<int, int> &targets = alignments.m_alignS2T[s];

		std::map<int, int>::const_iterator iter;
		for (iter = targets.begin(); iter != targets.end(); ++iter) {
			int t =iter->first;
			if (ruleAlignT.find(s) == ruleAlignT.end())
				continue;
			ruleAlignment += ruleAlignS[s] + "-" + SPrint(ruleAlignT[t]) + " ";
		}
	}

	for (size_t i = 0; i < nonTerms.size(); ++i) {
		map<string, int> &nt = nonTerms[i];
		ruleAlignment += SPrint(nt["rule_pos_s"]) + "-" + SPrint(nt["rule_pos_t"]) + " ";
	}

	/* TODO
	rule_s = Trim(rule_s);
	rule_t = Trim(rule_t);
	ruleAlignment = Trim(ruleAlignment);
	*/

	string rule_alignment_inv;
	vector<string> ruleAlignmentToks = Tokenize(ruleAlignment, "-");
	for (size_t i = 0; i < ruleAlignmentToks.size(); ++i) {
		const string &alignPoint = ruleAlignmentToks[i];
		vector<string> toks = Tokenize(alignPoint);
		assert(toks.size() == 2);
		rule_alignment_inv += toks[1] + "-" +toks[0];
	}
	//rule_alignment_inv = Trim(rule_alignment_inv); TODO

	// frame
	string frame;
	if (frameInput.find(-1) == frameInput.end())
		frame = frameInput[-1];

	int currently_included = 0;
	int start_t            = -1;
	targetBitmap.push_back(0);


}



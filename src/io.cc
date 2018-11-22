// -*- C++ -*-
/*
  Defines the I/O for both fnTBL and fnTBL-train.

  This file is part of the fnTBL distribution.

  Copyright (c) 2001 Johns Hopkins University and Radu Florian and Grace Ngai.

  Permission is hereby granted, free of charge, to any person obtaining
  a copy of this software, fnTBL version 1.0, and associated
  documentation files (the "Software"), to deal in the Software without
  restriction, including without limitation the rights to use, copy,
  modify, merge, publish, distribute, sublicense, and/or sell copies of
  the Software, and to permit persons to whom the Software is furnished
  to do so, subject to the following conditions:
  
  The above copyright notice and this permission notice shall be
  included in all copies or substantial portions of the Software.
  
  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
  NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
  LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
  OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
  WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.  
*/

#include <list>
#include <iostream>
#include <fstream>
#include "line_splitter.h"
#include "common.h"
#include "index.h"
#include "Params.h"
#include "trie.h"
#include "SubwordPartPredicate.h"
#include "Dictionary.h"
#include "Rule.h"
#include "PrefixSuffixAddPredicate.h"
#include "ContainsStringPredicate.h"
#include "hash_wrapper.h"
#include "CooccurrencePredicate.h"
#include <iterator>

using PredicateTemplate_vector = PredicateTemplate::PredicateTemplate_vector;
using rule_hash_set = HASH_NAMESPACE::hash_set<Rule>;
using position_vector = std::vector<std::pair<int, int>>;
using rule_vector = std::vector<Rule>;
using rulep_vector = std::vector<Rule*>;
using word_index_class = word_index<unsigned int, unsigned short>;

extern int V_flag;

extern wordType3D corpus;
extern wordType3DVector ruleTrace;
extern std::vector<scoreType> costs;
extern rule_hash allRules;
extern word_index_class corpusIndex;
extern word_index_class classifIndex;
extern word_index_class defaultIndex;
extern bool v_flag;

// When this parameter is turned on, indexing techniques are used to speed up the
// evaluation of the rules.
extern bool i_flag;
extern int corpus_size;

extern wordType UNK;

// this function is called at the end of a line to process the words.
void process_line(const string1D& features, int lineNum) {
    // Each sample should have feature_set_size features.
    static short int feature_set_size = RuleTemplate::name_map.size();

    int new_size = features.size() - PredicateTemplate::MaxBackwardLookup + PredicateTemplate::MaxForwardLookup;
    if (new_size > corpus[lineNum].capacity()) {
        wordType1D prev = corpus[lineNum].size() > 0 ? corpus[lineNum][0] : 0;

        corpus[lineNum].resize(new_size);
        auto p = new wordType[corpus[lineNum].capacity() * feature_set_size];

        delete[] prev;

        corpus[lineNum][0] = p;
    }
    else
        corpus[lineNum].resize(new_size);

    for (int j = 0; j < new_size; j++)
        corpus[lineNum][j] = corpus[lineNum][0] + feature_set_size * j;

    ruleTrace[lineNum].resize(new_size);

    static Dictionary& dict = Dictionary::GetDictionary();

    for (int i = 0; i < -PredicateTemplate::MaxBackwardLookup; i++) {
        wordType1D& vect = corpus[lineNum][i];
        if (vect == nullptr) {
            std::cerr << "Error - this vector might have not been 0!" << std::endl
                      << "Line: " << lineNum << ", index " << i << std::endl;
            exit(123);
        }
        std::fill(vect, vect + feature_set_size, dict["ZZZ"]); // ZZZ is our fake feature
        dict.increaseCount("ZZZ", feature_set_size);
    }

    int sample_no = -PredicateTemplate::MaxBackwardLookup, feature_ind = 0;
    for (auto thisFeature = features.begin();
         thisFeature != features.end(); ++thisFeature, feature_ind++) {
        line_splitter ls;
        ls.split(*thisFeature);

        if (ls.size() != feature_set_size) {
            std::cerr << "Example " << lineNum << " does not have " << feature_set_size << " features "
                      << "as it should:" << std::endl
                      << *thisFeature << std::endl
                      << "Please check the data file and restart!" << std::endl;
            exit(5);
        }

        wordType1D& vect = corpus[lineNum][sample_no];
        if (vect == nullptr) {
            std::cerr << "Error - this vector might have not been 0!" << std::endl
                      << "Line: " << lineNum << ", index " << sample_no << std::endl;
            exit(123);
        }

        for (int i = 0; i < feature_set_size; i++) {
            vect[i] = dict.increaseCount(ls[i]);
        }

        sample_no++;
    }

    for (int i = sample_no; i < sample_no + PredicateTemplate::MaxForwardLookup; i++) {
        wordType1D& vect = corpus[lineNum][i];
        if (vect == nullptr) {
            std::cerr << "Error - this vector might have not been 0!" << std::endl
                      << "Line: " << lineNum << ", index " << i << std::endl;
            exit(123);
        }
        std::fill(vect, vect + feature_set_size, dict["ZZZ"]); // ZZZ is our fake feature
        dict.increaseCount("ZZZ", feature_set_size);
    }
}

//  ------------------------------------------------------------------------------------- //
//  This function is called to read in the feature values and initialize the vocabulary.  //
//  It also makes sure that the class features values appear at the beginning of the      //
//  vocabulary and initializes the 2 tries used by PrefixSuffixAddPredicate.              //
//  There are 2 versions of this function: one for training and one for testing. There is //
//  a need for a version in the second case, because the extending should be done only    //
//  for words that have been seen in training. For instance, one rule that could be       //
//  learned is:                                                                           //
//     word_0::++3==++ing => VB                                                           //
//         (if adding 'ing' as a suffix results in a word, make it a verb)                //
//  could lead to wrong results if Beijing was not seen in the training set.              //
//  A real rule that lead to poor results is                                              //
//     word_0:1++=1++                                                                     //
//  in the context of '.', because the word '1.' was never seen in the training data,     //
//  but did appear in the test data.                                                      //
//  ------------------------------------------------------------------------------------- //

void studyData(const std::string& filename, const std::string& train_filename) {
    std::cerr << "Studying the data" << std::endl;
    std::istream* in;
    smart_open(in, filename);

    const Params& p = Params::GetParams();
    bool empty_lines_are_seps = p["EMPTY_LINES_ARE_SEPARATORS"] == "1";
    static Dictionary& dict = Dictionary::GetDictionary();

    if (!empty_lines_are_seps) {
        std::cerr << "Samples will be considered independent, and empty lines are discarded" << std::endl;
        if (std::max(-PredicateTemplate::MaxBackwardLookup, +PredicateTemplate::MaxForwardLookup) != 0) {
            std::cerr << "Warning!! The samples do not appear to be independent, but empty lines are not separators." << std::endl
                      << "This might lead to big problems. Continue at your own risk :). " << std::endl
                      << "(it is acceptable when considering a single sequence of features)" << std::endl;
            std::string answer;
            int no = 0;
            do {
                switch (no) {
                case 0:
                    std::cerr << "Do you want to continue? (y/n)" << std::endl;
                    break;
                case 1:
                    std::cerr << "Please answer y or n" << std::endl;
                    break;
                case 2:
                    std::cerr << "Read my lips: answer 'y' or 'n'" << std::endl;
                    break;
                default:
                    std::cerr << "You're waisting my time - just answer 'y' or 'n'" << std::endl;
                }
                std::cin >> answer;
                no++;
            } while (answer != "y" && answer != "Y" && answer != "n" && answer != "N");

            if (answer == "n" || answer == "N") {
                std::cerr << "Wise choice." << std::endl
                          << "Please correct the problem and restart." << std::endl;
                exit(1);
            }
            else {
                std::cerr << "OK - you seem to know what you are doing." << std::endl
                          << "However, please don't blame the authors for possible damages incurred to me, the computer." << std::endl;
            }
        }
    }

    std::string in_line;
    line_splitter ls;
    Dictionary words, real_words, classifications;

    corpus_size = 0;
    static wordType2D vect;
    static wordTypeVector features;
    ticker tk("Sentences read:", 1024);

    std::list<int> lst;
    for (int i = 0; i < SubwordPartPredicate::feature_len_pair_list.size(); i++) {
        if (std::find(lst.begin(), lst.end(), SubwordPartPredicate::feature_len_pair_list[i].first) == lst.end()) {
            lst.insert(lst.end(), SubwordPartPredicate::feature_len_pair_list[i].first);
        }
    }

    bool with_train_file = !train_filename.empty();
    if (with_train_file) {
        dict.readFromFile(train_filename);

        wordType real_start = dict.real_word_start_index(), real_end = dict.real_word_end_index();

        if (lst.size() > 0) {
            for (int i = real_start; i != real_end; ++i) {
                real_words.insert(dict[i]);
            }
        }
    }

    if (Params::GetParams()["LARGE_WORD_VOCABULARY"] != "") {
        std::istream* istr;
        smart_open(istr, Params::GetParams()["LARGE_WORD_VOCABULARY"]);
        std::string line;
        while (std::getline(*istr, line)) {
            real_words.insert(line);
        }
        delete istr;
    }

    classifications.insert("ZZZ");
    bool in_sent = false;
    std::string truth_sep = Params::GetParams()["TRUTH_SEPARATOR"];

    while (std::getline(*in, in_line)) {
        ls.split(in_line);

        if (ls.size() > 0) {
            features.resize(ls.size());
            for (int i = 0; i < ls.size(); i++) {
                words.insert(ls[i]);
            }

            if (!with_train_file) {
                for (std::list<int>::iterator p = lst.begin(); p != lst.end(); ++p) {
                    real_words.insert(ls[*p]);
                }
            }

            if (truth_sep == "") {
                classifications.insert(ls[ls.size() - 1]);
            }
            else {
                static line_splitter ts(truth_sep);
                ts.split(ls[ls.size() - 1]);
                for (int i = 0; i < ts.size(); i++)
                    classifications.insert(ts[i]);
            }

            classifications.insert(ls[ls.size() - 2]);

            in_sent = true;
            if (!empty_lines_are_seps) {
                tk.tick();
                corpus_size++;
            }
        }
        else {
            if (empty_lines_are_seps && in_sent) {
                corpus_size++;
                tk.tick();
            }
            in_sent = false;
        }
    }
    tk.clear();

    classifications.insert("FAKE_CLASS");
    //Sort the classes before adding them to the dictionary
    int1D vals(classifications.size());
    iota(vals.begin(), vals.end(), 0);
    sort(vals.begin(), vals.end(), ArrayIndexSorter<Dictionary>(classifications));

    for (int1D::iterator i = vals.begin(); i != vals.end(); ++i)
        dict.insert(classifications[*i]);

    dict.FixClasses();

    std::cerr << "Creating part-of-word indexes";

    if (in_sent && empty_lines_are_seps) {
        corpus_size++;
    }

    // Sort the real_words vocabulary first
    vals.resize(real_words.size());
    iota(vals.begin(), vals.end(), 0);
    sort(vals.begin(), vals.end(), ArrayIndexSorter<Dictionary>(real_words));

    dict.set_start();
    for (int1D::iterator i = vals.begin(); i != vals.end(); ++i) {
        auto word = real_words[*i];
        if (!with_train_file)
            dict.insert(word);
        if (V_flag >= 5)
            std::cerr << *i << " " << word << std::endl;
        dict.insert_in_direct_trie(std::string(word));
        dict.insert_in_reverse_trie(std::string(word));
    }
    dict.set_end();

    // Here should be done all the initializations of the individual caches
    PrefixSuffixAddPredicate::Initialize(dict.size());
    ContainsStringPredicate::Initialize(dict.size());

    for (auto& word : words) {
        dict.insert(word);
    }

    wordType_set word_set;
    for (auto& real_word : real_words) {
        int ind = dict[real_word];
        for (auto& Template : PredicateTemplate::Templates) {
            word_set.clear();
            Template.identify_strings(ind, word_set);
        }
    }
    for (int i = 0; i < strlen("Creating part-of-word indexes"); i++) {
        std::cerr << "\b";
    }

    delete in;
}

// Read a fixed number of lines of the data and store them in corpus
bool read_lines(std::istream& istr, int num_lines) {
    const Params& p = Params::GetParams();
    bool empty_lines_are_seps = p["EMPTY_LINES_ARE_SEPARATORS"] == "1";
    static string1D line;
    line.clear();
    static std::string str;
    bool read_something = false;
    int lines_read = 0;
    line_splitter ls;

    while (std::getline(istr, str)) {
        read_something = true;
        ls.split(str);
        if (ls.size() == 0) {
            if (line.size() > 0) {
                process_line(line, lines_read);
                line.clear();
                if (++lines_read == num_lines)
                    break;
            }
        }
        else {
            if (!empty_lines_are_seps) {
                line.push_back(str);
                process_line(line, lines_read);
                line.clear();
                if (++lines_read == num_lines)
                    break;
            }
            else
                line.push_back(str);
        }
    }

    if (!line.empty()) {
        process_line(line, lines_read);
        lines_read++;
    }
    corpus.resize(lines_read);

    return read_something;
}

// This function is called to read in the corpus.
void readInData(char* filename) {
    std::istream* in;

    const Params& p = Params::GetParams();
    bool empty_lines_are_seps = p["EMPTY_LINES_ARE_SEPARATORS"] == "1";
    ticker tk("Sentences read:", 128);

    smart_open(in, filename);

    std::cerr << "Reading " << corpus_size << " sentences !" << std::endl;
    corpus.resize(corpus_size);
    ruleTrace.resize(corpus_size);
    string1D current_line;
    int lineNum = 0;
    std::string linestr;
    line_splitter ls;
    while (std::getline(*in, linestr)) {
        ls.split(linestr);
        if (ls.size() == 0) {
            if (current_line.size() > 0) {
                process_line(current_line, lineNum);
                lineNum++;
                tk.tick();
            }
            current_line.clear();
        }
        else {
            // If the samples are independent, then put each one in its own "sentence".
            if (!empty_lines_are_seps) {
                current_line.push_back(linestr);
                process_line(current_line, lineNum);
                lineNum++;
                current_line.clear();
                tk.tick();
            }
            else
                current_line.push_back(linestr);
        }
    }
    tk.clear();
    delete in;

    if (!current_line.empty()) {
        process_line(current_line, lineNum);
    }

    std::cerr << "Done reading data." << std::endl;
}

void generate_index(const std::set<int>& filter) {
    // Now create the indexes.
    // The generation is complicated by the fact that we have now even
    // indexes for partial words (prefixes and suffixes).
    // A prefix x will be identified by "x++". Hopefully, there will be no conflicts.
    // The index corresponding to the suffix x will be represented under "++x".
    // A negative prefix (the words that can be obtained by removing the prefix x)
    // will be indexed under "x--". Similarly, a negative suffix will be represented
    // as "--x".

    bool with_filter = filter.size() != 0;

    if (!with_filter)
        std::cerr << "Generating the index" << std::endl;

    Dictionary& dict = Dictionary::GetDictionary();
    static wordType fake_index = dict["ZZZ"];
    //ticker tk1("Processed lines:", 128);
    static std::set<int> seen;
    corpusIndex.resize(dict.size());

    int1D sizes(corpus.size());
    for (int i = 0; i < corpus.size(); i++)
        sizes[i] = corpus[i].size();

    corpusIndex.create_map(sizes);

    for (unsigned int i = 0; i < corpus.size(); i++) {
        short j = 0;
        int sent_max = corpus[i].size() - PredicateTemplate::MaxForwardLookup;
        for (; j < -PredicateTemplate::MaxBackwardLookup; j++) {
            corpusIndex.insert(fake_index, i, static_cast<unsigned short>(j));
            classifIndex.insert(fake_index, i, static_cast<unsigned short>(j));
        }

        for (; j < sent_max; j++) {
            wordType1D& vect = corpus[i][j];

            static wordType_set words;
            seen.clear();

            for (int k = 0; k < PredicateTemplate::Templates.size(); k++) {
                words.clear();
                PredicateTemplate::Templates[k].identify_strings(vect, words);
                for (wordType_set::iterator p = words.begin(); p != words.end(); ++p) {
                    seen.insert(dict.increaseCount(*p));
                }
            }

            if (V_flag >= 3)
                std::cerr << "##" << i << " " << j << " " << seen.size() << std::endl;

            if (filter.size() > 0) {
                for (std::set<int>::iterator p = seen.begin(); p != seen.end(); ++p)
                    if (filter.find(*p) != filter.end())
                        corpusIndex.insert(*p, i, static_cast<unsigned short>(j));
            }
            else
                for (std::set<int>::iterator p = seen.begin(); p != seen.end(); ++p)
                    corpusIndex.insert(*p, i, static_cast<unsigned short>(j));

            for (int k = TargetTemplate::STATE_START; k < TargetTemplate::STATE_START + TargetTemplate::TRUTH_SIZE; k++)
                classifIndex.insert(vect[k], i, static_cast<unsigned short>(j));
        }
        /*
        if (!with_filter) {
            tk1.tick();
        }
        //*/

        for (; j <= sent_max + PredicateTemplate::MaxForwardLookup; j++) {
            corpusIndex.insert(fake_index, i, static_cast<unsigned short>(j));
            classifIndex.insert(fake_index, i, static_cast<unsigned short>(j));
        }
    }
    /*
    if (!with_filter) {
        tk1.clear();
    }
    //*/

    corpusIndex.finalize();
    defaultIndex.finalize();
}

void clear_corpus() {
    for (wordType3D::iterator i = corpus.begin(); i != corpus.end(); ++i)
        delete[] i->begin()[0];
}

void printCorpusState(std::ostream& out, bool printRT) {
    static int feature_set_size = RuleTemplate::name_map.size();
    Dictionary& dict = Dictionary::GetDictionary();
    bool empty_line_are_seps = Params::GetParams()["EMPTY_LINES_ARE_SEPARATORS"] == "1";
    int TRUTH_SIZE = TargetTemplate::TRUTH_SIZE,
        TRUTH_START = TargetTemplate::TRUTH_START,
        STATE_START = TargetTemplate::STATE_START;

    for (int i = 0; i < static_cast<int>(corpus.size()); i++) {
        for (int j = -PredicateTemplate::MaxBackwardLookup; j < static_cast<int>(corpus[i].size()) - PredicateTemplate::MaxForwardLookup; j++) {
            wordType1D& vect = corpus[i][j];
            for (int k = 0; k < feature_set_size - 2 * TRUTH_SIZE; k++)
                out << dict[vect[k]] << " ";
            for (int k = STATE_START; k < STATE_START + TRUTH_SIZE; k++)
                out << dict[vect[k]] << " ";
            for (int k = TRUTH_START; k < TRUTH_START + TRUTH_SIZE - 1; k++)
                out << dict[vect[k]] << " ";
            out << dict[vect[TRUTH_START + TRUTH_SIZE - 1]];

            if (printRT) {
                out << " | ";
                std::copy(ruleTrace[i][j].begin(), ruleTrace[i][j].end(), std::ostream_iterator<int>(out, " "));
            }
            out << std::endl;
        }
        if (empty_line_are_seps) // Only if samples are not independent
            out << std::endl;
    }
}

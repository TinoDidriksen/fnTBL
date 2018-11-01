/*
  Defines the implementation for the Dictionary class.

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

#include "Dictionary.h"
#include "common.h"
#include "line_splitter.h"
#include "Params.h"
#include <numeric>

int Dictionary::num_classes = 0;

const std::string& Dictionary::getString(wordType index) const {
    if (index >= word_index.size() || index == unknown_index) {
        was_unknown = true;
        return spelling_of_unknown;
    }
    was_unknown = false;
    return word_index[index];
}

int Dictionary::getCounts(wordType index) {
    if (index >= static_cast<int>(word_index.size())) {
        return 0;
    }
    return word_counts[index];
}

wordType Dictionary::getIndex(const std::string& word) const {
    int ind = word_index[word];
    if (ind == word_index.fake_index) {
        was_unknown = true;
        return unknown_index;
    }
    was_unknown = false;
    return ind;
}

wordType Dictionary::getIndex(const std::string& word) {
    wordType i = word_index[word];

    if (i == word_index.fake_index) {
        was_unknown = true;
        return word_index.insert(word);
    }

    was_unknown = false;
    return i;
}

wordType Dictionary::increaseCount(const std::string& word, unsigned int count) {
    wordType index = getIndex(word);
    word_counts[index] += count;
    return index;
}

wordType Dictionary::increaseCount(int index, unsigned int count) {
    word_counts[index] += count;
    return index;
}

void Dictionary::writeToFile(const std::string& file) const {
    std::ostream* ostr;
    smart_open(ostr, file);
    *ostr << "#real_word_indices: " << _real_word_start_index << " " << _real_word_end_index << std::endl;
    *ostr << "#number_of_classes: " << num_classes << std::endl;
    for (const auto& i : *this) {
        *ostr << i << std::endl;
    }
    delete ostr;
}

void Dictionary::readFromFile(const std::string& file) {
    std::istream* istr;
    try {
        // Open as-is
        smart_open(istr, file);
    }
    catch (...) {
        // ...if that fails, open filename relative to cwd
        std::string nf{ file };
        size_t off = 0;
        if ((off = file.find_last_of("\\/")) != std::string::npos) {
            nf = file.substr(off + 1);
        }

        try {
            smart_open(istr, nf);
        }
        catch (...) {
            // ...if that also fails, prepend _REL_PATH
            nf = Params::GetParams()["_REL_PATH"] + nf;
            smart_open(istr, nf);
        }
    }

    std::string line;
    line_splitter ls;

    while (istr->peek() == ('#')) {
        getline(*istr, line);
        ls.split(line);
        if (ls[0] == "#") {
            insert(line);
            break;
        }
        if (ls[0] == "#real_word_indices:") {
            _real_word_start_index = atoi1(ls[1]);
            _real_word_end_index = atoi1(ls[2]);
        }
        else if (ls[0] == "#number_of_classes:") {
            num_classes = atoi1(ls[1]);
        }
        else {
            std::cerr << "Unknown meta directive in the vocabulary file: " << ls[0] << "!!" << std::endl;
        }
    }

    while (std::getline(*istr, line)) {
        insert(line);
    }

    delete istr;
}

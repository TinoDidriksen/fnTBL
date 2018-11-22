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

std::unordered_set<std::string> Dictionary::uniq_strings;
int Dictionary::num_classes = 0;

std::string_view Dictionary::getString(wordType index) const {
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

wordType Dictionary::getIndex(std::string_view word) const {
    int ind = word_index[word];
    if (ind == word_index.fake_index) {
        was_unknown = true;
        return unknown_index;
    }
    was_unknown = false;
    return ind;
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
    std::string nf{ file };
    std::istream* istr;
    try {
        // Open as-is...
        smart_open(istr, nf);
    }
    catch (...) {
        // ...if that fails, open filename relative to cwd
        auto off = file.find_last_of("\\/");
        if (off != std::string::npos) {
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

	delete istr;
    data = mmap_region(nf);

    auto& env = word_index.mdb();
    env = lmdb::env::create();

    struct stat _stat {};
    stat(nf.c_str(), &_stat);
    auto data_mdb = _stat.st_mtime;
    auto data_size = _stat.st_size;

    std::string tmp{ nf };
    tmp.append(".mdb");
    auto stat_err = stat(tmp.c_str(), &_stat);

    env.open(tmp.c_str(), MDB_NOSUBDIR | MDB_WRITEMAP | MDB_NOSYNC, 0664);
    env.set_mapsize(data_size * 4);

    // If data file is newer or the database is empty, (re)create database
    MDB_stat mdbs;
    lmdb::env_stat(env, &mdbs);
    if (stat_err != 0 || data_mdb > _stat.st_mtime || mdbs.ms_entries == 0) {
        auto wtxn = lmdb::txn::begin(env);
        auto dbi = lmdb::dbi::open(wtxn);
        dbi.drop(wtxn); // Drop all existing entries, if any
        wtxn.commit();

        lmdb_writer wr(env);
        lmdb::val key;
        lmdb::val value;
        uint32_t num_unique{ 0 };

        auto insert_line = [&](std::string_view line) {
            key.assign(line.data(), line.size());
            value.assign(&num_unique, sizeof(num_unique));
            if (wr.dbi.put(wr.txn, key, value, MDB_NODUPDATA | MDB_NOOVERWRITE)) {
                ++num_unique;
            }
        };

        line_splitter_view ls;

        for (std::string_view line{ nextline(data) }; !line.empty(); line = nextline(data, line)) {
            if (line[0] == '#') {
                ls.split(line);

                if (ls[0] == "#") {
                    insert_line(line);
                }
                else if (ls[0] == "#real_word_indices:") {
                    ::put(wr, "__REAL_WORD_START_INDEX", atoi1(ls[1]));
                    ::put(wr, "__REAL_WORD_END_INDEX", atoi1(ls[2]));
                }
                else if (ls[0] == "#number_of_classes:") {
                    ::put(wr, "__NUM_CLASSES", atoi1(ls[1]));
                }
            }
            else {
                insert_line(line);
            }
        }

        key.assign("__NUM_ENTRIES");
        value.assign(&num_unique, sizeof(num_unique));
        wr.dbi.put(wr.txn, key, value, MDB_NODUPDATA | MDB_NOOVERWRITE);

        wr.txn.commit();
    }

    word_index.mdb_done();

    auto& rd = word_index.mdb_rd();
    _real_word_start_index = ::get(rd, "__REAL_WORD_START_INDEX");
    _real_word_end_index = ::get(rd, "__REAL_WORD_END_INDEX");
    num_classes = ::get(rd, "__NUM_CLASSES");
}

// -*- C++ -*-
/*
  Defines an atomic predicate type: a type of bag-of-words question.
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

#ifndef __CooccurrencePredicate_h__
#define __CooccurrencePredicate_h__

#include "AtomicPredicate.h"
// #include "trie.h"
#include "hash_wrapper.h"
#include "typedef.h"
#include "indexed_map.h"
#include "common.h"
#include <algorithm>

namespace HASH_NAMESPACE {
template<>
struct hash<std::pair<featureIndexType, relativePosType>> {
    size_t operator()(const std::pair<featureIndexType, relativePosType>& val) const {
        return 256 * val.first + val.second;
    }
};
}

class CooccurrencePredicate : public AtomicPredicate {
public:
    //   using word_trie = trie<wordType, wordTypeVector>;
    using word_map = HASH_NAMESPACE::hash_map<wordType, wordTypeVector>;
    using self = CooccurrencePredicate;
    using super = AtomicPredicate;

public:
    CooccurrencePredicate(relativePosType r, featureIndexType fid)
      : pos(r)
      , feature_id(fid) {
        values.insert(std::make_pair(feature_id, r));
    }

    CooccurrencePredicate(const self& p)
      : pos(p.pos)
      , feature_id(p.feature_id) {}

    ~CooccurrencePredicate() override = default;

    bool test(const wordType2D& corpus, int sample_ind, const wordType value) const override {
        wordType key = corpus[sample_ind][feature_id];
        word_map& ngram = tries[word_map_index[std::make_pair(feature_id, pos)]];

        auto i = ngram.find(key);
        if (i != ngram.end()) {
            word_map::const_iterator::value_type p = *i;
            return std::binary_search(p.second.begin(), p.second.end(), value);
        }
        {
            return false;
        }
    }

    double test(const wordType2D& corpus, int sample_ind, const wordType value, const float2D& probs) const override {
        return 1.0 * (value == corpus[sample_ind + pos][feature_id]);
    }

    self& operator=(const self& pred) {
        if (this != &pred) {
            pos = pred.pos;
            feature_id = pred.feature_id;
        }
        return *this;
    }

    void identify_strings(const wordType1D& word_id, wordType_set& words) const override;
    void identify_strings(wordType word_id, wordType_set& words) const override;
    void instantiate(const wordType2D& corpus, int sample_ind, wordTypeVector& instances) const override;
    void get_sample_differences(position_vector& positions) const override {
        positions.push_back(pos);
    }

    std::string printMe(wordType element) const override;

    void get_feature_ids(storage_vector& features) const override {
        features.push_back(feature_id);
    }

    void set_dependencies(std::vector<bit_vector>& dep) const override {
    }

    bool is_indexable() const override {
        return false;
    }

    static void Initialize(const std::string& rule_file = "");
    static bool IsInitialized;
    using index_pair = std::pair<featureIndexType, relativePosType>;
    static indexed_map<index_pair, short int> word_map_index;
    static std::set<index_pair> values;
    static std::vector<word_map> tries;

protected:
    relativePosType pos;
    featureIndexType feature_id;
};

template<class type>
std::ostream& operator<<(std::ostream& ostr, const std::vector<type>& sv) {
    ostr << sv.size();
    for (int i = 0; i < sv.size(); i++)
        ostr << " " << sv[i];
    return ostr;
}

template<class type>
std::istream& operator>>(std::istream& istr, std::vector<type>& sv) {
    int sz;
    istr >> sz;
    sv.resize(sz);
    for (int i = 0; i < sz; i++)
        istr >> sv[i];
    return istr;
}


#endif

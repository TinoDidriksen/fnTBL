// -*- C++ -*-
/*
  Defines an atomic predicate type: returns true if the argument
  is a given feature.

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

#ifndef __SingleFeaturePredicate_h__
#define __SingleFeaturePredicate_h__

#include "AtomicPredicate.h"
#include "typedef.h"
#include "Dictionary.h"

class SingleFeaturePredicate : public AtomicPredicate {
protected:
    using storage_type = featureIndexType;
    relativePosType sample_difference;

    storage_type feature_id;

public:
    SingleFeaturePredicate(relativePosType sample, storage_type feature)
      : sample_difference(sample)
      , feature_id(feature) {}
    SingleFeaturePredicate(const SingleFeaturePredicate& pred)
      : sample_difference(pred.sample_difference)
      , feature_id(pred.feature_id) {}

    ~SingleFeaturePredicate() override = default;

    bool test(const wordType2D& corpus, int sample_ind, const wordType value) const override {
        return corpus[sample_ind + sample_difference][feature_id] == value;
    }

    double test(const wordType2D& corpus, int sample_ind, const wordType value, const float2D& context_prob) const override {
        return feature_id < Dictionary::num_classes ? context_prob[sample_ind + sample_difference][value] : 1.0;
    }

    std::string printMe(wordType instance) const override;

    SingleFeaturePredicate& operator=(const SingleFeaturePredicate& pred) {
        if (this != &pred) {
            sample_difference = pred.sample_difference;
            feature_id = pred.feature_id;
        }
        return *this;
    }

    void instantiate(const wordType2D& corpus, int sample_ind, wordTypeVector& instances) const override {
        instances.resize(1);
        instances[0] = corpus[sample_ind + sample_difference][feature_id];
    }

    void identify_strings(const wordType1D& word_id, wordType_set& words) const override {
        identify_strings(word_id[feature_id], words);
    }

    void identify_strings(wordType word_id, wordType_set& words) const override {
        // 	static Dictionary& dict = Dictionary::GetDictionary();
        words.insert(word_id);
    }

    void get_sample_differences(position_vector& positions) const override {
        positions.push_back(sample_difference);
    }

    void get_feature_ids(storage_vector& features) const override {
        features.push_back(feature_id);
    }

    void set_dependencies(std::vector<bit_vector>& dep) const override;

    bool is_indexable() const override {
        return true;
    }
};

#include "Predicate.h"

inline void SingleFeaturePredicate::set_dependencies(std::vector<bit_vector>& dep) const {
    //   cerr << dep.size() << " " << sample_difference-PredicateTemplate::MaxBackwardLookup << " "
    // 	   << dep[sample_difference-PredicateTemplate::MaxBackwardLookup].size() << " " << (int)feature_id << endl;
    dep[sample_difference - PredicateTemplate::MaxBackwardLookup][feature_id] = true;
    //   cerr << "Out of here" << endl;
}

inline std::string SingleFeaturePredicate::printMe(wordType instance) const {
    Dictionary& dict = Dictionary::GetDictionary();

    if (std::max(-PredicateTemplate::MaxBackwardLookup, +PredicateTemplate::MaxForwardLookup) == 0) {
        return std::string(PredicateTemplate::name_map[feature_id]) + "=" + std::string(dict[instance]);
    }
    {
        return std::string(PredicateTemplate::name_map[feature_id]) + "_" + itoa(sample_difference) + "=" + std::string(dict[instance]);
    }
}

#endif

// -*- C++ -*-
/*
  Defines a type of atomic predicate - returns true if the word argument
  contains a given character.

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

#ifndef __SubwordPartPredicate_h__
#define __SubwordPartPredicate_h__

#include "SingleFeaturePredicate.h"
#include "svector.h"

class SubwordPartPredicate : public SingleFeaturePredicate {
    using self = SubwordPartPredicate;
    using super = SingleFeaturePredicate;

public:
    using rep_vector = std::vector<std::pair<featureIndexType, char>>;
    using bool2D = std::vector<bit_vector>;
    using wordType_svector = svector<wordType>;
    using word_list_rep_type = std::vector<std::vector<wordType_svector>>;

protected:
    char len;

public:
    SubwordPartPredicate(relativePosType sample, storage_type feature, char length = 1)
      : super(sample, feature)
      , len(length) {
    }

    SubwordPartPredicate(const self& p)

      = default;

    static rep_vector feature_len_pair_list;

    void addToList() {
        feature_len_pair_list.push_back(std::make_pair(feature_id, len));
    }

    bool test(const wordType2D& corpus, int sample_ind, wordType value) const override = 0;

    double test(const wordType2D& corpus, int sample_ind, const wordType value, const float2D& /*context_prob*/) const override {
        if (test(corpus, sample_ind, value)) {
            return 1.0;
        }
        {
            return 0.0;
        }
    }

    static void Initialize(int size, word_list_rep_type& feature_lookup, bool2D& seen);

    bool is_indexable() const override {
        return true;
    }
};

#endif

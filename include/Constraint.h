// -*- c++ -*-
/*
  This class implements global constraints on the rules. They restrict
  the application of the rule, such that no new classes are assigned
  to the samples.
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

#ifndef __Constraint_h__
#define __Constraint_h__

#include "typedef.h"
#include <string>
#include "svector.h"
#include "trie.h"
#include "hash_wrapper.h"
#include "Target.h"
#include "bit_vector.h"
#include <vector>

namespace HASH_NAMESPACE {
template<>
struct hash<std::vector<wordType>> {
    size_t operator()(const std::vector<wordType>& v) const {
        size_t s = 0;
        auto i = v.begin();
        for (; i != v.end(); ++i) {
            s += 17 * s + *i;
        }
        return s;
    }
};
} // namespace std

class Constraint {
public:
    using wordTypeVector = std::vector<wordType>;
    using rep_type = HASH_NAMESPACE::hash_map<wordTypeVector, bit_vector>;

    Constraint(const std::string& line1);
    Constraint() = default;

    bool test(const wordType1D& feature_vector, int class_id) const;
    bool test(const wordType1D& feature_vector, const Target& target) const;

    bool operator()(const wordType1D& feature_vector, int class_id) const {
        return test(feature_vector, class_id);
    }
    bool operator()(const wordType1D& feature_vector, const Target& target) const {
        return test(feature_vector, target);
    }

private:
    void initialize_vector(std::vector<wordType>& v, const wordType1D& feature_vector) const {
        v.resize(features.size());
        int p = 0;
        for (auto f = features.begin(); f != features.end(); ++f, ++p) {
            v[p] = feature_vector[*f];
        }
    }

    rep_type constraint;
    wordTypeVector features;
    featureIndexType target_feature;
};

class ConstraintSet {
public:
    using constraint_vector = std::vector<Constraint>;

    ConstraintSet(const std::string& file_name) {
        read(file_name);
    }

    ConstraintSet() = default;

    void read(const std::string& file_name);

    bool test(const wordType1D& feature_vector, int class_id) const {
        for (const auto& constraint : constraints) {
            if (!constraint.test(feature_vector, class_id)) {
                return false;
            }
        }

        return true;
    }

    bool operator()(const wordType1D& feature_vector, int class_id) const {
        return test(feature_vector, class_id);
    }

    bool test(const wordType1D& feature_vector, const Target& target) const {
        static wordType fake_rule_index = Dictionary::GetDictionary()["FAKE_CLASS"];

        if (target.vals[0] == fake_rule_index) { // Constraints don't apply on FAKE_CLASS classes
            return true;
        }

        for (const auto& constraint : constraints) {
            if (!constraint.test(feature_vector, target)) {
                return false;
            }
        }

        return true;
    }

    bool operator()(const wordType1D& feature_vector, const Target& target) const {
        return test(feature_vector, target);
    }

private:
    constraint_vector constraints;
};


#endif

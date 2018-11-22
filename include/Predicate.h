// -*- C++ -*-
/*
  Defines the predicate class and associated containers.

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

#ifndef __Predicate_h__
#define __Predicate_h__

#include "AtomicPredicate.h"
#include "typedef.h"
#include "Dictionary.h"
#include "svector.h"
#include "hash_wrapper.h"
#include <vector>

#include <functional>
#include <numeric>

#include "bit_vector.h"
// A predicate template is a predicate having the features uninstantiated.
// A predicate consists of a pointer to a PredicateTemplate and a set
// of instantiations. By using this design, we can save space, because
// the templates are represented only once.

class PredicateTemplate {
public:
    using string_vector = std::vector<std::string>;
    using PredicateTemplate_vector = std::vector<PredicateTemplate>;
    using ptest_type = AtomicPredicate*;
    using test_vector_type = std::vector<ptest_type>;
    using self = PredicateTemplate;

    PredicateTemplate()
      : tests(0)
      , dependencies(0) {}
    PredicateTemplate(const std::vector<std::string>& ls);

    PredicateTemplate(const PredicateTemplate& pt)
      : tests(pt.tests) {
    }

    ~PredicateTemplate() = default;

    void deallocate() {
        for (auto& test : tests) {
            delete test;
        }
    }

    static Dictionary name_map;
    static string_vector TemplateNames;
    static PredicateTemplate_vector Templates;
    static HASH_NAMESPACE::hash_map<std::string, std::string> variables;

    static relativePosType MaxBackwardLookup, MaxForwardLookup; //

    static void Initialize();
    const AtomicPredicate& operator[](int i) const {
        return *tests[i];
    }

    void instantiate(const wordType2D& corpus, int sample_ind, wordType2DVector& instances) const;
    void identify_strings(wordType word_id, wordType_set& words) const;
    void identify_strings(const wordType1D& word_id, wordType_set& words) const;

    static int FindTemplate(const std::string& t_name) {
        auto i = find(TemplateNames.begin(), TemplateNames.end(), t_name);
        if (i == TemplateNames.end()) {
            return -1;
        }
        return distance(TemplateNames.begin(), i);
    }

    void set_dependencies() {
        static short int feature_set_size = PredicateTemplate::name_map.size() - 1;
        dependencies.resize(PredicateTemplate::MaxForwardLookup - PredicateTemplate::MaxBackwardLookup + feature_set_size);
        for (auto& dependencie : dependencies) {
            dependencie.resize(feature_set_size);
        }

        for (auto& test : tests) {
            test->set_dependencies(dependencies);
        }
    }

    // This method returns true if the predicate template depends on
    // the specified position.
    bool depends_on(relativePosType position, featureIndexType feature) const {
        return dependencies[position - MaxBackwardLookup][feature];
    }

    bool depends_on(relativePosType position, featureIndexType1D& features) const {
        for (unsigned char feature : features) {
            if (dependencies[position - MaxBackwardLookup][feature]) {
                return true;
            }
        }
        return false;
    }

    void swap(self& t) {
        tests.swap(t.tests);
        dependencies.swap(t.dependencies);
    }

public:
    test_vector_type tests;
    std::vector<bit_vector> dependencies;
};

// Each predicate has an attached template and an associated vector of values.
class Predicate {
public:
    using order_rep_type = unsigned char;
    using token_vector_type = svector<wordType>;

    Predicate(string1D& ruleComponents) {
        create_from_words(ruleComponents);
    }

    Predicate(int tid, const wordType1D& tok)
      : template_id(tid) {
        static int feature_set_size = PredicateTemplate::name_map.size();
        tokens.resize(feature_set_size);
        std::copy(tok, tok + feature_set_size, tokens.begin());

        create_order();

        hashIndex = hashVal();
    }

    Predicate(int tid, const wordTypeVector& tok)
      : template_id(tid)
      , tokens(tok) {
        create_order();
        hashIndex = hashVal();
    }

    Predicate(const Predicate& p)
      : template_id(p.template_id)
      , hashIndex(p.hashIndex)
      , tokens(p.tokens) {
        allocate_order();
        std::copy(p.order, p.order + tokens.size(), order);
    }

    Predicate()
      : tokens(0)
    {}

    ~Predicate() {
        free_order();
        order = nullptr;
    }

    void create_from_words(string1D& ruleComponents);

    void create_order() {
        static Dictionary& dict = Dictionary::GetDictionary();
        int sz = tokens.size();

        static std::vector<int> counts;

        counts.resize(sz);
        // 	counts.clear();

        for (int i = 0; i < sz; i++) {
            counts[i] = dict.getCounts(tokens[i]);
        }

        allocate_order();

        std::iota(order, order + tokens.size(), 0);
        std::sort(order, order + tokens.size(), Sorter(counts));
    }

public:
    short int template_id{ -1 };
    int hashIndex{ 0 };
    token_vector_type tokens;
    order_rep_type* order{ nullptr };

public:
    int hashVal() {
        int value = 0;
        for (unsigned int& token : tokens) {
            value = 5 * value + token;
        }

        return 5 * value + template_id;
    }

    std::string printMe() const {
        const PredicateTemplate& templ = PredicateTemplate::Templates[template_id];

        token_vector_type::const_iterator token = tokens.begin();
        std::string str = templ[0].printMe(*token);
        int i = 1;
        for (++token; token != tokens.end(); ++token, ++i) {
            str += " " + templ[i].printMe(*token);
        }
        return str;
    }

    bool test(const wordType2D& corpus, int word) const;

    double test(const wordType2D& corpus, int word, const float2D& context_prob) const;

    bool operator==(const Predicate& pred) const {
        if (template_id != pred.template_id || tokens.size() != pred.tokens.size()) {
            return false;
        }

        int sz = tokens.size();
        for (order_rep_type* i = order; i != order + sz; ++i) {
            if (tokens[*i] != pred.tokens[*i]) {
                return false;
            }
        }

        return true;
    }

    Predicate& operator=(const Predicate& pred) {
        if (this != &pred) {
            hashIndex = pred.hashIndex;
            template_id = pred.template_id;
            free_order();
            tokens = pred.tokens;
            allocate_order();
            std::copy(pred.order, pred.order + tokens.size(), order);
        }
        return *this;
    }

    struct Sorter {
        const int1D& counts;
        Sorter(const int1D& myCounts)
          : counts(myCounts) {}
        bool operator()(int a, int b) {
            return counts[a] < counts[b];
        }
    };

    static void deallocate_all() {
        memory_pool.destroy();
    }

    static void clear_memory() {
        memory_pool.dump();
    }

    int get_least_frequent_feature_position() const {
        PredicateTemplate& pred_template = PredicateTemplate::Templates[template_id];

        for (int k = 0; k < tokens.size(); ++k) {
            if (pred_template.tests[order[k]]->is_indexable()) {
                return order[k];
            }
        }

        return -1;
    }

protected:
    static sized_memory_pool<order_rep_type> memory_pool;

    void allocate_order() {
        order = memory_pool.allocate(tokens.size());
    }

    void free_order() {
        memory_pool.deallocate(order, tokens.size());
    }
};

inline bool Predicate::test(const wordType2D& corpus, int word) const {
    PredicateTemplate& pred_template = PredicateTemplate::Templates[template_id];
    int sz = tokens.size();
    for (order_rep_type* feature = order; feature != order + sz; ++feature) {
        if (!pred_template[*feature].test(corpus, word, tokens[*feature])) {
            return false;
        }
    }

    return true;
}

inline double Predicate::test(const wordType2D& corpus, int word, const float2D& context_prob) const {
    PredicateTemplate& pred_template = PredicateTemplate::Templates[template_id];
    double prob = 1;
    int sz = tokens.size();
    for (order_rep_type* feature = order; feature != order + sz; ++feature) {
        prob *= pred_template[*feature].test(corpus, word, tokens[*feature], context_prob);
    }

    return prob;
}

#endif

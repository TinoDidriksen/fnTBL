// -*- C++ -*-

/*
  Defines the node in a probabilistic tree.

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

#ifndef __NODE_H
#define __NODE_H
#include "Rule.h"
#include "hash_wrapper.h"
#include <vector>
#include "typedef.h"
#include "indexed_map.h"

template<class type = int>
struct ReverseSorter {
    using rep_type = std::vector<type>;
    const rep_type& counts;
    ReverseSorter(const rep_type& v)
      : counts(v) {}
    bool operator()(int a, int b) {
        return counts[a] > counts[b];
    }
};

class Node {
public:
    using self = Node;

    using example_index = std::pair<unsigned int, unsigned short>;
    using example_index1D = std::vector<example_index>;
    using rule_hash_map = indexed_map<Rule, unsigned, HASH_NAMESPACE::hash_map<Rule, unsigned>>;

    using short_int1D_pair = std::pair<unsigned short, int1D>;
    using short_int1D_pair1D = std::vector<short_int1D_pair>;
    using rule_list_type = std::map<unsigned int, short_int1D_pair1D>;

    Node()
      : ID(LastID++) {}

    Node(example_index1D& myExamples);

    Node(std::string& data) {
        initialize(data);
    }

    Node(const Node& node)
      : ID(node.ID)
      , ruleID(node.ruleID)
      , examples(node.examples)
      , yesChild(node.yesChild)
      , noChild(node.noChild)
      , classCounts(node.classCounts)
      , probs(node.probs)
      , totalCount(node.totalCount)
      , entropy(node.entropy) {}

    ~Node() {
        delete yesChild;
        delete noChild;
    }

    void initialize(const std::string& line);

    void split(int rule_no);
    void splitExamples(const bit_vector& yes_sample);

    const self* findClassOfSample(const example_index& example) const;
    int getChunk();
    std::string probString() const;
    void createLeaf();
    void updateCounts();
    void growDT();
    void growDT(rule_hash_map& /*dt_rules*/, const rule_list_type& /*rules_applied*/);
    void computeEntropy();

protected:
    void createRules(rule_hash_map& /*dt_rules*/, rule_list_type& /*node_rules_applied*/);

    int findBestRule(rule_hash_map& /*dt_rules*/, const rule_list_type& /*node_rules_applied*/);
    void splitExamplesByRule(int bestRuleID, const rule_list_type& rules_applied);

public:
    static void addSimpleTemplates(int1D& /*simple_predicate_template_ids*/, int1D& /*simple_truth_template_ids*/);
    void computeProbs(const example_index& /*example*/, const float2D& hypothesis);

    Node& operator=(const Node& node) {
        if (this != &node) {
            ID = node.ID;
            ruleID = node.ruleID;
            examples = node.examples;
            yesChild = node.yesChild;
            noChild = node.noChild;
            classCounts = node.classCounts;
            totalCount = node.totalCount;
            entropy = node.entropy;
            probs = node.probs;
        }
        return *this;
    }

    friend std::ostream& operator<<(std::ostream& /*ostr*/, const self& /*nd*/);
    friend std::istream& operator>>(std::istream& /*istr*/, self& /*nd*/);

    void clearCountsAndProbs() {
        int1D ctemp;
        float1D ptemp;
        classCounts.swap(ctemp);
        probs.swap(ptemp);
    }

public:
    int ID;
    int ruleID{ -1 };
    example_index1D examples;

    Node* yesChild{ nullptr };
    Node* noChild{ nullptr };

    int1D classCounts;
    mutable float1D probs;
    mutable int totalCount;

    double entropy;

    static int LastID;
};


#endif

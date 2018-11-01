// -*- C++ -*-
/*
  Container class implementing a general trie.

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

/*
 *
 * Modified the trie structure such that it does not have to store useless children
 * (children that are null). For that, a boolean vector (a bit vector, basically)
 * is used to store which nodes have children. When accessing a child, if the node
 * is marked as not having children, a null poiter is returned. Otherwise, the right
 * child is returned.
 * Radu Florian, October 1999
 *
 */


/*  
 *	This is a representation of tries, which are efficient structures for representing
 *	sequences of elements. It is basically a n-ary tree, in which the leaves are sorted 
 *	in some way; therefore the access is easier. The sequences are represented in the 
 *	tree from the root down.
 *
 *  It can be seen as an efficient way to store a mapping from sequences to values.
 *
 *	It follows closely the definitions of the rb_tree in STL.
 *
 *	Copyright Radu Florian, December 1998
 */

#ifndef __trie_h_
#define __trie_h_

#include <algorithm>
#include <numeric>
#include <functional>
#include <vector>
#include <set>
#include <stack>
#include <queue>

#include "bit_vector.h"
#include "m_pair.h"
#include "linear_map.h"
#include "debug.h"
//#include <dmalloc.h>

// Checks whether the iterator i is between begin and end
template<class iterator>
bool valid_iterator(const iterator& begin, const iterator& i, const iterator& end) {
    return distance(begin, i) >= 0 && distance(i, end) >= 0;
}

template<class Key, class Value>
struct __trie_node {
    using value_type = Value;
    using key_type = Key;
    using map_type = linear_map<Key, Value, std::less<>>;
    using info_type = linear_map<Key, Value, std::less<>>;
    using self = __trie_node<Key, Value>;

    using link_type = self*;
    using base_ptr = self*;
    using base_const_ptr = const self*;
    using rep_type = std::vector<link_type>;

    using children_iterator = typename rep_type::iterator;
    using const_children_iterator = typename rep_type::const_iterator;
    using value_iterator = typename info_type::iterator;
    using const_value_iterator = typename info_type::const_iterator;
    using reverse_children_iterator = typename rep_type::reverse_iterator;
    using const_reverse_children_iterator = typename rep_type::const_reverse_iterator;

    using bit_vector = my_bit_vector<unsigned short>;

    //   static int count;

    __trie_node()
      : parent(nullptr) {
        // 	  count++;
    }

    __trie_node(const self& node)
      : parent(nullptr) {
        clone(&node);
        // 	count++;
    }

    virtual ~__trie_node() {
        // 	count--;
        // #if DEBUG >= 3
        //     if(depth==1)
        //       // if the node is a leaf, then it should have no children
        //       assert(children.size()==0);
        // #endif
        destroy();
    }

    unsigned short get_depth() const {
        return depth;
    }

    const info_type& get_vals() const {
        return vals;
    }

    // Clear has the same behaviour as the STL clear: it frees the memory, but it does not
    // deallocates it.
    virtual void clear() {
        for (auto i = children.begin(); i != children.end(); ++i) {
            delete *i;
            *i = nullptr;
        }
        children.clear();
        depth = 0;
        has_children.clear();
        vals.clear();
    }

    // This forcefully deallocates the memory
    virtual void clone(const self* node) {
        if (node == nullptr) {
            return;
        }

        //     children.clear();
        destroy();
        vals = node->vals;
        has_children = node->has_children;
        children.resize(node->children.size());
        int pos = 0;
        for (auto i = node->children.begin(); i != node->children.end(); ++i, ++pos) {
            children[pos] = this->duplicate(*i);
            if (*i) {
                children[pos]->parent = this;
            }
        }

        depth = node->depth;
    }

    virtual base_ptr duplicate(const base_ptr node) const {
        if (node == nullptr) {
            return nullptr;
        }
        return new self(*node);
    }

    template<class __node_type>
    void copy_support(const __node_type& /*node*/);

    virtual void resize(int n) {
        vals.resize(n);
        has_children.resize(n);
    }

    virtual base_ptr create_new_node() {
        return new self;
    }

    void add_child(int pos, base_ptr node = NULL) {
        if (node != 0)
            node->parent = this;

        children.insert(children.begin() + pos, node);
        has_children.insert(has_children.begin() + pos, true);
        has_children.reset();
        //     if(depth == 1)
        //       depth = 2;
        if (node && node->depth + 1 > depth)
            depth = node->depth + 1;
    }

    void push_back_child(base_ptr node = NULL) {
        if (node)
            node->parent = this;

        children.push_back(node);
        if (node && node->depth + 1 > depth)
            depth = node->depth + 1;
    }

    // Since usually we have a need for space, this method will eliminate
    // the unused space at the end of the children vector. This method
    // should be called when the tree stopped growing (if possible).
    void initialize() {
        depth = !vals.empty();
        for (auto i = children.begin(); i != children.end(); i++) {
            if (*i != nullptr) {
                (*i)->initialize();
                (*i)->parent = this;
                if (depth < (*i)->depth + 1) {
                    depth = (*i)->depth + 1;
                }
            }
        }
    }

    unsigned children_size() const {
        return children.size();
    }

    children_iterator children_begin() {
        return children.begin();
    }

    const_children_iterator children_begin() const {
        return children.begin();
    }

    children_iterator children_end() {
        return children.end();
    }

    const_children_iterator children_end() const {
        return children.end();
    }

    reverse_children_iterator children_rbegin() {
        return children.rbegin();
    }

    const_reverse_children_iterator children_rbegin() const {
        return children.rbegin();
    }

    reverse_children_iterator children_rend() {
        return children.rend();
    }

    const_reverse_children_iterator children_rend() const {
        return children.rend();
    }

    const base_ptr get_parent() const {
        return parent;
    }

    void insert(const key_type& key, const value_type& val) {
        std::pair<typename info_type::iterator, bool> res = vals.insert(make_pair(key, val));
        //     *(p.first) = val;
        if (res.second) {
            //       __trie_node::add_child(distance(vals.begin(), p.first), NULL);
            has_children.insert(has_children.begin() + (res.first - vals.begin()), false);
            has_children.reset();
        }
    }

    void erase(const key_type& key) {
        typename info_type::iterator i = vals.find(key);
        if (i != vals.end()) {
            int pos = (i - vals.begin());
            if (has_children[pos]) {
                children.erase(children.begin() + value_index_to_child_index(pos));
                has_children.erase(has_children.begin() + pos);
                has_children.reset();
            }
            vals.erase(i);
        }
    }

    virtual self& operator=(const self& nd) {
        //__trie_node::operator= (nd);
        vals = nd.vals;
        has_children = nd.has_children;
        return *this;
    }

    value_iterator values_begin() {
        return vals.begin();
    }

    const_value_iterator values_begin() const {
        return vals.begin();
    }

    value_iterator values_end() {
        return vals.end();
    }

    const_value_iterator values_end() const {
        return vals.end();
    }

    value_iterator values_find(const Key& v) {
        return vals.find(v);
    }

    const_value_iterator values_find(const Key& v) const {
        return vals.find(v);
    }

    unsigned values_size() const {
        return vals.size();
    }

    virtual self* child_on_pos(int pos) {
        if (has_children[pos]) {
            unsigned c_pos = value_index_to_child_index(pos);
            ON_DEBUG(assert(c_pos < children.size()));
            return children[c_pos];
        }
        return static_cast<self*>(nullptr);
    }

    virtual const self* child_on_pos(int pos) const {
        if (has_children[pos]) {
            unsigned c_pos = value_index_to_child_index(pos);
            ON_DEBUG(assert(c_pos < children.size()));
            return children[c_pos];
        }
        return static_cast<self*>(nullptr);
    }

    children_iterator iterator_in_parent_list() {
        ON_DEBUG(
          assert(parent != NULL));

        children_iterator it = find(parent->children.begin(), parent->children.end(), this);

        ON_DEBUG(
          assert(it != children.end()));
        return it;
    }

    // This function checks if the iterator is a valid children iterator
    template<class c_iterator>
    bool valid_children_iterator(const c_iterator& it) {
        return ::valid_iterator(children.begin(), it, children.end());
    }

    // Frees the memory associated with this node
    virtual void destroy() {
        vals.destroy();
        bit_vector temp;
        has_children.swap(temp);
        for (auto i = children.begin(); i != children.end(); ++i) {
            delete *i;
            *i = nullptr;
        }
        rep_type a;
        children.swap(a);
        depth = 0;
    }

    virtual void initialize_child_if_null(int pos) {
        if (children[pos] == NULL) {
            children[pos] = this->create_new_node();
            children[pos]->parent = this;
        }
    }

    virtual link_type insert_or_retrieve_child_at_pos(int pos) {
        int child_pos = value_index_to_child_index(pos, false);
        // 	has_children.resize(pos+1, false);
        if (!has_children[pos]) {
            children.insert(children.begin() + child_pos, 0);
            has_children.reset();
            __trie_node::initialize_child_if_null(child_pos);
            has_children[pos] = true;
        }
        return children[child_pos];
    }

    std::pair<typename info_type::iterator, bool> insert_value(const typename info_type::value_type& p) {
        std::pair<typename info_type::iterator, bool> ret = vals.insert(p);
        if (ret.second) {
            has_children.insert(has_children.begin() + distance(vals.begin(), ret.first), false);
            has_children.reset();
        }
        return ret;
    }

    void update_depth() {
        link_type nd = this;
        if (nd->depth == 0) {
            nd->depth = !vals.empty();
        }

        while (nd->parent != nullptr) {
            if (nd->parent->depth < nd->depth + 1) {
                nd->parent->depth = nd->depth + 1;
            }
            nd = static_cast<link_type>(nd->parent);
        }
    }

    virtual void shrink() {
        rep_type c = children;
        children.swap(c);
        info_type v = vals;
        vals.swap(v);
        bit_vector temp_bv(has_children);
        has_children.swap(temp_bv);
    }

    virtual void swap(self& node) {
        children.swap(node.children);

        vals.swap(node.vals);
        has_children.swap(node.has_children);
    }

    template<class c_iterator>
    int position_of_child(const c_iterator& child_it) {
        int p1 = child_it - children.begin(), val = 0;
        bit_vector::iterator i = has_children.begin();
        for (; i != has_children.end() && val < p1; ++i)
            val += *i;

        return distance(has_children.begin(), i);
    }

    template<class v_iterator>
    children_iterator child_of(const v_iterator& value_it) {
        ON_DEBUG(
          assert(::valid_iterator(vals.begin(), value_it, vals.end())));
        unsigned v_i = value_it - vals.begin();
        if (has_children.size() <= v_i || !has_children[v_i]) {
            return children.end();
        }
        return children.begin() + value_index_to_child_index(v_i);
    }

    template<class v_iterator>
    const_children_iterator child_of(const v_iterator& value_it) const {
        unsigned v_i = value_it - vals.begin();
        if (has_children.size() <= v_i || !has_children[v_i])
            return children.end();
        return children.begin() + value_index_to_child_index(v_i);
    }

    template<class c_iterator>
    value_iterator value_for_child(const c_iterator& child_it) {
        ON_DEBUG(
          assert(valid_children_iterator(child_it)));
        return vals.begin() + child_index_to_value_index(child_it - children.begin());
    }

    template<class c_iterator>
    const_value_iterator value_for_child(const c_iterator& child_it) const {
        ON_DEBUG(
          assert(valid_children_iterator(child_it)));
        return vals.begin() + child_index_to_value_index(child_it - children.begin());
    }

    virtual int read_in_binary_format(std::istream& /*istr*/);
    virtual void write_in_binary_format(std::ostream& /*ostr*/) const;

    virtual int read_in_text_format(std::istream& /*istr*/);
    virtual void write_in_text_format(std::ostream& /*ostr*/) const;

    //   friend std::istream& operator >> (std::istream&, self&);
    //   friend std::ostream& operator << (std::ostream&, const self&);

    const bit_vector& get_has_children() const {
        return has_children;
    }

protected:
    int child_index_to_value_index(int c_i) {
        int v_i = 0, fc_i = 0;
        auto i = has_children.begin();
        for (; i != has_children.end() && fc_i <= c_i; ++i, ++v_i) {
            fc_i += *i;
        }
        ON_DEBUG(assert(fc_i == c_i + 1);)
        return v_i - 1;
    }

    int value_index_to_child_index(int v_i, bool check = false) const {
        if (v_i < 0) {
            return -1;
        }
        ON_DEBUG(assert(!check || has_children[v_i]));
        return has_children.count_bits_before_pos(v_i);
    }

protected:
    base_ptr parent;
    rep_type children;
    short unsigned int depth{ 0 };

    bit_vector has_children;
    info_type vals;
};

template<class Key, class Value>
std::istream& operator>>(std::istream& istr, __trie_node<Key, Value>& node) {
    node.read_in_text_format(istr);
    return istr;
}

template<class Key, class Value>
std::ostream& operator<<(std::ostream& ostr, const __trie_node<Key, Value>& node) {
    node.write_in_text_format(ostr);
    return ostr;
}

template<class Key, class Value>
int __trie_node<Key, Value>::read_in_text_format(std::istream& istr) {
    if (!(istr >> vals)) {
        return 0;
    }
    if (!has_children.read_in_text_format(istr, vals.size())) {
        return 0;
    }
    children.resize(has_children.count_bits_before_pos(has_children.size()));
    ON_DEBUG(
      unsigned val = 0;
      val = accumulate(has_children.begin(), has_children.end(), 0u);
      assert(children.size() == val);)
    return 1;
}

template<class Key, class Value>
void __trie_node<Key, Value>::write_in_text_format(std::ostream& ostr) const {
    ostr << " " << vals;
    has_children.write_in_text_format(ostr);
}


template<class Key, class Value>
int __trie_node<Key, Value>::read_in_binary_format(std::istream& istr) {
    if (!vals.read_in_binary_format(istr)) {
        return 0;
    }
    if (!has_children.read_in_binary_format(istr, vals.size())) {
        return 0;
    }
    children.resize(has_children.count_bits_before_pos(has_children.size()));
    ON_DEBUG(
      unsigned val = 0;
      val = accumulate(has_children.begin(), has_children.end(), 0u);
      assert(children.size() == val);)
    return 1;
}

template<class Key, class Value>
void __trie_node<Key, Value>::write_in_binary_format(std::ostream& ostr) const {
    vals.write_in_binary_format(ostr);
    //   int no_vals = vals.size();
    //   short no_bits = (no_vals >> 3) + ((no_vals & 0x07) > 0);
    //   reallocate_bits(no_bits);

    //   int pos = 0;
    //   fill(bits, bits+no_bits, 0);

    //   for(__trie_node::bit_vector::const_iterator b=has_children.begin() ; b!=has_children.end() ; ++b, ++pos)
    // 	bits[pos >> 3] |= *b << (pos & 0x7); // pos >> 3 == the quotient of pos divided by 8, pos & 0x7 == the reminder of same division
    //   ostr.write(static_cast<char*>(bits), sizeof(char)*no_bits);

    has_children.write_in_binary_format(ostr);
    ON_DEBUG(
      unsigned val = 0;
      val = accumulate(has_children.begin(), has_children.end(), 0u);
      assert(children.size() == val);)
}

template<class Key, class Value>
template<class __some_node_type>
void __trie_node<Key, Value>::copy_support(const __some_node_type& node) {
    vals.clear();
    vals.resize(node.values_size());
    vals.clear();
    children.clear();
    children.resize(node.children_size());

    for (typename __some_node_type::const_value_iterator i = node.values_begin(); i != node.values_end(); i++)
        vals.push_back(*i);

    has_children = node.get_has_children();
}

/*
  __trie_node_iterator: a basic iterator through a trie structure.
  It is a base class for both types of iterators: simple and level_iterator. 
  It corresponds to a path in the trie.

  The structure is stored using 2 objects:
  - a pointer to the last node in the trie path;
  - a stack of key-value pair iterators that give the
  key string followed to the last value. The last pair iterator in the
  stack is the value containing the actual value. The node is the one containing
  this last value;
*/

template<class Key, class Value, class MapType = linear_map<Key, Value>>
struct __trie_node_base_iterator {
    using map_type = MapType;
    using list_iterator = typename map_type::iterator;

    using node_type = __trie_node<Key, Value>;
    using node_ptr_type = node_type*;
    using self = __trie_node_base_iterator<Key, Value, MapType>;
    using link_type = node_ptr_type;
    using difference_type = ptrdiff_t;
    using iterator_category = std::bidirectional_iterator_tag;
    using value_iterator = typename node_type::value_iterator;
    using const_value_iterator = typename node_type::const_value_iterator;
    using children_iterator = typename node_type::children_iterator;
    using const_children_iterator = typename node_type::const_children_iterator;
    using base_ptr = typename node_type::base_ptr;

    /* This structure (iterator_stack) is used as a pool for the iterator list. Every time a new object is 
     created, a list of iterators is obtained from the pool. Each time an object dissapears
     it's stack goes to back to the pool. It's supposed to increase the efficiency of the
     iterator stack memory allocation
  */

    /* Now the stack itself, which is a vector of map_type iterators */
    struct iterator_stack : public std::vector<typename map_type::iterator> {
        unsigned _index; // The index of this particular stack in the iterator_stack_pool list
        using self = iterator_stack;
        using super = std::vector<typename map_type::iterator>;

        iterator_stack()
          : super() {
            _index = 0;
        }

        ~iterator_stack() = default;

        void release() {
            release_iters(_index);
        }

        self& operator=(const self& s) {
            super::operator=(s);
            return *this;
        }

        unsigned& index() {
            return _index;
        }

        unsigned index() const {
            return _index;
        }
    };

    using iterator_stack_type = iterator_stack;
    static std::vector<iterator_stack> iterator_stack_pool;
    static std::vector<unsigned> available_iterators;
    static const unsigned int pool_size = 1000l;
    static void Initialize() {
        iterator_stack_pool.resize(pool_size);
        available_iterators.resize(pool_size);
        std::iota(available_iterators.begin(), available_iterators.end(), 0);
        for (unsigned i = 0; i < pool_size; i++) {
            iterator_stack_pool[i]._index = available_iterators[i];
        }
    }

    static iterator_stack& get_iters() {
        if (available_iterators.empty()) {
            Initialize();
        }
        ON_DEBUG(
          if (available_iterators.empty()) {
              cerr << "Out of iterators !!" << endl;
              exit(1);
          })
        unsigned i = available_iterators.back();
        available_iterators.pop_back();
        iterator_stack_pool[i]._index = i;
        return iterator_stack_pool[i];
    }

    static void release_iters(unsigned i) {
        if (!iterator_stack_pool.empty()) {
            iterator_stack_pool[i].clear();
            available_iterators.push_back(i);
        }
    }

    base_ptr node;
    iterator_stack_type& value_iters;

    __trie_node_base_iterator()
      : value_iters(get_iters()) {}
    __trie_node_base_iterator(const self& it)
      : node(it.node)
      , value_iters(get_iters()) {
        value_iters = it.value_iters;
    }


    // Given a node in the trie, this constructor will
    // build the iterator whose value corresponds to the value
    // of the node's parent

    __trie_node_base_iterator(link_type x)
      : value_iters(get_iters()) {
        link_type node1 = x;
        node = node1;

        while (node1->get_parent()) {
            auto child = find(node1->get_parent()->children_begin(), node1->get_parent()->children_end(), node1);
            ON_DEBUG(
              assert(child != static_cast<node_type::children_iterator>(node1->get_parent()->children_end())));
            value_iters.push_back(static_cast<link_type>(node1->get_parent())->value_for_child(child));
            node1 = static_cast<link_type>(node1->get_parent());
        }

        reverse(value_iters.begin(), value_iters.end());
        ON_DEBUG(
          assert(node == 0 || node->get_parent() == 0 || node == *static_cast<link_type>(node->get_parent())->child_of(value_iters.back())));
    }

    //   Builds an iterator from a node and a value iterator in that last node.
    //   The last iterator in the stack it will be the transmitted iterator i
    // and the rest will be the ones corresponding to the node's sequence.

    __trie_node_base_iterator(link_type p, value_iterator i)
      : value_iters(get_iters()) {
        node = p;
        ON_DEBUG(
          assert(::valid_iterator(node->values_begin(), i, node->values_end())));
        value_iters.push_back(i);
        link_type node1 = static_cast<link_type>(node->get_parent());
        if (node1) {
            while (node1->get_parent()) {
                typename node_type::children_iterator child = find(node1->get_parent()->children_begin(), node1->get_parent()->children_end(), node1);
                ON_DEBUG(
                  assert(child != node1->get_parent()->children_end()));
                value_iters.push_back(node1->get_parent()->value_for_child(child));
                node1 = node1->get_parent();
            }
        }
    }

    /*
    Builds an iterator given a node and stack.
  */
    __trie_node_base_iterator(link_type nd, const iterator_stack_type& s)
      : node(nd)
      , value_iters(get_iters()) {
        value_iters = s;
    }

    virtual ~__trie_node_base_iterator() {
        value_iters.release();
    }

    self& operator=(const self& i) {
        if (this != &i) {
            value_iters = i.value_iters;
            node = i.node;
        }
        return *this;
    }

    /* Virtual pure methods that the descendents of this class have to implement */
    virtual void increment() = 0;
    virtual void decrement() = 0;

protected:
    void push_iterator(typename node_type::info_type::iterator i) {
        value_iters.push_back(i);
    }

    /* 
     Returns true if the current iterator is valid:
     1. node is null (in which case the iterator is null)
     _or_
     2a. the stack is empty and node->get_parent() is null 
     _and_
     2b. null is a valid iterator in the children list of its get_parent() and,
     furthermore, the top of the stack is a valid iterator in the list
     corresponding to the node
  */
    bool valid_iterator() {
        return node == NULL ||
               node->get_parent() == NULL ||
               ::valid_iterator(node->values_begin(), value_iters.back(), node->values_end());
    }
};

/* 
   __trie_node_base_iterator :  the trie base iterator.
   Iterates on all the stored sequences in a lexicographical manner.
*/

template<class Key, class Value, class MapType = linear_map<Key, Value>>
struct __trie_node_iterator : public __trie_node_base_iterator<Key, Value, MapType> {
    using node_type = __trie_node<Key, Value>;
    //   using base_ptr = node_type*;
    using iterator_category = std::bidirectional_iterator_tag;
    using difference_type = std::ptrdiff_t;
    using value_iterator = typename node_type::value_iterator;
    using children_iterator = typename node_type::children_iterator;
    using map_type = MapType;
    using rep_type = typename node_type::rep_type;
    using self = __trie_node_iterator<Key, Value, MapType>;
    using super = __trie_node_base_iterator<Key, Value, MapType>;
    using link_type = typename super::link_type;
    using base_ptr = typename super::base_ptr;

    /* The constructors call the parent constructors */
    __trie_node_iterator() = default;
    __trie_node_iterator(const self& i)
      : super(i) {}
    __trie_node_iterator(base_ptr node)
      : super(node) {}
    __trie_node_iterator(link_type p, value_iterator i)
      : super(p, i) {}
    __trie_node_iterator(base_ptr node, const typename super::iterator_stack_type& s)
      : super(node, s) {}

    /* 
     Increments the iterator such that it points to the next sequence in
     lexicographical order.
     Precondition: *this is a valid iterator;
     Postcondition: *this is a valid iterator;
  */

    void increment() override {
        if (!super::value_iters.empty()) {
            ON_DEBUG(
              assert(valid_iterator()););

            // Grab the last value iterator
            auto i = super::value_iters.back();
            auto c = super::node->child_of(i);


            if (c != super::node->children_end()) {
                // the current node has children i.e. there exists a possible continuation
                // of the current sequence
                i = (*c)->values_begin();
                super::node = *c;
            }
            else {
                // the current node has no children
                ++i;
                while (i == super::node->values_end()) {
                    super::value_iters.pop_back();
                    if (super::node->get_parent() == nullptr) { // reached the end !!
                        return;
                    }
                    super::node = super::node->get_parent();

                    i = super::value_iters.back();
                    ++i;
                }
                super::value_iters.pop_back();
            }
            super::value_iters.push_back(i);
        }

        ON_DEBUG(
          assert(valid_iterator()));
    }

    void decrement() override {
        ON_DEBUG(
          assert(valid_iterator()));

        if (!super::value_iters.empty()) {
            auto i = super::value_iters.back();
            auto c = super::node->child_of(i);

            if (c != super::node->children_end()) { // The current sequence has a possible continuation
                while (c != super::node->children_end()) {
                    super::value_iters.push_back(i);
                    i = super::node->values_end() - 1;
                    super::node = *c;
                    c = super::node->child_of(i);
                }
            }
            else {
                while (i == super::node->values_begin()) {
                    super::value_iters.pop_back();
                    if (super::node->get_parent() == nullptr) {
                        return;
                    }
                    super::node = super::node->get_parent();
                    i = super::value_iters.back();
                }
                super::value_iters.pop_back();
                --i;
                super::value_iters.push_back(i);
            }
        }

        ON_DEBUG(
          assert(valid_iterator()));
    }
};


/* 
   This class implement a special kind of iterator that will enumerate all
   stored sequences of a given length, in lexicographical order.
*/

template<class Key, class Value, class MapType = linear_map<Key, Value>>
struct __trie_node_level_iterator : public __trie_node_base_iterator<Key, Value, MapType> {
    using self = __trie_node_level_iterator<Key, Value, MapType>;
    using super = __trie_node_base_iterator<Key, Value, MapType>;
    using node_type = __trie_node<Key, Value>;
    using base_ptr = typename node_type::base_ptr;
    using iterator_category = std::bidirectional_iterator_tag;
    using difference_type = std::ptrdiff_t;
    using rep_type = typename node_type::rep_type;
    using link_type = typename super::link_type;
    using value_iterator = typename super::value_iterator;
    using children_iterator = typename super::children_iterator;

    __trie_node_level_iterator() {}
    __trie_node_level_iterator(const self& i)
      : super(i) {}
    __trie_node_level_iterator(base_ptr node)
      : super(node) {}
    __trie_node_level_iterator(link_type p, typename super::value_iterator i)
      : super(p, i) {}
    __trie_node_level_iterator(base_ptr node, const typename super::iterator_stack_type& s)
      : super(node, s) {}

    self& operator=(const self& i) {
        super::operator=(i);
        return *this;
    }

    self& operator=(const super& i) {
        super::operator=(i);
        return *this;
    }

    // This method moves the iterator to the next sequence (in lexicographical order) of same
    // length as the one represented by the current iterator
    // Reminder: super::value_iters.back is a valid iterator in the node->vals list

    void increment() override {
        ON_DEBUG(
          assert(valid_iterator()));

        if (!super::value_iters.empty()) {
            auto i = super::value_iters.back();
            int level = 0;

            ++i;
            children_iterator c1;
            while (i == super::node->values_end()) {
                super::value_iters.pop_back();

                if (super::node->get_parent() == nullptr) { // we have reached the end()
                    return;
                }

                super::node = super::node->get_parent();
                level++;
                i = super::value_iters.back();
                ++i;

                while (i != super::node->values_end()) {
                    if (super::node->child_of(i) != super::node->children_end()) {
                        c1 = super::node->child_of(i);
                        if ((*c1)->get_depth() >= level) {
                            // we have found a node with a "long-enough" continuation
                            break;
                        }
                    }
                    ++i;
                }
            }
            super::value_iters.back() = i;

            while (level > 0) {
                level--;
                super::node = *super::node->child_of(i);
                i = super::node->values_begin();
                if (level != 0) {
                    while (i != super::node->values_end()) {
                        c1 = super::node->child_of(i);
                        if (c1 != super::node->children_end() && (*c1)->get_depth() >= level) {
                            break;
                        }
                        ++i;
                    }
                }
                ON_DEBUG(
                  assert(i != super::node->values_end()));
                super::value_iters.push_back(i);
            }
        }

        ON_DEBUG(
          assert(valid_iterator()));
    }

    void decrement() override {
        ON_DEBUG(
          assert(valid_iterator()));

        if (!super::value_iters.empty()) {
            auto i = super::value_iters.back();
            int level = 0;

            while (i == super::node->values_begin()) {
                if (super::node->get_parent() == nullptr) {
                    return;
                }
                super::node = super::node->get_parent();
                level++;

                super::value_iters.pop_back();
                i = super::value_iters.back();
                if (i != super::node->values_begin()) {
                    --i;
                }

                while (i != super::node->values_begin()) {
                    link_type node1 = super::node->child_on_pos(i - super::node->values_begin());
                    if (node1 && node1->get_depth() >= level - 1) {
                        super::node = node1;
                        break;
                    }
                    --i;
                }
            }

            super::value_iters.back() = i;

            link_type node1 = super::node;
            while (level > 0) {
                level--;
                i = super::node->values_end() - 1;
                if (level != 0) {
                    while (i != super::node->values_end()) {
                        node1 = super::node->child_on_pos(i - super::node->values_begin());
                        if (node1 && node1->get_depth() >= level - 1) {
                            break;
                        }
                        --i;
                    }
                }
                super::node = node1;
                super::value_iters.push_back(i);
            }
        }

        ON_DEBUG(assert(super::node == 0 || valid_iterator()));
    }
};

// template <class Key, class Value, class Ref, class Ptr, class __trie_node_type, class base_iterator = __trie_node_iterator<Key, Value, linear_map<Key, Value> >
template<class Key, class Value, class Ref, class Ptr, class __trie_node_type, class base_iterator = __trie_node_iterator<Key, Value, linear_map<Key, Value>>>
struct __trie_iterator : public base_iterator {
    using super = base_iterator;
    using sequence_type = std::vector<Key>;
    using reference = Ref;
    using pointer = Ptr;
    using value_type = m_pair<const sequence_type&, reference>;
    using const_value_type = m_pair<const sequence_type&, const reference>;
    using node_type = __trie_node_type;

    using iterator = __trie_iterator<Key, Value, Ref, Ptr, node_type, base_iterator>;
    using const_iterator = __trie_iterator<Key, Value, const Ref, const Ptr, node_type, base_iterator>;
    using self = __trie_iterator<Key, Value, Ref, Ptr, node_type, base_iterator>;

    using value_iterator = typename base_iterator::value_iterator;
    using children_iterator = typename base_iterator::children_iterator;
    using iterator_stack_type = typename base_iterator::iterator_stack_type;

    using link_type = node_type*;
    using rep_type = typename node_type::rep_type;


    __trie_iterator() = default;
    __trie_iterator(link_type x)
      : base_iterator(x) {}
    __trie_iterator(link_type p, value_iterator i)
      : base_iterator(p, i)
      , sequence_cached(false) {}
    __trie_iterator(const iterator& it)
      : base_iterator(it) {}
    __trie_iterator(link_type x, const iterator_stack_type& s)
      : base_iterator(x, s)
      , sequence_cached(false) {}

    //   value_type operator*() {
    //     if(! sequence_cached)
    //       compute_sequence();
    //     return value_type(sequence, (*this->value_iters.back()).second);
    //   }

    //   const_value_type operator*() const {
    //     if(! sequence_cached)
    //       compute_sequence();
    //     return const_value_type(sequence, (*this->value_iters.back()).second);
    //   }
    value_type operator*() {
        if (!sequence_cached) {
            compute_sequence();
        }
        return value_type(sequence, (*this->value_iters.back()).second);
    }

    const_value_type operator*() const {
        if (!sequence_cached)
            compute_sequence();
        return const_value_type(sequence, (*this->value_iters.back()).second);
    }

    self& operator++() {
        sequence_cached = false;
        this->increment();
        return *this;
    }
    self operator++(int) {
        sequence_cached = false;
        self tmp = *this;
        this->increment();
        return tmp;
    }

    self& operator--() {
        sequence_cached = false;
        this->decrement();
        return *this;
    }
    self operator--(int) {
        sequence_cached = false;
        self tmp = *this;
        this->decrement();
        return tmp;
    }

#ifdef __STL_MEMBER_TEMPLATES
    template<class Iterator>
    self& operator=(const Iterator& i) {
        base_iterator::operator=(i);
        if (i.sequence_cached) {
            sequence_cached = 1;
            sequence = i.sequence;
        }
        return *this;
    }
#endif

    //   void compute_sequence() const {
    //     sequence.clear();
    //     link_type nd;
    //     if(super::value_iters.size()>1)
    //       nd = static_cast<link_type>((*super::value_iters[0])->parent);
    //     else
    //       nd = static_cast<link_type>(node);
    //     for(__trie_node_base_iterator::iterator_stack_type::const_iterator i=super::value_iters.begin() ; i!=super::value_iters.end() ; ++i) {
    //       sequence.push_back((nd->vals.begin() + (*i-nd->children.begin()))->first);
    //       nd = static_cast<link_type>(**i);
    //     }
    //     sequence_cached = true;
    //   }

    void compute_sequence() const {
        sequence.clear();
        for (auto i = super::value_iters.begin(); i != super::value_iters.end(); ++i) {
            sequence.push_back((*i)->first);
        }
        sequence_cached = true;
    }

    mutable sequence_type sequence;
    mutable bool sequence_cached{ false };
};

template<class Key, class Value, class MapType>
inline bool operator==(const __trie_node_base_iterator<Key, Value, MapType>& x,
  const __trie_node_base_iterator<Key, Value, MapType>& y) {
    return x.node == y.node && x.value_iters == y.value_iters;
}

template<class Key, class Value, class MapType>
inline bool operator!=(const __trie_node_base_iterator<Key, Value, MapType>& x,
  const __trie_node_base_iterator<Key, Value, MapType>& y) {
    return x.node != y.node || x.value_iters != y.value_iters;
}

// #ifndef __STL_CLASS_PARTIAL_SPECIALIZATION

// inline bidirectional_iterator_tag
// iterator_category(const __trie_base_iterator&) {
//   return bidirectional_iterator_tag();
// }

// inline __trie_base_iterator::difference_type*
// distance_type(const __trie_base_iterator&) {
//   return (__trie_base_iterator::difference_type*) 0;
// }

// template <class Value, class Ref, class Ptr>
// inline pair<vector<Value>, Ref> value_type(const __trie_iterator<Value, Ref, Ptr>&) {
//   return make_pair(vector<Value> (), 0);
// }

// inline __trie_base_iterator::difference_type*
// distance_type(const __trie_base_level_iterator&) {
//   return (__trie_base_iterator::difference_type*) 0;
// }

// template <class Value, class Ref, class Ptr>
// inline *pair<vector<Value>, Ref> value_type(const __trie_level_iterator<Value, Ref, Ptr>&) {
//   return (pair<vector<Value>, Ref> *)(0);
// }

// #endif /* __STL_CLASS_PARTIAL_SPECIALIZATION */

#include <iostream>

template<class Key, class Value, class __trie_node_type = __trie_node<Key, Value>>
class trie {
public:
    using void_pointer = void*;
    using base_ptr = __trie_node_type*;
    using node_type = __trie_node_type;

public:
    using key_type = std::vector<Key>;
    using value_type = m_pair<const key_type&, Value>;

    using link_type = __trie_node_type*;
    using link_const_type = const __trie_node_type*;
    using size_type = size_t;
    using difference_type = ptrdiff_t;
    using iterator = __trie_iterator<Key, Value, Value&, Value*, node_type>;
    using const_iterator = __trie_iterator<Key, Value, const Value&, const Value*, node_type>;
    using level_iterator = __trie_iterator<Key, Value, Value&, Value*, node_type, __trie_node_level_iterator<Key, Value>>;
    using const_level_iterator = __trie_iterator<Key, Value, const Value&, const Value*, node_type, __trie_node_level_iterator<Key, Value>>;
    using self = trie<Key, Value, node_type>;

protected:
    using level_iterator_vector = std::vector<level_iterator>;
    using iterator_stack_type = typename iterator::iterator_stack_type;

    link_type root;

    iterator my_end;
    const_iterator my_const_end;
    level_iterator my_level_end;
    const_level_iterator my_const_level_end;

    mutable level_iterator_vector begin_iterators;

    mutable bool initial_iterators_valid{ false };
    Value default_value;

public:
    trie(Value d = 0)
      : root(new node_type)
      , my_end(root)
      , my_const_end(root)
      , my_level_end(root)
      , my_const_level_end(root)
      ,

      default_value(d) {}

    trie(const self& t)
      : my_end()
      , my_const_end()
      , my_level_end()
      , my_const_level_end() {
        root = new node_type;
        root->clone(t.get_root());
        set_end_iterators();
        construct_initial_level_iterators();
        default_value = t.default_value;
    }

    virtual ~trie() {
        destroy();
        delete root;
    }

protected:
    void set_end_iterators() {
        my_end.node = root;
        my_const_end.node = root;
        my_level_end.node = root;
        my_const_level_end.node = root;
    }

public:
    // #ifdef __STL_MEMBER_TEMPLATES
    template<class InputIterator>
    std::pair<iterator, bool> insert(InputIterator start, InputIterator end);
    // #endif

    //   std::pair<iterator, bool> insert(Key* start, Key* end);

    std::pair<iterator, bool> insert(const key_type& v) {
        return insert(v.begin(), v.end());
    }

    Value& operator[](const key_type& v) {
        return value(v.begin(), v.end());
    }

    const Value operator[](const key_type& v) const {
        return value(v.begin(), v.end());
    }

    Value& value(const Key* start, const Key* end) {
        return (*(insert(start, end).first)).second;
    }

    Value value(const Key* start, const Key* end) const {
        const_iterator i = find(start, end);
        if (i == this->end())
            return default_value;
        return (*i).second;
    }

    // #ifdef __STL_MEMBER_TEMPLATES
    template<class InputIterator>
    Value& value(InputIterator start, InputIterator end) {
        static iterator i;
        i = insert(start, end).first;
        return (*i).second;
    }

    template<class InputIterator>
    Value value(InputIterator start, InputIterator end) const {
        static const_iterator i;
        i = find(start, end);
        if (i == this->end())
            return 0;
        return (*i).second;
    }
    // #endif

    void erase(const Key* start, const Key* end);
    void erase(const key_type& v) {
        erase(v.begin(), v.end());
    }

    // #ifdef __STL_MEMBER_TEMPLATES
    template<class InputIterator>
    void erase(InputIterator start, InputIterator end);
    // #endif

    //   const_iterator find(const Key* start, const Key* end) const;
    const_iterator find(const key_type& v) const {
        link_type node = root, parent = 0;
        const_iterator it;

        for (typename key_type::const_iterator i = v.begin(); i != v.end(); ++i) { // for each key in the sequence
            if (node == 0) {                                                       // At the previous step, the value did not have any continuations
                return this->end();
            }

            typename node_type::info_type::iterator j = node->values_find(*i);
            if (j == node->values_end()) { // the key was not found
                return this->end();
            }

            it.value_iters.push_back(j);
            parent = node;

            if (i != v.end() - 1) {
                node = static_cast<link_type>(node->child_on_pos(j - node->values_begin()));
            }
        }

        // As the node of the iterator, the last visited node will be assigned.
        it.node = parent;
        return it;
        //     return find(v.begin(), v.end());
    }

    //   iterator find(const Key* start, const Key* end);
    iterator find(const key_type& v) {
        link_type node = root, parent = 0;
        iterator it;

        for (typename key_type::const_iterator i = v.begin(); i != v.end(); ++i) { // for each key in the sequence
            if (node == 0)                                                         // At the previous step, the value did not have any continuations
                return this->end();

            typename node_type::info_type::iterator j = node->values_find(*i);
            if (j == node->values_end()) // the key was not found
                return this->end();

            it.value_iters.push_back(j);
            parent = node;

            if (i != end - 1)
                node = static_cast<link_type>(node->child_on_pos(j - node->values_begin()));
        }

        // As the node of the iterator, the last visited node will be assigned.
        it.node = parent;
        return it;
        //   }
        //     return find(v.begin(), v.end());
    }

    // This method will return a value instead of an iterator, to
    // conserve the resources for the iterator (constructor, destructor, etc).
    // If no value is found for the given key, the default value is returned.
    Value find_value(const key_type& v) const {
        return find_value(v.begin(), v.end());
    }

    const_iterator find(typename key_type::const_iterator start, typename key_type::const_iterator end) const;
    iterator find(typename key_type::const_iterator start, typename key_type::const_iterator end) {
        link_type node = root, parent = 0;
        iterator it;

        for (typename key_type::const_iterator i = start; i != end; ++i) { // for each key in the sequence
            if (node == 0)                                                 // At the previous step, the value did not have any continuations
                return this->end();

            typename node_type::info_type::iterator j = node->values_find(*i);
            if (j == node->values_end()) // the key was not found
                return this->end();

            it.value_iters.push_back(j);
            parent = node;

            if (i != end - 1)
                node = static_cast<link_type>(node->child_on_pos(j - node->values_begin()));
        }

        // As the node of the iterator, the last visited node will be assigned.
        it.node = parent;
        return it;
    }

    Value find_value(typename key_type::const_iterator start, typename key_type::const_iterator end) const;

    // #ifdef __STL_MEMBER_TEMPLATES
    //   template <class InputIterator>
    //   const_iterator find(InputIterator start, InputIterator end) const;

    //   template <class InputIterator>
    //   iterator find(InputIterator start, InputIterator end);

    //   template <class InputIterator>
    //   Value find_value(InputIterator start, InputIterator end) const;
    // #endif

    virtual self& operator=(const self& t) {
        if (this == &t) {
            return *this;
        }
        destroy();
        //     root = new node_type;
        root->clone(t.get_root());
        set_end_iterators();
        default_value = t.default_value;
        return *this;
    }

    void destroy() {
        root->destroy();
        begin_iterators.clear();
        invalidate_iterators();
    }

    void clear() {
        //     delete root;
        //     root = 0L;
        root->clear();
        begin_iterators.clear();
        invalidate_iterators();
    }

    void swap(self& t) {
        std::swap(root, t.root);
        std::swap(my_end, t.my_end);
        std::swap(my_const_end, t.my_const_end);
        std::swap(my_level_end, t.my_level_end);
        std::swap(my_const_level_end, t.my_const_level_end);
        std::swap(default_value, t.default_value);
        begin_iterators.swap(t.begin_iterators);
    }

    const link_type get_root() const {
        return root;
    }

    iterator begin() {
        if (root->values_size() > 0)
            return iterator(root, root->values_begin());
        else
            return end();
    }
    const_iterator begin() const {
        if (root->values_size() > 0)
            return const_iterator(root, root->values_begin());
        else
            return end();
    }

    iterator& end() {
        ON_DEBUG(assert(my_end.value_iters.size() == 0));
        return my_end;
    }

    const const_iterator& end() const {
        ON_DEBUG(assert(my_const_end.value_iters.size() == 0));
        return my_const_end;
    }

    level_iterator begin(int level) {
        validate_iterators();
        if (level <= root->get_depth())
            return begin_iterators[level];
        else
            return end(level);
    }

    const_level_iterator begin(int level) const {
        validate_iterators();
        if (level <= root->get_depth())
            return const_level_iterator(static_cast<link_type>(begin_iterators[level].node), begin_iterators[level].value_iters);
        else
            return end(level);
    }

    level_iterator& end(int level) {
        ON_DEBUG(assert(my_level_end.value_iters.size() == 0));
        return my_level_end;
    }

    const const_level_iterator& end(int level) const {
        ON_DEBUG(assert(my_const_level_end.value_iters.size() == 0));
        return my_const_level_end;
    }

#ifdef __STL_MEMBER_TEMPLATES
    template<class Iterator>
    void print_iterator(std::ostream&, const Iterator&);
#endif

    void shrink() {
        std::queue<link_type> q;
        q.push(root);
        while (!q.empty()) {
            link_type node = q.front();
            q.pop();
            if (node == 0)
                continue;
            node->shrink();
            for (typename node_type::rep_type::iterator i = node->children_begin(); i != node->children_end(); ++i)
                q.push(static_cast<link_type>(*i));
        }
    }

    iterator parent_of_iterator(iterator& i) {
        if (i.node->get_parent() == 0)
            return end();
        iterator_stack_type s = i.value_iters;
        s.pop_back();
        return iterator(static_cast<link_type>(i.node->get_parent()), s);
    }

    const_iterator parent_of_iterator(const_iterator& i) const {
        if (i.node->get_parent() == 0) {
            return end();
        }
        iterator_stack_type s = i.value_iters;
        s.pop_back();
        return const_iterator(static_cast<link_type>(i.node->get_parent()), s);
    }

    level_iterator parent_of_iterator(const level_iterator& i) {
        if (i.node->get_parent() == 0)
            return end(0);
        iterator_stack_type s = i.value_iters;
        s.pop_back();
        return level_iterator(static_cast<link_type>(i.node->get_parent()), s);
    }

    const_level_iterator parent_of_iterator(const_level_iterator& i) const {
        if (i.node->get_parent() == 0) {
            return end(0);
        }
        iterator_stack_type s = i.value_iters;
        s.pop_back();
        return const_level_iterator(static_cast<link_type>(i.node->get_parent()), s);
    }

    short int depth() const {
        return root->get_depth();
    }

    friend std::ostream& operator<<(std::ostream&, const self&);
    friend std::istream& operator>>(std::istream&, self&);

    int read_in_binary_format(std::istream& /*istr*/);
    void write_in_binary_format(std::ostream& /*ostr*/) const;

protected:
    void construct_initial_level_iterators() const {
        begin_iterators.resize(root->get_depth() + 1);
        iterator i(root, root->values_begin());
        std::set<int> s;
        for (int j = 1; j <= root->get_depth(); j++)
            s.insert(j);
        while (!s.empty()) {
            //       dmalloc_verify(0);
            if (s.erase(i.value_iters.size()) > 0) {
                begin_iterators[i.value_iters.size()] = i;
            }
            ++i;
            //       dmalloc_verify(0);
        }
        initial_iterators_valid = true;
    }

    void validate_iterators() const {
        if (!initial_iterators_valid)
            construct_initial_level_iterators();
    }
    void invalidate_iterators() const { initial_iterators_valid = false; }
};

// #ifdef __STL_MEMBER_TEMPLATES
template<class Key, class Value, class __trie_node_type>
template<class InputIterator>
std::pair<typename trie<Key, Value, __trie_node_type>::iterator, bool> trie<Key, Value, __trie_node_type>::insert(InputIterator start, InputIterator end) {
    link_type node = root;
    //   __trie_node_base_iterator::iterator_stack_type s;
    iterator it;
    bool inserted = false;
    int pos = 0;

    for (InputIterator i = start; i != end; ++i) {
        std::pair<typename node_type::info_type::iterator, bool> p = node->insert_value(std::pair<Key, Value>(*i, default_value));
        pos = p.first - node->values_begin();

        if (p.second) {                      // if the current value was inserted in the value list
            p.first->second = default_value; // first occurence -> the associated value is 0
            inserted = true;
        }

        if (i != end - 1) {
            node = static_cast<link_type>(node->insert_or_retrieve_child_at_pos(pos));
        }
        it.value_iters.push_back(p.first);
    }

    // Update the depths on the path from the root to the current node
    node->update_depth();

    if (inserted) {
        invalidate_iterators();
        // If the length of the sequence is greater than the root depth -
        // recompute the depth (we inserted a ``longest'' sequence)
        if (distance(start, end) > root->get_depth()) {
            root->initialize();
        }
    }
    it.node = node;
    return make_pair(it, inserted);
}

// template <class Key, class Value, class __trie_node_type>
// trie<Key, Value, __trie_node_type>::iterator
// trie<Key, Value, __trie_node_type>::find(typename key_type::const_iterator start, typename key_type::const_iterator end)

template<class Key, class Value, class __trie_node_type>
typename trie<Key, Value, __trie_node_type>::const_iterator
trie<Key, Value, __trie_node_type>::find(typename key_type::const_iterator start, typename key_type::const_iterator end) const {
    link_type node = root, parent = 0;
    //   __trie_node_base_iterator::iterator_stack_type s;
    const_iterator it;

    for (typename key_type::const_iterator i = start; i != end; ++i) { // for each key in the sequence
        if (node == 0)                                                 // At the previous step, the value did not have any continuations
            return this->end();

        typename node_type::value_iterator j = node->values_find(*i);
        if (j == node->values_end()) // the key was not found
            return this->end();

        it.value_iters.push_back(j);
        parent = node;

        if (i != end - 1)
            node = static_cast<link_type>(node->child_on_pos(j - node->values_begin()));
    }

    // As the node of the iterator, the last visited node will be assigned.
    it.node = parent;

    return it;
}

template<class Key, class Value, class __trie_node_type>
Value trie<Key, Value, __trie_node_type>::find_value(typename key_type::const_iterator start, typename key_type::const_iterator end) const {
    link_type node = root;
    typename node_type::value_iterator j;
    //   __trie_node_base_iterator::iterator_stack_type s;

    for (typename key_type::const_iterator i = start; i != end; ++i) { // for each key in the sequence
        if (node == 0)                                                 // At the previous step, the value did not have any continuations
            return default_value;

        j = node->values_find(*i);
        if (j == node->values_end()) // the key was not found
            return default_value;

        if (i != end - 1)
            node = static_cast<link_type>(node->child_on_pos(j - node->values_begin()));
        else
            return j->second;
    }
    return default_value;
}

template<class Key, class Value, class __trie_node_type>
template<class InputIterator>
void trie<Key, Value, __trie_node_type>::erase(InputIterator start, InputIterator end) {
    invalidate_iterators();
    iterator i = find(start, end);
    link_type node = i.node, node1;
    typename node_type::value_iterator p = i.value_iters.back();
    node->erase(p);
    while (node.values.size() == 0) {
        node1 = node;
        node = node->get_parent();
        delete node1;
        i.value_iters.pop_back();
        node->erase(i.value_iters.back());
    }
}
// #endif

#ifdef __STL_FUNCTION_TMPL_PARTIAL_ORDER
template<class Key, class Value, class __trie_node_type>
inline void swap(trie<Key, Value>& x,
  trie<Key, Value>& y) {
    x.swap(y);
}

template<class Key, class Value, class __trie_node_type>
template<class Iterator>
void trie<Key, Value, __trie_node_type>::print_iterator(std::ostream& ostr, const Iterator& i) {
    const iterator::value_type p = *i;
    for (iterator::sequence_type::const_iterator j = p.first.begin(); j != p.first.end(); j++)
        cout << *j;
    cout << " " << p.second;
}

#endif /* __STL_FUNCTION_TMPL_PARTIAL_ORDER */

template<class Key, class Value, class __trie_node_type>
std::ostream& operator<<(std::ostream& ostr, const trie<Key, Value, __trie_node_type>& t) {
    std::stack<__trie_node_type*> s;
    s.push(static_cast<__trie_node_type*>(t.root));
    __trie_node_type nd;
    while (!s.empty()) {
        __trie_node_type* c_node = s.top();
        s.pop();
        if (c_node) {
            c_node->write_in_text_format(ostr);

            for (typename __trie_node_type::reverse_children_iterator j = c_node->children_rbegin(); j != c_node->children_rend(); ++j)
                s.push(static_cast<__trie_node_type*>(*j));
        }
        else
            nd.write_in_text_format(ostr);
    }
    return ostr;
}

template<class Key, class Value, class __trie_node_type>
std::istream& operator>>(std::istream& istr, trie<Key, Value, __trie_node_type>& t) {
    t.clear();
    std::stack<typename __trie_node_type::reverse_children_iterator> s;
    typename std::iterator_traits<typename __trie_node_type::rep_type::iterator>::value_type c_node = 0;
    //   t.root = 0;
    bool first = true;
    typename __trie_node_type::rep_type tmp_vect;
    s.push(static_cast<typename __trie_node_type::reverse_children_iterator>(&c_node));
    __trie_node_type nd;

    while (!s.empty()) {
        typename __trie_node_type::reverse_children_iterator p_c_node = s.top();
        s.pop();
        if (!nd.read_in_text_format(istr))
            return istr;

        if (nd.values_size() != 0) {
            if (first)
                c_node = t.root;
            else {
                c_node = new __trie_node_type;
                *p_c_node = c_node;
            }

            first = false;
            c_node->swap(nd);

            for (typename __trie_node_type::reverse_children_iterator i = c_node->children_rbegin(); i != c_node->children_rend(); ++i) {
                *i = 0;
                s.push(static_cast<typename __trie_node_type::reverse_children_iterator>(i));
            }
        }
    }

    t.root->initialize();
    return istr;
}

template<class Key, class Value, class __trie_node_type>
int trie<Key, Value, __trie_node_type>::read_in_binary_format(std::istream& istr) {
    clear();
    std::stack<typename __trie_node_type::reverse_children_iterator> s;
    typename __trie_node_type::rep_type::value_type c_node = 0;
    //   root = 0;
    bool first = true;
    typename __trie_node_type::rep_type tmp_vect;
    s.push(static_cast<typename __trie_node_type::reverse_children_iterator>(&c_node));
    __trie_node_type nd;

    while (!s.empty()) {
        typename __trie_node_type::reverse_children_iterator p_c_node = s.top();
        s.pop();
        if (!nd.read_in_binary_format(istr))
            return 0;

        if (nd.values_size() != 0) {
            if (first)
                c_node = root;
            else {
                c_node = new __trie_node_type;
                *p_c_node = c_node;
            }
            nd.swap(*c_node);

            first = false;
            for (typename __trie_node_type::reverse_children_iterator i = c_node->children_rbegin(); i != c_node->children_rend(); ++i) {
                *i = 0;
                s.push(i);
            }
        }
    }
    root->initialize();
    return 1;
}

template<class Key, class Value, class __trie_node_type>
void trie<Key, Value, __trie_node_type>::write_in_binary_format(std::ostream& ostr) const {
    std::stack<__trie_node_type*> s;
    s.push(static_cast<__trie_node_type*>(root));
    int size;
    __trie_node_type nd;
    while (!s.empty()) {
        __trie_node_type* c_node = s.top();
        s.pop();
        if (c_node) {
            size = c_node->values_size();
            // 	  ON_DEBUG(assert(size>0));
            // Write the node
            c_node->write_in_binary_format(ostr);
            const __trie_node_type* c1 = c_node;
            // Push the children of this node onto the queue
            for (typename __trie_node_type::const_reverse_children_iterator j = c1->children_rbegin();
                 j != c1->children_rend();
                 ++j)
                s.push(static_cast<__trie_node_type*>(*j));
        }
        else
            nd.write_in_binary_format(ostr);
    }
}

template<class Key, class Value, class MapType>
std::vector<typename __trie_node_base_iterator<Key, Value, MapType>::iterator_stack> __trie_node_base_iterator<Key, Value, MapType>::iterator_stack_pool;

template<class Key, class Value, class MapType>
std::vector<unsigned int> __trie_node_base_iterator<Key, Value, MapType>::available_iterators;

#endif

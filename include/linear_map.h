// -*- C++ -*-

/*  linear_map - template
	
  This is an implementation of a map using arrays. It does not satisfy the running time
  constraints of a Sorted Associative Container (insertion and deletion are O(n)). 
  It is constructed mostly for searching; it has the advantage that the storage space is 
  kept as low as possible.

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

#ifndef __linear_map_h_
#define __linear_map_h_

#include <vector>
#include <utility>
#include <algorithm>
#include <functional>
#include <numeric>

#include "debug.h"

#ifndef __STL_LIMITED_DEFAULT_TEMPLATES
template<class Key, class T, class Compare = std::less<Key>>
#else
template<class Key, class T, class Compare>
#endif
class linear_map {
public:
    using key_type = Key;
    using data_type = T;
    using mapped_type = bool;
    using value_type = std::pair<Key, T>;
    using key_compare = std::less<char>;
    using self = linear_map<Key, T, Compare>;

    class value_compare {
        friend class linear_map<Key, T, Compare>;

    public:
        Compare comp;
        value_compare(Compare c)
          : comp(c) {}
        value_compare()
          : comp(Compare()) {}

    public:
        bool operator()(const value_type& x, const value_type& y) const {
            return comp(x.first, y.first);
        }
    };

    using compare_function_type = value_compare;

protected:
    using rep_type = std::vector<value_type>;

    rep_type t;
    static value_compare pair_comp;

public:
    using pointer = typename rep_type::pointer;
    using const_pointer = typename rep_type::const_pointer;
    using reference = typename rep_type::reference;
    using const_reference = typename rep_type::const_reference;
    using iterator = typename rep_type::iterator;
    using const_iterator = typename rep_type::const_iterator;
    using reverse_iterator = typename rep_type::reverse_iterator;
    using const_reverse_iterator = typename rep_type::const_reverse_iterator;
    using size_type = typename rep_type::size_type;
    using difference_type = typename rep_type::difference_type;

    linear_map() = default;
    //   explicit linear_map(Compare comp) : pair_comp(comp) {}

#ifdef __STL_MEMBER_TEMPLATES
    template<class InputIterator>
    linear_map(InputIterator first, InputIterator last)
      : t(distance(first, last)) { t.insert(first, last); }

    template<class InputIterator>
    linear_map(InputIterator first, InputIterator last, Compare comp)
      : t(distance(first, last)) { t.insert(first, last); }
#else  /* __STL_MEMBER_TEMPLATES */
    linear_map(const value_type* first, const value_type* last)
      : t(distance(first, last)) { t.insert(first, last); }

    linear_map(const value_type* first, const value_type* last, Compare comp)
      : t(distance(first, last)) { t.insert(first, last); }

    linear_map(const_iterator first, const_iterator last)
      : t(distance(first, last)) { t.insert(first, last); }
    linear_map(const_iterator first, const_iterator last, Compare comp)
      : t(distance(first, last)) { t.insert(first, last); }
#endif /* __STL_MEMBER_TEMPLATES */

    linear_map(const linear_map<Key, T, Compare>& x)
      : t(x.t) {}
    linear_map<Key, T, Compare>& operator=(const linear_map<Key, T, Compare>& x) {
        if (this == &x) {
            return *this;
        }
        t = x.t;
        return *this;
    }

    virtual ~linear_map() = default;

    // accessors
    //   key_compare key_comp() const { return pair_comp.comp; }
    //   value_compare value_comp() const { return pair_comp; }
    iterator begin() { return t.begin(); }
    const_iterator begin() const { return t.begin(); }
    iterator end() { return t.end(); }
    const_iterator end() const { return t.end(); }
    reverse_iterator rbegin() { return t.rbegin(); }
    const_reverse_iterator rbegin() const { return t.rbegin(); }
    reverse_iterator rend() { return t.rend(); }
    const_reverse_iterator rend() const { return t.rend(); }
    bool empty() const { return t.empty(); }
    size_type size() const { return t.size(); }
    size_type max_size() const { return t.max_size(); }
    T& operator[](const key_type& k) {
        return (*((insert(value_type(k, T()))).first)).second;
    }

    void swap(linear_map<Key, T, Compare>& x) {
        t.swap(x.t);
        // 	value_compare t = x.pair_comp;
        // 	x.pair_comp = pair_comp;
        // 	pair_comp = t;
    }

    std::pair<iterator, bool> insert(const value_type& x) {
        auto i = lower_bound(x.first);

        // If x is not smaller than i, then x is equal to i
        if (i != end() && !pair_comp(x, *i)) {
            return make_pair(i, false);
        }
        {
            return make_pair(t.insert(i, x), true);
        }
    }

    // Inserts x before pos, considering that pos is the correct position
    // for inserting
    iterator insert(iterator pos, const value_type& x) {
#ifdef DEBUG
        assert((pos == end() || pair_comp(x, *pos)) && (pos == begin() || pair_comp(*(pos - 1), x)));
#endif
        return t.insert(pos, x);
    }

#ifdef __STL_MEMBER_TEMPLATES
    template<class InputIterator>
    void insert(InputIterator first, InputIterator last) {
        for (InputIterator i = first; i != last; i++)
            insert(*i);
    }
#else
    void insert(const value_type* first, const value_type* last) {
        for (value_type* i = first; i != last; i++)
            insert(*i);
    }
    void insert(const_iterator first, const_iterator last) {
        for (const_iterator* i = first; i != last; i++)
            insert(*i);
    }
#endif /* __STL_MEMBER_TEMPLATES */

    void erase(iterator position) {
        t.erase(position);
    }

    size_type erase(const key_type& x) {
        iterator i = find(x);
        if (i != end())
            t.erase(i);
        return 0;
    }
    void erase(iterator first, iterator last) { t.erase(first, last); }

#ifdef __STL_MEMBER_TEMPLATES
    template<class InputIterator>
    void erase(InputIterator pos) {
        erase(pos->first);
    }

    template<class InputIterator>
    void erase(InputIterator first, InputIterator end) {
        for (InputIterator i = first; i != last; i++)
            erase(i);
    }
#endif

    void resize(int i) {
        t.resize(i);
    }

    void clear() {
        t.clear();
    }

    void destroy() {
        clear();
        rep_type t1;
        t.swap(t1);
    }

    iterator find(const key_type& x) {
        iterator i = lower_bound(x);
        return i != end() && !pair_comp.comp(x, i->first) ? i : end();
    }

    const_iterator find(const key_type& x) const {
        const_iterator i = lower_bound(x);
        return i != end() && !pair_comp.comp(x, i->first) ? i : end();
    }

    iterator find(const key_type& x, iterator lb, iterator ub) {
        iterator i = lower_bound(x, lb, ub);
        return i != end() && !pair_comp.comp(x, i->first) ? i : end();
    }

    const_iterator find(const key_type& x, const_iterator lb, const_iterator ub) const {
        const_iterator i = lower_bound(x, lb, ub);
        return i != end() && !pair_comp.comp(x, i->first) ? i : end();
    }

    iterator lower_bound(const key_type& x) {
        return std::lower_bound(begin(), end(), value_type(x, T()), pair_comp);
    }
    const_iterator lower_bound(const key_type& x) const {
        return std::lower_bound(begin(), end(), value_type(x, T()), pair_comp);
    }

    iterator lower_bound(const key_type& x, iterator lb, iterator ub) {
        return std::lower_bound(lb, ub, value_type(x, T()), pair_comp);
    }

    const_iterator lower_bound(const key_type& x, const_iterator lb, const_iterator ub) const {
        return std::lower_bound(lb, ub, value_type(x, T()), pair_comp);
    }

    iterator upper_bound(const key_type& x) {
        return std::upper_bound(begin(), end(), value_type(x, T()), pair_comp);
    }
    const_iterator upper_bound(const key_type& x) const {
        return std::upper_bound(begin(), end(), value_type(x, T()), pair_comp);
    }
    std::pair<iterator, iterator> equal_range(const key_type& x) {
        return std::equal_range(begin(), end(), value_type(x, T()), pair_comp);
    }
    std::pair<const_iterator, const_iterator> equal_range(const key_type& x) const {
        return std::equal_range(begin(), end(), value_type(x, T()), pair_comp);
    }

    friend bool operator==(const linear_map&, const linear_map&);
    friend bool operator<(const linear_map&, const linear_map&);

    void shrink() {
        t.shrink_to_fit();
    }

    int capacity() const {
        return t.capacity();
    }

    void push_back(const value_type& x) {
        if (t.size() > 0) {
            value_type y = t.back();
            assert(pair_comp(y, x));
        }
        t.push_back(x);
    }

    virtual int read_in_binary_format(std::istream& /*istr*/);
    virtual void write_in_binary_format(std::ostream& /*ostr*/) const;

    virtual int read_in_text_format(std::istream& /*istr*/);
    virtual void write_in_text_format(std::ostream& /*ostr*/) const;

    friend std::ostream& operator<<(std::ostream& ostr, const self& v) {
        v.write_in_text_format(ostr);
        return ostr;
    }

    friend std::istream& operator>>(std::istream& istr, self& v) {
        v.read_in_text_format(istr);
        return istr;
    }
};

template<class Key, class T, class Compare>
typename linear_map<Key, T, Compare>::value_compare linear_map<Key, T, Compare>::pair_comp; //(Compare());

template<class Key, class T, class Compare>
inline bool operator==(const linear_map<Key, T, Compare>& x,
  const linear_map<Key, T, Compare>& y) {
    return x.t == y.t;
}

template<class Key, class T, class Compare>
inline bool operator<(const linear_map<Key, T, Compare>& x,
  const linear_map<Key, T, Compare>& y) {
    return x.t < y.t;
}

#ifdef __STL_FUNCTION_TMPL_PARTIAL_ORDER

template<class Key, class T, class Compare>
inline void swap(linear_map<Key, T, Compare>& x,
  linear_map<Key, T, Compare>& y) {
    x.swap(y);
}

#endif /* __STL_FUNCTION_TMPL_PARTIAL_ORDER */

// This is broken in g++ 3.0
template<class Key, class T, class Compare>
inline int linear_map<Key, T, Compare>::read_in_binary_format(std::istream& istr) {
#if !defined(_MSC_VER) && (__GNUG__ < 3)
    int size;
    if (!(istr.read(static_cast<int*>(&size), sizeof(int))))
        return 0;
    ON_DEBUG(assert(size >= 0));
    t.clear();
    t.resize(size);
    //   static char tmp[100000];
    //   if (! (istr.read(static_cast<char*>(tmp), size*sizeof(value_type))))
    // 	return 0;
    // //   for(int i=0;i<size;i++)
    // // 	t.push_back(tmp);
    //   copy((T*)tmp, (T*)tmp+size, t.begin());
    if (!istr.read(static_cast<void*>(t.begin()), size * sizeof(value_type)))
        return 0;
#endif
    return 1;
}

template<class Key, class T, class Compare>
inline void linear_map<Key, T, Compare>::write_in_binary_format(std::ostream& ostr) const {
#if !defined(_MSC_VER) && (__GNUG__ < 3)
    int size = t.size();
    ostr.write(static_cast<void*>(&size), sizeof(int));
    ostr.write(static_cast<const void*>(t.begin()), size * sizeof(value_type));
#endif /* __GNUG_ */
}

template<class U1, class U2>
std::ostream& operator<<(std::ostream& ostr, const std::pair<U1, U2>& p) {
    return ostr << p.first << " " << p.second;
}

template<class U1, class U2>
std::istream& operator>>(std::istream& istr, std::pair<U1, U2>& p) {
    return istr >> p.first >> p.second;
}

template<class Key, class T, class Compare>
inline int linear_map<Key, T, Compare>::read_in_text_format(std::istream& istr) {
    int size;
    if (!(istr >> size)) {
        return 0;
    }
    resize(size);

    for (auto i = begin(); i != end(); ++i) {
        if (!(istr >> *i)) {
            return 0;
        }
    };
    return 1;
}

template<class Key, class T, class Compare>
inline void linear_map<Key, T, Compare>::write_in_text_format(std::ostream& ostr) const {
    ostr << size() << " ";
    for (auto i = begin(); i != end(); ++i) {
        ostr << *i << " ";
    }
}

#endif /* ! __linear_map */

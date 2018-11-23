// -*- C++ -*-
/*
  Defines a container of elements pretty much like a vector, but
  the elements can be accesses as a map as well (bidirectional map).
  There is a requirement on the types: the indexed type cannot be an
  integral type.

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

#ifndef __indexed_map_h__
#define __indexed_map_h__

#include <memory>
#include <vector>
#include "hash_wrapper.h"
#include <map>
#include <lmdb++.h>

struct lmdb_reader {
    lmdb::txn txn;
    lmdb::dbi dbi;

    lmdb_reader(lmdb::env& env)
        : txn(lmdb::txn::begin(env, nullptr, MDB_RDONLY))
        , dbi(lmdb::dbi::open(txn))
    {}
};

struct lmdb_writer {
    lmdb::txn txn;
    lmdb::dbi dbi;

    lmdb_writer(lmdb::env& env)
        : txn(lmdb::txn::begin(env))
        , dbi(lmdb::dbi::open(txn))
    {}
};

template<typename K>
inline uint32_t get(lmdb_reader& rd, const K& key) {
    throw new std::runtime_error("NOT IMPLEMENTED");
}

inline uint32_t get(lmdb_reader& rd, std::string_view key) {
    if (key.empty()) {
        return std::numeric_limits<uint32_t>::max();
    }
    lmdb::val k{ key.data(), key.size() };
    lmdb::val v{};
    auto rv = rd.dbi.get(rd.txn, k, v);
    if (!rv) {
        return std::numeric_limits<uint32_t>::max();
    }
    return *reinterpret_cast<uint32_t*>(v.data());
}

inline uint32_t get(lmdb_reader& rd, const char* key) {
    return ::get(rd, std::string_view{key});
}

inline bool put(lmdb_writer& wr, std::string_view k, uint32_t v) {
    lmdb::val key(k.data(), k.size());
    lmdb::val value(&v, sizeof(v));
    return wr.dbi.put(wr.txn, key, value, MDB_NODUPDATA | MDB_NOOVERWRITE);
}

template<class index2, class index1 = unsigned, class map_type = HASH_NAMESPACE::hash_map<index2, index1>>
class indexed_map {
public:
    using self = indexed_map<index2, index1, map_type>;
    using rep_type = std::vector<index2*>;

    //   using iterator = rep_type::iterator;
    //   using const_iterator = rep_type::const_iterator;
    using size_type = int;

    template<class Value, class Ref, class Ptr, class underlying_iterator>
    class generic_iterator {
    public:
        using value_type = Value;
        using difference_type = int;
        using reference = Ref;
        using pointer = Value*;
        using self = generic_iterator<Value, Ref, Ptr, underlying_iterator>;

        generic_iterator(underlying_iterator i)
          : it(i) {}

        generic_iterator(const self& i)
          : it(i.it) {}

        ~generic_iterator() = default;

        Ref operator*() {
            return *it;
        }

        Ptr operator->() {
            return *it;
        }

        self& operator++() {
            ++it;
            return *this;
        }

        self operator++(int) {
            self t = *this;
            ++it;
            return t;
        }

        self& operator--() {
            --it;
            return *it;
        }

        self operator--(int) {
            self temp = *this;
            --it;
            return temp;
        }

        bool operator==(const self& other) const {
            return it == other.it;
        }

        bool operator!=(const self& other) const {
            return it != other.it;
        }

        friend difference_type operator-(const self& it1, const self& it2) {
            return it1.it - it2.it;
        }

    private:
        underlying_iterator it;
    };

    using iterator = generic_iterator<index2, index2&, index2, typename std::vector<index2>::iterator>;
    using const_iterator = generic_iterator<const index2, const index2&, const index2, typename std::vector<index2>::const_iterator>;

    indexed_map()
      : fake_index(-1) {}

    // This should not be called at all...
private:
    indexed_map(const self& ra)
      : index_map(ra.index_map)
      , storage(ra.storage)
      , available_inds(ra.available_inds) {}

public:
    index1 insert(const index2& elem) {
        if (db.handle()) {
            auto v = ::get(*rd, elem);
            if (v != std::numeric_limits<uint32_t>::max()) {
                return v;
            }
        }

        auto f = index_map.find(elem);
        if (f == index_map.end()) {
            if (!available_inds.empty()) {
                f = index_map.insert(std::make_pair(elem, available_inds.back())).first;
                storage[available_inds.back()] = f->first;
                available_inds.pop_back();
            }
            else {
                f = index_map.insert(std::make_pair(elem, static_cast<index1>(storage.size()))).first;
                storage.push_back(f->first);
            }
        }
        return f->second;
    }

    size_type size() const {
        return storage.size() - available_inds.size();
    }

    index1 reverse_access(const index2& elem) const {
        if (db.handle()) {
            auto v = ::get(*rd, elem);
            if (v != std::numeric_limits<uint32_t>::max()) {
                return v;
            }
        }

        auto f = index_map.find(elem);
        if (f == index_map.end()) {
            return -1;
        }
        return f->second;
    }

    index1 operator[](const index2& elem) const {
        return reverse_access(elem);
    }

    const index2& direct_access(index1 ind) const {
        if (ind == fake_index || ind >= storage.size()) {
            return fake_elem;
        }

        return storage[ind];
    }

    const index2& operator[](index1 ind) const {
        return direct_access(ind);
    }

    const index2& direct_access(index1 ind) {
        if (ind == fake_index || ind >= storage.size())
            return fake_elem;

        return storage[ind];
    }

    const index2& operator[](index1 ind) {
        return direct_access(ind);
    }

    void remove_element(const index2& elem) {
        auto f = index_map.find(elem);
        if (f != index_map.end()) {
            available_inds.push_back(f->second);
            index_map.erase(f);
            storage[f->second] = index2{};
        }
    }

    iterator begin() {
        return storage.begin();
    }

    iterator end() {
        return storage.end();
    }

    const_iterator begin() const {
        return storage.begin();
    }

    const_iterator end() const {
        return storage.end();
    }

    iterator find(const index2& elem) {
        auto it = index_map.find(elem);
        if (it == index_map.end()) {
            return end();
        }
        return storage.begin() + it->second;
    }

    const_iterator find(const index2& elem) const {
        auto it = index_map.find(elem);
        if (it == index_map.end()) {
            return end();
        }
        return storage.begin() + it->second;
    }

    //   template <class iterator_type>
    //   void erase(iterator_type i) {
    // 	remove_element(*i);
    //   }

    void erase(index1 ind) {
        remove_element(storage[ind]);
    }

    void clear() {
        storage.clear();
        index_map.clear();
    }

    void destroy() {
        storage.clear();
        available_inds.clear();
        index_map.clear();
    }

    auto& mdb() {
        return db;
    }

    void mdb_done() {
        db.sync();

        rd = std::make_unique<lmdb_reader>(db);
        db_size = ::get(*rd, "__NUM_ENTRIES");
        storage.resize(db_size);

        auto cursor = lmdb::cursor::open(rd->txn, rd->dbi);
        lmdb::val key, value;
        while (cursor.get(key, value, MDB_NEXT)) {
            std::string_view k{key.data(), key.size()};
            if (k[0] == '_' && k[1] == '_') {
                continue;
            }
            uint32_t v = *reinterpret_cast<uint32_t*>(value.data());
            storage[v] = k;
        }
        cursor.close();
    }

    void mdb_commit() {
        if (!index_map.size()) {
            return;
        }

        rd.reset();

        lmdb_writer wr{ db };
        lmdb::val key;
        lmdb::val value;

        for (auto& kv : index_map) {
            uint32_t v = kv.second;
            key.assign(kv.first.data(), kv.first.size());
            value.assign(&v, sizeof(v));
            if (wr.dbi.put(wr.txn, key, value, MDB_NODUPDATA | MDB_NOOVERWRITE)) {
                ++db_size;
            }
        }

        key.assign("__NUM_ENTRIES");
        value.assign(&db_size, sizeof(db_size));
        wr.dbi.put(wr.txn, key, value);

        wr.txn.commit();

        index_map.clear();
        mdb_done();
    }

    auto& mdb_rd() {
        return *rd;
    }

    index2 fake_elem;
    index1 fake_index;

protected:
    lmdb::env db{ nullptr };
    std::unique_ptr<lmdb_reader> rd;
    uint32_t db_size{ 0 };

    map_type index_map;
    std::vector<index2> storage;
    std::vector<index1> available_inds;
};

#endif

// -*- C++ -*-
/*
  Defines an extension of bit_vector that can compute easily the
  number of bits that are set underbelow a given index.

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

#ifndef __my_bit_vector_h
#define __my_bit_vector_h

#include <vector>
#include <cassert>
#include <iostream>
#include "debug.h"

template<class int_type = unsigned short>
class my_bit_vector : public std::vector<bool> {
    using _Base = std::vector<bool>;
    //   mutable int_type last_pos, last_val;

public:
    explicit my_bit_vector()
      : _Base() {
        reset();
    }

    explicit my_bit_vector(size_type __n, bool __value)
      : _Base(__n, __value) {
        reset();
    }

    explicit my_bit_vector(size_type __n)
      : _Base(__n) {
        reset();
    }

    explicit my_bit_vector(const my_bit_vector& __x)
      : _Base(__x) {
        reset();
    }

    void reset() {
        // 	last_pos = static_cast<int_type>(-1);
        // 	last_val = 0;
    }

    int count_bits_before_pos(unsigned int pos) const {
        int cnt = 0;
        for (size_t i = 0; i < pos; ++i) {
            if (operator[](i)) {
                ++cnt;
            }
        }
        ON_DEBUG(assert(last_val <= last_pos));
        return cnt;
    }

    bool read_in_binary_format(std::istream& istr, int size = -1);
    void write_in_binary_format(std::ostream& ostr, int size = -1) const;

    bool read_in_text_format(std::istream& istr, int size = -1);
    void write_in_text_format(std::ostream& ostr, int size = -1) const;

    static std::vector<char> bits;
    static char mask[8];
};

template<class int_type>
std::vector<char> my_bit_vector<int_type>::bits(0);

template<class int_type>
char my_bit_vector<int_type>::mask[8] = { 0x1, 0x2, 0x4, 0x8, 0x10, 0x20, 0x40, static_cast<char>(0x80) };

template<class int_type>
bool my_bit_vector<int_type>::read_in_binary_format(std::istream& istr, int size) {
    if (size == -1) { //if the size was not specified, is to be read from the stream
        if (!istr.read(reinterpret_cast<char*>(&size), sizeof(int))) {
            return false;
        }
    }

    resize(size);

    unsigned char* my_bit_array = nullptr;
    throw std::runtime_error("read_in_binary_format not implemented");

    unsigned int no_ints = (size >> 5) + ((size & 0x1f) > 0);
    if (!istr.read(reinterpret_cast<char*>(my_bit_array), no_ints * sizeof(unsigned int))) {
        return false;
    }
    reset();
    return true;
}

template<class int_type>
void my_bit_vector<int_type>::write_in_binary_format(std::ostream& ostr, int size) const {
    if (size != -1) { // if size if specified, write it to the output stream
        ostr.write(reinterpret_cast<char*>(&size), sizeof(int));
    }
    else {
        size = this->size();
    }

    unsigned int no_ints = (size >> 5) + ((size & 0x1f) > 0);

    unsigned char* my_bit_array = nullptr;
    throw std::runtime_error("write_in_binary_format not implemented");

    ostr.write(reinterpret_cast<char*>(my_bit_array), no_ints * sizeof(unsigned int));
}

template<class int_type>
bool my_bit_vector<int_type>::read_in_text_format(std::istream& istr, int size) {
    if (size == -1) {
        if (!(istr >> size)) {
            return false;
        }
    }

    unsigned int no_bits = (size >> 3) + ((size & 0x07) > 0);
    bits.resize(no_bits);

    resize(size);
    char bt;
    for (auto bit = begin(); bit != end(); ++bit) {
        if (!(istr >> bt)) {
            return false;
        }
        *bit = (bt == '0' ? false : true);
    }
    reset();
    return true;
}

template<class int_type>
void my_bit_vector<int_type>::write_in_text_format(std::ostream& ostr, int size) const {
    if (size != -1) {
        ostr << size << " ";
    }

    for (auto bit = begin(); bit != end(); ++bit) {
        ostr << ((*bit) ? '1' : '0') << std::flush;
    }
}

#endif /* __my_bit_vector */

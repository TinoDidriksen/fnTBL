// -*- C++ -*-
/*
  A set of useful definitions and functions.

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

#ifndef _common_h_
#define _common_h_

#include <string>

#include <hash_wrapper.h>
#include <cmath>

#include <fstream>
#include <iostream>
#include <vector>
#include <stdexcept>
#include <string_view>

#define BOOST_DATE_TIME_NO_LIB 1
#include <boost/interprocess/file_mapping.hpp>
#include <boost/interprocess/mapped_region.hpp>
namespace bi = ::boost::interprocess;
struct mmap_region {
	bi::file_mapping fmap;
	bi::mapped_region mreg;
	const char* buf;
	size_t size;

	mmap_region() {}

	mmap_region(const std::string& fname)
		: fmap(fname.c_str(), bi::read_only)
		, mreg(fmap, bi::read_only)
		, buf(reinterpret_cast<const char*>(mreg.get_address()))
		, size(mreg.get_size()) {}
};

template<typename C>
inline auto is_space(const C& c) {
	return ((c == ' ') || (c == '\t') || (c == '\r') || (c == '\n'));
}

template<typename Str>
inline void trim(Str& str) {
	while (!str.empty() && is_space(str.back())) {
		str.pop_back();
	}
	while (!str.empty() && is_space(str.front())) {
		str.erase(str.begin());
	}
}

inline auto nextline(const char* start, const char* end, std::string_view line = {}) {
    // If we were already reading lines, start from the previous line's end
    if (!line.empty()) {
        start = &line.back() + 1;
    }

    // Skip leading space, which also skips the newline that the old back() might be parked behind
    // Also skips empty lines, which is desired behavior
    while (start < end && is_space(*start)) {
        ++start;
    }
    if (start >= end) {
        return std::string_view{};
    }

    // Find the next newline...
    const char* stop = start;
    while (stop < end && *stop != '\n') {
        ++stop;
    }
    // ...but trim trailing whitespace
    while (stop > start && is_space(stop[-1])) {
        --stop;
    }

    return std::string_view{ start, static_cast<size_t>(stop - start) };
}

inline auto nextline(mmap_region& reg, std::string_view line = {}) {
    return nextline(reg.buf, reg.buf + reg.size, line);
};

inline auto nextline(const std::string& buf, std::string_view line = {}) {
    return nextline(&buf[0], &buf[buf.size()], line);
};

template<size_t N>
inline auto split(std::string_view str) {
	std::array<std::string_view, N> strs;

	while (!str.empty() && is_space(str.front())) {
		str.remove_prefix(1);
	}

	auto start = &str.front();
	auto end = &str.back() + 1;

	// Split the first N-1 nicely...
	for (size_t n = 0; n < N - 1 && start < end; ++n) {
		auto stop = start;
		while (stop < end && !is_space(*stop)) {
			++stop;
		}

		strs[n] = std::string_view{ start, static_cast<size_t>(stop - start) };

		start = stop;
		while (start < end && is_space(*start)) {
			++start;
		}
	}
	// ...but the last part should just contain whatever is left in the input
	strs[N - 1] = std::string_view{ start, static_cast<size_t>(end - start) };

	return strs;
}

inline std::string itoa(int no) {
    return std::to_string(no);
}

const std::string& ftoa(double f, const std::string& format = "%f");

template<typename Str>
inline int atoi1(const Str& number) {
	return std::stoi(number);
}

inline int atoi1(std::string_view number) {
	char d[32];
	std::copy(number.begin(), number.end(), d);
	return atoi(d);
}

template<typename Str>
inline double atof1(const Str& number) {
	return std::stod(number);
}

inline void smart_open(std::istream*& f, std::string str) {
    if (str == "-") {
        f = &std::cin;
    }
    else if (str.size() > 3 && str.rfind(".gz") == str.size() - 3) {
        f = new std::ifstream(("|zcat " + str).c_str(), std::ios::binary);
    }
    else if (str.size() > 4 && str.rfind(".bz2") == str.size() - 4) {
        f = new std::ifstream(("|bzcat " + str).c_str(), std::ios::binary);
    }
    else {
        f = new std::ifstream(str.c_str(), std::ios::binary);
    }

    if (!*f) {
        std::string msg{ "Could not open the file " + str + " for reading ! Exiting...\n" };
        throw std::runtime_error(msg);
    }
}

inline void smart_open(std::ostream*& g, std::string str) {
    if (str == "-") {
        g = &std::cout;
    }
    else if (str.size() > 3 && str.rfind(".gz") == str.size() - 3) {
        g = new std::ofstream(("|gzip -c > " + str).c_str(), std::ios::binary);
    }
    else if (str.size() > 4 && str.rfind(".bz2") == str.size() - 4) {
        g = new std::ofstream(("|bzip2 -c > " + str).c_str(), std::ios::binary);
    }
    else {
        g = new std::ofstream(str.c_str(), std::ios::binary);
    }

    if (!*g) {
        std::cerr << "Could not open the file " << str << " for writing ! Exiting..." << std::endl;
        exit(112);
    }
}

using constit_type = unsigned short;

template<class Value>
inline Value sqr(const Value& val) {
    return val * val;
}

#if DEBUG >= 3
#    define ON_DEBUG(x) x
#else
#    define ON_DEBUG(x)
#endif

inline void tick(int current_line, bool force = false) {
    static unsigned prev_line = 1;
    if (current_line == 1) {
        std::cerr << "Processed lines: 0";
        prev_line = 1;
    }

    if ((current_line >> 6) > ((current_line - 1) >> 6) || force) {
        for (int p = 0; p <= log(static_cast<double>(prev_line + 1)) / log(10.0); p++) {
            std::cerr << "\b";
        }
        std::cerr << current_line;
        prev_line = current_line;
    }
}

class ticker {
    std::string initial_string;
    int interval{ 0 };
    int line{ 1 };
    int prev_line{ 0 };
    bool displayed{ false };
    std::ostream& stream;

public:
    ticker(const std::string s = "Processed lines: ", int i = 64, std::ostream& str = std::cerr)
      : initial_string(s)
      , stream(str) {
        interval = static_cast<int>(log(static_cast<double>(i)) / log(2.0));
    }

    void tick(int val = -1, bool force = false) {
        if (val != -1) {
            line = val;
        }

        if (!displayed) {
            stream << initial_string << " " << line;
            prev_line = line;
            displayed = true;
        }

        if ((line >> interval) > ((line - 1) >> interval) || force) {
            for (int p = 0; p <= log(static_cast<double>(prev_line + 1)) / log(10.0); p++) {
                stream << "\b";
            }
            stream << line;
            stream.flush();
            prev_line = line;
        }

        line++;
    }

    void clear() {
        std::string s = initial_string + itoa(prev_line);
        int l = s.length();
        for (int k = 0; k <= l; k++) {
            stream << "\b";
        }
        for (int k = 0; k <= l; k++) {
            stream << " ";
        }
        for (int k = 0; k <= l; k++) {
            stream << "\b";
        }

        line = 1;
        displayed = false;
    }
};

void log_me_in(int argc, char* argv[]);

template<class type, class alloc>
std::ostream& operator<<(std::ostream& ostr, const std::vector<type, alloc>& sv) {
    ostr << sv.size();
    for (int i = 0; i < sv.size(); i++)
        ostr << " " << sv[i];
    return ostr;
}

template<class type, class alloc>
std::istream& operator>>(std::istream& istr, std::vector<type, alloc>& sv) {
    int sz;
    istr >> sz;
    sv.resize(sz);
    for (int i = 0; i < sz; i++)
        istr >> sv[i];
    return istr;
}

template<class array_type = std::vector<int>>
struct ArrayIndexSorter {
    const array_type& counts;
    ArrayIndexSorter(const array_type& c)
      : counts(c) {}

    bool operator()(int i1, int i2) const {
        return counts[i1] < counts[i2];
    }
};

#endif

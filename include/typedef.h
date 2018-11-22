// -*- C++ -*-
/*
  Definitions for all the programs.
  This file is part of the fnTBL distribution.

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

#ifndef __TYPEDEF_H
#define __TYPEDEF_H

#include <string>
#include <set>
#include <vector>
#include <string_view>

using intType = int;
using scoreType = int; // the type of score for good/bad

// using wordType = unsigned int;

// The type for the data
using wordType = WORD_TYPE;

// The type for feature indices
using featureIndexType = POSITION_TYPE;

using wordType1D = wordType*;
using relativePosType = char;

using wordTypeVector = std::vector<wordType>;
using wordType2D = std::vector<wordType1D>;
using wordType2DVector = std::vector<wordTypeVector>;
using wordType3D = std::vector<wordType2D>;
using wordType3DVector = std::vector<wordType2DVector>;


// using featureIndexType = unsigned short;
using featureIndexType1D = std::vector<featureIndexType>;
using featureIndexType2D = std::vector<featureIndexType1D>;

using string1D = std::vector<std::string>;
using string1D_v = std::vector<std::string_view>;
using string_set = std::set<std::string>;
using wordType_set = std::set<wordType>;

using int1D = std::vector<int>;
using int2D = std::vector<int1D>;
using int3D = std::vector<int2D>;

using float1D = std::vector<float>;
using float2D = std::vector<float1D>;

using shortint1D = std::vector<unsigned short>;

using shortintPtr = short*;

static const std::string UNK_string = "UNKNOWN";

static const double EPSILON = 1e-5; // anything within this is the same

static const int THRESHOLDNUMRULES = 500;

static const int DEFAULTCOST = 1;

static const int WORD = 0;
static const int POS = 1;

#endif

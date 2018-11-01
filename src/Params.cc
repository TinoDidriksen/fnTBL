/*
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

#include "Params.h"
#include "line_splitter.h"
#include <iterator>
#include <string_view>

std::string* Params::nullStr = new std::string("");
std::string Params::filename = filename;

Params::Params(const char* filename) {
    if (filename == nullptr || std::string(filename).empty()) {
        std::cerr << "The parameter file is undefined !! Try setting the DDINF environment variable !" << std::endl;
        exit(1);
    }

    std::ifstream f(filename);

    std::string_view path{filename};
    while (!path.empty() && path.back() != '/' && path.back() != '\\') {
        path.remove_suffix(1);
    }
    commands["_REL_PATH"] = path;

    std::string line;
    line_splitter lineSplitter("# \r\t=;", false);
    int noWords = 0, lineNo = 0;

    while (std::getline(f, line)) {
        lineSplitter.split(line);
        if (lineSplitter.size() == 0 || lineSplitter[0] == "#" || (noWords = lineSplitter.size()) == 0) {
            continue;
        }
        if (noWords != 4) {
            std::cerr << "Error at line " << lineNo << " in file " << filename << "!!" << std::endl;
            std::cerr << "There are " << noWords << " words on line " << lineNo << " instead of 4:" << std::endl;
            for (int ii = 0; ii < lineSplitter.size() - 1; ++ii) {
                std::cerr << lineSplitter[ii] << ",";
            }
            std::cerr << lineSplitter[lineSplitter.size() - 1] << std::endl;
            exit(20);
        }
        std::string s = lineSplitter[2];
        unsigned pos;
        while ((pos = s.find("${", 0)) != static_cast<unsigned>(-1)) { // We have a variable ${NAME}
            unsigned pos1 = s.find("}", pos + 2);
            if (pos1 == static_cast<unsigned>(-1)) {
                std::cerr << "Error!! Variable not finished in the parameter file!" << std::endl;
                exit(1);
            }
            std::string sub;
            copy(s.begin() + pos + 2, s.begin() + pos1, std::inserter(sub, sub.begin()));
            if (commands.find(sub) == commands.end()) {
                std::cerr << "Error!! Variable " << sub << "not defined! Exiting..." << std::endl;
                exit(1);
            }
            std::string replacement = commands[sub];
            s.replace(pos, pos1 - pos + 1, commands[sub]);
        }
        commands[lineSplitter[0]] = s;
        lineNo++;
    }
    f.close();
}

std::ostream& operator<<(std::ostream& ostr, const Params& p) {
    for (const auto& command : p.commands) {
        ostr << command.first << " = " << command.second << " ; " << std::endl;
    }
    return ostr;
}

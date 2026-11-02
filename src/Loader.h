// Loader.h
// ---------------------------------------------------------------------------
// WHAT:  reads the cleaned data file (made by prepare_data.py) into Country objects.
// ---------------------------------------------------------------------------
#pragma once
#include <fstream>
#include <string>
#include <vector>
#include "Country.h"

// Cut one line at every ';'.  (Written by hand because std::getline drops an
// empty LAST cell, and an empty last cell is exactly what "no debt data" looks like.)
inline std::vector<std::string> splitLine(const std::string& line) {
    std::vector<std::string> parts;
    size_t start = 0;
    while (true) {
        size_t semi = line.find(';', start);
        if (semi == std::string::npos) { parts.push_back(line.substr(start)); break; }
        parts.push_back(line.substr(start, semi - start));
        start = semi + 1;
    }
    return parts;
}

// Text -> number.  An empty cell (or unreadable text) becomes MISSING.
inline double toNumber(const std::string& s) {
    if (s.empty()) return MISSING;
    try { return std::stod(s); } catch (...) { return MISSING; }
}

// Read the whole file into a vector of Country.  Returns false if it cannot be opened.
inline bool loadCountries(const std::string& path, std::vector<Country>& out) {
    std::ifstream file(path);
    if (!file) return false;
    std::string line;
    std::getline(file, line);                       // first line = column names, skip it
    while (std::getline(file, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();   // Windows line ends
        std::vector<std::string> f = splitLine(line);
        if (f.size() < 7) continue;                 // damaged line: skip it
        Country c;
        c.name         = f[0];
        c.iso          = f[1];
        c.gdp          = toNumber(f[2]);
        c.growth       = toNumber(f[3]);
        c.inflation    = toNumber(f[4]);
        c.unemployment = toNumber(f[5]);
        c.debt         = toNumber(f[6]);
        out.push_back(c);
    }
    return true;
}


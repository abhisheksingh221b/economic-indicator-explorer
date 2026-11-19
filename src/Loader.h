// Loader.h
// ---------------------------------------------------------------------------
// WHAT   Reads the cleaned data file (made by prepare_data.py) and creates one
//        Country object for every line.
//
// The file looks like this (columns are separated by ';'):
//   name;iso;gdp;growth;inflation;unemployment;debt
//   Afghanistan;AFG;413.76;2.27;-4.64;14.01;
//   An EMPTY cell (like the debt of Afghanistan) means "no data".
// ---------------------------------------------------------------------------
#pragma once
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include "Country.h"


// Cut one line into its cells at every ';'.
//   "Afghanistan;AFG;413.76"  ->  "Afghanistan", "AFG", "413.76"
// We add one extra ';' at the end first.  Then every cell, even an empty LAST
// cell, is followed by a ';' and getline treats all of them the same way.
inline std::vector<std::string> splitLine(const std::string& line) {
    std::vector<std::string> cells;
    std::stringstream stream(line + ";");
    std::string cell;
    while (std::getline(stream, cell, ';')) {
        cells.push_back(cell);
    }
    return cells;
}


// Turn text into a number.  An empty cell (or text that is not a number)
// becomes MISSING.
inline double toNumber(const std::string& text) {
    if (text.empty()) {
        return MISSING;
    }
    try {
        return std::stod(text);          // stod = "string to double"
    } catch (...) {
        return MISSING;                  // the text was not a number
    }
}


// Read the whole file into the vector "countries".
// Returns false if the file cannot be opened.
inline bool loadCountries(const std::string& path, std::vector<Country>& countries) {
    std::ifstream file(path);
    if (!file) {
        return false;
    }

    std::string line;
    std::getline(file, line);                        // the first line holds the column names: skip it

    while (std::getline(file, line)) {
        // Files made on Windows end every line with an invisible '\r'.  Remove it.
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }

        std::vector<std::string> cell = splitLine(line);
        if (cell.size() < 7) {
            continue;                                // a damaged line: skip it
        }

        Country c;
        c.name         = cell[0];
        c.iso          = cell[1];
        c.gdp          = toNumber(cell[2]);
        c.growth       = toNumber(cell[3]);
        c.inflation    = toNumber(cell[4]);
        c.unemployment = toNumber(cell[5]);
        c.debt         = toNumber(cell[6]);
        countries.push_back(c);
    }
    return true;
}

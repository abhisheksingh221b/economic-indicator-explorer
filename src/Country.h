// Country.h
// ---------------------------------------------------------------------------
// WHAT IS HERE
//   1. MISSING     - the number we use when the World Bank has no value.
//   2. Indicator   - names for the five numbers we store about each country.
//   3. Country     - one country and its five numbers.
//   4. Status      - GREEN / YELLOW / RED / NO_DATA, and the rules that decide it.
// Every other file includes this one.
// ---------------------------------------------------------------------------
#pragma once
#include <string>


// ---------------------------------------------------------------------------
// 1. MISSING VALUES
// Some cells in the World Bank file are empty (no data).  We store the special
// number -9999 for "no data".  We do NOT use -1, because -1 can be a real value
// (a country whose prices fell by 1 % has inflation = -1).  -9999 never happens.
// ---------------------------------------------------------------------------
const double MISSING = -9999;


// ---------------------------------------------------------------------------
// 2. THE FIVE INDICATORS
// An "enum" is a list of names that C++ turns into the numbers 0, 1, 2 ...
// So GDP is 0, GROWTH is 1, INFLATION is 2, UNEMPLOYMENT is 3, DEBT is 4,
// and NUM_INDICATORS is 5 (how many indicators there are).
// We write INFLATION instead of 2 because it is easier to read.
// ---------------------------------------------------------------------------
enum Indicator { GDP, GROWTH, INFLATION, UNEMPLOYMENT, DEBT, NUM_INDICATORS };

// The text we print for each indicator.  Position 0 belongs to GDP, position 1
// to GROWTH, and so on, in the same order as the enum above.
const char* const INDICATOR_NAME[NUM_INDICATORS] = {
    "GDP per capita (US$)", "GDP growth (%)", "Inflation (%)",
    "Unemployment (%)", "Govt debt (% of GDP)"
};

// Short names for the column headings of a table.
const char* const INDICATOR_SHORT[NUM_INDICATORS] = {
    "GDP/cap", "Growth", "Inflat.", "Unempl.", "Debt"
};


// ---------------------------------------------------------------------------
// 3. ONE COUNTRY
// A "struct" is a bundle of named values.  Every country is created ONCE, when
// the file is read.  The hash table and the trees do not copy it; they only
// remember WHERE it is (a pointer, written Country*).  So there is one copy of
// the data and no confusion.
// ---------------------------------------------------------------------------
struct Country {
    std::string name;
    std::string iso;                 // 3-letter code, e.g. IND
    double gdp          = MISSING;   // every number starts as "no data"
    double growth       = MISSING;
    double inflation    = MISSING;
    double unemployment = MISSING;
    double debt         = MISSING;

    // get(ind) returns the number for indicator "ind".
    // Example: c.get(INFLATION) gives the same as c.inflation.
    // This lets other code loop over all indicators with a simple for-loop.
    double get(int ind) const {
        if (ind == GDP)          return gdp;
        if (ind == GROWTH)       return growth;
        if (ind == INFLATION)    return inflation;
        if (ind == UNEMPLOYMENT) return unemployment;
        return debt;
    }

    // has(ind) is true when this country has a real value for the indicator.
    bool has(int ind) const {
        return get(ind) != MISSING;
    }
};


// ---------------------------------------------------------------------------
// 4. COLOURS: GREEN, YELLOW, RED and a 4th colour for "no data"
// A missing value is not good and not bad, it is simply unknown, so it must not
// be shown as green.  It gets its own status (shown in grey).
// ---------------------------------------------------------------------------
enum Status { GREEN, YELLOW, RED, NO_DATA };

// A Threshold holds the "lines" for ONE indicator: where yellow starts and
// where red starts.
//   higherIsWorse = true   inflation, unemployment, debt (a big number is bad)
//   higherIsWorse = false  growth (a small number is bad)
struct Threshold {
    bool   used;            // false for GDP per capita (rich is not "good", poor is not "bad")
    bool   higherIsWorse;
    double yellow;          // the value where YELLOW starts
    double red;             // the value where RED starts
};

// The starting lines.  These are common rules of thumb, NOT official World Bank
// limits.  The user can change them while the program runs.
inline Threshold defaultThreshold(int ind) {
    Threshold t;
    t.used = true;
    t.higherIsWorse = true;

    if (ind == GROWTH) {            // growth below 2 % is yellow, below 0 % is red
        t.higherIsWorse = false;
        t.yellow = 2;
        t.red = 0;
    } else if (ind == INFLATION) {  // 6 % or more is yellow, 10 % or more is red
        t.yellow = 6;
        t.red = 10;
    } else if (ind == UNEMPLOYMENT) {
        t.yellow = 6;
        t.red = 8;
    } else if (ind == DEBT) {       // 60 % of GDP is yellow, 90 % is red
        t.yellow = 60;
        t.red = 90;
    } else {                        // GDP per capita: no colour at all
        t.used = false;
        t.yellow = 0;
        t.red = 0;
    }
    return t;
}

// Decide the colour of ONE value, using the lines in t.
inline Status statusOf(double value, const Threshold& t) {
    if (value == MISSING) return NO_DATA;

    if (t.higherIsWorse) {
        if (value >= t.red)    return RED;
        if (value >= t.yellow) return YELLOW;
        return GREEN;
    } else {
        if (value < t.red)     return RED;
        if (value < t.yellow)  return YELLOW;
        return GREEN;
    }
}

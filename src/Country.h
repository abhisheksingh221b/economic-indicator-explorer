// Country.h
// ---------------------------------------------------------------------------
// WHAT IS HERE:  the Country record, the five indicators, and the green /
// yellow / red rules.  Every other file includes this one.
// ---------------------------------------------------------------------------
#pragma once
#include <string>

// The World Bank leaves some cells empty (no data).  We store a special number
// for "no data".  We do NOT use -1, because -1 is a real value (e.g. a country
// whose prices fell by 1 % has inflation = -1).  -9999 can never be a real value.
const double MISSING = -9999.0;

// The five indicators.  Using names instead of the numbers 0..4 makes the code
// readable (INFLATION is clearer than 2).  NUM_INDICATORS is simply "5".
enum Indicator { GDP = 0, GROWTH = 1, INFLATION = 2, UNEMPLOYMENT = 3, DEBT = 4, NUM_INDICATORS = 5 };

// Text used when printing.  The index matches the enum above.
const char* const INDICATOR_NAME[NUM_INDICATORS] = {
    "GDP per capita (US$)", "GDP growth (%)", "Inflation (%)",
    "Unemployment (%)", "Govt debt (% of GDP)"};
const char* const INDICATOR_SHORT[NUM_INDICATORS] = {"GDP/cap", "Growth", "Inflat.", "Unempl.", "Debt"};

// ONE country = ONE Country object.  It is created once (when the file is
// read) and every data structure only stores a POINTER to it, so the data is
// never copied.
struct Country {
    std::string name;
    std::string iso;
    double gdp          = MISSING;
    double growth       = MISSING;
    double inflation    = MISSING;
    double unemployment = MISSING;
    double debt         = MISSING;

    // get(ind) lets the rest of the program ask "give me indicator number ind"
    // without a separate if-statement for each indicator.
    double get(int ind) const {
        switch (ind) {
            case GDP:          return gdp;
            case GROWTH:       return growth;
            case INFLATION:    return inflation;
            case UNEMPLOYMENT: return unemployment;
            default:           return debt;
        }
    }
    bool has(int ind) const { return get(ind) != MISSING; }
};

// ---------------------------------------------------------------------------
// Colours.  Green / Yellow / Red as you asked.  A 4th colour, GREY, is used
// for "no data", because a missing value is not good and not bad - it is
// simply unknown, and it must not be shown as green.
// ---------------------------------------------------------------------------
enum Status { GREEN, YELLOW, RED, NO_DATA };

// A threshold says where yellow and red start for one indicator.
//   higherIsWorse = true  : inflation, unemployment, debt  (big number = trouble)
//   higherIsWorse = false : growth                          (small number = trouble)
struct Threshold {
    bool   used;           // false for GDP per capita: rich is not "good", poor is not "bad"
    bool   higherIsWorse;
    double yellow;         // value where YELLOW starts
    double red;            // value where RED starts
};

// Default cut-offs.  These are common rules of thumb, NOT official World Bank
// limits.  The user can change them while the program runs.
inline Threshold defaultThreshold(int ind) {
    switch (ind) {
        case GROWTH:       return {true, false, 2.0, 0.0};   // under 2 % yellow, negative = red
        case INFLATION:    return {true, true,  6.0, 10.0};  // 6 % yellow, 10 % red
        case UNEMPLOYMENT: return {true, true,  6.0, 8.0};   // 6 % yellow, 8 % red
        case DEBT:         return {true, true, 60.0, 90.0};  // 60 % yellow, 90 % red
        default:           return {false, true, 0.0, 0.0};   // GDP per capita: no colour
    }
}

// Decide the colour of one value.
inline Status statusOf(double value, const Threshold& t) {
    if (value == MISSING) return NO_DATA;
    if (t.higherIsWorse) {
        if (value >= t.red)    return RED;
        if (value >= t.yellow) return YELLOW;
        return GREEN;
    }
    if (value < t.red)    return RED;      // growth below the red line (e.g. below 0)
    if (value < t.yellow) return YELLOW;
    return GREEN;
}

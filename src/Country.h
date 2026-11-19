
#pragma once
#include <string>


using namespace std;



const double MISSING = -9999;



enum Indicator { GDP, GROWTH, INFLATION, UNEMPLOYMENT, DEBT, NUM_INDICATORS };

const char* const INDICATOR_NAME[NUM_INDICATORS] = {
    "GDP per capita (US$)", "GDP growth (%)", "Inflation (%)",
    "Unemployment (%)", "Govt debt (% of GDP)"
};


const char* const INDICATOR_SHORT[NUM_INDICATORS] = {
    "GDP/cap", "Growth", "Inflat.", "Unempl.", "Debt"
};



struct Country {
    string name;
    string iso;                 // 3-letter code, e.g. IND
    double gdp          = MISSING;   // every number starts as "no data"
    double growth       = MISSING;
    double inflation    = MISSING;
    double unemployment = MISSING;
    double debt         = MISSING;

   
    double get(int ind) const {
        if (ind == GDP)          return gdp;
        if (ind == GROWTH)       return growth;
        if (ind == INFLATION)    return inflation;
        if (ind == UNEMPLOYMENT) return unemployment;
        return debt;
    }

    bool has(int ind) const {
        return get(ind) != MISSING;
    }
};



enum Status { GREEN, YELLOW, RED, NO_DATA };


struct Threshold {
    bool   used;            
    bool   higherIsWorse;
    double yellow;          
    double red;           
};


inline Threshold defaultThreshold(int ind) {
    Threshold t;
    t.used = true;
    t.higherIsWorse = true;

    if (ind == GROWTH) {           
        t.higherIsWorse = false;
        t.yellow = 2;
        t.red = 0;
    } else if (ind == INFLATION) {  
        t.yellow = 6;
        t.red = 10;
    } else if (ind == UNEMPLOYMENT) {
        t.yellow = 6;
        t.red = 8;
    } else if (ind == DEBT) {       
        t.yellow = 60;
        t.red = 90;
    } else {                       
        t.used = false;
        t.yellow = 0;
        t.red = 0;
    }
    return t;
}


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

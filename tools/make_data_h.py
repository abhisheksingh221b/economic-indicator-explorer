"""Turns tools/countries_2023.csv into src/Data.h (run only when you want new data).
Usage:  python3 tools/make_data_h.py tools/countries_2023.csv src/Data.h
"""
import csv, sys

src, dst = sys.argv[1], sys.argv[2]
rows = list(csv.reader(open(src, encoding="utf-8"), delimiter=";"))[1:]

def num(text):
    return "MISSING" if text == "" else text

out = []
out.append("""// Data.h
// ---------------------------------------------------------------------------
// WHAT   The World Bank data (year 2023) written directly into the program:
//        217 countries, 5 numbers each.  No data file is needed, so there is
//        no file-path problem on any computer.
//
// HOW IT WAS MADE   tools/prepare_data.py cleaned the 200 MB World Bank file,
//        then tools/make_data_h.py turned the result into this file.  You never
//        edit it by hand.
//
// MISSING means "the World Bank has no value".  Each line is:
//   name, 3-letter code, GDP per capita, growth, inflation, unemployment, debt
// ---------------------------------------------------------------------------
#pragma once
#include <vector>
#include "Country.h"

struct DataRow {
    const char* name;
    const char* iso;
    double gdp, growth, inflation, unemployment, debt;
};

const DataRow DATA[] = {""")
for r in rows:
    name = r[0].replace('"', '\\"')
    out.append('    {"%s", "%s", %s, %s, %s, %s, %s},' % (name, r[1], num(r[2]), num(r[3]), num(r[4]), num(r[5]), num(r[6])))
out.append("""};

const int DATA_COUNT = sizeof(DATA) / sizeof(DATA[0]);


// Turn the table above into a vector of Country objects.
inline vector<Country> builtInCountries() {
    vector<Country> countries;
    for (int i = 0; i < DATA_COUNT; i++) {
        Country c;
        c.name         = DATA[i].name;
        c.iso          = DATA[i].iso;
        c.gdp          = DATA[i].gdp;
        c.growth       = DATA[i].growth;
        c.inflation    = DATA[i].inflation;
        c.unemployment = DATA[i].unemployment;
        c.debt         = DATA[i].debt;
        countries.push_back(c);
    }
    return countries;
}
""")
open(dst, "w", encoding="utf-8").write("\n".join(out))

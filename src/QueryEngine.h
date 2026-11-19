// QueryEngine.h
// ---------------------------------------------------------------------------
// WHAT   The "brain" of the program.  It owns the hash table and the five BSTs
//        and answers every question.  It only RETURNS answers; it never prints
//        anything.
//
// WHY SEPARATE FROM THE MENU
//   Printing and typing live in Menu.h.  Keeping the thinking apart from the
//   screen makes each file shorter and lets the self-test check the answers
//   without any screen.
// ---------------------------------------------------------------------------
#pragma once
#include <algorithm>
#include <string>
#include <vector>
#include "BST.h"
#include "Country.h"
#include "HashTable.h"

// A number bigger than any real value.  Used for ranges with no upper limit
// ("everything from 10 upwards") and, as -BIG, for no lower limit.
const double BIG = 1e12;


// ---------------------------------------------------------------------------
// Small structs that carry answers back to the menu.
// ---------------------------------------------------------------------------

// Answer of the early warning for ONE indicator.
struct Warning {
    vector<Country*> red;       // the red countries, worst first
    vector<Country*> yellow;    // the yellow countries, worst first
    int green  = 0;             // green countries are only COUNTED (the list would be too long)
    int noData = 0;             // countries with no value for this indicator
};

class QueryEngine {
public:
    // The vector of countries must NOT change after this point, because the
    // hash table and the trees keep pointers into it.
    QueryEngine(vector<Country>& countries) : all(countries) {
        for (int ind = 0; ind < NUM_INDICATORS; ind++) {
            withData[ind] = 0;
        }

        for (int i = 0; i < (int)all.size(); i++) {
            Country* c = &all[i];                      // the address of country number i

            hash.insert(c);                            // every country goes into the hash table

            // A country goes into a tree only if it HAS a value for that indicator.
            for (int ind = 0; ind < NUM_INDICATORS; ind++) {
                if (c->has(ind)) {
                    trees[ind].insert(c->get(ind), c);
                    withData[ind]++;
                }
            }
        }
    }

    int totalCountries() const {
        return (int)all.size();
    }

    // How many countries have a real value for this indicator.
    int countWithData(int ind) const {
        return withData[ind];
    }

    // -----------------------------------------------------------------------
    // FEATURE 1: look up one country by name (hash table)
    // -----------------------------------------------------------------------
    Country* lookup(const string& name) const {
        return hash.search(name);
    }

    // -----------------------------------------------------------------------
    // FEATURE 2: range filter (BST)
    // All countries whose value for this indicator is from low to high.
    // -----------------------------------------------------------------------
    vector<Country*> rangeFilter(int ind, double low, double high) const {
        return trees[ind].rangeQuery(low, high);
    }

    // -----------------------------------------------------------------------
    // FEATURE 3: early warning for one indicator.
    // We ask the tree two questions (two range queries); GREEN is the rest,
    // found by subtraction.  We never look at every country one by one.
    // -----------------------------------------------------------------------
    Warning earlyWarning(int ind, const Threshold& t) const {
        Warning w;

        if (t.higherIsWorse) {
            // Inflation, unemployment, debt: a big value is bad.
            //   RED    = value >= red line
            //   YELLOW = value >= yellow line, but below the red line
            w.red = trees[ind].rangeQuery(t.red, BIG);

            vector<Country*> yellowAndRed = trees[ind].rangeQuery(t.yellow, BIG);
            for (int i = 0; i < (int)yellowAndRed.size(); i++) {
                if (yellowAndRed[i]->get(ind) < t.red) {
                    w.yellow.push_back(yellowAndRed[i]);
                }
            }

            // The tree gives small -> big.  We want the WORST (biggest) first.
            reverse(w.red.begin(), w.red.end());
            reverse(w.yellow.begin(), w.yellow.end());
        } else {
            // Growth: a small value is bad.
            //   RED    = value < red line
            //   YELLOW = value >= red line, but below the yellow line
            vector<Country*> low = trees[ind].rangeQuery(-BIG, t.red);
            for (int i = 0; i < (int)low.size(); i++) {
                if (low[i]->get(ind) < t.red) {          // (the range includes t.red itself, so check)
                    w.red.push_back(low[i]);
                }
            }

            vector<Country*> middle = trees[ind].rangeQuery(t.red, t.yellow);
            for (int i = 0; i < (int)middle.size(); i++) {
                double v = middle[i]->get(ind);
                if (v >= t.red && v < t.yellow) {
                    w.yellow.push_back(middle[i]);
                }
            }
            // The tree already gives small -> big, which is worst first here.
        }

        w.green  = withData[ind] - (int)w.red.size() - (int)w.yellow.size();
        w.noData = totalCountries() - withData[ind];
        return w;
    }

    // -----------------------------------------------------------------------
    // Used only to suggest names when a lookup fails.
    // Example: "korea" -> "Korea, Rep." and "Korea, Dem. People's Rep."
    // -----------------------------------------------------------------------
    vector<Country*> namesContaining(const string& text) const {
        vector<Country*> found;
        string wanted = toLowerCase(text);
        for (int i = 0; i < (int)all.size(); i++) {
            if (toLowerCase(all[i].name).find(wanted) != string::npos) {
                found.push_back(&all[i]);
            }
        }
        return found;
    }

private:
    vector<Country>& all;               // all the countries (owned by main)
    HashTable hash;                     // finds a country by name
    BST       trees[NUM_INDICATORS];    // trees[INFLATION] is the inflation tree, and so on
    int       withData[NUM_INDICATORS]; // how many countries have a value, per indicator
};

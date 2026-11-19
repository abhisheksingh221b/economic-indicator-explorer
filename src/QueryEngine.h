// QueryEngine.h
// ---------------------------------------------------------------------------
// WHAT   The "brain" of the program.  It owns the hash table and the five AVL
//        trees and answers every question.  It only RETURNS answers; it never
//        prints anything.
//
// WHY SEPARATE FROM THE MENU
//   Printing and typing live in Menu.h.  Keeping the thinking apart from the
//   screen makes each file shorter and lets the self-test check the answers
//   without any screen.
// ---------------------------------------------------------------------------
#pragma once
#include <algorithm>
#include <cmath>
#include <map>
#include <string>
#include <vector>
#include "AVLTree.h"
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
    std::vector<Country*> red;       // the red countries, worst first
    std::vector<Country*> yellow;    // the yellow countries, worst first
    int green  = 0;                  // green countries are only COUNTED (the list would be too long)
    int noData = 0;                  // countries with no value for this indicator
};

// One similar country found by the peer finder.
struct Peer {
    Country* country;
    double   distance;               // smaller means more similar
};

// Used by the combined warning: a country and how many indicators are red for it.
struct MultiRed {
    Country* country;
    int      redCount;
};


class QueryEngine {
public:
    // The vector of countries must NOT change after this point, because the
    // hash table and the trees keep pointers into it.
    QueryEngine(std::vector<Country>& countries) : all(countries) {
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
    Country* lookup(const std::string& name) const {
        return hash.search(name);
    }

    // -----------------------------------------------------------------------
    // FEATURE 2: range filter (AVL tree)
    // All countries whose value for this indicator is from low to high.
    // -----------------------------------------------------------------------
    std::vector<Country*> rangeFilter(int ind, double low, double high) const {
        return trees[ind].rangeQuery(low, high);
    }

    // -----------------------------------------------------------------------
    // FEATURE 3: top-k ranking (AVL tree)
    // -----------------------------------------------------------------------
    std::vector<Country*> topK(int ind, int k, bool highest) const {
        return trees[ind].topK(k, highest);
    }

    // -----------------------------------------------------------------------
    // FEATURE 4: early warning for one indicator.
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

            std::vector<Country*> yellowAndRed = trees[ind].rangeQuery(t.yellow, BIG);
            for (int i = 0; i < (int)yellowAndRed.size(); i++) {
                if (yellowAndRed[i]->get(ind) < t.red) {
                    w.yellow.push_back(yellowAndRed[i]);
                }
            }

            // The tree gives small -> big.  We want the WORST (biggest) first.
            std::reverse(w.red.begin(), w.red.end());
            std::reverse(w.yellow.begin(), w.yellow.end());
        } else {
            // Growth: a small value is bad.
            //   RED    = value < red line
            //   YELLOW = value >= red line, but below the yellow line
            std::vector<Country*> low = trees[ind].rangeQuery(-BIG, t.red);
            for (int i = 0; i < (int)low.size(); i++) {
                if (low[i]->get(ind) < t.red) {          // (the range includes t.red itself, so check)
                    w.red.push_back(low[i]);
                }
            }

            std::vector<Country*> middle = trees[ind].rangeQuery(t.red, t.yellow);
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
    // Combined warning: countries that are RED on two or more indicators.
    // thr[] holds the red/yellow lines of the five indicators.
    // -----------------------------------------------------------------------
    std::vector<MultiRed> redOnSeveral(const Threshold thr[NUM_INDICATORS]) const {
        // A map works like a dictionary: country -> how many red flags it has.
        std::map<Country*, int> redFlags;

        for (int ind = 0; ind < NUM_INDICATORS; ind++) {
            if (!thr[ind].used) continue;                // GDP per capita has no red/green

            Warning w = earlyWarning(ind, thr[ind]);
            for (int i = 0; i < (int)w.red.size(); i++) {
                redFlags[w.red[i]]++;                    // one more red flag for this country
            }
        }

        // Keep only the countries with 2 or more red flags.
        std::vector<MultiRed> result;
        std::map<Country*, int>::iterator it;
        for (it = redFlags.begin(); it != redFlags.end(); ++it) {
            if (it->second >= 2) {
                MultiRed m;
                m.country  = it->first;
                m.redCount = it->second;
                result.push_back(m);
            }
        }

        std::sort(result.begin(), result.end(), moreFlagsFirst);
        return result;
    }

    // -----------------------------------------------------------------------
    // FEATURE 5: peer finder - countries that look like the target country.
    //   Step 1: ask the GDP tree for countries with GDP within +-25 % of the target.
    //   Step 2: keep only those whose inflation is within +-2 points.
    //   Step 3: give each a "distance" (smaller = more similar), sort, and
    //           return the closest ones.
    // -----------------------------------------------------------------------
    std::vector<Peer> findPeers(const Country* target, int howMany) const {
        std::vector<Peer> peers;

        // Without both numbers we cannot compare.
        if (!target->has(GDP) || !target->has(INFLATION)) {
            return peers;
        }

        double gdp = target->gdp;
        double inflation = target->inflation;

        std::vector<Country*> similarGdp = trees[GDP].rangeQuery(0.75 * gdp, 1.25 * gdp);
        for (int i = 0; i < (int)similarGdp.size(); i++) {
            Country* c = similarGdp[i];

            if (c == target) continue;                   // a country is not its own peer
            if (!c->has(INFLATION)) continue;
            if (std::fabs(c->inflation - inflation) > 2.0) continue;

            // distance = (how far the GDP is, as a share of the target's GDP)
            //          + (how far the inflation is, measured in units of 2 points)
            // Inside the filter both parts are between 0 and 1, so neither one
            // counts more than the other.
            double gdpGap       = std::fabs(c->gdp - gdp) / gdp;
            double inflationGap = std::fabs(c->inflation - inflation) / 2.0;

            Peer p;
            p.country  = c;
            p.distance = gdpGap + inflationGap;
            peers.push_back(p);
        }

        std::sort(peers.begin(), peers.end(), closerFirst);

        if ((int)peers.size() > howMany) {
            peers.resize(howMany);                       // keep only the closest ones
        }
        return peers;
    }

    // -----------------------------------------------------------------------
    // Used only to suggest names when a lookup fails.
    // Example: "korea" -> "Korea, Rep." and "Korea, Dem. People's Rep."
    // -----------------------------------------------------------------------
    std::vector<Country*> namesContaining(const std::string& text) const {
        std::vector<Country*> found;
        std::string wanted = toLowerCase(text);
        for (int i = 0; i < (int)all.size(); i++) {
            if (toLowerCase(all[i].name).find(wanted) != std::string::npos) {
                found.push_back(&all[i]);
            }
        }
        return found;
    }

private:
    std::vector<Country>& all;          // all the countries (owned by main)
    HashTable hash;                     // finds a country by name
    AVLTree   trees[NUM_INDICATORS];    // trees[INFLATION] is the inflation tree, and so on
    int       withData[NUM_INDICATORS]; // how many countries have a value, per indicator

    // Two small "which one comes first?" rules for std::sort.
    static bool moreFlagsFirst(const MultiRed& a, const MultiRed& b) {
        if (a.redCount != b.redCount) {
            return a.redCount > b.redCount;              // most red flags first
        }
        return a.country->name < b.country->name;        // same count: alphabetical
    }

    static bool closerFirst(const Peer& a, const Peer& b) {
        return a.distance < b.distance;                  // smallest distance first
    }
};

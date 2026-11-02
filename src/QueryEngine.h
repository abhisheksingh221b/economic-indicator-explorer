// QueryEngine.h
// ---------------------------------------------------------------------------
// WHAT:  the "brain".  It owns the hash table and the five AVL trees and
//        answers the questions.  It only RETURNS answers; it never prints.
// WHY SEPARATE:  printing and typing live in Menu.h.  Keeping them apart means
//        the logic can be tested without a screen, and each file stays short.
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

const double BIG = 1e12;   // "bigger than any real value" - used for open-ended ranges

// Result of the early-warning check for ONE indicator.
struct Warning {
    std::vector<Country*> red;      // worst first
    std::vector<Country*> yellow;   // worst first
    int green  = 0;                 // just a count (the list would be very long)
    int noData = 0;                 // countries with no value for this indicator
};

// One similar country found by the peer finder.
struct Peer {
    Country* country;
    double   distance;              // smaller = more similar
};

// Used by the "combined" warning: a country and how many indicators are red.
struct MultiRed {
    Country* country;
    int      redCount;
};

class QueryEngine {
public:
    // The countries vector must NOT change after this point (we store pointers into it).
    explicit QueryEngine(std::vector<Country>& countries) : all(countries) {
        for (Country& c : all) {
            hash.insert(&c);                                  // every country goes in the hash table
            for (int ind = 0; ind < NUM_INDICATORS; ind++)
                if (c.has(ind)) {                             // missing values are skipped
                    trees[ind].insert(c.get(ind), &c);
                    withData[ind]++;
                }
        }
    }

    int totalCountries() const { return (int)all.size(); }
    int countWithData(int ind) const { return withData[ind]; }

    // ---- feature 1: lookup by name (hash table) ----
    Country* lookup(const std::string& name) const { return hash.search(name); }

    // ---- feature 2: range filter (AVL tree) ----
    std::vector<Country*> rangeFilter(int ind, double lo, double hi) const {
        return trees[ind].rangeQuery(lo, hi);
    }

    // ---- feature 3: top-k ranking (AVL tree) ----
    std::vector<Country*> topK(int ind, int k, bool highest) const {
        return trees[ind].topK(k, highest);
    }

    // ---- feature 4: early warning (two range queries on one tree) ----
    Warning earlyWarning(int ind, const Threshold& t) const {
        Warning w;
        if (t.higherIsWorse) {
            // red = value >= red line ; yellow = from yellow line up to (not including) red line
            w.red = trees[ind].rangeQuery(t.red, BIG);
            for (Country* c : trees[ind].rangeQuery(t.yellow, BIG))
                if (c->get(ind) < t.red) w.yellow.push_back(c);
            std::reverse(w.red.begin(), w.red.end());          // trees give small->big; we want worst (biggest) first
            std::reverse(w.yellow.begin(), w.yellow.end());
        } else {
            // growth: red = value < red line ; yellow = from red line up to (not including) yellow line
            for (Country* c : trees[ind].rangeQuery(-BIG, t.red))
                if (c->get(ind) < t.red) w.red.push_back(c);   // already worst (smallest) first
            for (Country* c : trees[ind].rangeQuery(t.red, t.yellow))
                if (c->get(ind) >= t.red && c->get(ind) < t.yellow) w.yellow.push_back(c);
        }
        w.green  = withData[ind] - (int)w.red.size() - (int)w.yellow.size();
        w.noData = totalCountries() - withData[ind];
        return w;
    }

    // Combined warning: countries that are RED on two or more indicators.
    std::vector<MultiRed> redOnSeveral(const Threshold thr[NUM_INDICATORS]) const {
        std::map<Country*, int> counter;                       // country -> number of red flags
        for (int ind = 0; ind < NUM_INDICATORS; ind++) {
            if (!thr[ind].used) continue;                      // GDP per capita has no red/green
            Warning w = earlyWarning(ind, thr[ind]);
            for (Country* c : w.red) counter[c]++;
        }
        std::vector<MultiRed> out;
        for (const auto& pair : counter)
            if (pair.second >= 2) out.push_back({pair.first, pair.second});
        std::sort(out.begin(), out.end(), [](const MultiRed& a, const MultiRed& b) {
            if (a.redCount != b.redCount) return a.redCount > b.redCount;   // most red flags first
            return a.country->name < b.country->name;
        });
        return out;
    }

    // ---- feature 5: peer finder ----
    // Step 1: GDP tree range query  -> countries with GDP within +-25 % of the target.
    // Step 2: keep those whose inflation is within +-2 points.
    // Step 3: rank by a distance number and return the closest `howMany`.
    std::vector<Peer> findPeers(const Country* target, int howMany) const {
        std::vector<Peer> peers;
        if (!target->has(GDP) || !target->has(INFLATION)) return peers;   // cannot compare without both

        double g = target->gdp, i = target->inflation;
        for (Country* c : trees[GDP].rangeQuery(0.75 * g, 1.25 * g)) {
            if (c == target || !c->has(INFLATION)) continue;
            if (std::fabs(c->inflation - i) > 2.0) continue;
            // distance = relative GDP gap + inflation gap measured in "2-point units".
            // Inside the filter both parts are between 0 and 1, so neither one dominates.
            double d = std::fabs(c->gdp - g) / g + std::fabs(c->inflation - i) / 2.0;
            peers.push_back({c, d});
        }
        std::sort(peers.begin(), peers.end(), [](const Peer& a, const Peer& b) { return a.distance < b.distance; });
        if ((int)peers.size() > howMany) peers.resize(howMany);
        return peers;
    }

    // Used only to suggest names when a lookup fails ("Korea" -> "Korea, Rep.").
    std::vector<Country*> namesContaining(const std::string& text) const {
        std::vector<Country*> out;
        std::string t = lower(text);
        for (Country& c : all)
            if (lower(c.name).find(t) != std::string::npos) out.push_back(&c);
        return out;
    }

private:
    std::vector<Country>& all;
    HashTable hash;
    AVLTree   trees[NUM_INDICATORS];   // trees[INFLATION] is the inflation tree, etc.
    int       withData[NUM_INDICATORS] = {0, 0, 0, 0, 0};

    static std::string lower(std::string s) {
        for (char& ch : s) ch = (char)std::tolower((unsigned char)ch);
        return s;
    }
};

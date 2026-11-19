#pragma once
#include <algorithm>
#include <string>
#include <vector>
#include "BST.h"
#include "Country.h"
#include "HashTable.h"

const double BIG = 1e12;

struct Warning {
    vector<Country*> red;
    vector<Country*> yellow;
    int green  = 0;
    int noData = 0;
};

class QueryEngine {
public:
    QueryEngine(vector<Country>& countries) : all(countries) {
        for (int ind = 0; ind < NUM_INDICATORS; ind++) {
            withData[ind] = 0;
        }

        for (int i = 0; i < (int)all.size(); i++) {
            Country* c = &all[i];

            hash.insert(c);

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

    int countWithData(int ind) const {
        return withData[ind];
    }

    Country* lookup(const string& name) const {
        return hash.search(name);
    }

    vector<Country*> rangeFilter(int ind, double low, double high) const {
        return trees[ind].rangeQuery(low, high);
    }

    Warning earlyWarning(int ind, const Threshold& t) const {
        Warning w;

        if (t.higherIsWorse) {
            w.red = trees[ind].rangeQuery(t.red, BIG);

            vector<Country*> yellowAndRed = trees[ind].rangeQuery(t.yellow, BIG);
            for (int i = 0; i < (int)yellowAndRed.size(); i++) {
                if (yellowAndRed[i]->get(ind) < t.red) {
                    w.yellow.push_back(yellowAndRed[i]);
                }
            }

            reverse(w.red.begin(), w.red.end());
            reverse(w.yellow.begin(), w.yellow.end());
        } else {
            vector<Country*> low = trees[ind].rangeQuery(-BIG, t.red);
            for (int i = 0; i < (int)low.size(); i++) {
                if (low[i]->get(ind) < t.red) {
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
        }

        w.green  = withData[ind] - (int)w.red.size() - (int)w.yellow.size();
        w.noData = totalCountries() - withData[ind];
        return w;
    }

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
    vector<Country>& all;
    HashTable hash;
    BST       trees[NUM_INDICATORS];
    int       withData[NUM_INDICATORS];
};

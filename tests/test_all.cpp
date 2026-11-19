// tests/test_all.cpp  -  the automatic self-test.   Run it with:  make test
//
// IDEA: for every feature we compute the answer TWICE:
//   1. the fast way (hash table / AVL tree, the way the program does it), and
//   2. the slow-but-obviously-correct way (a plain loop over all countries).
// If the two answers are ever different, the test reports a failure.
// There are 18 checks in total.
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>
#include "Loader.h"
#include "QueryEngine.h"

int passed = 0;
int failed = 0;

// Record one check: if "ok" is true it passed, otherwise print what failed.
void check(bool ok, const string& message) {
    if (ok) {
        passed++;
    } else {
        failed++;
        printf("FAIL: %s\n", message.c_str());
    }
}

// A very small random number generator (so the test does not need extra libraries).
// It always produces the same numbers, so the test gives the same result every time.
unsigned int seed = 12345;
unsigned int randomNumber() {
    seed = seed * 1103515245u + 12345u;
    return (seed / 65536u) % 32768u;
}

// ---------------------------------------------------------------------------
// Sorting rules used by the slow "reference" answers.
// The indicator being sorted is stored here so the rules can use it.
// ---------------------------------------------------------------------------
int sortIndicator = 0;

// Smallest value first; equal values: alphabetical by name.
bool lowToHigh(Country* a, Country* b) {
    if (a->get(sortIndicator) != b->get(sortIndicator)) {
        return a->get(sortIndicator) < b->get(sortIndicator);
    }
    return a->name < b->name;
}

// Biggest value first; equal values: reverse alphabetical (this is the exact
// reverse of lowToHigh, which is what "top-k highest" produces).
bool highToLow(Country* a, Country* b) {
    return lowToHigh(b, a);
}

bool closerFirst(const Peer& a, const Peer& b) {
    return a.distance < b.distance;
}

// Make a copy of a text in capital letters / small letters.
string toUpperCase(string text) {
    for (int i = 0; i < (int)text.size(); i++) {
        if (text[i] >= 'a' && text[i] <= 'z') text[i] = text[i] - ('a' - 'A');
    }
    return text;
}


// ---------------------------------------------------------------------------
// The checks, grouped by feature.  Each function runs some of the 18 checks.
// ---------------------------------------------------------------------------

// Checks 3, 4, 5: the hash table.
void testHashTable(QueryEngine& engine, vector<Country>& countries) {
    bool allFound = true;
    for (int i = 0; i < (int)countries.size(); i++) {
        Country* c = &countries[i];
        // Every country must be found by its exact name, in CAPITALS and in small letters.
        if (engine.lookup(c->name) != c) allFound = false;
        if (engine.lookup(toUpperCase(c->name)) != c) allFound = false;
        if (engine.lookup(toLowerCase(c->name)) != c) allFound = false;
    }
    check(allFound, "hash table finds all 217 countries (exact, UPPER, lower case)");
    check(engine.lookup("Atlantis") == nullptr, "unknown country -> nullptr");
    check(engine.lookup("") == nullptr, "empty name -> nullptr");
}

// Check 6: range queries.
void testRange(QueryEngine& engine, vector<Country>& countries) {
    bool ok = true;

    for (int ind = 0; ind < NUM_INDICATORS; ind++) {
        // Collect all real values of this indicator.
        vector<double> values;
        for (int i = 0; i < (int)countries.size(); i++) {
            if (countries[i].has(ind)) values.push_back(countries[i].get(ind));
        }

        for (int round = 0; round < 300; round++) {
            // Take the ends of the range from REAL values, so equal values and
            // exact edges are tested too.
            double a = values[randomNumber() % values.size()];
            double b = values[randomNumber() % values.size()];
            if (a > b) { double temp = a; a = b; b = temp; }

            vector<Country*> fast = engine.rangeFilter(ind, a, b);

            vector<Country*> slow;
            for (int i = 0; i < (int)countries.size(); i++) {
                Country* c = &countries[i];
                if (c->has(ind) && c->get(ind) >= a && c->get(ind) <= b) slow.push_back(c);
            }
            sortIndicator = ind;
            sort(slow.begin(), slow.end(), lowToHigh);

            if (fast != slow) ok = false;        // same countries AND same order
        }
    }
    check(ok, "range query == brute force (1500 random ranges, incl. ties)");
}

// Check 7: top-k.
void testTopK(QueryEngine& engine, vector<Country>& countries) {
    bool ok = true;
    int sizes[4] = {1, 5, 10, 500};

    for (int ind = 0; ind < NUM_INDICATORS; ind++) {
        for (int s = 0; s < 4; s++) {
            int k = sizes[s];
            for (int direction = 0; direction < 2; direction++) {
                bool highest = (direction == 0);

                vector<Country*> fast = engine.topK(ind, k, highest);

                vector<Country*> slow;
                for (int i = 0; i < (int)countries.size(); i++) {
                    if (countries[i].has(ind)) slow.push_back(&countries[i]);
                }
                sortIndicator = ind;
                if (highest) sort(slow.begin(), slow.end(), highToLow);
                else         sort(slow.begin(), slow.end(), lowToHigh);
                if ((int)slow.size() > k) slow.resize(k);

                if (fast != slow) ok = false;
            }
        }
    }
    check(ok, "top-k == sorted list (all indicators, k = 1,5,10,500, highest and lowest)");
}

// Check 8: early warning for each indicator that has colours.
void testEarlyWarning(QueryEngine& engine, vector<Country>& countries) {
    bool ok = true;

    for (int ind = GROWTH; ind < NUM_INDICATORS; ind++) {
        Threshold t = defaultThreshold(ind);
        Warning w = engine.earlyWarning(ind, t);

        // Slow way: look at every country and count the statuses.
        int red = 0, yellow = 0, green = 0, noData = 0;
        for (int i = 0; i < (int)countries.size(); i++) {
            Status s = statusOf(countries[i].get(ind), t);
            if (s == RED)         red++;
            else if (s == YELLOW) yellow++;
            else if (s == GREEN)  green++;
            else                  noData++;
        }
        if ((int)w.red.size() != red || (int)w.yellow.size() != yellow) ok = false;
        if (w.green != green || w.noData != noData) ok = false;

        // Every country in the red list must really be red, and so on.
        for (int i = 0; i < (int)w.red.size(); i++) {
            if (statusOf(w.red[i]->get(ind), t) != RED) ok = false;
        }
        for (int i = 0; i < (int)w.yellow.size(); i++) {
            if (statusOf(w.yellow[i]->get(ind), t) != YELLOW) ok = false;
        }

        // The red list must be "worst first".
        for (int i = 1; i < (int)w.red.size(); i++) {
            double before = w.red[i - 1]->get(ind);
            double now    = w.red[i]->get(ind);
            if (t.higherIsWorse && before < now) ok = false;
            if (!t.higherIsWorse && before > now) ok = false;
        }
    }
    check(ok, "early warning red/yellow/green/no-data == brute force (4 indicators)");
}

// Count how many indicators (growth, inflation, unemployment, debt) are RED for a country.
int countRedFlags(Country* c, Threshold thr[]) {
    int reds = 0;
    for (int ind = GROWTH; ind < NUM_INDICATORS; ind++) {
        if (statusOf(c->get(ind), thr[ind]) == RED) reds++;
    }
    return reds;
}

// Check 9: the combined warning.
void testCombined(QueryEngine& engine, vector<Country>& countries) {
    Threshold thr[NUM_INDICATORS];
    for (int ind = 0; ind < NUM_INDICATORS; ind++) {
        thr[ind] = defaultThreshold(ind);
    }

    vector<MultiRed> fast = engine.redOnSeveral(thr);

    // Slow way: count the countries with 2 or more red flags.
    int expected = 0;
    for (int i = 0; i < (int)countries.size(); i++) {
        if (countRedFlags(&countries[i], thr) >= 2) expected++;
    }

    bool ok = ((int)fast.size() == expected);
    for (int i = 0; i < (int)fast.size(); i++) {
        if (countRedFlags(fast[i].country, thr) != fast[i].redCount) ok = false;
    }
    check(ok, "combined warning == brute force");
}

// Check 10: the peer finder, for EVERY country.
void testPeers(QueryEngine& engine, vector<Country>& countries) {
    bool ok = true;

    for (int t = 0; t < (int)countries.size(); t++) {
        Country* target = &countries[t];
        vector<Peer> fast = engine.findPeers(target, 5);

        // Slow way: compare the target with every other country.
        vector<Peer> slow;
        if (target->has(GDP) && target->has(INFLATION)) {
            for (int i = 0; i < (int)countries.size(); i++) {
                Country* c = &countries[i];
                if (c == target) continue;
                if (!c->has(GDP) || !c->has(INFLATION)) continue;
                if (c->gdp < 0.75 * target->gdp || c->gdp > 1.25 * target->gdp) continue;
                if (fabs(c->inflation - target->inflation) > 2.0) continue;

                Peer p;
                p.country  = c;
                p.distance = fabs(c->gdp - target->gdp) / target->gdp
                           + fabs(c->inflation - target->inflation) / 2.0;
                slow.push_back(p);
            }
        }
        sort(slow.begin(), slow.end(), closerFirst);
        if (slow.size() > 5) slow.resize(5);

        if (fast.size() != slow.size()) {
            ok = false;
            continue;
        }
        for (int i = 0; i < (int)fast.size(); i++) {
            if (fast[i].distance != slow[i].distance) ok = false;
        }
    }
    check(ok, "peer finder == brute force for every country");
}

// Checks 11 to 18: the AVL tree on its own, with 5000 made-up countries.
void testAvlTree() {
    vector<Country> many(5000);
    for (int i = 0; i < 5000; i++) {
        many[i].name = "C" + to_string(100000 + i);    // C100000, C100001, ... (alphabetical = numeric)
        many[i].gdp = i;
    }

    // Tree 1: values inserted in ASCENDING order.  This is the worst case: a
    // plain tree would turn into one long line.
    AVLTree sortedTree;
    for (int i = 0; i < 5000; i++) {
        sortedTree.insert(many[i].gdp, &many[i]);
    }

    // Tree 2: the same values in a shuffled order.
    vector<int> order(5000);
    for (int i = 0; i < 5000; i++) order[i] = i;
    for (int i = 4999; i > 0; i--) {                         // shuffle
        int j = randomNumber() % (i + 1);
        int temp = order[i]; order[i] = order[j]; order[j] = temp;
    }
    AVLTree randomTree;
    for (int i = 0; i < 5000; i++) {
        randomTree.insert(many[order[i]].gdp, &many[order[i]]);
    }

    // Tree 3: 5000 countries that ALL have the same value (7).
    AVLTree tieTree;
    for (int i = 0; i < 5000; i++) {
        tieTree.insert(7.0, &many[i]);
    }

    check(sortedTree.isBalanced(), "AVL balanced after 5000 ascending inserts");
    check(randomTree.isBalanced(), "AVL balanced after 5000 random inserts");
    check(tieTree.isBalanced(), "AVL balanced when all 5000 keys are equal");

    vector<Country*> tied = tieTree.rangeQuery(7.0, 7.0);
    check(tied.size() == 5000, "equal keys: no country lost (5000 of 5000 kept)");

    bool sortedByName = true;
    for (int i = 1; i < (int)tied.size(); i++) {
        if (tied[i - 1]->name > tied[i]->name) sortedByName = false;
    }
    check(sortedByName, "equal keys come out sorted by name");

    check(sortedTree.rangeQuery(100, 104).size() == 5, "range 100..104 returns 5");
    check(sortedTree.rangeQuery(1e9, 2e9).empty(), "empty range returns nothing");

    AVLTree emptyTree;
    check(emptyTree.topK(3, true).empty() && emptyTree.rangeQuery(0, 1).empty(), "empty tree is safe");
}


int main() {
    // Checks 1 and 2: the data file loads and has 217 countries.
    vector<Country> countries;
    check(loadCountries("data/countries_2023.csv", countries), "data file loads");

    QueryEngine engine(countries);
    check(engine.totalCountries() == 217, "217 countries loaded");

    testHashTable(engine, countries);
    testRange(engine, countries);
    testTopK(engine, countries);
    testEarlyWarning(engine, countries);
    testCombined(engine, countries);
    testPeers(engine, countries);
    testAvlTree();

    printf("\n%d checks passed, %d failed\n", passed, failed);
    if (failed == 0) return 0;
    return 1;
}

// tests/test_all.cpp - automatic self-test.  Run with:  make test
// It checks every data structure and feature against a slow-but-obviously-correct
// "brute force" answer (plain loops over all countries).
#include <algorithm>
#include <cstdio>
#include <random>
#include "Loader.h"
#include "QueryEngine.h"

static int passed = 0, failed = 0;
#define CHECK(cond, msg) do { if (cond) passed++; else { failed++; std::printf("FAIL: %s\n", msg); } } while (0)

int main() {
    std::vector<Country> countries;
    CHECK(loadCountries("data/countries_2023.csv", countries), "data file loads");
    QueryEngine engine(countries);
    CHECK(engine.totalCountries() == 217, "217 countries loaded");

    // ---- hash table: every country is found, any letter case, and nonsense is not found
    bool allFound = true;
    for (Country& c : countries) {
        std::string up = c.name, low = c.name;
        for (char& ch : up)  ch = (char)toupper((unsigned char)ch);
        for (char& ch : low) ch = (char)tolower((unsigned char)ch);
        if (engine.lookup(c.name) != &c || engine.lookup(up) != &c || engine.lookup(low) != &c) allFound = false;
    }
    CHECK(allFound, "hash table finds all 217 countries (exact, UPPER, lower case)");
    CHECK(engine.lookup("Atlantis") == nullptr, "unknown country -> nullptr");
    CHECK(engine.lookup("") == nullptr, "empty name -> nullptr");

    // ---- range query: compare with brute force, for every indicator, 300 random ranges
    std::mt19937 rng(42);
    bool rangeOk = true;
    for (int ind = 0; ind < NUM_INDICATORS; ind++)
        for (int t = 0; t < 300; t++) {
            std::vector<double> vals;
            for (Country& c : countries) if (c.has(ind)) vals.push_back(c.get(ind));
            std::sort(vals.begin(), vals.end());
            double a = vals[rng() % vals.size()], b = vals[rng() % vals.size()];   // use real values so ties/edges are tested
            if (a > b) std::swap(a, b);
            std::vector<Country*> fast = engine.rangeFilter(ind, a, b), slow;
            for (Country& c : countries) if (c.has(ind) && c.get(ind) >= a && c.get(ind) <= b) slow.push_back(&c);
            std::sort(slow.begin(), slow.end(), [&](Country* x, Country* y) {
                if (x->get(ind) != y->get(ind)) return x->get(ind) < y->get(ind);
                return x->name < y->name; });
            if (fast != slow) rangeOk = false;      // same countries AND same order (value, then name)
        }
    CHECK(rangeOk, "range query == brute force (1500 random ranges, incl. ties)");

    // ---- top-k: compare with sorting
    bool topOk = true;
    for (int ind = 0; ind < NUM_INDICATORS; ind++)
        for (int k : {1, 5, 10, 500})
            for (bool highest : {true, false}) {
                std::vector<Country*> fast = engine.topK(ind, k, highest), slow;
                for (Country& c : countries) if (c.has(ind)) slow.push_back(&c);
                std::sort(slow.begin(), slow.end(), [&](Country* x, Country* y) {
                    if (x->get(ind) != y->get(ind)) return highest ? x->get(ind) > y->get(ind) : x->get(ind) < y->get(ind);
                    return highest ? x->name > y->name : x->name < y->name; });
                if ((int)slow.size() > k) slow.resize(k);
                if (fast != slow) topOk = false;
            }
    CHECK(topOk, "top-k == sorted list (all indicators, k = 1,5,10,500, highest and lowest)");

    // ---- early warning: counts and membership vs brute force
    bool warnOk = true;
    for (int ind = GROWTH; ind < NUM_INDICATORS; ind++) {
        Threshold t = defaultThreshold(ind);
        Warning w = engine.earlyWarning(ind, t);
        int r = 0, y = 0, g = 0, n = 0;
        for (Country& c : countries) {
            Status s = statusOf(c.get(ind), t);
            if (s == RED) r++; else if (s == YELLOW) y++; else if (s == GREEN) g++; else n++;
        }
        if ((int)w.red.size() != r || (int)w.yellow.size() != y || w.green != g || w.noData != n) warnOk = false;
        for (Country* c : w.red)    if (statusOf(c->get(ind), t) != RED)    warnOk = false;
        for (Country* c : w.yellow) if (statusOf(c->get(ind), t) != YELLOW) warnOk = false;
        // worst first
        for (size_t i = 1; i < w.red.size(); i++)
            if (t.higherIsWorse ? w.red[i-1]->get(ind) < w.red[i]->get(ind) : w.red[i-1]->get(ind) > w.red[i]->get(ind)) warnOk = false;
    }
    CHECK(warnOk, "early warning red/yellow/green/no-data == brute force (4 indicators)");

    // combined
    Threshold thr[NUM_INDICATORS];
    for (int i = 0; i < NUM_INDICATORS; i++) thr[i] = defaultThreshold(i);
    std::vector<MultiRed> multi = engine.redOnSeveral(thr);
    bool multiOk = true; size_t expected = 0;
    for (Country& c : countries) {
        int reds = 0;
        for (int i = GROWTH; i < NUM_INDICATORS; i++) if (statusOf(c.get(i), thr[i]) == RED) reds++;
        if (reds >= 2) expected++;
    }
    if (multi.size() != expected) multiOk = false;
    for (auto& m : multi) {
        int reds = 0;
        for (int i = GROWTH; i < NUM_INDICATORS; i++) if (statusOf(m.country->get(i), thr[i]) == RED) reds++;
        if (reds != m.redCount) multiOk = false;
    }
    CHECK(multiOk, "combined warning == brute force");

    // ---- peer finder vs brute force, for every country that has GDP and inflation
    bool peerOk = true;
    for (Country& t : countries) {
        std::vector<Peer> fast = engine.findPeers(&t, 5);
        std::vector<Peer> slow;
        if (t.has(GDP) && t.has(INFLATION))
            for (Country& c : countries)
                if (&c != &t && c.has(GDP) && c.has(INFLATION) && c.gdp >= 0.75 * t.gdp && c.gdp <= 1.25 * t.gdp
                    && std::fabs(c.inflation - t.inflation) <= 2.0)
                    slow.push_back({&c, std::fabs(c.gdp - t.gdp) / t.gdp + std::fabs(c.inflation - t.inflation) / 2.0});
        std::sort(slow.begin(), slow.end(), [](const Peer& a, const Peer& b) { return a.distance < b.distance; });
        if (slow.size() > 5) slow.resize(5);
        if (fast.size() != slow.size()) { peerOk = false; continue; }
        for (size_t i = 0; i < fast.size(); i++) if (fast[i].distance != slow[i].distance) peerOk = false;
    }
    CHECK(peerOk, "peer finder == brute force for every country");

    // ---- AVL tree on its own: balance must hold even for the WORST input (already sorted)
    std::vector<Country> many(5000);
    AVLTree sortedTree, randomTree, tieTree;
    for (int i = 0; i < 5000; i++) { many[i].name = "C" + std::to_string(100000 + i); many[i].gdp = i; }
    for (int i = 0; i < 5000; i++) sortedTree.insert(many[i].gdp, &many[i]);          // ascending: a plain BST would become a line
    std::vector<int> order(5000); for (int i = 0; i < 5000; i++) order[i] = i;
    std::shuffle(order.begin(), order.end(), rng);
    for (int i : order) randomTree.insert(many[i].gdp, &many[i]);
    for (int i = 0; i < 5000; i++) tieTree.insert(7.0, &many[i]);                     // every value identical
    CHECK(sortedTree.isBalanced(), "AVL balanced after 5000 ascending inserts");
    CHECK(randomTree.isBalanced(), "AVL balanced after 5000 random inserts");
    CHECK(tieTree.isBalanced(), "AVL balanced when all 5000 keys are equal");
    CHECK(tieTree.rangeQuery(7.0, 7.0).size() == 5000, "equal keys: no country lost (5000 of 5000 kept)");
    std::vector<Country*> tied = tieTree.rangeQuery(7.0, 7.0);
    bool byName = true;
    for (size_t i = 1; i < tied.size(); i++) if (tied[i-1]->name > tied[i]->name) byName = false;
    CHECK(byName, "equal keys come out sorted by name");
    CHECK(sortedTree.rangeQuery(100, 104).size() == 5, "range 100..104 returns 5");
    CHECK(sortedTree.rangeQuery(1e9, 2e9).empty(), "empty range returns nothing");
    AVLTree empty;
    CHECK(empty.topK(3, true).empty() && empty.rangeQuery(0, 1).empty(), "empty tree is safe");

    std::printf("\n%d checks passed, %d failed\n", passed, failed);
    return failed == 0 ? 0 : 1;
}

// Menu.h
// ---------------------------------------------------------------------------
// WHAT:  everything the user SEES and TYPES: the menu, input checking,
//        coloured tables.  All the real work is delegated to QueryEngine.
// ---------------------------------------------------------------------------
#pragma once
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>
#include "QueryEngine.h"

// ---- colours (ANSI escape codes; they work in Linux, macOS and Windows 10+) ----
const char* const C_GREEN  = "\033[32m";
const char* const C_YELLOW = "\033[33m";
const char* const C_RED    = "\033[31m";
const char* const C_GREY   = "\033[90m";   // the 4th colour: "no data"
const char* const C_BOLD   = "\033[1m";
const char* const C_RESET  = "\033[0m";

class Menu {
public:
    explicit Menu(QueryEngine& e) : engine(e) {
        for (int i = 0; i < NUM_INDICATORS; i++) thr[i] = defaultThreshold(i);
    }

    void run() {
        while (true) {
            std::cout << "\n" << C_BOLD << "=========== ECONOMIC INDICATOR EXPLORER ===========" << C_RESET << "\n"
                      << " 1  Country lookup\n"
                      << " 2  Range filter\n"
                      << " 3  Top-k ranking\n"
                      << " 4  Early warning dashboard\n"
                      << " 5  Peer finder\n"
                      << " 0  Exit\n";
            int choice = readInt("Your choice", 0, 5);     // validates: only 0..5 accepted
            if (choice == 0) { std::cout << "Goodbye.\n"; return; }
            if (choice == 1) lookup();
            if (choice == 2) rangeFilter();
            if (choice == 3) topK();
            if (choice == 4) earlyWarning();
            if (choice == 5) peerFinder();
        }
    }

private:
    QueryEngine& engine;
    Threshold    thr[NUM_INDICATORS];     // current green/yellow/red lines (user can change them)
    static const int MAX_ROWS = 50;       // longest table we print before cutting it off

    // =====================================================================
    //  INPUT HELPERS - keep asking until the user types something valid
    // =====================================================================
    static std::string readLine(const std::string& prompt) {
        std::cout << prompt << ": ";
        std::string line;
        if (!std::getline(std::cin, line)) { std::cout << "\n"; std::exit(0); }   // input ended (Ctrl-D / end of file)
        while (!line.empty() && (line.back() == '\r' || line.back() == ' ')) line.pop_back();
        size_t start = 0;
        while (start < line.size() && line[start] == ' ') start++;
        return line.substr(start);
    }
    static int readInt(const std::string& prompt, int lo, int hi) {
        while (true) {
            std::string s = readLine(prompt + " [" + std::to_string(lo) + "-" + std::to_string(hi) + "]");
            try {
                size_t used = 0;
                int v = std::stoi(s, &used);
                if (used == s.size() && v >= lo && v <= hi) return v;
            } catch (...) {}
            std::cout << "  Please type a whole number from " << lo << " to " << hi << ".\n";
        }
    }
    static double readDouble(const std::string& prompt) {
        while (true) {
            std::string s = readLine(prompt);
            try {
                size_t used = 0;
                double v = std::stod(s, &used);
                if (used == s.size()) return v;
            } catch (...) {}
            std::cout << "  Please type a number (for example 5.5).\n";
        }
    }
    // Which indicator?  `from` = first number allowed (2 hides GDP for the early warning).
    static int pickIndicator(int from) {
        for (int i = from - 1; i < NUM_INDICATORS; i++)
            std::cout << "  " << (i + 1) << "  " << INDICATOR_NAME[i] << "\n";
        return readInt("Indicator", from, NUM_INDICATORS) - 1;
    }

    // =====================================================================
    //  OUTPUT HELPERS
    // =====================================================================
    static const char* colorOf(Status s) {
        if (s == GREEN)  return C_GREEN;
        if (s == YELLOW) return C_YELLOW;
        if (s == RED)    return C_RED;
        return C_GREY;
    }
    static const char* wordOf(Status s) {
        if (s == GREEN)  return "GREEN";
        if (s == YELLOW) return "YELLOW";
        if (s == RED)    return "RED";
        return "NO DATA";
    }
    // Colour alone is not enough (some people cannot tell red from green, and some
    // terminals have no colour) so a status is always printed as a WORD as well.

    // Pad with spaces to a fixed width.  Counts characters, not bytes, so names
    // such as "Cote d'Ivoire" with accents still line up (UTF-8 aware).
    static std::string padRight(const std::string& s, int width) {
        int chars = 0;
        for (unsigned char ch : s) if ((ch & 0xC0) != 0x80) chars++;
        return chars >= width ? s : s + std::string(width - chars, ' ');
    }
    static std::string padLeft(const std::string& s, int width) {
        return (int)s.size() >= width ? s : std::string(width - s.size(), ' ') + s;
    }
    static std::string fmt(double v, int decimals) {
        if (v == MISSING) return "-";
        char buf[40];
        std::snprintf(buf, sizeof buf, "%.*f", decimals, v);
        return buf;
    }

    void printHeader() const {
        std::cout << C_BOLD << padRight("  #", 6) << padRight("Country", 30);
        for (int i = 0; i < NUM_INDICATORS; i++) std::cout << padLeft(INDICATOR_SHORT[i], 10);
        std::cout << C_RESET << "\n" << std::string(86, '-') << "\n";
    }

    // One table row.  The column the user asked about (`highlight`) is coloured.
    void printRow(int rank, const Country* c, int highlight) const {
        std::cout << padRight("  " + std::to_string(rank), 6) << padRight(c->name, 30);
        for (int i = 0; i < NUM_INDICATORS; i++) {
            std::string cell = padLeft(fmt(c->get(i), i == GDP ? 0 : 2), 10);
            if (i == highlight && thr[i].used) std::cout << colorOf(statusOf(c->get(i), thr[i])) << cell << C_RESET;
            else                               std::cout << cell;
        }
        std::cout << "\n";
    }

    void printList(const std::vector<Country*>& list, int highlight) const {
        if (list.empty()) { std::cout << "  (no countries found)\n"; return; }
        printHeader();
        int shown = 0;
        for (Country* c : list) {
            if (shown == MAX_ROWS) break;
            printRow(++shown, c, highlight);
        }
        if ((int)list.size() > MAX_ROWS)
            std::cout << "  ... and " << (list.size() - MAX_ROWS) << " more (showing the first " << MAX_ROWS << ").\n";
        std::cout << "  Found: " << list.size() << " countries.\n";
    }

    // Ask for a country name and find it in the hash table.  If it is not found,
    // suggest similar names (e.g. "Somalia" -> "Somalia, Fed. Rep.").  Used by lookup and peer finder.
    Country* askForCountry() {
        std::string name = readLine("Country name");
        Country* c = engine.lookup(name);                 // hash table: O(1) on average
        if (c != nullptr) return c;
        std::cout << "  '" << name << "' not found.";
        std::vector<Country*> maybe = engine.namesContaining(name);
        if (!maybe.empty() && !name.empty()) {
            std::cout << " Did you mean:";
            for (size_t i = 0; i < maybe.size() && i < 5; i++) std::cout << (i ? " |" : "") << " " << maybe[i]->name;
        }
        std::cout << "\n";
        return nullptr;
    }

    // =====================================================================
    //  FEATURE 1 - country lookup
    // =====================================================================
    void lookup() {
        Country* c = askForCountry();
        if (c == nullptr) return;
        std::cout << "\n" << C_BOLD << c->name << " (" << c->iso << ")" << C_RESET << "\n";
        for (int i = 0; i < NUM_INDICATORS; i++) {
            std::cout << "  " << padRight(INDICATOR_NAME[i], 24) << padLeft(fmt(c->get(i), i == GDP ? 0 : 2), 10);
            if (thr[i].used) {
                Status s = statusOf(c->get(i), thr[i]);
                std::cout << "   " << colorOf(s) << "[" << wordOf(s) << "]" << C_RESET;
            } else if (!c->has(i)) {
                std::cout << "   " << C_GREY << "[NO DATA]" << C_RESET;
            }
            std::cout << "\n";
        }
    }

    // =====================================================================
    //  FEATURE 2 - range filter
    // =====================================================================
    void rangeFilter() {
        int ind = pickIndicator(1);
        double lo = readDouble("Lowest value");
        double hi = readDouble("Highest value");
        if (lo > hi) { std::swap(lo, hi); std::cout << "  (swapped so that low <= high)\n"; }
        printList(engine.rangeFilter(ind, lo, hi), ind);   // ascending order
    }

    // =====================================================================
    //  FEATURE 3 - top-k ranking
    // =====================================================================
    void topK() {
        int ind = pickIndicator(1);
        int k = readInt("How many countries (k)", 1, 100);
        int dir = readInt("1 = highest values, 2 = lowest values", 1, 2);
        printList(engine.topK(ind, k, dir == 1), ind);
    }

    // =====================================================================
    //  FEATURE 4 - early warning dashboard
    // =====================================================================
    void earlyWarning() {
        std::cout << "  Choose indicator (or 6 for the combined check):\n";
        for (int i = GROWTH; i < NUM_INDICATORS; i++) std::cout << "  " << (i + 1) << "  " << INDICATOR_NAME[i] << "\n";
        std::cout << "  6  Combined: countries RED on two or more indicators\n";
        int pick = readInt("Choice", 2, 6) - 1;
        if (pick == NUM_INDICATORS) { combinedWarning(); return; }
        int ind = pick;

        showThresholds(ind);
        if (readLine("Press Enter to keep these limits, or type c to change them") == "c") {
            thr[ind].yellow = readDouble("  Yellow starts at");
            thr[ind].red    = readDouble("  Red starts at");
            // sanity check: for 'higher is worse' red must be above yellow, otherwise below
            bool ok = thr[ind].higherIsWorse ? thr[ind].red >= thr[ind].yellow : thr[ind].red <= thr[ind].yellow;
            if (!ok) { thr[ind] = defaultThreshold(ind); std::cout << "  Red must be the more serious line. Defaults restored.\n"; }
        }

        Warning w = engine.earlyWarning(ind, thr[ind]);
        std::cout << "\n" << C_BOLD << "EARLY WARNING: " << INDICATOR_NAME[ind] << C_RESET << "\n";
        std::cout << C_RED << "  RED    : " << w.red.size() << " countries" << C_RESET << "\n";
        printList(w.red, ind);
        std::cout << C_YELLOW << "  YELLOW : " << w.yellow.size() << " countries" << C_RESET << "\n";
        printList(w.yellow, ind);
        std::cout << C_GREEN << "  GREEN  : " << w.green << " countries" << C_RESET << " (not listed)\n";
        std::cout << C_GREY << "  NO DATA: " << w.noData << " countries" << C_RESET << "\n";
        std::cout << "  A flag means 'look closer', not 'a crisis will happen'.\n";
    }

    void showThresholds(int ind) const {
        const Threshold& t = thr[ind];
        std::cout << "  Current limits for " << INDICATOR_NAME[ind] << ":\n";
        if (t.higherIsWorse)
            std::cout << "    " << C_YELLOW << "YELLOW" << C_RESET << " from " << t.yellow << "   " << C_RED << "RED" << C_RESET << " from " << t.red << "   (higher is worse)\n";
        else
            std::cout << "    " << C_YELLOW << "YELLOW" << C_RESET << " below " << t.yellow << "   " << C_RED << "RED" << C_RESET << " below " << t.red << "   (lower is worse)\n";
    }

    void combinedWarning() {
        std::vector<MultiRed> list = engine.redOnSeveral(thr);
        std::cout << "\n" << C_BOLD << "COMBINED WARNING: RED on two or more of growth, inflation, unemployment, debt" << C_RESET << "\n";
        if (list.empty()) { std::cout << "  (none)\n"; return; }
        std::cout << C_BOLD << padRight("  #", 6) << padRight("Country", 30) << "Red flags" << C_RESET << "\n";
        int shown = 0;
        for (const MultiRed& m : list) {
            if (shown == MAX_ROWS) break;
            std::cout << padRight("  " + std::to_string(++shown), 6) << padRight(m.country->name, 30) << C_RED << m.redCount << C_RESET << "\n";
        }
        if ((int)list.size() > MAX_ROWS) std::cout << "  ... and " << (list.size() - MAX_ROWS) << " more.\n";
        std::cout << "  Found: " << list.size() << " countries.  (Debt data exists for only some countries.)\n";
    }

    // =====================================================================
    //  FEATURE 5 - peer finder
    // =====================================================================
    void peerFinder() {
        Country* target = askForCountry();                // hash table finds the country first
        if (target == nullptr) return;
        if (!target->has(GDP) || !target->has(INFLATION)) {
            std::cout << "  " << target->name << " has no GDP or inflation value, so peers cannot be found.\n";
            return;
        }
        int howMany = readInt("How many peers", 1, 20);
        std::vector<Peer> peers = engine.findPeers(target, howMany);
        std::cout << "\n" << C_BOLD << "Peers of " << target->name << C_RESET
                  << "  (GDP/cap " << fmt(target->gdp, 0) << ", inflation " << fmt(target->inflation, 2) << ")\n";
        if (peers.empty()) { std::cout << "  No country has GDP within +-25% and inflation within +-2 points.\n"; return; }
        printHeader();
        int rank = 0;
        for (const Peer& p : peers) printRow(++rank, p.country, INFLATION);
        std::cout << "  Peers = GDP per capita within +-25% and inflation within +-2 points; closest first.\n";
    }
};

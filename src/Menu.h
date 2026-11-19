// Menu.h
// ---------------------------------------------------------------------------
// WHAT   Everything the user SEES and TYPES: the menu, checking what was typed,
//        and printing the coloured tables.
//        All the real work (searching, ranking, warnings) is done by
//        QueryEngine.  The Menu only asks it and prints the answer.
//
// The file has four parts:
//   1. colours
//   2. helpers for reading input
//   3. helpers for printing
//   4. the five features (one function each)
// ---------------------------------------------------------------------------
#pragma once
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include "QueryEngine.h"


// ---------------------------------------------------------------------------
// 1. COLOURS
// A terminal changes colour when it receives these special texts ("ANSI escape
// codes").  Print a colour code, then the text, then C_RESET to go back to normal.
// They work on Linux, macOS and Windows 10 or newer.
// ---------------------------------------------------------------------------
const char* const C_GREEN  = "\033[32m";
const char* const C_YELLOW = "\033[33m";
const char* const C_RED    = "\033[31m";
const char* const C_GREY   = "\033[90m";   // the 4th colour: "no data"
const char* const C_BOLD   = "\033[1m";
const char* const C_RESET  = "\033[0m";

// The longest table we print.  A longer list is cut off with "... and N more".
const int MAX_ROWS = 50;


class Menu {
public:
    Menu(QueryEngine& e) : engine(e) {
        // Start with the standard green/yellow/red lines for every indicator.
        for (int ind = 0; ind < NUM_INDICATORS; ind++) {
            thr[ind] = defaultThreshold(ind);
        }
    }

    // Show the menu again and again until the user chooses 0.
    void run() {
        while (true) {
            std::cout << "\n" << C_BOLD << "=========== ECONOMIC INDICATOR EXPLORER ===========" << C_RESET << "\n";
            std::cout << " 1  Country lookup\n";
            std::cout << " 2  Range filter\n";
            std::cout << " 3  Top-k ranking\n";
            std::cout << " 4  Early warning dashboard\n";
            std::cout << " 5  Peer finder\n";
            std::cout << " 0  Exit\n";

            int choice = readInt("Your choice", 0, 5);   // only 0 to 5 is accepted

            if (choice == 0) {
                std::cout << "Goodbye.\n";
                return;
            }
            if (choice == 1) lookup();
            if (choice == 2) rangeFilter();
            if (choice == 3) topK();
            if (choice == 4) earlyWarning();
            if (choice == 5) peerFinder();
        }
    }

private:
    QueryEngine& engine;                 // does all the real work
    Threshold    thr[NUM_INDICATORS];    // the current yellow/red lines (the user may change them)

    // =======================================================================
    // 2. READING INPUT
    // Each helper keeps asking until the user types something valid, so wrong
    // input can never crash the program.
    // =======================================================================

    // Show a question and read one line of text.
    std::string readLine(const std::string& question) {
        std::cout << question << ": ";

        std::string line;
        if (!std::getline(std::cin, line)) {
            // There is no more input (for example the user pressed Ctrl-D): stop.
            std::cout << "\n";
            std::exit(0);
        }

        // Remove spaces (and the invisible '\r' of Windows) from the end ...
        while (!line.empty() && (line.back() == ' ' || line.back() == '\r')) {
            line.pop_back();
        }
        // ... and spaces from the start.
        int start = 0;
        while (start < (int)line.size() && line[start] == ' ') {
            start++;
        }
        return line.substr(start);
    }

    // Read a whole number between low and high.
    int readInt(const std::string& question, int low, int high) {
        while (true) {
            std::string text = readLine(question + " [" + std::to_string(low) + "-" + std::to_string(high) + "]");

            try {
                size_t used = 0;
                int number = std::stoi(text, &used);     // stoi = "string to int"
                // Accept only if the WHOLE text was a number and it is in range.
                if (used == text.size() && number >= low && number <= high) {
                    return number;
                }
            } catch (...) {
                // The text was not a number at all: fall through to the message below.
            }
            std::cout << "  Please type a whole number from " << low << " to " << high << ".\n";
        }
    }

    // Read any number, e.g. 5.5 or -3.
    double readDouble(const std::string& question) {
        while (true) {
            std::string text = readLine(question);

            try {
                size_t used = 0;
                double number = std::stod(text, &used);  // stod = "string to double"
                if (used == text.size()) {
                    return number;
                }
            } catch (...) {
                // not a number: fall through to the message below
            }
            std::cout << "  Please type a number (for example 5.5).\n";
        }
    }

    // Show the list of indicators and let the user pick one.
    // "first" is the first number offered: 1 shows all five, 2 hides GDP per
    // capita (the early warning has no colour for GDP).
    // Returns the indicator number 0 to 4.
    int pickIndicator(int first) {
        for (int i = first - 1; i < NUM_INDICATORS; i++) {
            std::cout << "  " << (i + 1) << "  " << INDICATOR_NAME[i] << "\n";
        }
        int number = readInt("Indicator", first, NUM_INDICATORS);
        return number - 1;
    }

    // Ask for a country name and find it with the hash table.  If there is no
    // such country, suggest similar names ("Somalia" -> "Somalia, Fed. Rep.").
    // Returns nullptr when nothing was found.
    Country* askForCountry() {
        std::string name = readLine("Country name");

        Country* country = engine.lookup(name);          // the hash table finds it
        if (country != nullptr) {
            return country;
        }

        std::cout << "  '" << name << "' not found.";
        std::vector<Country*> similar = engine.namesContaining(name);
        if (!similar.empty() && !name.empty()) {
            std::cout << " Did you mean:";
            for (int i = 0; i < (int)similar.size() && i < 5; i++) {
                if (i > 0) std::cout << " |";
                std::cout << " " << similar[i]->name;
            }
        }
        std::cout << "\n";
        return nullptr;
    }

    // =======================================================================
    // 3. PRINTING
    // =======================================================================

    // The colour code that belongs to a status.
    const char* colourOf(Status s) {
        if (s == GREEN)  return C_GREEN;
        if (s == YELLOW) return C_YELLOW;
        if (s == RED)    return C_RED;
        return C_GREY;
    }

    // The WORD that belongs to a status.  We always print the word as well as
    // the colour, because some people cannot tell red from green and some
    // terminals show no colour.
    const char* wordOf(Status s) {
        if (s == GREEN)  return "GREEN";
        if (s == YELLOW) return "YELLOW";
        if (s == RED)    return "RED";
        return "NO DATA";
    }

    // Add spaces AFTER the text until it is "width" characters wide.
    // A letter like the "o" in "Cote d'Ivoire" may use 2 bytes in the file, so
    // we count characters, not bytes.  (Bytes 10xxxxxx are the 2nd, 3rd ...
    // byte of a letter; we do not count them.)
    std::string padRight(const std::string& text, int width) {
        int characters = 0;
        for (int i = 0; i < (int)text.size(); i++) {
            unsigned char byte = text[i];
            if ((byte & 0xC0) != 0x80) {
                characters++;
            }
        }
        if (characters >= width) return text;
        return text + std::string(width - characters, ' ');
    }

    // Add spaces BEFORE the text (used for numbers, which line up on the right).
    std::string padLeft(const std::string& text, int width) {
        if ((int)text.size() >= width) return text;
        return std::string(width - text.size(), ' ') + text;
    }

    // Turn a number into text with a fixed number of decimals.
    // A missing value is shown as "-".
    std::string formatNumber(double value, int decimals) {
        if (value == MISSING) return "-";
        std::ostringstream out;
        out << std::fixed << std::setprecision(decimals) << value;
        return out.str();
    }

    // How many decimals to show: GDP per capita is a big number, so none.
    int decimalsFor(int ind) {
        if (ind == GDP) return 0;
        return 2;
    }

    // The heading line of a table of countries.
    void printHeader() {
        std::cout << C_BOLD << padRight("  #", 6) << padRight("Country", 30);
        for (int i = 0; i < NUM_INDICATORS; i++) {
            std::cout << padLeft(INDICATOR_SHORT[i], 10);
        }
        std::cout << C_RESET << "\n";
        std::cout << std::string(86, '-') << "\n";
    }

    // One row of the table.  The column the user asked about ("highlight") is
    // printed in green, yellow or red.
    void printRow(int rank, const Country* c, int highlight) {
        std::cout << padRight("  " + std::to_string(rank), 6) << padRight(c->name, 30);

        for (int i = 0; i < NUM_INDICATORS; i++) {
            std::string cell = padLeft(formatNumber(c->get(i), decimalsFor(i)), 10);

            if (i == highlight && thr[i].used) {
                Status s = statusOf(c->get(i), thr[i]);
                std::cout << colourOf(s) << cell << C_RESET;
            } else {
                std::cout << cell;
            }
        }
        std::cout << "\n";
    }

    // Print a whole list of countries as a table (at most MAX_ROWS rows).
    void printList(const std::vector<Country*>& list, int highlight) {
        if (list.empty()) {
            std::cout << "  (no countries found)\n";
            return;
        }

        printHeader();
        for (int i = 0; i < (int)list.size() && i < MAX_ROWS; i++) {
            printRow(i + 1, list[i], highlight);
        }

        if ((int)list.size() > MAX_ROWS) {
            std::cout << "  ... and " << (list.size() - MAX_ROWS) << " more (showing the first " << MAX_ROWS << ").\n";
        }
        std::cout << "  Found: " << list.size() << " countries.\n";
    }

    // =======================================================================
    // 4. THE FIVE FEATURES
    // =======================================================================

    // ---- Feature 1: country lookup (uses the hash table) ----
    void lookup() {
        Country* c = askForCountry();
        if (c == nullptr) return;

        std::cout << "\n" << C_BOLD << c->name << " (" << c->iso << ")" << C_RESET << "\n";

        for (int i = 0; i < NUM_INDICATORS; i++) {
            std::cout << "  " << padRight(INDICATOR_NAME[i], 24)
                      << padLeft(formatNumber(c->get(i), decimalsFor(i)), 10);

            if (thr[i].used) {
                // This indicator has colours: show the status as a word in colour.
                Status s = statusOf(c->get(i), thr[i]);
                std::cout << "   " << colourOf(s) << "[" << wordOf(s) << "]" << C_RESET;
            } else if (!c->has(i)) {
                // GDP per capita has no colour, but we still say when it is missing.
                std::cout << "   " << C_GREY << "[NO DATA]" << C_RESET;
            }
            std::cout << "\n";
        }
    }

    // ---- Feature 2: range filter (uses an AVL tree) ----
    void rangeFilter() {
        int ind = pickIndicator(1);
        double low  = readDouble("Lowest value");
        double high = readDouble("Highest value");

        if (low > high) {                                 // the user typed them the wrong way round
            double temp = low;
            low = high;
            high = temp;
            std::cout << "  (swapped so that low <= high)\n";
        }

        printList(engine.rangeFilter(ind, low, high), ind);
    }

    // ---- Feature 3: top-k ranking (uses an AVL tree) ----
    void topK() {
        int ind = pickIndicator(1);
        int k = readInt("How many countries (k)", 1, 100);
        int direction = readInt("1 = highest values, 2 = lowest values", 1, 2);

        bool highest = (direction == 1);
        printList(engine.topK(ind, k, highest), ind);
    }

    // ---- Feature 4: early warning dashboard (uses AVL trees) ----
    void earlyWarning() {
        std::cout << "  Choose indicator (or 6 for the combined check):\n";
        for (int i = GROWTH; i < NUM_INDICATORS; i++) {
            std::cout << "  " << (i + 1) << "  " << INDICATOR_NAME[i] << "\n";
        }
        std::cout << "  6  Combined: countries RED on two or more indicators\n";

        int choice = readInt("Choice", 2, 6);
        if (choice == 6) {
            combinedWarning();
            return;
        }
        int ind = choice - 1;

        // Show the current lines and let the user change them.
        showThresholds(ind);
        if (readLine("Press Enter to keep these limits, or type c to change them") == "c") {
            thr[ind].yellow = readDouble("  Yellow starts at");
            thr[ind].red    = readDouble("  Red starts at");

            // Sense check.  Where a high value is bad, red must be the HIGHER
            // line.  Where a low value is bad (growth), red must be the LOWER line.
            bool sensible;
            if (thr[ind].higherIsWorse) {
                sensible = thr[ind].red >= thr[ind].yellow;
            } else {
                sensible = thr[ind].red <= thr[ind].yellow;
            }
            if (!sensible) {
                thr[ind] = defaultThreshold(ind);
                std::cout << "  Red must be the more serious line. Defaults restored.\n";
            }
        }

        // Ask the engine and print the answer.
        Warning w = engine.earlyWarning(ind, thr[ind]);

        std::cout << "\n" << C_BOLD << "EARLY WARNING: " << INDICATOR_NAME[ind] << C_RESET << "\n";

        std::cout << C_RED << "  RED    : " << w.red.size() << " countries" << C_RESET << "\n";
        printList(w.red, ind);

        std::cout << C_YELLOW << "  YELLOW : " << w.yellow.size() << " countries" << C_RESET << "\n";
        printList(w.yellow, ind);

        // Green countries are only counted; a list of 100 "fine" countries would hide the important ones.
        std::cout << C_GREEN << "  GREEN  : " << w.green << " countries" << C_RESET << " (not listed)\n";
        std::cout << C_GREY  << "  NO DATA: " << w.noData << " countries" << C_RESET << "\n";
        std::cout << "  A flag means 'look closer', not 'a crisis will happen'.\n";
    }

    // Print the current yellow/red lines of one indicator.
    void showThresholds(int ind) {
        const Threshold& t = thr[ind];
        std::cout << "  Current limits for " << INDICATOR_NAME[ind] << ":\n";

        if (t.higherIsWorse) {
            std::cout << "    " << C_YELLOW << "YELLOW" << C_RESET << " from " << t.yellow
                      << "   " << C_RED << "RED" << C_RESET << " from " << t.red
                      << "   (higher is worse)\n";
        } else {
            std::cout << "    " << C_YELLOW << "YELLOW" << C_RESET << " below " << t.yellow
                      << "   " << C_RED << "RED" << C_RESET << " below " << t.red
                      << "   (lower is worse)\n";
        }
    }

    // Countries that are RED on two or more indicators.
    void combinedWarning() {
        std::vector<MultiRed> list = engine.redOnSeveral(thr);

        std::cout << "\n" << C_BOLD << "COMBINED WARNING: RED on two or more of growth, inflation, unemployment, debt" << C_RESET << "\n";
        if (list.empty()) {
            std::cout << "  (none)\n";
            return;
        }

        std::cout << C_BOLD << padRight("  #", 6) << padRight("Country", 30) << "Red flags" << C_RESET << "\n";
        for (int i = 0; i < (int)list.size() && i < MAX_ROWS; i++) {
            std::cout << padRight("  " + std::to_string(i + 1), 6) << padRight(list[i].country->name, 30)
                      << C_RED << list[i].redCount << C_RESET << "\n";
        }

        if ((int)list.size() > MAX_ROWS) {
            std::cout << "  ... and " << (list.size() - MAX_ROWS) << " more.\n";
        }
        std::cout << "  Found: " << list.size() << " countries.  (Debt data exists for only some countries.)\n";
    }

    // ---- Feature 5: peer finder (hash table + AVL tree) ----
    void peerFinder() {
        Country* target = askForCountry();               // the hash table finds the country first
        if (target == nullptr) return;

        if (!target->has(GDP) || !target->has(INFLATION)) {
            std::cout << "  " << target->name << " has no GDP or inflation value, so peers cannot be found.\n";
            return;
        }

        int howMany = readInt("How many peers", 1, 20);
        std::vector<Peer> peers = engine.findPeers(target, howMany);

        std::cout << "\n" << C_BOLD << "Peers of " << target->name << C_RESET
                  << "  (GDP/cap " << formatNumber(target->gdp, 0)
                  << ", inflation " << formatNumber(target->inflation, 2) << ")\n";

        if (peers.empty()) {
            std::cout << "  No country has GDP within +-25% and inflation within +-2 points.\n";
            return;
        }

        printHeader();
        for (int i = 0; i < (int)peers.size(); i++) {
            printRow(i + 1, peers[i].country, INFLATION);
        }
        std::cout << "  Peers = GDP per capita within +-25% and inflation within +-2 points; closest first.\n";
    }
};

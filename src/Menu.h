// Menu.h
// ---------------------------------------------------------------------------
// WHAT   Everything the user SEES and TYPES: the menu and the tables.
//        All the real work (searching, ranking, warnings) is done by
//        QueryEngine.  The Menu only asks it and prints the answer.
//
// The file has four parts:
//   1. colours
//   2. asking the user for input
//   3. printing tables
//   4. the five features (one function each)
// ---------------------------------------------------------------------------
#pragma once
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>
#include "QueryEngine.h"


// ---------------------------------------------------------------------------
// 1. COLOURS
// A terminal changes colour when it receives these special texts.
// Print a colour code, then the text, then C_RESET to go back to normal.
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
            cout << "\n" << C_BOLD << "=========== ECONOMIC INDICATOR EXPLORER ===========" << C_RESET << "\n";
            cout << " 1  Country lookup\n";
            cout << " 2  Range filter\n";
            cout << " 3  Top-k ranking\n";
            cout << " 4  Early warning dashboard\n";
            cout << " 5  Peer finder\n";
            cout << " 0  Exit\n";

            int choice = askInt("Your choice", 0, 5);

            if (choice == 0) {
                cout << "Goodbye.\n";
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
    // 2. ASKING THE USER
    // Each helper keeps asking until the user types something valid, so wrong
    // input never crashes the program.
    // How cin works: "cin >> x" reads a value into x.  If the user types
    // letters where a number is expected, cin goes into an error state
    // (cin.fail() is true).  We then clear the error and throw the line away.
    // =======================================================================

    // Ask for a whole number from low to high.
    int askInt(string question, int low, int high) {
        int number;
        while (true) {
            cout << question << " [" << low << "-" << high << "]: ";
            cin >> number;

            if (cin.fail()) {
                if (cin.eof()) exit(0);       // there is no more input: stop the program
                cin.clear();                  // clear the error state
                cin.ignore(1000, '\n');       // throw away what was typed
            } else {
                cin.ignore(1000, '\n');       // throw away the rest of the line
                if (number >= low && number <= high) {
                    return number;
                }
            }
            cout << "  Please type a whole number from " << low << " to " << high << ".\n";
        }
    }

    // Ask for any number, such as 5.5 or -3.
    double askNumber(string question) {
        double number;
        while (true) {
            cout << question << ": ";
            cin >> number;

            if (cin.fail()) {
                if (cin.eof()) exit(0);
                cin.clear();
                cin.ignore(1000, '\n');
                cout << "  Please type a number (for example 5.5).\n";
            } else {
                cin.ignore(1000, '\n');
                return number;
            }
        }
    }

    // Ask for a line of text (a country name can contain spaces).
    string askText(string question) {
        cout << question << ": ";
        string text;
        getline(cin, text);
        if (cin.fail()) exit(0);              // there is no more input: stop the program
        return text;
    }

    // Show the list of indicators and let the user pick one.
    // "first" is the first number offered: 1 shows all five, 2 hides GDP per
    // capita (the early warning has no colour for GDP).
    // Returns the indicator number 0 to 4.
    int pickIndicator(int first) {
        for (int i = first - 1; i < NUM_INDICATORS; i++) {
            cout << "  " << (i + 1) << "  " << INDICATOR_NAME[i] << "\n";
        }
        int number = askInt("Indicator", first, NUM_INDICATORS);
        return number - 1;
    }

    // Ask for a country name and find it with the hash table.  If there is no
    // such country, suggest similar names ("Somalia" -> "Somalia, Fed. Rep.").
    // Returns nullptr when nothing was found.
    Country* askForCountry() {
        string name = askText("Country name");

        Country* country = engine.lookup(name);          // the hash table finds it
        if (country != nullptr) {
            return country;
        }

        cout << "  '" << name << "' not found.";
        vector<Country*> similar = engine.namesContaining(name);
        if (!similar.empty() && !name.empty()) {
            cout << " Did you mean:";
            for (int i = 0; i < (int)similar.size() && i < 5; i++) {
                if (i > 0) cout << " |";
                cout << " " << similar[i]->name;
            }
        }
        cout << "\n";
        return nullptr;
    }

    // =======================================================================
    // 3. PRINTING
    // How columns work: "setw(10)" makes the NEXT thing printed take up 10
    // characters.  "right" puts the extra spaces before it, "left" after it.
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

    // Print one number, 10 characters wide, in the given colour.
    // A missing value is printed as "-".  GDP per capita is a big number, so it
    // gets no decimals; the other indicators get 2.
    void printNumber(double value, int ind, const char* colour) {
        int decimals = 2;
        if (ind == GDP) decimals = 0;

        cout << colour << right << setw(10);
        if (value == MISSING) {
            cout << "-";
        } else {
            cout << fixed << setprecision(decimals) << value;
        }
        cout << C_RESET;
        normalNumbers();
    }

    // "fixed" and "setprecision" stay switched on until we switch them off.
    // This puts numbers back to normal (6 prints as 6, not 6.00).
    void normalNumbers() {
        cout.unsetf(ios::fixed);
        cout.precision(6);
    }

    // The heading line of a table of countries.
    void printHeader() {
        cout << C_BOLD << left << setw(6) << "  #" << setw(30) << "Country" << right;
        for (int i = 0; i < NUM_INDICATORS; i++) {
            cout << setw(10) << INDICATOR_SHORT[i];
        }
        cout << C_RESET << "\n";
        cout << string(86, '-') << "\n";
    }

    // One row of the table.  The column the user asked about ("highlight") is
    // printed in green, yellow or red.
    void printRow(int rank, const Country* c, int highlight) {
        string rankText = "  " + to_string(rank);
        cout << left << setw(6) << rankText << setw(30) << c->name;

        for (int i = 0; i < NUM_INDICATORS; i++) {
            const char* colour = "";                      // no colour
            if (i == highlight && thr[i].used) {
                colour = colourOf(statusOf(c->get(i), thr[i]));
            }
            printNumber(c->get(i), i, colour);
        }
        cout << "\n";
    }

    // Print a whole list of countries as a table (at most MAX_ROWS rows).
    void printList(const vector<Country*>& list, int highlight) {
        if (list.empty()) {
            cout << "  (no countries found)\n";
            return;
        }

        printHeader();
        for (int i = 0; i < (int)list.size() && i < MAX_ROWS; i++) {
            printRow(i + 1, list[i], highlight);
        }

        if ((int)list.size() > MAX_ROWS) {
            cout << "  ... and " << (list.size() - MAX_ROWS) << " more (showing the first " << MAX_ROWS << ").\n";
        }
        cout << "  Found: " << list.size() << " countries.\n";
    }

    // =======================================================================
    // 4. THE FIVE FEATURES
    // =======================================================================

    // ---- Feature 1: country lookup (uses the hash table) ----
    void lookup() {
        Country* c = askForCountry();
        if (c == nullptr) return;

        cout << "\n" << C_BOLD << c->name << " (" << c->iso << ")" << C_RESET << "\n";

        for (int i = 0; i < NUM_INDICATORS; i++) {
            cout << "  " << left << setw(24) << INDICATOR_NAME[i];
            printNumber(c->get(i), i, "");

            if (thr[i].used) {
                // This indicator has colours: show the status as a word in colour.
                Status s = statusOf(c->get(i), thr[i]);
                cout << "   " << colourOf(s) << "[" << wordOf(s) << "]" << C_RESET;
            } else if (!c->has(i)) {
                // GDP per capita has no colour, but we still say when it is missing.
                cout << "   " << C_GREY << "[NO DATA]" << C_RESET;
            }
            cout << "\n";
        }
    }

    // ---- Feature 2: range filter (uses an AVL tree) ----
    void rangeFilter() {
        int ind = pickIndicator(1);
        double low  = askNumber("Lowest value");
        double high = askNumber("Highest value");

        if (low > high) {                                 // typed the wrong way round: swap them
            double temp = low;
            low = high;
            high = temp;
            cout << "  (swapped so that low <= high)\n";
        }

        printList(engine.rangeFilter(ind, low, high), ind);
    }

    // ---- Feature 3: top-k ranking (uses an AVL tree) ----
    void topK() {
        int ind = pickIndicator(1);
        int k = askInt("How many countries (k)", 1, 100);
        int direction = askInt("1 = highest values, 2 = lowest values", 1, 2);

        bool highest = (direction == 1);
        printList(engine.topK(ind, k, highest), ind);
    }

    // ---- Feature 4: early warning dashboard (uses AVL trees) ----
    void earlyWarning() {
        cout << "  Choose indicator (or 6 for the combined check):\n";
        for (int i = GROWTH; i < NUM_INDICATORS; i++) {
            cout << "  " << (i + 1) << "  " << INDICATOR_NAME[i] << "\n";
        }
        cout << "  6  Combined: countries RED on two or more indicators\n";

        int choice = askInt("Choice", 2, 6);
        if (choice == 6) {
            combinedWarning();
            return;
        }
        int ind = choice - 1;

        // Show the current lines and let the user change them.
        showThresholds(ind);
        if (askText("Press Enter to keep these limits, or type c to change them") == "c") {
            thr[ind].yellow = askNumber("  Yellow starts at");
            thr[ind].red    = askNumber("  Red starts at");

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
                cout << "  Red must be the more serious line. Defaults restored.\n";
            }
        }

        // Ask the engine and print the answer.
        Warning w = engine.earlyWarning(ind, thr[ind]);

        cout << "\n" << C_BOLD << "EARLY WARNING: " << INDICATOR_NAME[ind] << C_RESET << "\n";

        cout << C_RED << "  RED    : " << w.red.size() << " countries" << C_RESET << "\n";
        printList(w.red, ind);

        cout << C_YELLOW << "  YELLOW : " << w.yellow.size() << " countries" << C_RESET << "\n";
        printList(w.yellow, ind);

        // Green countries are only counted; a list of 100 "fine" countries would hide the important ones.
        cout << C_GREEN << "  GREEN  : " << w.green << " countries" << C_RESET << " (not listed)\n";
        cout << C_GREY  << "  NO DATA: " << w.noData << " countries" << C_RESET << "\n";
        cout << "  A flag means 'look closer', not 'a crisis will happen'.\n";
    }

    // Print the current yellow/red lines of one indicator.
    void showThresholds(int ind) {
        const Threshold& t = thr[ind];
        cout << "  Current limits for " << INDICATOR_NAME[ind] << ":\n";

        if (t.higherIsWorse) {
            cout << "    " << C_YELLOW << "YELLOW" << C_RESET << " from " << t.yellow
                 << "   " << C_RED << "RED" << C_RESET << " from " << t.red
                 << "   (higher is worse)\n";
        } else {
            cout << "    " << C_YELLOW << "YELLOW" << C_RESET << " below " << t.yellow
                 << "   " << C_RED << "RED" << C_RESET << " below " << t.red
                 << "   (lower is worse)\n";
        }
    }

    // Countries that are RED on two or more indicators.
    void combinedWarning() {
        vector<MultiRed> list = engine.redOnSeveral(thr);

        cout << "\n" << C_BOLD << "COMBINED WARNING: RED on two or more of growth, inflation, unemployment, debt" << C_RESET << "\n";
        if (list.empty()) {
            cout << "  (none)\n";
            return;
        }

        cout << C_BOLD << left << setw(6) << "  #" << setw(30) << "Country" << "Red flags" << C_RESET << "\n";
        for (int i = 0; i < (int)list.size() && i < MAX_ROWS; i++) {
            string rankText = "  " + to_string(i + 1);
            cout << left << setw(6) << rankText << setw(30) << list[i].country->name
                 << C_RED << list[i].redCount << C_RESET << "\n";
        }

        if ((int)list.size() > MAX_ROWS) {
            cout << "  ... and " << (list.size() - MAX_ROWS) << " more.\n";
        }
        cout << "  Found: " << list.size() << " countries.  (Debt data exists for only some countries.)\n";
    }

    // ---- Feature 5: peer finder (hash table + AVL tree) ----
    void peerFinder() {
        Country* target = askForCountry();               // the hash table finds the country first
        if (target == nullptr) return;

        if (!target->has(GDP) || !target->has(INFLATION)) {
            cout << "  " << target->name << " has no GDP or inflation value, so peers cannot be found.\n";
            return;
        }

        int howMany = askInt("How many peers", 1, 20);
        vector<Peer> peers = engine.findPeers(target, howMany);

        cout << "\n" << C_BOLD << "Peers of " << target->name << C_RESET
             << "  (GDP/cap " << fixed << setprecision(0) << target->gdp
             << ", inflation " << setprecision(2) << target->inflation << ")\n";
        normalNumbers();

        if (peers.empty()) {
            cout << "  No country has GDP within +-25% and inflation within +-2 points.\n";
            return;
        }

        printHeader();
        for (int i = 0; i < (int)peers.size(); i++) {
            printRow(i + 1, peers[i].country, INFLATION);
        }
        cout << "  Peers = GDP per capita within +-25% and inflation within +-2 points; closest first.\n";
    }
};

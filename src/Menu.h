// Menu.h
// ---------------------------------------------------------------------------
// WHAT   Everything the user SEES and TYPES: the menu and the tables.
//        All the real work (searching, ranking, warnings) is done by
//        QueryEngine.  The Menu only asks it and prints the answer.
//
// The file has four parts:
//   1. asking the user for input
//   2. printing tables
//   3. the three features (one function each)
// ---------------------------------------------------------------------------
#pragma once
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>
#include "QueryEngine.h"


// The longest table we print.  A longer list is cut off with "... and N more".
const int MAX_ROWS = 50;


class Menu {
public:
    Menu(QueryEngine& e) : engine(e) {
        // Set the green/yellow/red lines for every indicator (they stay fixed).
        for (int ind = 0; ind < NUM_INDICATORS; ind++) {
            thr[ind] = defaultThreshold(ind);
        }
    }

    // Show the menu again and again until the user chooses 0.
    void run() {
        while (true) {
            cout << "\n=========== ECONOMIC INDICATOR EXPLORER ===========\n";
            cout << " 1  Country lookup\n";
            cout << " 2  Range filter\n";
            cout << " 3  Early warning dashboard\n";
            cout << " 0  Exit\n";

            int choice = askInt("Your choice", 0, 3);

            if (choice == 0) {
                cout << "Goodbye.\n";
                return;
            }
            if (choice == 1) lookup();
            if (choice == 2) rangeFilter();
            if (choice == 3) earlyWarning();
        }
    }

private:
    QueryEngine& engine;                 // does all the real work
    Threshold    thr[NUM_INDICATORS];    // the yellow/red lines of each indicator

    // =======================================================================
    // 1. ASKING THE USER
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
    // capita (the early warning has no warning lines for GDP).
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
    // 2. PRINTING
    // How columns work: "setw(10)" makes the NEXT thing printed take up 10
    // characters.  "right" puts the extra spaces before it, "left" after it.
    // =======================================================================

    // The WORD that belongs to a status.  The program prints words only (no
    // colours), so it looks the same on every computer and terminal.
    const char* wordOf(Status s) {
        if (s == GREEN)  return "GREEN";
        if (s == YELLOW) return "YELLOW";
        if (s == RED)    return "RED";
        return "NO DATA";
    }

    // Print one number, 10 characters wide.
    // A missing value is printed as "-".  GDP per capita is a big number, so it
    // gets no decimals; the other indicators get 2.
    void printNumber(double value, int ind) {
        int decimals = 2;
        if (ind == GDP) decimals = 0;

        cout << right << setw(10);
        if (value == MISSING) {
            cout << "-";
        } else {
            cout << fixed << setprecision(decimals) << value;
        }
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
        cout << left << setw(6) << "  #" << setw(30) << "Country" << right;
        for (int i = 0; i < NUM_INDICATORS; i++) {
            cout << setw(10) << INDICATOR_SHORT[i];
        }
        cout << "\n";
        cout << string(86, '-') << "\n";
    }

    // One row of the table: rank, country name, the five numbers.
    void printRow(int rank, const Country* c) {
        string rankText = "  " + to_string(rank);
        cout << left << setw(6) << rankText << setw(30) << c->name;

        for (int i = 0; i < NUM_INDICATORS; i++) {
            printNumber(c->get(i), i);
        }
        cout << "\n";
    }

    // Print a whole list of countries as a table (at most MAX_ROWS rows).
    void printList(const vector<Country*>& list) {
        if (list.empty()) {
            cout << "  (no countries found)\n";
            return;
        }

        printHeader();
        for (int i = 0; i < (int)list.size() && i < MAX_ROWS; i++) {
            printRow(i + 1, list[i]);
        }

        if ((int)list.size() > MAX_ROWS) {
            cout << "  ... and " << (list.size() - MAX_ROWS) << " more (showing the first " << MAX_ROWS << ").\n";
        }
        cout << "  Found: " << list.size() << " countries.\n";
    }

    // =======================================================================
    // 3. THE THREE FEATURES
    // =======================================================================

    // ---- Feature 1: country lookup (uses the hash table) ----
    void lookup() {
        Country* c = askForCountry();
        if (c == nullptr) return;

        cout << "\n" << c->name << " (" << c->iso << ")\n";

        for (int i = 0; i < NUM_INDICATORS; i++) {
            cout << "  " << left << setw(24) << INDICATOR_NAME[i];
            printNumber(c->get(i), i);

            if (thr[i].used) {
                // This indicator has warning lines: show the status as a word.
                Status s = statusOf(c->get(i), thr[i]);
                cout << "   [" << wordOf(s) << "]";
            } else if (!c->has(i)) {
                // GDP per capita has no warning lines, but we still say when it is missing.
                cout << "   [NO DATA]";
            }
            cout << "\n";
        }
    }

    // ---- Feature 2: range filter (uses a BST) ----
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

        printList(engine.rangeFilter(ind, low, high));
    }

    // ---- Feature 3: early warning dashboard (uses BSTs) ----
    void earlyWarning() {
        int ind = pickIndicator(2);              // 2 = start at growth (GDP per capita has no warning lines)

        showThresholds(ind);                     // tell the user which lines are used

        // Ask the engine and print the answer.
        Warning w = engine.earlyWarning(ind, thr[ind]);

        cout << "\nEARLY WARNING: " << INDICATOR_NAME[ind] << "\n";

        cout << "  RED    : " << w.red.size() << " countries\n";
        printList(w.red);

        cout << "  YELLOW : " << w.yellow.size() << " countries\n";
        printList(w.yellow);

        // Green countries are only counted; a list of 100 "fine" countries would hide the important ones.
        cout << "  GREEN  : " << w.green << " countries (not listed)\n";
        cout << "  NO DATA: " << w.noData << " countries\n";
        cout << "  A flag means 'look closer', not 'a crisis will happen'.\n";
    }

    // Print the yellow/red lines of one indicator.
    void showThresholds(int ind) {
        const Threshold& t = thr[ind];
        cout << "  Current limits for " << INDICATOR_NAME[ind] << ":\n";

        if (t.higherIsWorse) {
            cout << "    YELLOW from " << t.yellow
                 << "   RED from " << t.red
                 << "   (higher is worse)\n";
        } else {
            cout << "    YELLOW below " << t.yellow
                 << "   RED below " << t.red
                 << "   (lower is worse)\n";
        }
    }
};

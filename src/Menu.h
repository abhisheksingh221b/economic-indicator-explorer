#pragma once
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>
#include "QueryEngine.h"

const int MAX_ROWS = 50;

class Menu {
public:
    Menu(QueryEngine& e) : engine(e) {
        for (int ind = 0; ind < NUM_INDICATORS; ind++) {
            thr[ind] = defaultThreshold(ind);
        }
    }

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
    QueryEngine& engine;
    Threshold    thr[NUM_INDICATORS];

    int askInt(string question, int low, int high) {
        int number;
        while (true) {
            cout << question << " [" << low << "-" << high << "]: ";
            cin >> number;

            if (cin.fail()) {
                if (cin.eof()) exit(0);
                cin.clear();
                cin.ignore(1000, '\n');
            } else {
                cin.ignore(1000, '\n');
                if (number >= low && number <= high) {
                    return number;
                }
            }
            cout << "  Please type a whole number from " << low << " to " << high << ".\n";
        }
    }

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

    string askText(string question) {
        cout << question << ": ";
        string text;
        getline(cin, text);
        if (cin.fail()) exit(0);
        return text;
    }

    int pickIndicator(int first) {
        for (int i = first - 1; i < NUM_INDICATORS; i++) {
            cout << "  " << (i + 1) << "  " << INDICATOR_NAME[i] << "\n";
        }
        int number = askInt("Indicator", first, NUM_INDICATORS);
        return number - 1;
    }

    Country* askForCountry() {
        string name = askText("Country name");

        Country* country = engine.lookup(name);
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

    const char* wordOf(Status s) {
        if (s == GREEN)  return "GREEN";
        if (s == YELLOW) return "YELLOW";
        if (s == RED)    return "RED";
        return "NO DATA";
    }

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

    void normalNumbers() {
        cout.unsetf(ios::fixed);
        cout.precision(6);
    }

    void printHeader() {
        cout << left << setw(6) << "  #" << setw(30) << "Country" << right;
        for (int i = 0; i < NUM_INDICATORS; i++) {
            cout << setw(10) << INDICATOR_SHORT[i];
        }
        cout << "\n";
        cout << string(86, '-') << "\n";
    }

    void printRow(int rank, const Country* c) {
        string rankText = "  " + to_string(rank);
        cout << left << setw(6) << rankText << setw(30) << c->name;

        for (int i = 0; i < NUM_INDICATORS; i++) {
            printNumber(c->get(i), i);
        }
        cout << "\n";
    }

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

    void lookup() {
        Country* c = askForCountry();
        if (c == nullptr) return;

        cout << "\n" << c->name << " (" << c->iso << ")\n";

        for (int i = 0; i < NUM_INDICATORS; i++) {
            cout << "  " << left << setw(24) << INDICATOR_NAME[i];
            printNumber(c->get(i), i);

            if (thr[i].used) {
                Status s = statusOf(c->get(i), thr[i]);
                cout << "   [" << wordOf(s) << "]";
            } else if (!c->has(i)) {
                cout << "   [NO DATA]";
            }
            cout << "\n";
        }
    }

    void rangeFilter() {
        int ind = pickIndicator(1);
        double low  = askNumber("Lowest value");
        double high = askNumber("Highest value");

        if (low > high) {
            double temp = low;
            low = high;
            high = temp;
            cout << "  (swapped so that low <= high)\n";
        }

        printList(engine.rangeFilter(ind, low, high));
    }

    void earlyWarning() {
        int ind = pickIndicator(2);

        showThresholds(ind);

        Warning w = engine.earlyWarning(ind, thr[ind]);

        cout << "\nEARLY WARNING: " << INDICATOR_NAME[ind] << "\n";

        cout << "  RED    : " << w.red.size() << " countries\n";
        printList(w.red);

        cout << "  YELLOW : " << w.yellow.size() << " countries\n";
        printList(w.yellow);

        cout << "  GREEN  : " << w.green << " countries (not listed)\n";
        cout << "  NO DATA: " << w.noData << " countries\n";
        cout << "  A flag means 'look closer', not 'a crisis will happen'.\n";
    }

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

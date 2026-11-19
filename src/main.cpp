// main.cpp
// ---------------------------------------------------------------------------
// WHAT   Where the program starts.  It does three things:
//          1. takes the built-in data (Data.h),
//          2. builds the hash table and the BSTs (QueryEngine),
//          3. shows the menu.
// RUN    g++ -std=c++17 -o explorer src/main.cpp      then      ./explorer
// There is no data file and nothing that depends on the operating system.
// ---------------------------------------------------------------------------
#include <iostream>
#include <string>
#include <vector>
#include "Data.h"
#include "Menu.h"


int main() {
    // STEP 1: the countries.
    // The vector must be completely filled BEFORE the QueryEngine is built.
    // The hash table and the trees remember the ADDRESS of every country, and a
    // vector that is still growing may move its contents to a new place, which
    // would make those addresses wrong.
    vector<Country> countries = builtInCountries();

    // STEP 2: build the hash table and the five trees.
    QueryEngine engine(countries);

    cout << "Loaded " << engine.totalCountries() << " countries (World Bank data, 2023)\n";
    cout << "Countries with data -> GDP: " << engine.countWithData(GDP)
         << ", growth: " << engine.countWithData(GROWTH)
         << ", inflation: " << engine.countWithData(INFLATION)
         << ", unemployment: " << engine.countWithData(UNEMPLOYMENT)
         << ", debt: " << engine.countWithData(DEBT) << "\n";

    // STEP 3: show the menu until the user exits.
    Menu menu(engine);
    menu.run();
    return 0;
}

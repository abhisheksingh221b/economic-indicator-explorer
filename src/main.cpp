// main.cpp
// ---------------------------------------------------------------------------
// WHAT   Where the program starts.  It does three things:
//          1. reads the data file,
//          2. builds the hash table and the AVL trees (QueryEngine),
//          3. shows the menu.
// RUN    ./explorer data/countries_2023.csv
// ---------------------------------------------------------------------------
#include <iostream>
#include <string>
#include <vector>
#include "Loader.h"
#include "Menu.h"

// This part is only compiled on Windows.  It switches the Windows console to a
// mode where accented letters and colours display correctly.
#ifdef _WIN32
#include <windows.h>
#endif


int main(int argc, char* argv[]) {
#ifdef _WIN32
    SetConsoleOutputCP(65001);                       // 65001 = UTF-8 (accented letters)
    HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    GetConsoleMode(console, &mode);
    SetConsoleMode(console, mode | 0x0004);          // 0x0004 = allow colour codes
#endif

    // Which data file?  The one typed after the program name, or a default.
    string path = "data/countries_2023.csv";
    if (argc > 1) {
        path = argv[1];
    }

    // STEP 1: read the file.
    // The vector must be completely filled BEFORE the QueryEngine is built.
    // The hash table and the trees remember the ADDRESS of every country, and a
    // vector that is still growing may move its contents to a new place, which
    // would make those addresses wrong.
    vector<Country> countries;
    if (!loadCountries(path, countries) || countries.empty()) {
        cout << "Cannot read data file: " << path << "\n";
        cout << "Usage: ./explorer data/countries_2023.csv\n";
        return 1;                                    // 1 means "something went wrong"
    }

    // STEP 2: build the hash table and the five trees.
    QueryEngine engine(countries);

    cout << "Loaded " << engine.totalCountries() << " countries from " << path << "\n";
    cout << "Data: World Bank WDI.  Countries with data -> GDP: " << engine.countWithData(GDP)
              << ", growth: " << engine.countWithData(GROWTH)
              << ", inflation: " << engine.countWithData(INFLATION)
              << ", unemployment: " << engine.countWithData(UNEMPLOYMENT)
              << ", debt: " << engine.countWithData(DEBT) << "\n";

    // STEP 3: show the menu until the user exits.
    Menu menu(engine);
    menu.run();
    return 0;
}

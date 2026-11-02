// main.cpp
// ---------------------------------------------------------------------------
// WHAT:  starts the program: reads the data file, builds the indexes, runs
//        the menu.
// Run:   ./explorer data/countries_2023.csv
// ---------------------------------------------------------------------------
#include <iostream>
#include <string>
#include <vector>
#include "Loader.h"
#include "Menu.h"

#ifdef _WIN32
#include <windows.h>
#endif

int main(int argc, char* argv[]) {
#ifdef _WIN32
    SetConsoleOutputCP(65001);                      // show accented letters correctly
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);     // switch on colour support in the Windows console
    DWORD mode = 0;
    GetConsoleMode(h, &mode);
    SetConsoleMode(h, mode | 0x0004);
#endif

    std::string path = (argc > 1) ? argv[1] : "data/countries_2023.csv";

    // The vector is filled COMPLETELY before the indexes are built.  The indexes
    // keep pointers into it, and a vector that is still growing may move its
    // contents and make those pointers invalid.
    std::vector<Country> countries;
    if (!loadCountries(path, countries) || countries.empty()) {
        std::cout << "Cannot read data file: " << path << "\n"
                  << "Usage: ./explorer data/countries_2023.csv\n";
        return 1;
    }

    QueryEngine engine(countries);                  // builds the hash table and the 5 AVL trees
    std::cout << "Loaded " << engine.totalCountries() << " countries from " << path << "\n"
              << "Data: World Bank WDI.  Countries with data -> GDP: " << engine.countWithData(GDP)
              << ", growth: " << engine.countWithData(GROWTH)
              << ", inflation: " << engine.countWithData(INFLATION)
              << ", unemployment: " << engine.countWithData(UNEMPLOYMENT)
              << ", debt: " << engine.countWithData(DEBT) << "\n";

    Menu menu(engine);
    menu.run();
    return 0;
}

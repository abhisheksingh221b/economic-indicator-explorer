
#include <iostream>
#include <string>
#include <vector>
#include "Data.h"
#include "Menu.h"


int main() {

    vector<Country> countries = builtInCountries();


    QueryEngine engine(countries);

    cout << "Loaded " << engine.totalCountries() << " countries (World Bank data, 2023)\n";
    cout << "Countries with data -> GDP: " << engine.countWithData(GDP)
         << ", growth: " << engine.countWithData(GROWTH)
         << ", inflation: " << engine.countWithData(INFLATION)
         << ", unemployment: " << engine.countWithData(UNEMPLOYMENT)
         << ", debt: " << engine.countWithData(DEBT) << "\n";


    Menu menu(engine);
    menu.run();
    return 0;
}

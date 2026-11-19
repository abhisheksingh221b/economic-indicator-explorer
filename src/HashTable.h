// HashTable.h
// ---------------------------------------------------------------------------
// WHAT   A hash table: finds a country by NAME almost instantly.
//
// THE IDEA
//   Imagine a cupboard with numbered drawers (called "buckets").
//   To store "India" we turn the name into a drawer number (that is the "hash
//   function"), then put the country in that drawer.
//   To find "India" later we turn the name into the SAME drawer number and open
//   only that one drawer.  We never search through all 217 countries.
//
// COLLISIONS
//   Two different names can give the same drawer number.  We simply let a drawer
//   hold a small list of countries ("separate chaining").  When we search, we
//   open the drawer and check the few names inside.
// ---------------------------------------------------------------------------
#pragma once
#include <string>
#include <vector>
#include "Country.h"


// Helper: make a copy of a word in lower case, so "India" and "INDIA" become "india".
inline std::string toLowerCase(std::string text) {
    for (int i = 0; i < (int)text.size(); i++) {
        char letter = text[i];
        if (letter >= 'A' && letter <= 'Z') {
            text[i] = letter + ('a' - 'A');          // move from capital to small letter
        }
    }
    return text;
}


class HashTable {
public:
    // Start with 101 drawers (101 is a prime number).  The table makes itself
    // bigger when it fills up, so we never have to guess the right size.
    HashTable() {
        buckets.resize(101);
        count = 0;
    }

    // Put one country into the table.
    void insert(Country* country) {
        // If the table is more than 75 % full, the drawers get crowded and
        // searching slows down.  So we first make the table bigger.
        if (count + 1 > 0.75 * buckets.size()) {
            rehash();
        }

        int drawer = hashName(country->name, buckets.size());
        buckets[drawer].push_back(country);
        count++;
    }

    // Find a country by name.  Returns nullptr (meaning "nothing") if the name
    // does not exist.  Capital or small letters do not matter.
    Country* search(const std::string& name) const {
        int drawer = hashName(name, buckets.size());

        // Look only inside that one drawer.
        const std::vector<Country*>& drawerContents = buckets[drawer];
        for (int i = 0; i < (int)drawerContents.size(); i++) {
            if (toLowerCase(drawerContents[i]->name) == toLowerCase(name)) {
                return drawerContents[i];
            }
        }
        return nullptr;
    }

private:
    // buckets[0], buckets[1], ... are the drawers.  Each drawer is a list
    // (a vector) of country pointers.
    std::vector< std::vector<Country*> > buckets;
    int count;                                   // how many countries are stored

    // THE HASH FUNCTION: turns a name into a drawer number from 0 to size-1.
    //
    // Method: start with 0.  For every letter, multiply the running number by 31
    // and add the letter's code.  Then take the remainder after dividing by the
    // table size, so the number always fits a real drawer.
    //
    // Example for "india" with 431 drawers:
    //   i: 0*31 + 105 = 105
    //   n: 105*31 + 110 = 3365
    //   d: 3365*31 + 100 = 104415
    //   i: 104415*31 + 105 = 3236970
    //   a: 3236970*31 + 97 = 100346167      ->  100346167 % 431 = 316  (drawer 316)
    //
    // Why multiply by 31?  So that the ORDER of the letters matters, and "cab"
    // and "bac" land in different drawers.
    // We take the remainder after every letter so the number never gets huge.
    int hashName(const std::string& name, int size) const {
        std::string word = toLowerCase(name);    // so "India" and "india" give the same drawer
        long long number = 0;
        for (int i = 0; i < (int)word.size(); i++) {
            number = (number * 31 + (unsigned char)word[i]) % size;
        }
        return (int)number;
    }

    // Is n a prime number?  (A prime has no divisor except 1 and itself.)
    // A prime table size spreads the names more evenly over the drawers.
    bool isPrime(int n) const {
        if (n < 2) return false;
        for (int d = 2; d * d <= n; d++) {
            if (n % d == 0) return false;
        }
        return true;
    }

    // REHASH: make the table about twice as big (next prime number), then put
    // every country in again.
    // We must put them in again (not just copy the drawers) because the drawer
    // number depends on the table size.
    void rehash() {
        int newSize = buckets.size() * 2;
        while (!isPrime(newSize)) {
            newSize++;
        }

        std::vector< std::vector<Country*> > newBuckets(newSize);
        for (int b = 0; b < (int)buckets.size(); b++) {
            for (int i = 0; i < (int)buckets[b].size(); i++) {
                Country* country = buckets[b][i];
                int drawer = hashName(country->name, newSize);
                newBuckets[drawer].push_back(country);
            }
        }
        buckets = newBuckets;
    }
};

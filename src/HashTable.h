// HashTable.h
// ---------------------------------------------------------------------------
// WHAT:  a hash table that finds a country by NAME in O(1) average time.
// HOW :  "separate chaining" - the table is an array of buckets and each
//        bucket is a small list.  Two names that land in the same bucket
//        simply sit in the same list (that is a "collision").
// ---------------------------------------------------------------------------
#pragma once
#include <cctype>
#include <list>
#include <string>
#include <vector>
#include "Country.h"

class HashTable {
public:
    HashTable() : buckets(101), count(0) {}   // start small (101 is prime); it grows by itself

    // Put a country into the table.
    void insert(Country* c) {
        // If the table is more than 75 % full, chains get long and lookups
        // slow down, so we first make the table bigger ("rehash").
        if (count + 1 > 0.75 * buckets.size()) rehash();
        buckets[hashName(c->name, buckets.size())].push_back(c);
        count++;
    }

    // Find a country by name.  Returns nullptr when there is no such country.
    // Upper/lower case does not matter ("india" finds "India").
    Country* search(const std::string& name) const {
        const std::list<Country*>& chain = buckets[hashName(name, buckets.size())];
        for (Country* c : chain)                       // walk the (short) chain
            if (sameName(c->name, name)) return c;
        return nullptr;
    }

private:
    std::vector<std::list<Country*>> buckets;
    int count;

    // POLYNOMIAL HASH:  h = h*31 + (next letter).   Letters are made lower case
    // first so "India" and "india" give the same number.  % size squeezes the
    // big number into a valid bucket position 0 .. size-1.
    static size_t hashName(const std::string& name, size_t size) {
        unsigned long h = 0;                           // unsigned: overflow just wraps round, never negative
        for (char ch : name)
            h = h * 31 + (unsigned char)std::tolower((unsigned char)ch);
        return h % size;
    }

    // Case-insensitive comparison of two names.
    static bool sameName(const std::string& a, const std::string& b) {
        if (a.size() != b.size()) return false;
        for (size_t i = 0; i < a.size(); i++)
            if (std::tolower((unsigned char)a[i]) != std::tolower((unsigned char)b[i])) return false;
        return true;
    }

    // A prime table size spreads names more evenly, so we always pick a prime.
    static bool isPrime(size_t n) {
        if (n < 2) return false;
        for (size_t i = 2; i * i <= n; i++)
            if (n % i == 0) return false;
        return true;
    }

    // Build a table about twice as big and put every country in again.
    // We must re-insert (not copy) because the bucket number depends on the size.
    void rehash() {
        size_t newSize = buckets.size() * 2;
        while (!isPrime(newSize)) newSize++;
        std::vector<std::list<Country*>> bigger(newSize);
        for (const auto& chain : buckets)
            for (Country* c : chain)
                bigger[hashName(c->name, newSize)].push_back(c);
        buckets = std::move(bigger);
    }
};

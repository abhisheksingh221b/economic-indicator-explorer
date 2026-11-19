
#pragma once
#include <cctype>
#include <string>
#include <vector>
#include "Country.h"



inline string toLowerCase(string text) {
    for (int i = 0; i < (int)text.size(); i++) {
        text[i] = tolower(text[i]);          
    }
    return text;
}


class HashTable {
public:
    
    HashTable() {
        buckets.resize(101);
        count = 0;
    }

    
    void insert(Country* country) {
        
        if (count + 1 > 0.75 * buckets.size()) {
            rehash();
        }

        int drawer = hashName(country->name, buckets.size());
        buckets[drawer].push_back(country);
        count++;
    }

    
    Country* search(const string& name) const {
        int drawer = hashName(name, buckets.size());

       
        const vector<Country*>& drawerContents = buckets[drawer];
        for (int i = 0; i < (int)drawerContents.size(); i++) {
            if (toLowerCase(drawerContents[i]->name) == toLowerCase(name)) {
                return drawerContents[i];
            }
        }
        return nullptr;
    }

private:
    
    vector< vector<Country*> > buckets;
    int count;                                   

    
    int hashName(const string& name, int size) const {
        string word = toLowerCase(name);    
        long long number = 0;
        for (int i = 0; i < (int)word.size(); i++) {
            number = (number * 31 + (unsigned char)word[i]) % size;
        }
        return (int)number;
    }

   
    bool isPrime(int n) const {
        if (n < 2) return false;
        for (int d = 2; d * d <= n; d++) {
            if (n % d == 0) return false;
        }
        return true;
    }

    
    void rehash() {
        int newSize = buckets.size() * 2;
        while (!isPrime(newSize)) {
            newSize++;
        }

        vector< vector<Country*> > newBuckets(newSize);
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

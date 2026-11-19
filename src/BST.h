
#pragma once
#include <vector>
#include "Country.h"


struct Node {
    double   key;        
    Country* rec;        
    Node*    left;       
    Node*    right;      

    Node(double k, Country* c) {
        key = k;
        rec = c;
        left = nullptr;
        right = nullptr;
    }
};


class BST {
public:
    BST() {
        root = nullptr;                 
    }

    ~BST() {
        deleteAll(root);                
    }

 
    void insert(double key, Country* country) {
        root = insertInto(root, key, country);
    }

    
    vector<Country*> rangeQuery(double low, double high) const {
        vector<Country*> result;
        collectRange(root, low, high, result);
        return result;
    }

private:
    Node* root;

    
    bool comesBefore(double keyA, Country* countryA, double keyB, Country* countryB) const {
        if (keyA != keyB) {
            return keyA < keyB;
        }
        return countryA->name < countryB->name;
    }

   
    Node* insertInto(Node* node, double key, Country* country) {
        if (node == nullptr) {
            return new Node(key, country);              
        }

        if (comesBefore(key, country, node->key, node->rec)) {
            node->left = insertInto(node->left, key, country);
        } else {
            node->right = insertInto(node->right, key, country);
        }
        return node;
    }

    
    void collectRange(Node* node, double low, double high, vector<Country*>& result) const {
        if (node == nullptr) return;

       
        if (node->key >= low) {
            collectRange(node->left, low, high, result);
        }

        
        if (node->key >= low && node->key <= high) {
            result.push_back(node->rec);
        }

        
        if (node->key <= high) {
            collectRange(node->right, low, high, result);
        }
    }

   
    void deleteAll(Node* node) {
        if (node == nullptr) return;
        deleteAll(node->left);
        deleteAll(node->right);
        delete node;
    }
};

// BST.h
// ---------------------------------------------------------------------------
// WHAT   A sorted tree (a "binary search tree", BST).  We build ONE tree for
//        each indicator (an inflation tree, a debt tree, ...).  A tree keeps
//        the countries sorted by that indicator, which makes this question
//        fast: "give me everything between X and Y" (a "range query").
//
// THE IDEA
//   The tree is made of boxes called nodes.  Each node holds one value and has
//   up to two children.  Smaller values go to the LEFT, bigger values to the
//   RIGHT.  Reading the tree left -> node -> right gives the values in order.
//
// THE KNOWN WEAKNESS
//   This tree does NOT balance itself.  If values arrive already sorted
//   (3, 4, 5, 6 ...) it turns into one long line and becomes slow.  Our data
//   arrives in alphabetical order of country name, NOT sorted by value, so the
//   trees stay reasonably short.  (A self-balancing "AVL tree" would fix this
//   weakness; it is the natural next improvement.)
// ---------------------------------------------------------------------------
#pragma once
#include <vector>
#include "Country.h"


// ONE BOX OF THE TREE
struct Node {
    double   key;        // the indicator value, e.g. inflation 5.4
    Country* rec;        // which country this value belongs to
    Node*    left;       // child with smaller values (nullptr = no child)
    Node*    right;      // child with bigger values  (nullptr = no child)

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
        root = nullptr;                 // an empty tree has no root
    }

    ~BST() {
        deleteAll(root);                // give the memory back when the tree dies
    }

    // Add one country with its value.
    void insert(double key, Country* country) {
        root = insertInto(root, key, country);
    }

    // All countries whose value is from low to high (both included),
    // smallest value first.
    vector<Country*> rangeQuery(double low, double high) const {
        vector<Country*> result;
        collectRange(root, low, high, result);
        return result;
    }

    // Number of levels (an empty tree has 0).  Only used by the self-test.
    int height() const {
        return heightOf(root);
    }

private:
    Node* root;

    // -----------------------------------------------------------------------
    // WHICH VALUE COMES FIRST?
    // Normally the smaller value.  If two countries have the SAME value, the one
    // whose name comes first in the alphabet is first.  This way two countries
    // with equal inflation are both kept (nobody is lost).
    // -----------------------------------------------------------------------
    bool comesBefore(double keyA, Country* countryA, double keyB, Country* countryB) const {
        if (keyA != keyB) {
            return keyA < keyB;
        }
        return countryA->name < countryB->name;
    }

    // -----------------------------------------------------------------------
    // INSERT
    // Walk down the tree to find the empty place (going left or right at each
    // node) and put the new node there.  This is "recursion": the function
    // calls itself on a smaller part of the tree.  It returns the node that is
    // at the top of that part.
    // -----------------------------------------------------------------------
    Node* insertInto(Node* node, double key, Country* country) {
        if (node == nullptr) {
            return new Node(key, country);               // found the empty place
        }

        if (comesBefore(key, country, node->key, node->rec)) {
            node->left = insertInto(node->left, key, country);
        } else {
            node->right = insertInto(node->right, key, country);
        }
        return node;
    }

    // -----------------------------------------------------------------------
    // RANGE QUERY: collect every value from low to high.
    // We visit a side only if it can still contain answers.  That is why the
    // search is fast: whole parts of the tree are skipped.
    // -----------------------------------------------------------------------
    void collectRange(Node* node, double low, double high, vector<Country*>& result) const {
        if (node == nullptr) return;

        // Smaller values are on the left.  If this node is already below
        // "low", everything on its left is too small, so we skip the left side.
        if (node->key >= low) {
            collectRange(node->left, low, high, result);
        }

        // Is this node itself inside the range?
        if (node->key >= low && node->key <= high) {
            result.push_back(node->rec);
        }

        // Bigger values are on the right.  If this node is already above
        // "high", everything on its right is too big, so we skip the right side.
        if (node->key <= high) {
            collectRange(node->right, low, high, result);
        }
    }

    // Number of levels below and including this node.
    int heightOf(Node* node) const {
        if (node == nullptr) return 0;
        int leftHeight  = heightOf(node->left);
        int rightHeight = heightOf(node->right);
        if (leftHeight > rightHeight) return leftHeight + 1;
        return rightHeight + 1;
    }

    // Delete every node (children first, then the node itself).
    void deleteAll(Node* node) {
        if (node == nullptr) return;
        deleteAll(node->left);
        deleteAll(node->right);
        delete node;
    }
};

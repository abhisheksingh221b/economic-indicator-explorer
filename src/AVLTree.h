// AVLTree.h
// ---------------------------------------------------------------------------
// WHAT:  a self-balancing binary search tree.  We build ONE tree per indicator
//        (inflation tree, debt tree, ...).  The tree keeps countries sorted by
//        the indicator value, which makes "range" and "top-k" questions fast.
// WHY AVL: after every insert the tree checks itself and, if one side has
//        become too tall, it rotates.  So its height stays about log2(n) and
//        searching never degrades to a slow straight line.
// ---------------------------------------------------------------------------
#pragma once
#include <algorithm>
#include <vector>
#include "Country.h"

// One box in the tree.
struct Node {
    double   key;        // the indicator value (e.g. inflation 5.4)
    Country* rec;        // pointer to the country this value belongs to
    Node*    left  = nullptr;
    Node*    right = nullptr;
    int      height = 0; // height of the subtree below (and including) this node
    Node(double k, Country* c) : key(k), rec(c) {}
};

class AVLTree {
public:
    AVLTree() : root(nullptr) {}
    ~AVLTree() { destroy(root); }
    AVLTree(const AVLTree&) = delete;              // a copy would share nodes and delete them twice
    AVLTree& operator=(const AVLTree&) = delete;

    void insert(double key, Country* c) { root = insert(root, key, c); }

    // All countries with lo <= value <= hi, smallest value first.
    // Cost: O(log n) to reach the start + O(k) to collect the k answers.
    std::vector<Country*> rangeQuery(double lo, double hi) const {
        std::vector<Country*> out;
        collectRange(root, lo, hi, out);
        return out;
    }

    // The k countries with the highest (or lowest) value.
    std::vector<Country*> topK(int k, bool highest) const {
        std::vector<Country*> out;
        if (highest) collectHigh(root, k, out);
        else         collectLow(root, k, out);
        return out;
    }

    // Only used by test_avl.cpp to prove the tree really is balanced.
    // The menu never calls it.
    bool isBalanced() const { return check(root); }

private:
    Node* root;

    // ---- height helpers -------------------------------------------------
    // Height counts EDGES: a leaf has height 0 and "no node" has height -1.
    static int height(Node* n) { return n ? n->height : -1; }
    static void updateHeight(Node* n) { n->height = 1 + std::max(height(n->left), height(n->right)); }
    // balance factor = (left height) - (right height); AVL allows only -1, 0, +1.
    static int balanceFactor(Node* n) { return height(n->left) - height(n->right); }

    // ---- ordering rule --------------------------------------------------
    // Sort by value first.  If two countries have the SAME value, sort by name.
    // So no two nodes are ever "equal" and no country is lost.
    static bool before(double k1, const Country* c1, double k2, const Country* c2) {
        if (k1 != k2) return k1 < k2;
        return c1->name < c2->name;
    }

    // ---- rotations ------------------------------------------------------
    /*       y                x
              / \              / \
             x   C   --->     A   y        (rotateRight; rotateLeft is the mirror)
            / \                  / \
           A   B                B   C                                        */
    static Node* rotateRight(Node* y) {
        Node* x = y->left;
        y->left = x->right;
        x->right = y;
        updateHeight(y);     // y is lower now, so update it first
        updateHeight(x);
        return x;            // x is the new top of this part of the tree
    }
    static Node* rotateLeft(Node* x) {
        Node* y = x->right;
        x->right = y->left;
        y->left = x;
        updateHeight(x);
        updateHeight(y);
        return y;
    }

    // Check one node and fix it if it is out of balance.
    static Node* rebalance(Node* n) {
        updateHeight(n);
        int bf = balanceFactor(n);
        if (bf > 1) {                                           // left side too tall
            if (balanceFactor(n->left) < 0)                     // "left-right" case:
                n->left = rotateLeft(n->left);                  //   first straighten the left child
            return rotateRight(n);
        }
        if (bf < -1) {                                          // right side too tall
            if (balanceFactor(n->right) > 0)                    // "right-left" case
                n->right = rotateRight(n->right);
            return rotateLeft(n);
        }
        return n;                                               // already balanced
    }

    // ---- insert (recursive) ---------------------------------------------
    static Node* insert(Node* n, double key, Country* c) {
        if (n == nullptr) return new Node(key, c);              // found the empty spot
        if (before(key, c, n->key, n->rec)) n->left  = insert(n->left, key, c);
        else                                n->right = insert(n->right, key, c);
        return rebalance(n);                                    // fix balance on the way back up
    }

    // ---- range query ----------------------------------------------------
    // We only visit a side when it can still contain answers - that is what
    // makes the search O(log n + k) instead of O(n).
    static void collectRange(Node* n, double lo, double hi, std::vector<Country*>& out) {
        if (n == nullptr) return;
        if (n->key >= lo) collectRange(n->left, lo, hi, out);   // left side may hold values >= lo
        if (n->key >= lo && n->key <= hi) out.push_back(n->rec);// this node is an answer
        if (n->key <= hi) collectRange(n->right, lo, hi, out);  // right side may hold values <= hi
    }

    // ---- top-k ----------------------------------------------------------
    // Highest first = visit right side, node, left side (reverse in-order),
    // and stop as soon as we have k results.
    static void collectHigh(Node* n, int k, std::vector<Country*>& out) {
        if (n == nullptr || (int)out.size() >= k) return;
        collectHigh(n->right, k, out);
        if ((int)out.size() < k) out.push_back(n->rec);
        collectHigh(n->left, k, out);
    }
    static void collectLow(Node* n, int k, std::vector<Country*>& out) {
        if (n == nullptr || (int)out.size() >= k) return;
        collectLow(n->left, k, out);
        if ((int)out.size() < k) out.push_back(n->rec);
        collectLow(n->right, k, out);
    }

    // ---- cleanup and self-check ----------------------------------------
    static void destroy(Node* n) {
        if (n == nullptr) return;
        destroy(n->left);
        destroy(n->right);
        delete n;
    }
    static bool check(Node* n) {
        if (n == nullptr) return true;
        int bf = balanceFactor(n);
        if (bf < -1 || bf > 1) return false;
        if (n->height != 1 + std::max(height(n->left), height(n->right))) return false;
        return check(n->left) && check(n->right);
    }
};

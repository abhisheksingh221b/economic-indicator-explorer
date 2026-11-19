// AVLTree.h
// ---------------------------------------------------------------------------
// WHAT   A sorted tree that keeps itself balanced.  We build ONE tree for each
//        indicator (an inflation tree, a debt tree, ...).  A tree keeps the
//        countries sorted by that indicator, which makes two kinds of question
//        fast: "everything between X and Y" and "the k biggest".
//
// BINARY SEARCH TREE (the basic idea)
//   The tree is made of boxes called nodes.  Each node holds one value and has
//   up to two children.  Smaller values go to the LEFT, bigger values to the
//   RIGHT.  Reading the tree left -> node -> right gives the values in order.
//
// THE PROBLEM, AND THE "AVL" FIX
//   If values arrive already sorted (3, 4, 5, 6 ...) a plain tree becomes one
//   long line and searching is slow.  An AVL tree checks itself after every
//   insert.  If one side becomes too tall it fixes itself with a "rotation".
//   So the tree stays short and wide and every search stays fast.
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
    int      height;     // how many steps down to the lowest leaf below this node

    Node(double k, Country* c) {
        key = k;
        rec = c;
        left = nullptr;
        right = nullptr;
        height = 0;      // a new node has no children, so its height is 0
    }
};


class AVLTree {
public:
    AVLTree() {
        root = nullptr;                 // an empty tree has no root
    }

    ~AVLTree() {
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

    // The k countries with the highest values (highest = true)
    // or the lowest values (highest = false).
    vector<Country*> topK(int k, bool highest) const {
        vector<Country*> result;
        if (highest) {
            collectHighest(root, k, result);
        } else {
            collectLowest(root, k, result);
        }
        return result;
    }

    // Only used by the self-test (make test) to prove the tree is balanced.
    // The menu never calls it.
    bool isBalanced() const {
        return checkBalance(root);
    }

private:
    Node* root;

    // -----------------------------------------------------------------------
    // HEIGHT AND BALANCE
    //   Height = number of steps down to the lowest leaf.
    //   A leaf (no children) has height 0.  "No node" counts as -1.
    //   Balance factor = (height of left side) - (height of right side).
    //   An AVL tree only allows a balance factor of -1, 0 or +1.
    // -----------------------------------------------------------------------
    int heightOf(Node* node) const {
        if (node == nullptr) return -1;
        return node->height;
    }

    void updateHeight(Node* node) {
        int leftHeight  = heightOf(node->left);
        int rightHeight = heightOf(node->right);
        if (leftHeight > rightHeight) {
            node->height = leftHeight + 1;
        } else {
            node->height = rightHeight + 1;
        }
    }

    int balanceFactor(Node* node) const {
        return heightOf(node->left) - heightOf(node->right);
    }

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
    // ROTATIONS: lift one node up and push another down, without breaking the
    // sorted order.  Picture (rotateRight at y; rotateLeft is the mirror image):
    /*
              y                       x
             / \                     / \
            x   C     ------>       A   y
           / \                         / \
          A   B                       B   C
    */
    // Order before and after is the same:  A < x < B < y < C.
    // -----------------------------------------------------------------------
    Node* rotateRight(Node* y) {
        Node* x = y->left;
        y->left = x->right;     // B moves over to y
        x->right = y;           // y becomes the right child of x
        updateHeight(y);        // y is lower now, so fix its height first
        updateHeight(x);
        return x;               // x is the new top of this part of the tree
    }

    Node* rotateLeft(Node* x) {
        Node* y = x->right;
        x->right = y->left;     // B moves over to x
        y->left = x;            // x becomes the left child of y
        updateHeight(x);
        updateHeight(y);
        return y;               // y is the new top of this part of the tree
    }

    // Check one node.  If it is out of balance, fix it with rotation(s).
    // There are four cases:
    //   1. left side too tall, and the left child leans left   -> one right rotation
    //   2. left side too tall, and the left child leans right  -> left rotation on the child, then right rotation
    //   3. right side too tall, and the right child leans right -> one left rotation
    //   4. right side too tall, and the right child leans left  -> right rotation on the child, then left rotation
    Node* rebalance(Node* node) {
        updateHeight(node);
        int balance = balanceFactor(node);

        if (balance > 1) {                               // left side is too tall
            if (balanceFactor(node->left) < 0) {         // case 2
                node->left = rotateLeft(node->left);
            }
            return rotateRight(node);                    // case 1 (and the end of case 2)
        }

        if (balance < -1) {                              // right side is too tall
            if (balanceFactor(node->right) > 0) {        // case 4
                node->right = rotateRight(node->right);
            }
            return rotateLeft(node);                     // case 3 (and the end of case 4)
        }

        return node;                                     // already balanced, nothing to do
    }

    // -----------------------------------------------------------------------
    // INSERT
    // Walk down the tree to find the empty place (going left or right at each
    // node), put the new node there, then on the way back up check every node
    // with rebalance().  This is "recursion": the function calls itself on a
    // smaller part of the tree.  It returns the node that is now at the top.
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
        return rebalance(node);
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

    // -----------------------------------------------------------------------
    // TOP-K
    // For the HIGHEST values we read the tree backwards: right side first, then
    // the node, then the left side.  We stop as soon as we have k countries.
    // For the LOWEST values we do the opposite: left, node, right.
    // -----------------------------------------------------------------------
    void collectHighest(Node* node, int k, vector<Country*>& result) const {
        if (node == nullptr) return;
        if ((int)result.size() >= k) return;             // we already have enough

        collectHighest(node->right, k, result);
        if ((int)result.size() < k) {
            result.push_back(node->rec);
        }
        collectHighest(node->left, k, result);
    }

    void collectLowest(Node* node, int k, vector<Country*>& result) const {
        if (node == nullptr) return;
        if ((int)result.size() >= k) return;

        collectLowest(node->left, k, result);
        if ((int)result.size() < k) {
            result.push_back(node->rec);
        }
        collectLowest(node->right, k, result);
    }

    // -----------------------------------------------------------------------
    // CLEAN-UP AND SELF-CHECK
    // -----------------------------------------------------------------------
    // Delete every node (children first, then the node itself).
    void deleteAll(Node* node) {
        if (node == nullptr) return;
        deleteAll(node->left);
        deleteAll(node->right);
        delete node;
    }

    // True if every node has a balance factor of -1, 0 or +1 and a correct height.
    bool checkBalance(Node* node) const {
        if (node == nullptr) return true;

        int balance = balanceFactor(node);
        if (balance < -1 || balance > 1) return false;

        int leftHeight  = heightOf(node->left);
        int rightHeight = heightOf(node->right);
        int expected = (leftHeight > rightHeight ? leftHeight : rightHeight) + 1;
        if (node->height != expected) return false;

        return checkBalance(node->left) && checkBalance(node->right);
    }
};

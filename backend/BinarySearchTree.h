#pragma once
#include <iostream>
#include "Case.h"
using namespace std;

// Binary Search Tree of cases, ordered by case ID.
// Insert, search and delete: O(log n) on average, O(n) in the worst case.
// In-order traversal (left, node, right) visits cases sorted by ID: O(n).

struct BSTNode {
    Case data;
    BSTNode* left;
    BSTNode* right;
    BSTNode(Case c) : data(c), left(nullptr), right(nullptr) {}
};

class BST {
private:
    BSTNode* root;

    BSTNode* insert(BSTNode* node, Case c) {
        if (!node) return new BSTNode(c);
        if (c.id < node->data.id)      node->left  = insert(node->left, c);
        else if (c.id > node->data.id) node->right = insert(node->right, c);
        else cout << "Case ID already exists.\n";
        return node;
    }

    BSTNode* search(BSTNode* node, int id) {
        if (!node || node->data.id == id) return node;
        if (id < node->data.id) return search(node->left, id);
        return search(node->right, id);
    }

    BSTNode* findMin(BSTNode* node) {
        while (node->left) node = node->left;
        return node;
    }

    BSTNode* remove(BSTNode* node, int id) {
        if (!node) return nullptr;
        if (id < node->data.id)      node->left  = remove(node->left, id);
        else if (id > node->data.id) node->right = remove(node->right, id);
        else {
            // zero or one child: the child takes its place
            if (!node->left)  { BSTNode* r = node->right; delete node; return r; }
            if (!node->right) { BSTNode* l = node->left;  delete node; return l; }
            // two children: copy the smallest node of the right side, then delete that one
            BSTNode* m = findMin(node->right);
            node->data = m->data;
            node->right = remove(node->right, m->data.id);
        }
        return node;
    }

    void inorder(BSTNode* node) {
        if (!node) return;
        inorder(node->left);
        cout << "ID " << node->data.id << " | " << node->data.name
             << " | Age " << node->data.age
             << " | Last seen: N" << node->data.lastSeenNode << "\n";
        inorder(node->right);
    }

    // In-order traversal that stores a pointer to each case (sorted by ID)
    void collect(BSTNode* node, Case* out[], int& count, int maxCount) {
        if (!node) return;
        collect(node->left, out, count, maxCount);
        if (count < maxCount) out[count++] = &node->data;
        collect(node->right, out, count, maxCount);
    }

    void destroy(BSTNode* node) {
        if (!node) return;
        destroy(node->left);
        destroy(node->right);
        delete node;
    }

public:
    BST() : root(nullptr) {}
    ~BST() { destroy(root); }

    void insert(Case c)    { root = insert(root, c); }
    Case* search(int id)   { BSTNode* n = search(root, id); return n ? &n->data : nullptr; }
    void remove(int id)    { root = remove(root, id); }
    void displayAll()      { inorder(root); }

    int getAll(Case* out[], int maxCount) {
        int count = 0;
        collect(root, out, count, maxCount);
        return count;
    }
};
#pragma once
#include <iostream>
#include "Case.h"
using namespace std;

// Singly linked list that stores every reported sighting (the history).
// add: O(1) because we keep a tail pointer. Traversal, delete: O(n).
class SightingList {
    struct Node {
        Sighting data;
        Node* next;
        Node(Sighting s) : data(s), next(nullptr) {}
    };
    Node* head;
    Node* tail;

public:
    SightingList() : head(nullptr), tail(nullptr) {}

    ~SightingList() {
        while (head) {
            Node* t = head;
            head = head->next;
            delete t;
        }
    }

    // Insert at the end
    void add(Sighting s) {
        Node* n = new Node(s);
        if (tail) tail->next = n;
        else head = n;
        tail = n;
    }

    // Traverse and print the sightings of one case
    void displayForCase(int caseId) {
        bool found = false;
        for (Node* cur = head; cur; cur = cur->next) {
            if (cur->data.caseId == caseId) {
                cout << "  Node N" << cur->data.node << " at " << cur->data.time << "\n";
                found = true;
            }
        }
        if (!found) cout << "  No sightings recorded.\n";
    }

    // Delete the newest sighting of one case (used by "Cancel Last Sighting")
    void removeLastForCase(int caseId) {
        Node* target = nullptr;
        Node* targetPrev = nullptr;
        Node* prev = nullptr;
        for (Node* cur = head; cur; prev = cur, cur = cur->next) {
            if (cur->data.caseId == caseId) { target = cur; targetPrev = prev; }
        }
        if (!target) return;
        if (targetPrev) targetPrev->next = target->next;
        else head = target->next;
        if (target == tail) tail = targetPrev;
        delete target;
    }

    // Delete all sightings of a closed case
    void removeForCase(int caseId) {
        while (head && head->data.caseId == caseId) {
            Node* t = head;
            head = head->next;
            delete t;
        }
        if (!head) { tail = nullptr; return; }
        Node* cur = head;
        while (cur->next) {
            if (cur->next->data.caseId == caseId) {
                Node* t = cur->next;
                cur->next = t->next;
                delete t;
            } else {
                cur = cur->next;
            }
        }
        tail = cur;
    }

    // Copy the list into an array (oldest first). Returns how many were copied.
    int getAll(Sighting out[], int maxCount) {
        int n = 0;
        for (Node* cur = head; cur && n < maxCount; cur = cur->next) out[n++] = cur->data;
        return n;
    }
};
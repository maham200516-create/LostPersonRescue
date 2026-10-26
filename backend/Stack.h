#pragma once

// Our own stack (linked nodes). Last in, first out.
template <typename T>
class Stack {
    struct Node {
        T data;
        Node* next;
        Node(T d) : data(d), next(nullptr) {}
    };
    Node* top; 
 
public:
    Stack() : top(nullptr) {}

    ~Stack() {
        while (top) {
            Node* temp = top;
            top = top->next;
            delete temp;
        }
    }

    void push(T value) {
        Node* newnode = new Node(value);
        newnode->next = top;
        top = newnode;
    }

    bool pop(T& out) {
        if (!top) return false;
        Node* temp = top;
        out = temp->data;
        top = top->next;
        delete temp;
        return true;
    }

    bool isEmpty() const { return top == nullptr; }
};
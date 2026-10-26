#pragma once
 
// Our own queue (linked nodes). First in, first out.
template <typename T>
class Queue {
    struct Node {
        T data;
        Node* next;
        Node(T d) : data(d), next(nullptr) {}
    };
    Node* front;
    Node* rear;
    int count;

public:
    Queue() : front(nullptr), rear(nullptr), count(0) {}

    ~Queue() {
        while (front) {
            Node* temp = front;
            front = front->next;
            delete temp;
        }
    }

    void enqueue(T value) {
        Node* newnode = new Node(value);
        if (rear) rear->next = newnode;
        else front = newnode;
        rear = newnode;
        count++;
    }

    bool dequeue(T& out) {
        if (!front) return false;
        Node* temp = front;
        out = temp->data;
        front = front->next;
        if (!front) rear = nullptr;
        delete temp;
        count--;
        return true;
    }

    bool isEmpty() const { return front == nullptr; }
    int size() const { return count; }
};
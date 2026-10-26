#pragma once
#include <iostream>
#include <string>
#include "Queue.h"
#include "Stack.h"
using namespace std;

// Graph of 20 locations stored as an adjacency list (linked list per node).
class Graph {
    struct Edge {
        int to;
        Edge* next;
    };
    Edge* adj[21];          // index 1 to 20
    string names[21];
    int edgeA[40], edgeB[40];   // each road stored once (for the UI)
    int edgeCount;

    void addEdge(int a, int b) {
        Edge* e1 = new Edge{b, adj[a]}; adj[a] = e1;
        Edge* e2 = new Edge{a, adj[b]}; adj[b] = e2;
        edgeA[edgeCount] = a;
        edgeB[edgeCount] = b;
        edgeCount++;
    }

public:
    Graph() : edgeCount(0) {
        for (int i = 0; i <= 20; i++) adj[i] = nullptr;

        string n[21] = {"", "Park Gate", "Main Market", "Bridge", "Lake Side", "School",
            "Bus Stop", "Hospital", "Old Mosque", "Forest Edge", "Railway Station",
            "River Bend", "Hilltop", "Farmhouse", "Crossroads", "Old Mill", "Lookout",
            "Meadow", "Pine Grove", "Waterfall", "Old Station"};
        for (int i = 1; i <= 20; i++) names[i] = n[i];

        // Roads (same as the Figma map)
        addEdge(1,2);   addEdge(1,12);  addEdge(1,19);  addEdge(2,3);   addEdge(2,13);
        addEdge(2,19);  addEdge(3,4);   addEdge(3,14);  addEdge(4,5);   addEdge(4,15);
        addEdge(5,6);   addEdge(5,16);  addEdge(6,7);   addEdge(6,16);  addEdge(7,8);
        addEdge(7,20);  addEdge(8,9);   addEdge(8,20);  addEdge(9,10);  addEdge(9,18);
        addEdge(10,11); addEdge(10,17); addEdge(11,12); addEdge(11,13); addEdge(11,17);
        addEdge(12,13); addEdge(13,14); addEdge(13,19); addEdge(14,15); addEdge(14,17);
        addEdge(15,16); addEdge(15,18); addEdge(16,20); addEdge(17,18); addEdge(18,20);
    }

    ~Graph() {
        for (int i = 1; i <= 20; i++) {
            while (adj[i]) {
                Edge* t = adj[i];
                adj[i] = adj[i]->next;
                delete t;
            }
        }
    }

    string getName(int node) { return names[node]; }
    int getEdgeCount() { return edgeCount; }
    int edgeFrom(int i) { return edgeA[i]; }
    int edgeTo(int i) { return edgeB[i]; }

    void display() {
        for (int i = 1; i <= 20; i++) {
            cout << "N" << i << " " << names[i] << " -> ";
            for (Edge* e = adj[i]; e; e = e->next) cout << "N" << e->to << " ";
            cout << "\n";
        }
    }

    // BFS: fills order[] with nodes in search order, level[node] = ring number.
    // Returns how many nodes were reached. Each node is visited only once.
    int bfs(int start, int order[], int level[]) {
        bool visited[21] = {false};
        Queue<int> q;
        int count = 0;

        q.enqueue(start);
        visited[start] = true;
        level[start] = 0;

        while (!q.isEmpty()) {
            int cur;
            q.dequeue(cur);
            order[count++] = cur;
            for (Edge* e = adj[cur]; e; e = e->next) {
                if (!visited[e->to]) {
                    visited[e->to] = true;
                    level[e->to] = level[cur] + 1;
                    q.enqueue(e->to);
                }
            }
        }
        return count;
    }

    // DFS using our stack: follows one trail as deep as possible.
    void dfs(int start) {
        bool visited[21] = {false};
        Stack<int> s;
        s.push(start);

        while (!s.isEmpty()) {
            int cur;
            s.pop(cur);
            if (visited[cur]) continue;
            visited[cur] = true;
            cout << "N" << cur << " " << names[cur] << "\n";
            for (Edge* e = adj[cur]; e; e = e->next)
                if (!visited[e->to]) s.push(e->to);
        }
    }
};
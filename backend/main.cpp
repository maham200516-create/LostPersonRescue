#include <iostream>
#include "BinarySearchTree.h"
#include "LinkedList.h"
#include "Queue.h"
#include "Graph.h"
using namespace std;

int main() {
    BST cases;
    SightingList sightings;
    Graph map;
    Queue<Team> teams;
    
    teams.enqueue({"Alpha Team"});
    teams.enqueue({"Bravo Team"});
    teams.enqueue({"Charlie Team"});
    teams.enqueue({"Delta Team"});

    int choice;
    do {
        cout << "\n===== Lost Person Search & Rescue =====\n"
             << "1. Register missing person\n"
             << "2. Search case by ID\n"
             << "3. Display all cases (sorted by ID)\n"
             << "4. Close case (delete)\n"
             << "5. Report sighting\n"
             << "6. Show sighting history\n"
             << "7. Start search (BFS + team dispatch)\n"
             << "8. Show map connections\n"
             << "9. Trail search (DFS)\n"
             << "0. Exit\nChoice: ";
        cin >> choice;
        cin.ignore();

        if (choice == 1) {
            int id, age, node, urg;
            string name, gender, desc;
            cout << "Case ID: ";           cin >> id;   cin.ignore();
            cout << "Name: ";              getline(cin, name);
            cout << "Age: ";               cin >> age;  cin.ignore();
            cout << "Gender: ";            getline(cin, gender);
            cout << "Description: ";       getline(cin, desc);
            cout << "Last seen node (1-20): "; cin >> node; cin.ignore();
            cout << "Urgency (1=High, 2=Medium, 3=Low): "; cin >> urg; cin.ignore();
            if (node < 1 || node > 20) { cout << "Invalid node.\n"; continue; }
            cases.insert({id, name, age, gender, desc, node, urg});
            cout << "Case registered.\n";
        }
        else if (choice == 2) {
            int id;
            cout << "Case ID: "; cin >> id; cin.ignore();
            Case* c = cases.search(id);
            if (c) cout << c->name << ", age " << c->age << ", last seen N"
                        << c->lastSeenNode << " (" << map.getName(c->lastSeenNode) << ")\n";
            else cout << "Case not found.\n";
        }
        else if (choice == 3) {
            cases.displayAll();
        }
        else if (choice == 4) {
            int id;
            cout << "Case ID to close: "; cin >> id; cin.ignore();
            if (cases.search(id)) {
                cases.remove(id);
                sightings.removeForCase(id);
                cout << "Case closed.\n";
            } else cout << "Case not found.\n";
        }
        else if (choice == 5) {
            int id, node;
            string time;
            cout << "Case ID: "; cin >> id; cin.ignore();
            Case* c = cases.search(id);
            if (!c) { cout << "Case not found.\n"; continue; }
            cout << "Sighting node (1-20): "; cin >> node; cin.ignore();
            if (node < 1 || node > 20) { cout << "Invalid node.\n"; continue; }
            cout << "Time (e.g. 14:30): "; getline(cin, time);
            sightings.add({id, node, time});
            c->lastSeenNode = node;
            cout << "Sighting recorded. Search center is now N" << node << ".\n";
        }
        else if (choice == 6) {
            int id;
            cout << "Case ID: "; cin >> id; cin.ignore();
            sightings.displayForCase(id);
        }
        else if (choice == 7) {
            int id;
            cout << "Case ID: "; cin >> id; cin.ignore();
            Case* c = cases.search(id);
            if (!c) { cout << "Case not found.\n"; continue; }

            int order[21], level[21];
            int n = map.bfs(c->lastSeenNode, order, level);
            cout << "\nSearching outward from N" << c->lastSeenNode << "\n";
            for (int i = 0; i < n; i++) {
                int node = order[i];
                Team t;
                teams.dequeue(t);       // next available team
                cout << "Ring " << level[node] << ": " << t.name << " searches N"
                     << node << " " << map.getName(node) << "\n";
                teams.enqueue(t);       // team goes back to the end of the queue
            }
        }
        else if (choice == 8) {
            map.display();
        }
        else if (choice == 9) {
            int node;
            cout << "Start node (1-20): "; cin >> node; cin.ignore();
            if (node < 1 || node > 20) { cout << "Invalid node.\n"; continue; }
            map.dfs(node);
        }
    } while (choice != 0);

    return 0;
}
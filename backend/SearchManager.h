#pragma once
#include <string>
#include <cstdlib>
#include <ctime>
#include "Case.h"
#include "BinarySearchTree.h"
#include "LinkedList.h"
#include "Queue.h"
#include "Graph.h"
using namespace std;

const int MAX_CASES = 200;
const int MAX_SIGHTINGS = 500;
const int TEAM_COUNT = 6;

// One team sent to one node. "wave" = which group of teams it travelled with.
struct Step {
    int node;
    int team;     // index of the team
    int wave;
};

struct SearchResult {
    bool ok;          // false if the case does not exist
    bool nothingLeft; // every location is already searched
    bool found;
    int foundAt;
    int ring;         // which ring (distance from the sighting) was searched
    int stepCount;
    Step steps[20];
};

// Where each data structure is used:
//   BST            -> stores all cases by ID
//   SightingList   -> linked list of every reported sighting (history)
//   Case::trail    -> array-based STACK of sighting nodes (undo = pop)
//   Case::searched -> VISITED ARRAY, so no node is searched twice per case
//   Graph (BFS)    -> adjacency list of the map, rings spread outward with a queue
//   Queue<int>     -> "available" = teams waiting, "pending" = nodes waiting for a team
//   Bubble sort    -> most urgent cases first
class SearchManager {
private:
    BST cases;
    SightingList sightings;
    Graph graph_;
    Team teams[TEAM_COUNT];
    Queue<int> available;     // queue of free team indexes (first come, first served)
    int nextId;

    // Urgent cases may use more teams at the same time
    int teamsFor(int urgency) {
        if (urgency == 1) return TEAM_COUNT;   // High
        if (urgency == 2) return 4;            // Medium
        return 2;                              // Low
    }

    void releaseTeam(int i) {
        teams[i].status = "Available";
        teams[i].node = 0;
        teams[i].caseId = 0;
        available.enqueue(i);                  // back to the end of the queue
    }

    void releaseAllTeams() {
        for (int i = 0; i < TEAM_COUNT; i++)
            if (teams[i].status == "Searching") releaseTeam(i);
    }

    void releaseTeamsOf(int caseId) {
        for (int i = 0; i < TEAM_COUNT; i++)
            if (teams[i].status == "Searching" && teams[i].caseId == caseId) releaseTeam(i);
    }

    void markFound(Case* c, int node, int ring) {
        c->found = true;
        c->foundAt = node;
        c->foundRing = ring;
        releaseTeamsOf(c->id);
    }

    // Simulation: the real location is a random node 1 to 4 rings from where the person was first seen.
    int pickHiddenNode(int start) {
        int order[21], level[21];
        int n = graph_.bfs(start, order, level);
        int candidates[20];
        int count = 0;
        for (int i = 0; i < n; i++)
            if (level[order[i]] >= 1 && level[order[i]] <= 4) candidates[count++] = order[i];
        if (count == 0) return start;
        return candidates[rand() % count];
    }

public:
    SearchManager() : nextId(1) {
        srand((unsigned)time(nullptr));
        const char* names[TEAM_COUNT] = {"Alpha Team", "Bravo Team", "Charlie Team",
                                         "Delta Team", "Echo Team", "Foxtrot Team"};
        for (int i = 0; i < TEAM_COUNT; i++) {
            teams[i].name = names[i];
            available.enqueue(i);
        }
    }

    // ---------- cases ----------

    // Register: BST insert, O(log n) average. The registered place is the bottom of the trail stack.
    int registerCase(const string& name, int age, const string& gender, const string& desc,
                     int node, int urgency, long long registeredAt) {
        if (node < 1 || node > 20) return 0;
        if (urgency < 1 || urgency > 3) urgency = 2;
        Case c{};
        c.id = nextId++;
        c.name = name;
        c.age = age;
        c.gender = gender;
        c.description = desc;
        c.lastSeenNode = node;
        c.urgency = urgency;
        c.registeredAt = registeredAt;
        c.trail[0] = node;
        c.trailCount = 1;
        c.hiddenNode = pickHiddenNode(node);
        cases.insert(c);
        return c.id;
    }

    // Close: BST delete + remove the case's sightings from the linked list
    bool closeCase(int id) {
        if (!cases.search(id)) return false;
        releaseTeamsOf(id);
        cases.remove(id);
        sightings.removeForCase(id);
        return true;
    }

    // ---------- sightings ----------

    // New sighting: add to the linked list and PUSH the node on the trail stack.
    bool addSighting(int caseId, int node, const string& time) {
        Case* c = cases.search(caseId);
        if (!c || c->found || node < 1 || node > 20 || c->trailCount >= MAX_TRAIL) return false;
        sightings.add({caseId, node, time});
        c->trail[c->trailCount++] = node;      // push
        c->lastSeenNode = node;
        return true;
    }

    // Undo: POP the newest sighting; the search center falls back to the previous one.
    // Nodes that were already searched stay searched (the teams really went there).
    bool cancelLastSighting(int caseId) {
        Case* c = cases.search(caseId);
        if (!c || c->found || c->trailCount <= 1) return false;   // trail[0] is the registered place
        c->trailCount--;                                          // pop
        c->lastSeenNode = c->trail[c->trailCount - 1];
        sightings.removeLastForCase(caseId);
        return true;
    }

    // ---------- the search ----------

    // One click = one ring. BFS (queue) gives the rings, the visited array skips searched nodes,
    // and teams are taken from the "available" queue in waves when there are not enough.
    SearchResult search(int caseId) {
        SearchResult r{};
        Case* c = cases.search(caseId);
        if (!c) return r;
        r.ok = true;

        if (c->found) {
            r.found = true;
            r.foundAt = c->foundAt;
            r.ring = c->foundRing;
            return r;
        }

        releaseAllTeams();      // teams from the previous click have finished

        // The person is exactly at the sighting point
        if (c->hiddenNode == c->lastSeenNode) {
            markFound(c, c->lastSeenNode, 0);
            r.found = true;
            r.foundAt = c->foundAt;
            r.ring = 0;
            return r;
        }

        int order[21], level[21];
        int n = graph_.bfs(c->lastSeenNode, order, level);      // O(V + E)

        // nearest ring that still has unsearched nodes (order[] is sorted by ring)
        int ring = 0;
        for (int i = 0; i < n; i++) {
            if (level[order[i]] > 0 && !c->searched[order[i]]) { ring = level[order[i]]; break; }
        }
        if (ring == 0) { r.nothingLeft = true; return r; }
        r.ring = ring;

        Queue<int> pending;     // nodes of this ring waiting for a team
        for (int i = 0; i < n; i++)
            if (level[order[i]] == ring && !c->searched[order[i]]) pending.enqueue(order[i]);

        int perWave = teamsFor(c->urgency);
        int wave = 0;
        while (!pending.isEmpty() && !r.found) {
            int used[TEAM_COUNT];
            int usedCount = 0;
            while (!pending.isEmpty() && usedCount < perWave && !available.isEmpty() && !r.found) {
                int node, t;
                pending.dequeue(node);
                available.dequeue(t);                  // next team in line
                teams[t].status = "Searching";
                teams[t].node = node;
                teams[t].caseId = caseId;
                c->searched[node] = true;              // visited: never searched again
                Step s = {node, t, wave};
                r.steps[r.stepCount++] = s;
                used[usedCount++] = t;
                if (node == c->hiddenNode) { r.found = true; r.foundAt = node; }
            }
            if (usedCount == 0) break;
            if (!pending.isEmpty() && !r.found) {      // wave finished, teams go back in the queue
                for (int i = 0; i < usedCount; i++) releaseTeam(used[i]);
                wave++;
            }
        }
        if (r.found) markFound(c, r.foundAt, ring);
        return r;
    }

    // ---------- data for the page ----------

    // Bubble sort, most urgent first. Equal urgency keeps ID order. O(n^2).
    void sortByUrgency(Case* a[], int n) {
        for (int i = 0; i < n - 1; i++)
            for (int j = 0; j < n - 1 - i; j++)
                if (a[j]->urgency > a[j + 1]->urgency) {
                    Case* t = a[j];
                    a[j] = a[j + 1];
                    a[j + 1] = t;
                }
    }

    int getSortedCases(Case* out[], int maxCount) {
        int n = cases.getAll(out, maxCount);
        sortByUrgency(out, n);
        return n;
    }

    int getSightings(Sighting out[], int maxCount) { return sightings.getAll(out, maxCount); }
    int teamCount() { return TEAM_COUNT; }
    Team& team(int i) { return teams[i]; }
    Graph& graph() { return graph_; }
};
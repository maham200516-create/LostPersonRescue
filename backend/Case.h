#pragma once
#include <string>
using namespace std;

const int MAX_TRAIL = 30;     // maximum sightings kept per case

struct Case {
    int id;
    string name;
    int age;
    string gender;
    string description;
    int lastSeenNode;             // current search center = top of the trail stack
    int urgency;                  // 1 = High, 2 = Medium, 3 = Low
    long long registeredAt = 0;   // used for the "time missing" timer

    bool searched[21] = {};       // visited array: nodes this case's search already covered
    int trail[MAX_TRAIL] = {};    // array-based stack of sighting nodes (top = trail[trailCount-1])
    int trailCount = 0;
    int hiddenNode = 0;           // simulated real location, never sent to the page
    bool found = false;
    int foundAt = 0;
    int foundRing = 0;
};

struct Sighting {
    int caseId;
    int node;
    string time;
};

struct Team {
    string name;
    string status = "Available";
    int node = 0;
    int caseId = 0;
};
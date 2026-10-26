// server.cpp - web layer only. All the data structure work is in SearchManager.h
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0A00
#endif
#include "httplib.h"
#include <iostream>
#include <string>
#include <mutex>
#include <ctime>
#include <cstdlib>
#include "SearchManager.h"
using namespace std;

// ---------- tiny JSON helpers (our requests are flat objects) ----------
string esc(const string& s) {
    string o;
    for (char c : s) {
        if (c == '"') o += "\\\"";
        else if (c == '\\') o += "\\\\";
        else if (c == '\n') o += "\\n";
        else if (c == '\t') o += "\\t";
        else if ((unsigned char)c < 0x20) o += ' ';
        else o += c;
    }
    return o;
}

size_t findValue(const string& j, const string& key) {
    size_t p = j.find("\"" + key + "\"");
    if (p == string::npos) return string::npos;
    p = j.find(':', p);
    if (p == string::npos) return string::npos;
    p++;
    while (p < j.size() && (j[p] == ' ' || j[p] == '\n' || j[p] == '\t' || j[p] == '\r')) p++;
    return p;
}

int getInt(const string& j, const string& key) {
    size_t p = findValue(j, key);
    if (p == string::npos) return 0;
    return atoi(j.c_str() + p);
}

string getStr(const string& j, const string& key) {
    size_t p = findValue(j, key);
    if (p == string::npos || p >= j.size() || j[p] != '"') return "";
    p++;
    string out;
    while (p < j.size() && j[p] != '"') {
        if (j[p] == '\\' && p + 1 < j.size()) {
            p++;
            if (j[p] == 'n') out += '\n';
            else if (j[p] == 't') out += '\t';
            else out += j[p];
        } else out += j[p];
        p++;
    }
    return out;
}

string boolStr(bool b) { return b ? "true" : "false"; }

// ---------- builds the JSON the page asks for ----------
string buildState(SearchManager& mgr) {
    long long now = time(nullptr);
    string j = "{\"persons\":[";

    Case* all[MAX_CASES];
    int n = mgr.getSortedCases(all, MAX_CASES);
    for (int i = 0; i < n; i++) {
        Case& c = *all[i];
        if (i) j += ",";
        j += "{\"id\":" + to_string(c.id)
           + ",\"name\":\"" + esc(c.name) + "\""
           + ",\"age\":" + to_string(c.age)
           + ",\"gender\":\"" + esc(c.gender) + "\""
           + ",\"description\":\"" + esc(c.description) + "\""
           + ",\"lastSeenNode\":" + to_string(c.lastSeenNode)
           + ",\"urgency\":" + to_string(c.urgency)
           + ",\"missingSeconds\":" + to_string(now - c.registeredAt)
           + ",\"found\":" + boolStr(c.found)
           + ",\"foundAt\":" + to_string(c.foundAt)
           + ",\"foundRing\":" + to_string(c.foundRing)
           + ",\"searched\":[";
        bool first = true;
        for (int k = 1; k <= 20; k++) {
            if (c.searched[k]) {
                if (!first) j += ",";
                first = false;
                j += to_string(k);
            }
        }
        j += "],\"trail\":[";
        for (int k = 0; k < c.trailCount; k++) {
            if (k) j += ",";
            j += to_string(c.trail[k]);
        }
        j += "]}";
    }

    j += "],\"teams\":[";
    for (int i = 0; i < mgr.teamCount(); i++) {
        Team& t = mgr.team(i);
        if (i) j += ",";
        j += "{\"name\":\"" + esc(t.name) + "\",\"status\":\"" + t.status
           + "\",\"node\":" + to_string(t.node)
           + ",\"caseId\":" + to_string(t.caseId) + "}";
    }

    j += "],\"edges\":[";
    Graph& g = mgr.graph();
    for (int i = 0; i < g.getEdgeCount(); i++) {
        if (i) j += ",";
        j += "[" + to_string(g.edgeFrom(i)) + "," + to_string(g.edgeTo(i)) + "]";
    }

    j += "],\"sightings\":[";
    static Sighting ss[MAX_SIGHTINGS];
    int sn = mgr.getSightings(ss, MAX_SIGHTINGS);
    for (int i = sn - 1; i >= 0; i--) {                  // newest first
        if (i != sn - 1) j += ",";
        j += "{\"caseId\":" + to_string(ss[i].caseId)
           + ",\"node\":" + to_string(ss[i].node)
           + ",\"time\":\"" + esc(ss[i].time) + "\"}";
    }
    j += "]}";
    return j;
}

void sendError(httplib::Response& res, int status, const string& msg) {
    res.status = status;
    res.set_content("{\"error\":\"" + esc(msg) + "\"}", "application/json");
}

int main() {
    SearchManager mgr;
    mutex m;                                   // one request at a time touches the data

    // Sample cases so the demo is not empty (delete these lines if you don't want them)
    long long now = time(nullptr);
    mgr.registerCase("Elena Marchetti", 34, "Female", "Red jacket, blue jeans, brown hiking boots. Has a small blue backpack.", 7, 1, now - 14820);
    mgr.registerCase("James Okafor", 67, "Male", "Gray sweater, khaki pants, uses a wooden walking cane.", 14, 1, now - 7440);
    mgr.registerCase("Lily Chen", 8, "Female", "Yellow raincoat, purple backpack, pigtails.", 3, 2, now - 3660);
    mgr.registerCase("Tomas Reyes", 52, "Male", "Green fishing vest, cargo shorts, orange cap.", 10, 3, now - 1920);

    httplib::Server svr;
    if (!svr.set_mount_point("/", "../Frontend"))
        cout << "Warning: Frontend folder not found. Run this from the backend folder.\n";

    svr.Get("/api/state", [&](const httplib::Request&, httplib::Response& res) {
        lock_guard<mutex> lock(m);
        res.set_content(buildState(mgr), "application/json");
    });

    svr.Post("/api/register", [&](const httplib::Request& req, httplib::Response& res) {
        lock_guard<mutex> lock(m);
        int id = mgr.registerCase(getStr(req.body, "name"), getInt(req.body, "age"),
                                  getStr(req.body, "gender"), getStr(req.body, "description"),
                                  getInt(req.body, "lastSeenNode"), getInt(req.body, "urgency"),
                                  time(nullptr));
        if (id == 0) { sendError(res, 400, "Invalid location."); return; }
        res.set_content("{\"id\":" + to_string(id) + "}", "application/json");
    });

    svr.Post("/api/sighting", [&](const httplib::Request& req, httplib::Response& res) {
        lock_guard<mutex> lock(m);
        bool ok = mgr.addSighting(getInt(req.body, "caseId"), getInt(req.body, "node"), getStr(req.body, "time"));
        if (!ok) { sendError(res, 400, "Sighting rejected (case already found, or too many sightings)."); return; }
        res.set_content("{\"ok\":true}", "application/json");
    });

    svr.Post("/api/cancel", [&](const httplib::Request& req, httplib::Response& res) {
        lock_guard<mutex> lock(m);
        if (!mgr.cancelLastSighting(getInt(req.body, "caseId"))) {
            sendError(res, 400, "Nothing to cancel.");
            return;
        }
        res.set_content("{\"ok\":true}", "application/json");
    });

    svr.Post("/api/search", [&](const httplib::Request& req, httplib::Response& res) {
        lock_guard<mutex> lock(m);
        SearchResult r = mgr.search(getInt(req.body, "caseId"));
        if (!r.ok) { sendError(res, 404, "Case not found."); return; }
        string out = "{\"ring\":" + to_string(r.ring)
                   + ",\"nothingLeft\":" + boolStr(r.nothingLeft)
                   + ",\"found\":" + boolStr(r.found)
                   + ",\"foundAt\":" + to_string(r.foundAt)
                   + ",\"steps\":[";
        for (int i = 0; i < r.stepCount; i++) {
            if (i) out += ",";
            out += "{\"node\":" + to_string(r.steps[i].node)
                 + ",\"wave\":" + to_string(r.steps[i].wave)
                 + ",\"team\":\"" + esc(mgr.team(r.steps[i].team).name) + "\"}";
        }
        out += "]}";
        res.set_content(out, "application/json");
    });

    svr.Post("/api/close", [&](const httplib::Request& req, httplib::Response& res) {
        lock_guard<mutex> lock(m);
        if (!mgr.closeCase(getInt(req.body, "caseId"))) { sendError(res, 404, "Case not found."); return; }
        res.set_content("{\"ok\":true}", "application/json");
    });

    cout << "Server running. Open http://localhost:8080 in your browser.\n";
    system("start http://localhost:8080");
    svr.listen("127.0.0.1", 8080);
    return 0;
}
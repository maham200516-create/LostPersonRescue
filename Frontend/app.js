// ---------- Map layout (positions are for drawing only) ----------
const NODES = {
  1:{name:"Park Gate",x:65,y:132},     2:{name:"Main Market",x:240,y:70},   3:{name:"Bridge",x:415,y:58},
  4:{name:"Lake Side",x:590,y:58},     5:{name:"School",x:760,y:72},        6:{name:"Bus Stop",x:905,y:158},
  7:{name:"Hospital",x:912,y:318},     8:{name:"Old Mosque",x:812,y:452},   9:{name:"Forest Edge",x:632,y:463},
  10:{name:"Railway Station",x:435,y:458}, 11:{name:"River Bend",x:232,y:443}, 12:{name:"Hilltop",x:62,y:322},
  13:{name:"Farmhouse",x:205,y:228},   14:{name:"Crossroads",x:390,y:220},  15:{name:"Old Mill",x:565,y:228},
  16:{name:"Lookout",x:742,y:255},     17:{name:"Meadow",x:365,y:352},      18:{name:"Pine Grove",x:598,y:362},
  19:{name:"Waterfall",x:165,y:182},   20:{name:"Old Station",x:848,y:368}
};
const URG = { 1:{t:"High",c:"#f87171",bg:"rgba(248,113,113,.15)"},
              2:{t:"Medium",c:"#fb923c",bg:"rgba(251,146,60,.15)"},
              3:{t:"Low",c:"#fbbf24",bg:"rgba(251,191,36,.15)"} };
const TEAMC = { Available:{c:"#34d399",bg:"rgba(52,211,153,.15)"},
                Searching:{c:"#60a5fa",bg:"rgba(96,165,250,.15)"},
                Returned:{c:"#64748b",bg:"rgba(100,116,139,.15)"} };
const NODE_STYLE = {
  searched:  ["rgba(22,163,74,.65)",  "#22c55e", 1.5],
  sighting:  ["rgba(239,68,68,.55)",  "#ef4444", 2],
  searching: ["rgba(251,146,60,.12)", "#fb923c", 2],
  found:     ["rgba(250,204,21,.45)", "#facc15", 3],
  unsearched:["rgba(51,65,85,.85)",   "rgba(148,163,184,.3)", 1.5]
};

// ---------- State (all real data comes from the C++ server) ----------
let state = { persons:[], teams:[], edges:[], sightings:[] };
let selectedCase = null, selectedNode = null, searching = false, loadedAt = Date.now();
let anim = {}, animTeams = {}, animCase = null;   // temporary colors while a search animates

const $ = id => document.getElementById(id);
const sleep = ms => new Promise(r => setTimeout(r, ms));
const h = s => String(s).replace(/[&<>"]/g, c => ({ "&":"&amp;", "<":"&lt;", ">":"&gt;", '"':"&quot;" }[c]));
const caseCode = id => "SAR-" + String(id).padStart(3, "0");
const nname = id => NODES[id] ? NODES[id].name : "";

async function api(path, body) {
  const opt = body ? { method:"POST", headers:{ "Content-Type":"application/json" }, body:JSON.stringify(body) } : undefined;
  const res = await fetch(path, opt);
  return res.json();
}

async function loadState() {
  try {
    state = await api("/api/state");
    loadedAt = Date.now();
    if (!state.persons.find(p => p.id === selectedCase))
      selectedCase = state.persons.length ? state.persons[0].id : null;
    $("statusText").textContent = ` · ${state.persons.filter(p => !p.found).length} active search operations in progress`;
    renderAll();
  } catch (e) {
    $("statusText").textContent = " · Cannot reach the C++ server. Is it running?";
  }
}

// ---------- Screens ----------
function show(name) {
  document.querySelectorAll(".screen").forEach(s => s.classList.remove("active"));
  $(name).classList.add("active");
  renderAll();
}

function renderAll() { renderStats(); renderPersons(); renderTeams(); renderDetail(); drawMap(); updateSearchBtn(); }

// Map colors for the SELECTED case only
function mapNodes() {
  const p = state.persons.find(x => x.id === selectedCase);
  const out = [];
  for (let i = 1; i <= 20; i++) {
    let s = "unsearched";
    if (p && p.searched.includes(i)) s = "searched";
    if (p && p.found && p.foundAt === i) s = "found";
    else if (p && !p.found && p.lastSeenNode === i) s = "sighting";
    if (animCase === selectedCase && anim[i]) s = anim[i];
    out.push({ id: i, state: s });
  }
  return out;
}

function statsHtml(searchedCount) {
  const s = [
    ["Active Cases",    state.persons.filter(p => !p.found).length, "#f87171"],
    ["Teams Available", state.teams.filter(t => t.status === "Available").length, "#34d399"],
    ["Nodes Searched",  searchedCount, "#60a5fa"],
    ["Sightings",       state.sightings.length, "#fbbf24"]
  ];
  return s.map(x => `<div class="stat"><span>${x[0]}</span><b style="color:${x[2]}">${x[1]}</b></div>`).join("");
}

function renderStats() {
  const total = state.persons.reduce((a, p) => a + p.searched.length, 0);
  $("homeStats").innerHTML = statsHtml(total);
  $("trackStats").innerHTML = statsHtml(mapNodes().filter(n => n.state === "searched" || n.state === "found").length);
}

function fmt(sec) {
  const hh = Math.floor(sec / 3600), m = Math.floor((sec % 3600) / 60);
  return hh > 0 ? `${hh}h ${m}m` : `${m}m ${sec % 60}s`;
}

function renderPersons() {
  $("personList").innerHTML = state.persons.map(p => {
    const u = URG[p.urgency];
    const right = p.found
      ? `<span style="color:#facc15;font-weight:700">✅ Found</span>`
      : `<span class="mono mt" data-base="${p.missingSeconds}">${fmt(p.missingSeconds)}</span>`;
    return `<button class="pcard ${p.id === selectedCase ? "sel" : ""}" onclick="selectCase(${p.id})">
      <div class="r"><div><b>${h(p.name)}</b><div class="muted">Age ${p.age} · ${h(p.gender)}</div></div>
        <span class="tag" style="background:${u.bg};color:${u.c}">${u.t}</span></div>
      <div class="r"><span>📍 N${p.lastSeenNode} · ${nname(p.lastSeenNode)}</span>${right}</div>
    </button>`;
  }).join("") || `<p class="muted">No cases yet. Register one with the + button.</p>`;
}

function renderTeams() {
  $("teamList").innerHTML = state.teams.map(t => {
    const c = TEAMC[t.status] || TEAMC.Available;
    return `<div class="tcard"><div class="r"><b>${t.name}</b>
      <span class="tag" style="background:${c.bg};color:${c.c}">${t.status}</span></div>
      ${t.node ? `<small>📍 N${t.node} · ${nname(t.node)} · ${caseCode(t.caseId)}</small>` : ""}</div>`;
  }).join("");
}

function renderDetail() {
  const p = state.persons.find(x => x.id === selectedCase);
  if (!p) { $("detail").innerHTML = `<p class="muted" style="margin:auto">Select a case to view details</p>`; return; }
  const u = URG[p.urgency];
  const list = state.sightings.filter(s => s.caseId === p.id);
  const foundHtml = p.found
    ? `<div class="sight" style="border-color:rgba(250,204,21,.4);color:#facc15;margin:0 0 10px"><span>✅ <b>FOUND</b> at N${p.foundAt} · ${nname(p.foundAt)} (ring ${p.foundRing})</span></div>` : "";
  $("detail").innerHTML = `
    <div class="d-head"><div><b>${h(p.name)}</b>
      <div class="muted">Age ${p.age} · ${h(p.gender)} · Last seen: N${p.lastSeenNode} · ${nname(p.lastSeenNode)}</div></div>
      <div class="bar"><span class="tag" style="background:${u.bg};color:${u.c}">${u.t} Priority</span>
      ${!p.found && p.trail.length > 1 ? `<button class="btn" onclick="cancelLast(${p.id})">Cancel Last Sighting</button>` : ""}
      <button class="btn" onclick="closeCase(${p.id})">Close Case</button></div></div>
    <div class="d-body">${foundHtml}${p.description ? `<p>${h(p.description)}</p>` : ""}
      <span class="muted">SIGHTING HISTORY (${list.length})</span>
      ${list.map(s => `<div class="sight"><span>🔴 Sighted at <b style="color:#e2e8f0">N${s.node} · ${nname(s.node)}</b></span><span class="mono">${h(s.time)}</span></div>`).join("")
        || `<p class="muted" style="margin-top:6px">No sightings reported yet.</p>`}</div>`;
}

function selectCase(id) { selectedCase = id; renderStats(); renderPersons(); renderDetail(); drawMap(); updateSearchBtn(); }

// ---------- Map ----------
function neighbors(id) {
  return state.edges.flatMap(e => e[0] === id ? [e[1]] : e[1] === id ? [e[0]] : []);
}

function drawMap() {
  if (!state.edges.length) { $("map").innerHTML = ""; return; }
  const nodes = mapNodes();
  const p = state.persons.find(x => x.id === selectedCase);
  const trail = p ? p.trail : [];
  const center = p ? p.lastSeenNode : 0;
  const done = s => s === "searched" || s === "found";
  const st = {}; nodes.forEach(n => st[n.id] = n.state);

  let teamAt = {};
  if (animCase === selectedCase) teamAt = animTeams;
  else state.teams.forEach(t => { if (t.node && t.caseId === selectedCase) (teamAt[t.node] = teamAt[t.node] || []).push(t); });

  const R = 21;
  let s = `<svg viewBox="0 0 960 510"><rect width="960" height="510" fill="#0d1b2a" rx="10"/>
    <polygon points="22,22 258,18 268,278 22,278" fill="rgba(22,163,74,.045)" stroke="rgba(22,163,74,.09)"/>
    <ellipse cx="435" cy="26" rx="78" ry="19" fill="rgba(56,189,248,.09)" stroke="rgba(56,189,248,.2)"/>
    <path d="M 310,58 Q 326,128 295,202 Q 272,255 242,298 Q 220,322 200,335" stroke="rgba(56,189,248,.22)" stroke-width="7" fill="none" stroke-linecap="round"/>
    <polygon points="438,342 700,338 700,395 438,395" fill="rgba(22,163,74,.045)" stroke="rgba(22,163,74,.09)"/>`;
  state.edges.forEach(e => {
    const a = NODES[e[0]], b = NODES[e[1]];
    const both = done(st[e[0]]) && done(st[e[1]]);
    s += `<line x1="${a.x}" y1="${a.y}" x2="${b.x}" y2="${b.y}" stroke="${both ? "rgba(34,197,94,.5)" : "rgba(255,255,255,.1)"}" stroke-width="${both ? 2 : 1.5}" stroke-linecap="round"/>`;
  });
  // the person's path: dashed line through the sightings in order
  if (trail.length > 1) {
    const pts = trail.map(i => NODES[i].x + "," + NODES[i].y).join(" ");
    s += `<polyline points="${pts}" fill="none" stroke="rgba(251,191,36,.4)" stroke-width="2" stroke-dasharray="6 5" stroke-linejoin="round"/>`;
  }
  nodes.forEach(n => {
    const q = NODES[n.id], v = NODE_STYLE[n.state] || NODE_STYLE.unsearched;
    s += `<g onclick="clickNode(${n.id})" style="cursor:pointer">`;
    if (n.id === selectedNode) s += `<circle cx="${q.x}" cy="${q.y}" r="${R + 6}" fill="none" stroke="rgba(255,255,255,.22)" stroke-width="1.5"/>`;
    if (trail.includes(n.id) && n.id !== center) s += `<circle cx="${q.x}" cy="${q.y}" r="${R + 4}" fill="none" stroke="rgba(251,191,36,.7)" stroke-width="1.5" stroke-dasharray="3 3"/>`;
    if (n.state === "sighting") s += `<circle cx="${q.x}" cy="${q.y}" r="${R + 4}" fill="none" stroke="#ef4444" stroke-width="1.5">
      <animate attributeName="r" values="${R + 4};${R + 16};${R + 4}" dur="1.8s" repeatCount="indefinite"/>
      <animate attributeName="opacity" values=".7;0;.7" dur="1.8s" repeatCount="indefinite"/></circle>`;
    s += `<circle cx="${q.x}" cy="${q.y}" r="${R}" fill="${v[0]}" stroke="${v[1]}" stroke-width="${v[2]}"/>
      <text x="${q.x}" y="${q.y + 1}" text-anchor="middle" dominant-baseline="middle" fill="#e2e8f0" font-size="16" font-weight="700" style="pointer-events:none">N${n.id}</text>
      <text x="${q.x}" y="${q.y + R + 19}" text-anchor="middle" fill="rgba(148,163,184,.92)" font-size="14.5" style="pointer-events:none">${q.name}</text>`;
    (teamAt[n.id] || []).forEach((t, i) => {
      s += `<circle cx="${q.x + R + 5 + i * 9}" cy="${q.y - R - 3}" r="4.5" fill="#3b82f6" stroke="#1e40af"/>`;
    });
    s += `</g>`;
  });
  s += `</svg>`;
  if (selectedNode) {
    const q = NODES[selectedNode], n = nodes.find(x => x.id === selectedNode);
    const top = q.y < 510 * 0.45;
    s += `<div class="tip" style="left:${q.x / 960 * 100}%;top:${q.y / 510 * 100}%;transform:${top ? "translate(-50%,18px)" : "translate(-50%,calc(-100% - 18px))"}">
      <div class="between"><b class="mono">N${selectedNode}</b><span class="tag" style="background:rgba(255,255,255,.07)">${n ? n.state : ""}</span></div>
      <p class="muted" style="margin:5px 0">${q.name}</p>
      <p class="muted">Connects to: ${neighbors(selectedNode).map(x => "N" + x).join(", ")}</p></div>`;
  }
  $("map").innerHTML = s;
}

function clickNode(id) { selectedNode = selectedNode === id ? null : id; drawMap(); }

// ---------- Search (one ring per click; the order comes from C++) ----------
function updateSearchBtn() {
  const b = $("searchBtn");
  const p = state.persons.find(x => x.id === selectedCase);
  b.disabled = !p || p.found || searching;
  b.textContent = searching ? "Searching…"
    : (p && p.found) ? "✅ Found"
    : (p && p.searched.length > 0) ? "🔍 Search Next Ring" : "🔍 Start Search";
}

async function startSearch() {
  const p = state.persons.find(x => x.id === selectedCase);
  if (searching || !p || p.found) return;
  searching = true; updateSearchBtn();
  const caseId = selectedCase;
  try {
    const res = await api("/api/search", { caseId });
    if (res.error) {
      alert(res.error);
    } else if (res.nothingLeft) {
      $("log").textContent = "Every location is already searched.";
    } else {
      anim = {}; animTeams = {}; animCase = caseId;
      const waves = {};
      res.steps.forEach(s => (waves[s.wave] = waves[s.wave] || []).push(s));
      for (const w of Object.keys(waves).map(Number).sort((a, b) => a - b)) {
        animTeams = {};
        waves[w].forEach(s => { anim[s.node] = "searching"; (animTeams[s.node] = animTeams[s.node] || []).push(s.team); });
        $("log").textContent = `Ring ${res.ring}, group ${w + 1}: ` + waves[w].map(s => `${s.team} → N${s.node}`).join(" · ");
        drawMap(); await sleep(1300);
        waves[w].forEach(s => anim[s.node] = "searched");
        drawMap(); await sleep(700);
      }
      if (res.found) {
        anim[res.foundAt] = "found";
        $("log").textContent = `FOUND at N${res.foundAt} · ${nname(res.foundAt)} (ring ${res.ring})`;
        drawMap(); await sleep(1500);
      } else {
        $("log").textContent = `Ring ${res.ring} searched. Person not found. Click "Search Next Ring".`;
      }
    }
  } catch (e) { alert("Could not reach the C++ server."); }
  anim = {}; animTeams = {}; animCase = null;
  searching = false;
  await loadState();
}

// ---------- Actions ----------
async function closeCase(id) {
  if (!confirm("Close this case? It will be deleted.")) return;
  await api("/api/close", { caseId: id });
  await loadState();
}

async function cancelLast(id) {
  const res = await api("/api/cancel", { caseId: id });
  if (res.error) alert(res.error);
  await loadState();
}

function fillNodeSelect(el) {
  el.innerHTML = Object.keys(NODES).map(i => `<option value="${i}">N${i} · ${NODES[i].name}</option>`).join("");
}

// register form
let urgency = 2;
$("urgBtns").addEventListener("click", e => {
  if (!e.target.dataset.u) return;
  urgency = Number(e.target.dataset.u);
  document.querySelectorAll("#urgBtns button").forEach(b => b.classList.toggle("on", b === e.target));
});
$("regForm").addEventListener("submit", async e => {
  e.preventDefault();
  try {
    const res = await api("/api/register", {
      name: $("fName").value, age: Number($("fAge").value), gender: $("fGender").value,
      description: $("fDesc").value, lastSeenNode: Number($("fNode").value), urgency
    });
    if (res.error) { alert(res.error); return; }
    selectedCase = res.id;
    e.target.reset(); urgency = 2;
    document.querySelectorAll("#urgBtns button").forEach(b => b.classList.toggle("on", b.dataset.u === "2"));
    await loadState();
    show("track");
  } catch (err) { alert("Could not reach the C++ server."); }
});

// sighting modal
function openModal() {
  if (!state.persons.length) { alert("Register a case first."); return; }
  const p = state.persons.find(x => x.id === selectedCase);
  if (p && p.found) { alert("This person has already been found."); return; }
  $("mCase").innerHTML = state.persons.filter(x => !x.found).map(x =>
    `<option value="${x.id}" ${x.id === selectedCase ? "selected" : ""}>${caseCode(x.id)} — ${h(x.name)}</option>`).join("");
  $("mTime").value = new Date().toTimeString().slice(0, 5);
  $("modal").classList.add("open");
}
function closeModal() { $("modal").classList.remove("open"); }
async function submitSighting() {
  const res = await api("/api/sighting", { caseId: Number($("mCase").value), node: Number($("mNode").value), time: $("mTime").value });
  if (res.error) { alert(res.error); return; }
  closeModal();
  await loadState();
}

// ---------- Clock and live timers ----------
setInterval(() => {
  const now = new Date();
  document.querySelectorAll("[data-clock]").forEach(e => e.textContent = now.toLocaleTimeString("en-US", { hour12:false }));
  document.querySelectorAll("[data-date]").forEach(e => e.textContent = now.toLocaleDateString("en-US", { weekday:"long", month:"long", day:"numeric" }));
  const extra = Math.floor((Date.now() - loadedAt) / 1000);
  document.querySelectorAll(".mt").forEach(e => e.textContent = fmt(Number(e.dataset.base) + extra));
}, 1000);

// ---------- Start ----------
$("log").insertAdjacentHTML("beforebegin",
  '<span><i style="background:rgba(250,204,21,.45);border-color:#facc15"></i>Found</span>' +
  '<span><i style="background:transparent;border-color:rgba(251,191,36,.7);border-style:dashed"></i>Earlier sighting</span>');
fillNodeSelect($("fNode"));
fillNodeSelect($("mNode"));
loadState();
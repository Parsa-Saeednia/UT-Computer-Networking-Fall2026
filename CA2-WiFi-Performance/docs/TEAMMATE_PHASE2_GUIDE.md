# Teammate Onboarding & Handover Guide: Phase 2 (Performance Anomaly & Airtime DRR)

**Course:** Computer Networks (Fall 2026 / 1405–1406) — University of Tehran  
**Instructors:** Prof. Nasser Yazdani | TAs: Sajjad Taghizadeh, Aliakbar Arshadi  
**Team Members:**  
- Parsa Saeednia (`810102460`) — *Phase 1 Lead*  
- Mohammad Hossein Mazaheri (`810102585`) — *Phase 2 Lead*

---

## 1. Project Status & What Has Been Done

- **Project Scaffolding:** Fully configured with OMNeT++ / INET project files (`.project`, `.cproject`, `.nedfolders`, `.oppbuildspec`).
- **Group Parameters:** Fixed for `810102460 810102585` in `params.ini` and `params.json`.
- **Phase 1 (DCF Contention & Hidden Terminals):** 
  - Completed all theoretical derivations (Q3-1 through Q3-10).
  - All simulations executed (`Timing`, `Sweep` 50 runs, `CwTrace`, `Hidden` 10 runs, `HiddenTrace`).
  - Analysis code and plots completed in `phase1/phase1.ipynb`.
  - Comprehensive analytical report written in `docs/` and artifact logs.

---

## 2. Your Mission: Phase 2 (30 Points)

Phase 2 investigates the **802.11 Performance Anomaly** (where slow stations transmitting at low rates monopolize the airtime and drag down high-speed stations), and requires implementing an **Airtime Deficit Round Robin (DRR) Scheduler** in C++ for the Access Point.

### 2.1 Key Group Parameters for Phase 2
- Frame payload: $L = 1350\text{ Bytes}$
- Number of stations: $n_2 = 4$
- Slow station rate (`host[0]`): $R_{slow} = \mathbf{9\text{ Mbps}}$
- Fast stations rate (`host[1..3]`): $R_{fast} = \mathbf{36\text{ Mbps}}$
- Server link: 1 Gbps wired Ethernet to AP
- Scheduler Quantum: $\mathbf{500\,\mu\text{s}}$

### 2.2 Files to Modify & Inspect in `phase2/`
1. `AirtimeDrrScheduler.cc` & `AirtimeDrrScheduler.h`:  
   **The only C++ code you need to write!** Currently contains simple round-robin; you will implement airtime DRR.
2. `AirtimeSchedulerBase.h`:  
   Read-only base class providing:
   - `getNumQueues()`: Returns number of per-station queues ($n_2 = 4$).
   - `hasPacket(i)`: Returns true if queue $i$ has pending packets.
   - `headAirtime(i)`: Returns transmission airtime of head packet without SIFS/ACK (as a `simtime_t`).
3. `omnetpp.ini`:  
   Pre-configured scenarios:
   - `[Config Uplink]`: Stations send to Server.
   - `[Config DownlinkFifo]`: Server sends to all stations via single shared FIFO queue at AP.
   - `[Config DownlinkAirtime]`: Server sends via `AirtimeFairQueue` using your `AirtimeDrrScheduler`.
4. `phase2.ipynb`:  
   Jupyter notebook for loading results, computing Jain's fairness index, comparing throughputs, and plotting.

### 2.3 Airtime DRR Algorithm Specification (from `CA2.pdf`, page 12)
```
schedulePacket():
    if no queue has a packet:
        return -1
    loop:
        if queue[current] is empty:
            deficit[current] = 0        # Idle queue saves nothing (no credit hoarding)
            current = next(current)
        else if deficit[current] <= 0:
            deficit[current] += quantum # Add quantum (500 us) per round
            current = next(current)
        else:
            deficit[current] -= headAirtime(current)
            return current
```
- Variables `current` and array `deficit` must be preserved between calls (`member variables` of `AirtimeDrrScheduler`).
- Default quantum $= 500\,\mu\text{s}$.
- Keep functions concise ($\le 20-25$ lines) and free of redundant inline comments.

---

## 3. Git & Branching Conventions

- **Repository:** Private GitHub repository shared with TAs (`AliakbarArshadi` and `sajadTaghizadeh`).
- **Phase 2 Branch Name:**  
  ```bash
  git checkout -b MohammadHosseinMazaheri/feature/ca2-phase2
  ```
- **Commit Format:** Conventional Commits:
  - `feat(phase2): implement airtime deficit round robin scheduler in C++`
  - `sim(phase2): run Uplink, DownlinkFifo, and DownlinkAirtime simulations`
  - `docs(phase2): complete analytical answers and report sections`

---

## 4. Compilation & Execution Commands

### 4.1 Building `libca2.so` in WSL2 / Linux
From the root project directory `CA2-WiFi-Performance/`:
```bash
# 1. Source OMNeT++ environment
source /home/<user>/omnetpp-6.4.0/setenv

# 2. Build the shared library with INET link
opp_makemake -f --deep --make-so -o ca2 -O out -DINET_IMPORT \
  -I$INET_ROOT/src -L$INET_ROOT/src -lINET
make MODE=release -j4
```

### 4.2 Running Phase 2 Simulations Headlessly
```bash
cd phase2

# Run Uplink (5 repetitions)
opp_runall -j4 opp_run -m -u Cmdenv -c Uplink -n ..:$INET_ROOT/src -l $INET_ROOT/src/INET -l ../libca2.so omnetpp.ini

# Run Downlink FIFO (5 repetitions)
opp_runall -j4 opp_run -m -u Cmdenv -c DownlinkFifo -n ..:$INET_ROOT/src -l $INET_ROOT/src/INET -l ../libca2.so omnetpp.ini

# Run Downlink Airtime Fair (5 repetitions)
opp_runall -j4 opp_run -m -u Cmdenv -c DownlinkAirtime -n ..:$INET_ROOT/src -l $INET_ROOT/src/INET -l ../libca2.so omnetpp.ini
```

---

## 5. Kickstarting Your Antigravity Session for Phase 2

When you open a new Antigravity chat to work on Phase 2, paste this exact prompt:

```text
Hello! I am Mohammad Hossein Mazaheri (810102585), working on Phase 2 of Computer Networks CA2 (Wi-Fi Network Performance). My teammate Parsa Saeednia (810102460) has already completed Phase 1 and set up the repository scaffolding.

Please read:
1. `docs/SPECIFICATION.md` and `docs/TEAMMATE_PHASE2_GUIDE.md`
2. `params.json` and `params.ini`
3. `CA2.pdf` (pages 10 to 14)

My task is Phase 2:
- Implement the Airtime Deficit Round Robin (DRR) Scheduler in `phase2/AirtimeDrrScheduler.cc` and `.h` using quantum = 500 us.
- Compile `libca2.so` using `opp_makemake` with INET 4.7.0.
- Execute simulations for `Uplink`, `DownlinkFifo`, and `DownlinkAirtime`.
- Answer all Phase 2 questions (Q4-1 through Q4-9).
- Update `phase2/phase2.ipynb` with data parsing, Jain's fairness index calculations, and plots.

Let's begin by reviewing the scheduler design and starting the C++ implementation on branch `MohammadHosseinMazaheri/feature/ca2-phase2`.
```

# Computer Assignment 2: Analysis and Evaluation of Wi-Fi Network Performance

**Course**: Computer Networks (Fall 2026 / 1405–1406)  
**Department**: Electrical and Computer Engineering, University of Tehran  
**Instructor**: Prof. Nasser Yazdani  
**Teaching Assistants**: Sajjad Taghizadeh, Aliakbar Arshadi  
**Target Environment**: OMNeT++ 6.4.0 + INET 4.7.0  

---

## 1. Project Overview

In shared wireless local area networks (IEEE 802.11 / Wi-Fi), stations access an unguided radio medium without a centralized arbiter. This assignment conducts analytical modeling, discrete-event simulation, and comparative evaluation across three core Wi-Fi performance challenges:

1. **Phase 1: Channel Contention, DCF & Slotted Contention Model (20 pts)**  
   Investigating Distributed Coordination Function (DCF), exponential backoff, collision dynamics, slotted contention mathematical modeling, and hidden terminal mitigation via RTS/CTS.
2. **Phase 2: Performance Anomaly & Airtime Fairness (30 pts)**  
   Investigating the classical 802.11 performance anomaly where low-rate stations degrade overall throughput, followed by the implementation of an Airtime Deficit Round Robin (DRR) packet scheduler in C++ for the Access Point.
3. **Phase 3: Station Roaming & The Sticky Client Problem (15 pts)**  
   Evaluating handover delays, channel scanning, and signal degradation under log-normal shadowing, and designing a hysteresis-based finite state machine (FSM) to resolve sticky client anomalies.

---

## 2. Group Scenario Parameters

Generated via `ca2_params.py` for students **810102460** and **810102585**:

| Parameter | Value | Unit | Description |
|:---|:---:|:---:|:---|
| **$L$** | `1350` | B | Payload per frame (Phase 1 & Phase 2) |
| **$R$** | `12` | Mbps | Data bitrate (802.11a base rate) |
| **$n_2$** | `4` | - | Stations in Phase 2 (`host[0]` is the slow station) |
| **$R_{slow}$** | `9` | Mbps | Bitrate of the slow station (`host[0]`) |
| **$R_{fast}$** | `36` | Mbps | Bitrate of the high-speed stations (`host[1..n2-1]`) |
| **$\alpha$** | `3.2` | - | Path-loss exponent (Phase 3) |
| **$\sigma$** | `3` | dB | Shadowing standard deviation (Phase 3) |
| **$v$** | `1.75` | m/s | Mobile station walking speed (Phase 3) |
| **$BI$** | `100` | ms | Beacon interval of both Access Points |
| **$n_{Ch}$** | `6` | - | Number of scanned channels ($0 \dots n_{Ch}-1$) |
| **$D$** | `51` | m | Distance between `ap1` and `ap2` in Phase 3 |

---

## 3. Phase 1: Channel Contention, DCF & Slotted Model

### 3.1 Scenario Configuration
- **Network**: `Contention.ned` under `phase1/`.
- **Topology**: One Access Point (AP) surrounded by $n$ stations placed symmetrically on a circle of radius 10 m.
- **Physical Layer**: 802.11a in the 5 GHz band; saturation conditions (infinite packet queues at all stations).
- **Configurations in `omnetpp.ini`**:
  - `Timing`: Single station with event logging (`.elog`) enabled.
  - `Hidden`: Two stations at 100 m on opposite sides of AP (out of carrier sensing range of each other).
  - `HiddenTrace`: 50 ms run with event logging for Sequence Chart visualization.
  - `Sweep`: Parameter sweep over $n \in \{1, 2, 3, 5, 8, 10, 15, 20, 25, 30\}$ with $\ge 5$ independent random seeds.

### 3.2 DCF Protocol Foundations
- **Carrier Sensing**: Senses medium for $DIFS = SIFS + 2 \cdot \sigma$.
- **Backoff**: Uniform random integer $k \in [0, CW]$, where $CW$ doubles upon unacknowledged frame ($CW_{min} \to CW_{max}$) and resets to $CW_{min}$ upon success.
- **PPDU Duration**:
  $$T_{PPDU}(B, r) = 20\,\mu\text{s} + 4\,\mu\text{s} \cdot \left\lceil \frac{16 + 8B + 6}{N_{DBPS}} \right\rceil$$
  where $B$ is the MAC frame size in bytes, $r$ is bitrate in Mbps, and $N_{DBPS} = 4r$.

### 3.3 Slotted Mathematical Model
- Slotted transmission probability $\tau$:
  $$\tau_0 = \frac{1}{1 + CW_{min}/2} = \frac{2}{CW_{min} + 2}$$
- Slot outcomes:
  $$P_{idle} = (1 - \tau)^n, \quad P_s = n\tau(1 - \tau)^{n-1}, \quad P_c = 1 - P_{idle} - P_s$$
- Network Throughput $S$:
  $$S = \frac{P_s \cdot 8L}{P_{idle}\sigma + P_s T_s + P_c T_c}$$
  where:
  $$T_s = DIFS + T_{DATA} + SIFS + T_{ACK}, \quad T_c = DIFS + T_{DATA}$$
- Collision probability estimation from simulation statistics:
  $$\hat{p} = \frac{N_{retry} + N_{drop}}{N_{retry} + N_{first}}, \quad \hat{\tau} = 1 - (1 - \hat{p})^{\frac{1}{n-1}}$$

### 3.4 Hidden Terminals & RTS/CTS
- Two stations at 100 m with detection threshold $-77\text{ dBm}$. Free-space path loss formula:
  $$P_r = P_t \left( \frac{\lambda}{4\pi d} \right)^2$$
- Compare collision and throughput with `rtsThreshold = 3000B` (RTS disabled) vs `rtsThreshold = 100B` (RTS enabled).
- Quantify Jain's Fairness Index:
  $$J = \frac{(\sum_{i=1}^n S_i)^2}{n \sum_{i=1}^n S_i^2}$$

---

## 4. Phase 2: Performance Anomaly & Airtime Fairness

### 4.1 Scenario Configuration
- **Network**: `Anomaly.ned` under `phase2/`.
- **Topology**: Server connected to AP via 1 Gbps wired Ethernet; $n_2 = 4$ wireless stations at 10 m radius.
- **Traffic**: Saturated UDP flow with payload $L = 1350\text{ B}$.
- **Rates**: `host[0]` transmits/receives at $R_{slow} = 9\text{ Mbps}$; `host[1..3]` at $R_{fast} = 36\text{ Mbps}$.
- **Configurations in `omnetpp.ini`**:
  - `Uplink`: Stations send to Server.
  - `DownlinkFifo`: Server sends to all stations via single shared FIFO queue at AP.
  - `DownlinkAirtime`: Server sends to all stations via `AirtimeFairQueue` using `AirtimeDrrScheduler`.

### 4.2 Airtime Deficit Round Robin (DRR) Scheduler
To eliminate performance anomaly without starving low-rate stations, the scheduler allocates equal airtime rather than equal packet counts:
- Base class: `AirtimeSchedulerBase` provides:
  - `getNumQueues()`
  - `hasPacket(i)`
  - `headAirtime(i)`: Transmission airtime of head packet without SIFS/ACK.
- `schedulePacket()` algorithm requirements:
  1. Returns `-1` only when all per-station queues are completely empty.
  2. Guarantees equal long-term airtime for all active queues.
  3. Resets deficit to `0` when an idle queue is encountered (no credit hoarding).
  4. Quantum: $\text{quantum} = 500\,\mu\text{s}$ added per round when $\text{deficit} \le 0$.
  5. Deducts `headAirtime(current)` upon serving a packet from queue `current`.

---

## 5. Phase 3: Station Roaming & Sticky Clients

### 5.1 Scenario Configuration
- **Network**: `Roaming.ned` under `phase3/`.
- **Topology**: Two APs (`ap1` on channel 1, `ap2` on channel 3) separated by distance $D = 51\text{ m}$.
- **Mobility**: Mobile station moves parallel to AP baseline from $x = 0$ to $x = D$ at speed $v = 1.75\text{ m/s}$.
- **Channel Model**: Log-distance path loss with log-normal shadowing ($X \sim \mathcal{N}(0, \sigma^2)$):
  $$P_r(d) = P_t - PL(d_0) - 10\alpha \log_{10}\left(\frac{d}{d_0}\right) + X$$
  where receiver sensitivity threshold is $S = -85\text{ dBm}$.

### 5.2 Sticky Client Anomaly & State Machine Design
- Standard INET `Ieee80211AgentSta` initiates scanning only after losing connection for $3.5 \times BI = 350\text{ ms}$, causing extreme sticky client behavior and voice interruption.
- Custom Roaming Decision Machine:
  - Moving average over $N$ beacon samples: $\bar{P}_{AP} = \frac{1}{N} \sum_{k=1}^N P_{AP}[k]$.
  - Handover initiation threshold $T$: Initiate scanning/handover when $\bar{P}_{current} < T$.
  - Hysteresis margin $H \in \{0, 2, 4, 6, 8, 10\}\text{ dB}$: Handover to candidate AP only when $\bar{P}_{candidate} \ge \bar{P}_{current} + H$.

---

## 6. Deliverables & Submission Checklist

### Primary Archive: `CA2_<SID1>_<SID2>.zip`
1. **`report.pdf`**: Comprehensive academic report with:
   - Group parameter table.
   - Predictions vs. experimental results tables for all 3 phases.
   - Analytical calculations and equations.
   - Simulation charts with 95% confidence intervals.
   - Sequence Chart annotated captures.
2. **`ca2/` Project Directory**:
   - `phase1/`: `Contention.ned`, `omnetpp.ini`, `phase1.ipynb`, `results/` (`.sca`, `.vec`, `.elog` for short trace).
   - `phase2/`: `Anomaly.ned`, `AirtimeFairQueue.ned`, `AirtimeSchedulerBase.*`, `AirtimeDrrScheduler.cc` & `.h` (completed), `omnetpp.ini`, `phase2.ipynb`, `results/`.
   - `phase3/`: `Roaming.ned`, `omnetpp.ini`, `phase3.ipynb`, `results/`.
   - Root: `package.ned`, `ca2_params.py`, `params.ini`, `params.json`, `.project`, `.cproject`, `.nedfolders`, `.oppbuildspec`.
3. **Excluded Items**:
   - Do **NOT** include `out/`, `Makefile`, shared libraries (`libca2.so`), or full OMNeT++/INET source folders.

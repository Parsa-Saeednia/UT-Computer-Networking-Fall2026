# Computer Networks — Computer Assignment 2
## Analysis and Evaluation of Wi-Fi Network Performance

**Department of Electrical and Computer Engineering**  
**University of Tehran**  

- **Instructor**: Prof. Nasser Yazdani  
- **Teaching Assistants**: Sajjad Taghizadeh, Aliakbar Arshadi  
- **Authors**: Parsa Saeednia (810102460), Mohammad Hossein Mazaheri (810102585)  
- **Simulation Platform**: OMNeT++ 6.4.0 + INET 4.7.0  

---

## 1. Directory Structure

```text
CA2-WiFi-Performance/
├── docs/
│   └── SPECIFICATION.md          # Comprehensive assignment specification and formulas
├── phase1/
│   ├── Contention.ned            # Saturated N-station star topology on circular boundary
│   ├── omnetpp.ini               # Simulation configs: Timing, Hidden, HiddenTrace, Sweep
│   ├── phase1.ipynb              # Analysis notebook for DCF contention and analytical model
│   └── results/                  # Simulation output vectors (.vec) and scalars (.sca)
├── phase2/
│   ├── AirtimeDrrScheduler.cc/.h # C++ implementation of Deficit Round Robin (DRR) scheduler
│   ├── AirtimeFairQueue.ned      # Per-destination multi-queue compound module
│   ├── AirtimeSchedulerBase.*    # Base scheduling framework provided by INET
│   ├── Anomaly.ned               # Heterogeneous rate topology (1 slow + (n-1) fast stations)
│   ├── RateTableDcf.ned          # Rate selection module for asymmetric link data rates
│   ├── omnetpp.ini               # Configs: Uplink, DownlinkFifo, DownlinkAirtime
│   ├── phase2.ipynb              # Anomaly calculations, Jain's index & delay analysis
│   └── results/                  # Simulation metrics
├── phase3/
│   ├── Roaming.ned               # Dual-AP corridor scenario with mobile station
│   ├── omnetpp.ini               # Configs: Walk, Survey, ScanNeighbor
│   ├── phase3.ipynb              # Shadowing analysis, signal trajectory & FSM handover evaluation
│   └── results/                  # Mobility logs and signal power records
├── report/                       # Report generation assets, figures and LaTeX/PDF documents
├── ca2_params.py                 # Parameter generator script for student IDs
├── package.ned                   # Top-level OMNeT++ package declaration
├── params.ini                    # Group-specific scenario parameters (included by omnetpp.ini)
├── params.json                   # Structured JSON parameters read by Jupyter analysis notebooks
└── README.md                     # This academic guide
```

---

## 2. Group Parameters (810102460 & 810102585)

Generated via `python3 ca2_params.py 810102460 810102585`:

| Parameter | Value | Description |
|:---|:---:|:---|
| **$L$** | 1350 B | Payload per frame |
| **$R$** | 12 Mbps | 802.11a base bitrate |
| **$n_2$** | 4 | Number of stations in Phase 2 (`host[0]` is slow) |
| **$R_{slow}$** | 9 Mbps | Bitrate of slow station (`host[0]`) |
| **$R_{fast}$** | 36 Mbps | Bitrate of fast stations (`host[1..3]`) |
| **$\alpha$** | 3.2 | Path-loss exponent |
| **$\sigma$** | 3 dB | Log-normal shadowing standard deviation |
| **$v$** | 1.75 m/s | Mobile station movement speed |
| **$BI$** | 100 ms | Beacon interval for both APs |
| **$n_{Ch}$** | 6 | Channels scanned during active probe ($0 \dots 5$) |
| **$D$** | 51 m | Distance between `ap1` and `ap2` |

---

## 3. Environment Setup & Execution

### 3.1 Prerequisites
- **OMNeT++**: Version `6.4.0`
- **INET Framework**: Version `4.7.0`
- **Python**: $\ge 3.8$ with `numpy`, `scipy`, `pandas`, `matplotlib`, and `jupyter`

In each terminal session, initialize the OMNeT++ environment:
```bash
cd <path-to-omnetpp-6.4.0>
source setenv
```

### 3.2 Compiling Phase 2 C++ Scheduler
Phase 2 includes custom C++ scheduler code (`AirtimeDrrScheduler.cc`). Build the project from terminal or OMNeT++ IDE:

```bash
cd CA2-WiFi-Performance
opp_makemake -f --deep --make-so -o ca2 -O out -DINET_IMPORT \
  -I$INET_ROOT/src -L$INET_ROOT/src -lINET'$(D)'
make MODE=release -j$(nproc)
```

### 3.3 Running Simulations
#### Interactive GUI Mode (Qtenv)
```bash
cd CA2-WiFi-Performance/phase1
inet -u Qtenv omnetpp.ini
```

#### Headless Batch Parameter Sweeps (Cmdenv)
To run parameter studies across multiple CPU cores in parallel:
```bash
cd CA2-WiFi-Performance/phase1
opp_runall -j$(nproc) inet -u Cmdenv -n .. -c Sweep omnetpp.ini
```

---

## 4. Academic Integrity & LLM Usage Disclosure
All simulation code, analytical models, and reports are developed for the Computer Networks course at the University of Tehran. Generative AI tools have been utilized for scaffolding, repository structuring, and mathematical formula verification in compliance with course policies.

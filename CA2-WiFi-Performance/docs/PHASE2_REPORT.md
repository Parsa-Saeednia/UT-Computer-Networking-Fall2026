# Computer Assignment 2 — Phase 2 Complete Report
## The 802.11 Performance Anomaly & Airtime Deficit Round Robin (DRR) Scheduling

**Course:** Computer Networks (Fall 2026 / 1405–1406) — University of Tehran  
**Instructor:** Prof. Nasser Yazdani  
**Teaching Assistants:** Sajjad Taghizadeh, Aliakbar Arshadi  
**Students:**  
- Parsa Saeednia (`810102460`)  
- Mohammad Hossein Mazaheri (`810102585`)  

**Assigned Phase 2 Parameters (`params.json` / `params.ini`):**  
- Data Payload: $L = 1350\text{ Bytes}$ (UDP payload)  
- Station Count: $n_2 = 4$ (`host[0]` slow, `host[1..3]` fast, placed on a $10\text{ m}$ circle around `ap`)  
- Slow Bitrate (`host[0]`): $R_{slow} = 9\text{ Mbps}$ (802.11a optional rate $\implies$ ACK at $6\text{ Mbps}$)  
- Fast Bitrate (`host[1..3]`): $R_{fast} = 36\text{ Mbps}$ (802.11a optional rate $\implies$ ACK at $24\text{ Mbps}$)  
- Wired Link (`server` $\leftrightarrow$ `ap`): $1\text{ Gbps}$ Ethernet  
- Default DRR Scheduler Quantum: $500\,\mu\text{s}$  
- Measurement Window: $T = 10.0\text{ s}$ ($12\text{ s}$ simulated minus $2\text{ s}$ warm-up, 5 repetitions per configuration, $t_{4,0.975} = 2.776$)

---

## 1. Summary of Pre-Simulation Predictions vs. Simulation Results

| Scenario / Question | Pre-Simulation Prediction | Theoretical Reasoning | Simulation Result | Cause of Discrepancy / Validation |
|:---|:---|:---|:---:|:---|
| **4.1 (`Uplink` vs `DownlinkFifo`)** | In both cases, all stations achieve nearly equal throughput ($J_S \approx 1$), while `host[0]` consumes $>50\%$ of active airtime ($J_a \ll 1$). `Uplink` achieves lower total throughput than `DownlinkFifo`. | DCF equalizes long-term frame access probability across saturated stations regardless of bitrate. In `Uplink`, 4 senders contend and collide; in `DownlinkFifo`, only the AP transmits (zero collisions). | `Uplink`: $S_{tot} = 13.7240\text{ Mbps}$, $J_S = 0.9939$, $J_a = 0.7355$<br>`DownlinkFifo`: $S_{tot} = 14.9591\text{ Mbps}$, $J_S = 0.9997$, $J_a = 0.6882$ | **Validated:** `host[0]` consumes $54.14\%$ of active channel airtime in `DownlinkFifo`, while collisions in `Uplink` lower $S_{tot}$ by $8.26\%$. |
| **4.1 (`DownlinkFifo`, Q4-3)** | Each station obtains $S_i \approx 3.7422\text{ Mbps}$ ($S_{tot} \approx 14.9688\text{ Mbps}$); removing `host[0]` raises each fast station to $7.4766\text{ Mbps}$ ($\approx 50\%$ penalty). | With zero collisions and equal frame counts across stations in a saturated FIFO queue, each cycle of 4 frames spends $T_{ex,0} + 3 T_{ex,fast} = 2886.0\,\mu\text{s}$ to deliver $4L$ bytes. | $S_{tot} = 14.9591 \pm 0.1107\text{ Mbps}$ ($S_i \approx 3.7398\text{ Mbps}$, error $0.065\%$) | **Validated:** Analytical model matches simulation within $0.065\%$; `host[0]` cuts fast-station throughput by $50.04\%$. |
| **4.2 (`DownlinkAirtime`, Q4-4/4-5)** | Equal `headAirtime` allocation yields $S_0 = 1.5552\text{ Mbps}$, $S_{1..3} = 5.9246\text{ Mbps}$, $S_{tot} = 19.3292\text{ Mbps}$ ($+29.1\%$ vs FIFO), and $J_a \approx 1.0$. | DRR allocates equal $N_i T_{DATA,i}$ across backlogged stations; fast stations send $1280/336 \approx 3.81\times$ more frames than `host[0]`. | $S_0 = 1.5546\text{ Mbps}$, $S_{1..3} = 5.9232\text{ Mbps}$, $S_{tot} = 19.3240 \pm 0.0063\text{ Mbps}$ ($+29.18\%$), $J_a = 0.9989$ | **Validated:** $0.027\%$ total error ($<1\%$ requirement). $J_a = 0.9989 < 1$ because `headAirtime` omits $\text{SIFS}+T_{ACK}$, which fast frames incur $3.81\times$ more often. |
| **4.3 (Queueing Delay, Q4-7)** | Little's Law predicts uniform $\approx 72.20\text{ ms}$ delay in `DownlinkFifo` ($K=100$) vs $\approx 347.37\text{ ms}$ (`host[0]`) and $\approx 91.17\text{ ms}$ (`host[1..3]`) in `DownlinkAirtime` ($K_i=50$). | Under saturation, drop-tail queues remain full ($L_q \approx K$). Reducing `host[0]`'s service rate from $347\text{ pkt/s}$ to $144\text{ pkt/s}$ with $K_0=50$ inflates $W = K_0/\lambda_0$. | FIFO mean: $74.26\text{ ms}$ (`host[0]`), $\approx 73.30\text{ ms}$ (`host[1..3]`)<br>DRR mean: $349.09\text{ ms}$ (`host[0]`), $\approx 92.09\text{ ms}$ (`host[1..3]`) | **Validated:** Matches Little's Law within $\sim 1\text{--}2\text{ ms}$ (attributable to wired hop, MAC service time, and in-progress frame). |
| **4.3 (Low-Load `host[0]`, Q4-8)** | Under `exponential(25ms)` ($\approx 0.432\text{ Mbps}$), `host[0]` receives 100% of offered load in DRR with tiny delay ($\sim 6\text{ ms}$), whereas shared FIFO floods and tail-drops `host[0]` packets with high delay ($\sim 50\text{ ms}$). | In DRR, `queue[0]` is isolated and rarely holds $>1$ packet, while its idle deficit resets to $0$ without starving light traffic. In FIFO, $60{,}000\text{ pkt/s}$ from `host[1..3]` keep the shared queue full. | FIFO: `host[0]` starved to $0.0132\text{ Mbps}$ ($50.02\text{ ms}$ delay)<br>DRR: `host[0]` gets $0.4389\text{ Mbps}$ ($100\%$ of offered $0.4383\text{ Mbps}$, $6.05\text{ ms}$ delay) | **Validated:** Queue isolation + DRR delivers $100\%$ of `host[0]`'s load with $8.3\times$ lower delay while giving fast stations $7.037\text{ Mbps}$ each. |

---

## 2. Part 4.1: Performance Anomaly in Uplink & Downlink

### Question 4-1: Encapsulation Stack, MPDU Size, and PPDU Air-Time Durations

In Phase 2 (`Anomaly.ned`), `UdpBasicApp` traffic traverses the UDP/IPv4 stack before reaching the 802.11a interface (`Ieee80211LlcLpd`, INET 4.7.0):

1. **UDP Application Payload:** $L = 1350\text{ Bytes}$ (`UdpBasicAppData`).
2. **UDP Header (`UdpHeader`):** $8\text{ Bytes}$ $\to 1358\text{ Bytes}$.
3. **IPv4 Header (`Ipv4Header`):** $20\text{ Bytes}$ $\to 1378\text{ Bytes}$.
4. **LLC + SNAP Header (`Ieee8022LlcSnapHeader`):** $8\text{ Bytes}$ ($3\text{ B}$ LLC + $5\text{ B}$ SNAP OUI/EtherType `0x0800` inserted by `Ieee8022Llc` for IPv4 packets) $\to 1386\text{ Bytes}$.
5. **IEEE 802.11 MAC Data Header (`Ieee80211DataHeader`):** $24\text{ Bytes}$ (non-QoS, 3-address format).
6. **IEEE 802.11 MAC Trailer (`Ieee80211MacTrailer` / FCS):** $4\text{ Bytes}$.

Therefore, the total MAC Protocol Data Unit (MPDU) size and ACK frame size are:
$$B_{DATA} = 1350 + 8 + 20 + 8 + 24 + 4 = \mathbf{1414\text{ Bytes}} \quad (11312\text{ bits}), \qquad B_{ACK} = \mathbf{14\text{ Bytes}} \quad (112\text{ bits})$$

Using the 802.11a 20 MHz OFDM PPDU duration formula ($N_{DBPS} = 4r$, $\sigma = 9\,\mu\text{s}$, $\text{SIFS} = 16\,\mu\text{s}$, $\text{DIFS} = 34\,\mu\text{s}$, $CW_{min} = 15$):
$$T_{PPDU}(B, r) = 20\,\mu\text{s} + 4\,\mu\text{s} \cdot \left\lceil \frac{16 + 8B + 6}{4r} \right\rceil$$

Because IEEE 802.11a mandatory basic rates are $\{6, 12, 24\}\text{ Mbps}$, INET's `RateSelection::computeResponseAckFrameMode` downgrades optional data rates to the highest slower mandatory rate for control responses (`9 Mbps` $\to$ `6 Mbps`, `36 Mbps` $\to$ `24 Mbps`):

- **Slow Station (`host[0]`, $R_{slow} = 9\text{ Mbps}$, $R_{ACK,slow} = 6\text{ Mbps}$):**
  $$T_{DATA,slow} = 20 + 4 \left\lceil \frac{16 + 8(1414) + 6}{36} \right\rceil = 20 + 4 \left\lceil \frac{11334}{36} \right\rceil = 20 + 4(315) = \mathbf{1280.0\,\mu\text{s}}$$
  $$T_{ACK,slow} = 20 + 4 \left\lceil \frac{16 + 8(14) + 6}{24} \right\rceil = 20 + 4 \left\lceil \frac{134}{24} \right\rceil = 20 + 4(6) = \mathbf{44.0\,\mu\text{s}}$$
  - Active exchange airtime: $T_{air,slow} = T_{DATA,slow} + \text{SIFS} + T_{ACK,slow} = 1280 + 16 + 44 = \mathbf{1340.0\,\mu\text{s}}$
  - Mean DCF cycle duration: $T_{ex,slow} = \text{DIFS} + \frac{CW_{min}}{2}\sigma + T_{air,slow} = 34 + 67.5 + 1340 = \mathbf{1441.5\,\mu\text{s}}$

- **Fast Stations (`host[1..3]`, $R_{fast} = 36\text{ Mbps}$, $R_{ACK,fast} = 24\text{ Mbps}$):**
  $$T_{DATA,fast} = 20 + 4 \left\lceil \frac{11334}{144} \right\rceil = 20 + 4(79) = \mathbf{336.0\,\mu\text{s}}$$
  $$T_{ACK,fast} = 20 + 4 \left\lceil \frac{134}{96} \right\rceil = 20 + 4(2) = \mathbf{28.0\,\mu\text{s}}$$
  - Active exchange airtime: $T_{air,fast} = T_{DATA,fast} + \text{SIFS} + T_{ACK,fast} = 336 + 16 + 28 = \mathbf{380.0\,\mu\text{s}}$
  - Mean DCF cycle duration: $T_{ex,fast} = \text{DIFS} + \frac{CW_{min}}{2}\sigma + T_{air,fast} = 34 + 67.5 + 380 = \mathbf{481.5\,\mu\text{s}}$

---

### Question 4-2: `Uplink` vs. `DownlinkFifo` Simulation Results

From 5 independent repetitions ($T = 10\text{ s}$, 95% confidence intervals via $t_{4,0.975} = 2.776$):

| Configuration / Station | Bitrate (Mbps) | Throughput $S_i \pm 95\%$ CI (Mbps) | Channel Airtime $a_i \pm 95\%$ CI | Share of Active Airtime ($a_i / \sum a_j$) |
|:---|:---:|:---:|:---:|:---:|
| **`Uplink` (`host[0]`)** | $9$ | $3.1208 \pm 0.1163$ | $0.3872 \pm 0.0144$ | $50.93\%$ |
| **`Uplink` (`host[1]`)** | $36$ | $3.4098 \pm 0.2926$ | $0.1200 \pm 0.0103$ | $15.78\%$ |
| **`Uplink` (`host[2]`)** | $36$ | $3.7880 \pm 0.1712$ | $0.1333 \pm 0.0060$ | $17.53\%$ |
| **`Uplink` (`host[3]`)** | $36$ | $3.4055 \pm 0.1142$ | $0.1198 \pm 0.0040$ | $15.76\%$ |
| **`Uplink` Total / Fairness** | — | **$S_{tot} = 13.7240 \pm 0.2473$** | **$\sum a_i = 0.7603$** | **$J_S = 0.9939 \pm 0.0034$, $J_a = 0.7355 \pm 0.0262$** |
| **`DownlinkFifo` (`host[0]`)** | $9$ | $3.7524 \pm 0.0533$ | $0.4656 \pm 0.0066$ | **$54.14\%$** |
| **`DownlinkFifo` (`host[1]`)** | $36$ | $3.7552 \pm 0.0872$ | $0.1321 \pm 0.0031$ | $15.37\%$ |
| **`DownlinkFifo` (`host[2]`)** | $36$ | $3.7262 \pm 0.1070$ | $0.1311 \pm 0.0038$ | $15.25\%$ |
| **`DownlinkFifo` (`host[3]`)** | $36$ | $3.7254 \pm 0.1186$ | $0.1311 \pm 0.0042$ | $15.24\%$ |
| **`DownlinkFifo` Total / Fairness** | — | **$S_{tot} = 14.9591 \pm 0.1107$** | **$\sum a_i = 0.8599$** | **$J_S = 0.9997 \pm 0.0001$, $J_a = 0.6882 \pm 0.0105$** |

- **Why throughput is nearly equal while channel airtime differs drastically:** Both DCF channel access (in `Uplink`) and single-queue FIFO packet service under symmetric Poisson arrivals (in `DownlinkFifo`) provide **frame-level fairness**: every station gets roughly the same number of transmitted frames $N_i$ (yielding $J_S \approx 0.994\text{--}0.9997$). However, each frame sent to or from `host[0]` occupies the wireless medium for $T_{air,slow} = 1340\,\mu\text{s}$ vs. $T_{air,fast} = 380\,\mu\text{s}$ for a fast station ($3.53\times$ longer), causing `host[0]` to monopolize channel time ($J_a = 0.6882$ in FIFO).
- **Channel time spent on slow frames in `DownlinkFifo`:** `host[0]` consumes **$a_0 = 0.4656$ ($46.56\%$)** of total wall-clock measurement time $T = 10\text{ s}$, which equals **$54.14\%$** of all active frame-exchange airtime ($\frac{0.4656}{0.8599}$, matching the analytical ratio $\frac{1340}{1340 + 3 \times 380} = 54.03\%$).

---

### Question 4-3: Zero Collisions & Analytical Model for `DownlinkFifo`

- **Why zero collisions occur in `DownlinkFifo`:** Only the AP (`ap`) transmits data frames on the wireless medium; the four stations (`host[0..3]`) only transmit deterministic `ACK` frames exactly one $\text{SIFS}$ after receiving a unicast frame addressed to them. Because there is only one contending DCF transmitter, simultaneous transmission attempts cannot happen ($P_c = 0$).
- **Derivation of Per-Station Throughput:** Because the server generates saturated, identical Poisson traffic (`exponential(50us)`) for all 4 stations into one shared drop-tail FIFO queue, each station receives an equal average share of frames ($N_0 \approx N_1 \approx N_2 \approx N_3 = N_s$). One complete cycle delivering 1 frame to each of the $n$ stations occupies expected time $\sum_{j=0}^{n-1} \left(\text{DIFS} + \frac{CW_{min}}{2}\sigma + T_{DATA,j} + \text{SIFS} + T_{ACK,j}\right)$ and delivers $8L$ payload bits to each station. Therefore:
  $$S = \frac{8L}{\sum_{j=0}^{n-1} \left(\text{DIFS} + \frac{CW_{min}}{2}\sigma + T_{DATA,j} + \text{SIFS} + T_{ACK,j}\right)}$$

- **Numerical Evaluation & Comparison:**
  - **With `host[0]` ($n=4$):**
    $$\sum_{j=0}^{3} T_{ex,j} = 1441.5 + 3(481.5) = 2886.0\,\mu\text{s}$$
    $$S_{model} = \frac{8 \times 1350\text{ bits}}{2886.0\,\mu\text{s}} = \mathbf{3.7422\text{ Mbps}} \implies S_{tot,model} = 4 \times 3.7422 = \mathbf{14.9688\text{ Mbps}}$$
    Compared to the 5-run simulation mean ($S_{tot,sim} = 14.9591\text{ Mbps}$), the relative error is only **$0.065\%$**.
  - **Without `host[0]` ($n - 1 = 3$ fast stations only):**
    $$\sum_{j=1}^{3} T_{ex,j} = 3(481.5) = 1444.5\,\mu\text{s}$$
    $$S_{fast\_only} = \frac{10800\text{ bits}}{1444.5\,\mu\text{s}} = \mathbf{7.4766\text{ Mbps}} \implies S_{tot,fast\_only} = \mathbf{22.4299\text{ Mbps}}$$
  - **Throughput Loss Inflicted on Each Fast Station:**
    $$\frac{7.4766 - 3.7422}{7.4766} = \mathbf{49.95\%} \text{ (analytical)}, \qquad \frac{7.4766 - 3.7356}{7.4766} = \mathbf{50.04\%} \text{ (simulation)}$$
    The single $9\text{ Mbps}$ station cuts the throughput of every $36\text{ Mbps}$ station in half!

---

## 3. Part 4.2: Airtime-Fair Scheduler Implementation

### Question 4-4: Analytical Throughput Prediction for `DownlinkAirtime`

In `DownlinkAirtime`, `AirtimeDrrScheduler` equalizes the cumulative `headAirtime` allocated to all backlogged stations, where `headAirtime(i)` $= T_{DATA,i}$. Thus, over measurement interval $T$, $N_j T_{DATA,j} = C$ for some constant $C$ across all $j \in \{0, \dots, n-1\}$, giving $N_j = C / T_{DATA,j}$. Since each frame still incurs a full collision-free DCF cycle $T_{ex,j} = \text{DIFS} + \frac{CW_{min}}{2}\sigma + T_{DATA,j} + \text{SIFS} + T_{ACK,j}$, total elapsed time satisfies:
$$\sum_{j=0}^{n-1} N_j T_{ex,j} = T \implies C \sum_{j=0}^{n-1} \frac{T_{ex,j}}{T_{DATA,j}} = T \implies N_i = \frac{T / T_{DATA,i}}{\sum_{j=0}^{n-1} \frac{T_{ex,j}}{T_{DATA,j}}}$$
Hence the predicted throughput of station $i$ is:
$$S_i = \frac{8L \cdot N_i}{T} = \frac{8L / T_{DATA,i}}{\sum_{j=0}^{n-1} \frac{T_{ex,j}}{T_{DATA,j}}}$$

Substituting our group parameters ($T_{DATA,0} = 1280\,\mu\text{s}$, $T_{DATA,1..3} = 336\,\mu\text{s}$, $T_{ex,0} = 1441.5\,\mu\text{s}$, $T_{ex,1..3} = 481.5\,\mu\text{s}$):
$$\sum_{j=0}^{3} \frac{T_{ex,j}}{T_{DATA,j}} = \frac{1441.5}{1280} + 3 \times \frac{481.5}{336} = 1.12617 + 4.29911 = 5.42528$$
$$S_{0,pred} = \frac{10800 / 1280}{5.42528} = \mathbf{1.5552\text{ Mbps}}, \qquad S_{1..3,pred} = \frac{10800 / 336}{5.42528} = \mathbf{5.9246\text{ Mbps}}$$
$$S_{tot,pred} = 1.5552 + 3(5.9246) = \mathbf{19.3292\text{ Mbps}}$$

---

### Question 4-5: `DownlinkAirtime` Simulation Results & Comparison

| Station | Predicted $S_{pred}$ (Mbps) | Measured $S_i \pm 95\%$ CI (Mbps) | Rel. Error | Channel Airtime $a_i \pm 95\%$ CI |
|:---|:---:|:---:|:---:|:---:|
| **`host[0]` (9 Mbps)** | $1.5552$ | $1.5546 \pm 0.0007$ | $0.043\%$ | $0.1929 \pm 0.0001$ |
| **`host[1]` (36 Mbps)** | $5.9246$ | $5.9229 \pm 0.0022$ | $0.029\%$ | $0.2084 \pm 0.0001$ |
| **`host[2]` (36 Mbps)** | $5.9246$ | $5.9234 \pm 0.0020$ | $0.022\%$ | $0.2084 \pm 0.0001$ |
| **`host[3]` (36 Mbps)** | $5.9246$ | $5.9232 \pm 0.0018$ | $0.025\%$ | $0.2084 \pm 0.0001$ |
| **Total / Fairness** | **$19.3292$** | **$19.3240 \pm 0.0063$** | **$0.027\%$** | **$\sum a_i = 0.8181$, $J_S = 0.8671$, $J_a = 0.9989$** |

- **Prediction Accuracy:** Every station matches the Question 4-4 prediction within **$0.022\%\text{--}0.043\%$** (far tighter than the $1\%$ threshold).
- **Impact vs. `DownlinkFifo`:**
  - Total network throughput **increased by $+29.18\%$** ($14.9591 \to 19.3240\text{ Mbps}$), while fast stations surged from $3.7356\text{ Mbps}$ to $5.9232\text{ Mbps}$ ($+58.56\%$).
  - Slow station (`host[0]`) throughput **decreased by $-58.57\%$** ($3.7524 \to 1.5546\text{ Mbps}$), trading its prior unfair airtime monopoly for its fair $25\%$ share.
- **Fairness Indices & Why $J_a \neq 1.0$:** Throughput fairness drops intentionally to $J_S = 0.8671$ (reflecting the $4\times$ bitrate disparity), while airtime fairness jumps from $0.6882$ to **$J_a = 0.9989$**. $J_a$ is slightly below $1.0000$ because `headAirtime(i)` measures only the DATA PPDU duration ($T_{DATA,i}$), whereas the metric $a_i$ also includes $\text{SIFS} + T_{ACK,i}$. Because fast stations transmit $5485 / 1439 \approx 3.81\times$ more frames than `host[0]`, they incur $3.81\times$ more $(\text{SIFS} + T_{ACK})$ overhead events per unit of data airtime ($\frac{T_{air,fast}}{T_{DATA,fast}} = \frac{380}{336} = 1.1310$ vs. $\frac{T_{air,slow}}{T_{DATA,slow}} = \frac{1340}{1280} = 1.0469$), giving each fast station $a_k = 0.2084$ vs. $a_0 = 0.1929$.

---

### Question 4-6: `schedulePacket()` Explanation & 10-Decision Manual Trace

#### Scheduler Operation (`phase2/AirtimeDrrScheduler.cc`)
`AirtimeDrrScheduler` maintains a round-robin pointer `current` and a persistent vector `deficit[i]` (initialized to `0`) for all $n$ per-station queues:
1. **Empty Check:** If `hasAnyPacket()` is false across all queues, return `-1` immediately.
2. **Idle Queue Reset:** If `!hasPacket(current)`, reset `deficit[current] = SIMTIME_ZERO` (preventing idle stations from hoarding credit) and advance `current = (current + 1) % n`.
3. **Quantum Replenishment:** Otherwise, if `deficit[current] <= SIMTIME_ZERO`, grant one `quantum` ($500\,\mu\text{s}$) to `deficit[current]` and advance `current = (current + 1) % n`.
4. **Packet Service:** Otherwise (`deficit[current] > SIMTIME_ZERO`), deduct `headAirtime(current)` from `deficit[current]` and return `current` without advancing `current`, allowing a fast station to burst multiple frames if remaining positive deficit permits, while allowing a slow station with `headAirtime > quantum` to transmit once and carry negative debt until repaid across subsequent rounds.

#### Manual Trace of First 10 Decisions (2 Backlogged Stations: `host[0]` @ $9\text{ Mbps}$, $1280\,\mu\text{s}$; `host[1]` @ $36\text{ Mbps}$, $336\,\mu\text{s}$; $\text{quantum} = 500\,\mu\text{s}$)

| Decision # | `current` at Entry | `deficit` Before ($\mu\text{s}$) | Internal Loop Steps (`current` / `deficit` updates) | Served Queue | Cost (`headAirtime`) | `deficit` After ($\mu\text{s}$) | `current` at Exit |
|:---:|:---:|:---:|:---|:---:|:---:|:---:|:---:|
| **1** | `0` | `[0, 0]` | `0` ($0\to+500$, `curr=1`), `1` ($0\to+500$, `curr=0`), `0` ($500>0$) | `host[0]` | $1280$ | `[-780, 500]` | `0` |
| **2** | `0` | `[-780, 500]` | `0` ($-780\to-280$, `curr=1`), `1` ($500>0$) | `host[1]` | $336$ | `[-280, 164]` | `1` |
| **3** | `1` | `[-280, 164]` | `1` ($164>0$) | `host[1]` | $336$ | `[-280, -172]` | `1` |
| **4** | `1` | `[-280, -172]` | `1` ($-172\to+328$, `curr=0`), `0` ($-280\to+220$, `curr=1`), `1` ($328>0$) | `host[1]` | $336$ | `[220, -8]` | `1` |
| **5** | `1` | `[220, -8]` | `1` ($-8\to+492$, `curr=0`), `0` ($220>0$) | `host[0]` | $1280$ | `[-1060, 492]` | `0` |
| **6** | `0` | `[-1060, 492]` | `0` ($-1060\to-560$, `curr=1`), `1` ($492>0$) | `host[1]` | $336$ | `[-560, 156]` | `1` |
| **7** | `1` | `[-560, 156]` | `1` ($156>0$) | `host[1]` | $336$ | `[-560, -180]` | `1` |
| **8** | `1` | `[-560, -180]` | `1` ($-180\to+320$, `curr=0`), `0` ($-560\to-60$, `curr=1`), `1` ($320>0$) | `host[1]` | $336$ | `[-60, -16]` | `1` |
| **9** | `1` | `[-60, -16]` | `1` ($-16\to+484$, `curr=0`), `0` ($-60\to+440$, `curr=1`), `1` ($484>0$) | `host[1]` | $336$ | `[440, 148]` | `1` |
| **10** | `1` | `[440, 148]` | `1` ($148>0$) | `host[1]` | $336$ | `[440, -188]` | `1` |

*(Across these 10 decisions, `host[0]` is served $2$ times [$2560\,\mu\text{s}$] and `host[1]` is served $8$ times [$2688\,\mu\text{s}$], and `host[0]` enters decision 11 with $+440\,\mu\text{s}$ ready to fire next.)*

---

## 4. Part 4.3: Analysis & Limitations

### Question 4-7: End-to-End Delay Prediction via Little's Law

Under saturated `exponential(50us)` arrivals ($20{,}000\text{ pkt/s}$ per flow), the AP's drop-tail queue stays continuously full ($L_q \approx K$). Applying Little's Law ($W = L_q / \lambda$):
- **`DownlinkFifo` (shared queue capacity $K_{FIFO} = 100$ packets):** Total departure rate $\lambda_{tot} = \frac{14.9591\text{ Mbps}}{10800\text{ bits}} = 1385.10\text{ pkt/s}$, predicting $W_{FIFO} = \frac{100}{1385.10} = \mathbf{72.20\text{ ms}}$ for every station.
- **`DownlinkAirtime` (per-station queue capacity $K_i = 50$ packets):**
  - Slow station (`host[0]`): $\lambda_0 = \frac{1.5546\text{ Mbps}}{10800\text{ bits}} = 143.94\text{ pkt/s} \implies W_{DRR,0} = \frac{50}{143.94} = \mathbf{347.37\text{ ms}}$.
  - Fast stations (`host[1..3]`): $\lambda_{1..3} = \frac{5.9231\text{ Mbps}}{10800\text{ bits}} = 548.44\text{ pkt/s} \implies W_{DRR,1..3} = \frac{50}{548.44} = \mathbf{91.17\text{ ms}}$.

| Station | FIFO Little's Law (ms) | FIFO Sim Mean $\pm$ CI (ms) | FIFO Sim Max (ms) | DRR Little's Law (ms) | DRR Sim Mean $\pm$ CI (ms) | DRR Sim Max (ms) |
|:---|:---:|:---:|:---:|:---:|:---:|:---:|
| **`host[0]` (9 Mbps)** | $72.20$ | $74.26 \pm 0.60$ | $87.32$ | $347.37$ | **$349.09 \pm 0.08$** | $352.30$ |
| **`host[1]` (36 Mbps)** | $72.20$ | $73.29 \pm 0.53$ | $86.45$ | $91.17$ | $92.05 \pm 0.12$ | $96.39$ |
| **`host[2]` (36 Mbps)** | $72.20$ | $73.27 \pm 0.57$ | $87.32$ | $91.17$ | $92.10 \pm 0.19$ | $95.90$ |
| **`host[3]` (36 Mbps)** | $72.20$ | $73.33 \pm 0.48$ | $86.34$ | $91.17$ | $92.10 \pm 0.17$ | $96.22$ |

- **Why Airtime DRR increases `host[0]`'s delay so sharply:** First, `host[0]`'s packet departure rate drops from $347.4\text{ pkt/s}$ in FIFO to $143.9\text{ pkt/s}$ ($2.41\times$ slower) under airtime fairness. Second, in `AirtimeFairQueue`, `host[0]` has its own $50$-packet buffer that fills completely with slow packets instead of sharing $100$ slots ($25$ packets on average per flow in FIFO). Doubling `host[0]`'s queued backlog ($25 \to 50$) while cutting its drain rate by $2.41\times$ increases its queueing delay by $2 \times 2.41 \approx 4.7\times$ ($74.26\text{ ms} \to 349.09\text{ ms}$).

---

### Question 4-8: Low-Load Slow Station (`*.server.app[0].sendInterval = exponential(25ms)`)

Here `host[0]`'s theoretical offered load is $40\text{ pkt/s} \times 10800\text{ bits} = 0.4320\text{ Mbps}$ (measured `server.app[0]` `packetSent:count` over $T=10\text{ s}$ is $0.4290 \pm 0.0174\text{ Mbps}$ in FIFO and $0.4383 \pm 0.0189\text{ Mbps}$ in DRR).

| Metric | `DownlinkFifoLowLoad` | `DownlinkAirtimeLowLoad` |
|:---|:---:|:---:|
| **`host[0]` Throughput (Mbps)** | **$0.0132 \pm 0.0044$** *(starved; $\sim 97\%$ dropped!)* | **$0.4389 \pm 0.0188$** *(100% of offered load delivered!)* |
| **`host[1..3]` Avg Throughput (Mbps)** | $7.4606$ ($7.48, 7.46, 7.44$) | $7.0371$ ($7.04, 7.04, 7.04$) |
| **Total Throughput $S_{tot}$ (Mbps)** | $22.3949 \pm 0.0137$ | $21.5503 \pm 0.0364$ |
| **`host[0]` Mean Delay (ms)** | **$50.02 \pm 0.15$** (max $51.81$) | **$6.05 \pm 0.05$** (max $26.58$) |
| **`host[1..3]` Mean Delay (ms)** | $49.05$ (max $51.94$) | $77.55$ (max $91.26$) |
| **Fairness ($J_S$ / $J_a$)** | $J_S = 0.7509$ / $J_a = 0.7531$ | $J_S = 0.7805$ / $J_a = 0.8503$ |

- **Does `host[0]` receive all its requested load?**
  - **In FIFO (`DownlinkFifoLowLoad`): No!** Because the three saturated fast flows bombard the single $100$-packet FIFO queue at $60{,}000\text{ pkt/s}$, the buffer is almost always full when a rare `host[0]` packet arrives ($40\text{ pkt/s}$), tail-dropping $\sim 97\%$ of `host[0]`'s frames (only $8\text{--}17$ frames survive in $10\text{ s}$).
  - **In Airtime DRR (`DownlinkAirtimeLowLoad`): Yes — $100\%$!** With queue isolation (`queue[0]`), zero `host[0]` packets are dropped, and `host[0]` receives $0.4389\text{ Mbps}$ ($a_0 = 5.45\%$ airtime), while the remaining $94.55\%$ of airtime is shared evenly among `host[1..3]` at $7.0371\text{ Mbps}$ each.
- **Delay Comparison:** `host[0]`'s mean delay drops by **$8.3\times$** (from $50.02\text{ ms}$ behind $99$ fast packets in FIFO to $6.05\text{ ms}$ in DRR).
- **Why DRR does not restrict a lightly loaded slow station:** DRR only enforces a $25\%$ airtime cap when all queues remain continuously backlogged. A light station demanding $5.45\%$ airtime never saturates its fair share; whenever its queue empties, `deficit[0]` resets cleanly to `0`, and each newly arriving frame is served within a few scheduling rounds without waiting behind saturated flows.

---

### Question 4-9: Can AP Queue Scheduling Fix the Uplink Anomaly?

**No.** In the downlink direction, the AP is the sole sender on the channel, so its internal MAC queue (`AirtimeFairQueue`) has complete control over transmission order and can enforce airtime fairness. In the **uplink** direction, each wireless station maintains its own independent MAC queue and contends autonomously for the shared medium via distributed CSMA/CA (DCF). The AP neither holds the stations' uplink frames nor decides which station wins the next backoff contention; therefore, scheduling the AP's downlink queue cannot alter uplink airtime contention. *(Solving uplink anomaly requires per-station MAC parameter adaptation such as EDCA/TXOP scaling or airtime-aware contention windows at the stations themselves.)*

---

### Supplemental: DRR Quantum Sensitivity Sweep (`QuantumSweep`)

Sweeping `quantum` across $\{200, 500, 1000, 2000\}\,\mu\text{s}$ (5 repetitions each) confirms that long-term throughput and airtime fairness are invariant to `quantum` under continuous saturation:

| Quantum ($\mu\text{s}$) | $S_0$ (Mbps) | $S_{1..3}$ Avg (Mbps) | $S_{tot} \pm 95\%$ CI (Mbps) | Airtime Fairness $J_a$ | `host[0]` Mean Delay (ms) |
|:---:|:---:|:---:|:---:|:---:|:---:|
| **$200$** | $1.5550$ | $5.9238$ | $19.3264 \pm 0.0213$ | $0.9989$ | $349.04$ |
| **$500$** | $1.5554$ | $5.9255$ | $19.3320 \pm 0.0103$ | $0.9989$ | $348.92$ |
| **$1000$** | $1.5548$ | $5.9244$ | $19.3279 \pm 0.0120$ | $0.9989$ | $349.02$ |
| **$2000$** | $1.5556$ | $5.9247$ | $19.3296 \pm 0.0097$ | $0.9989$ | $349.30$ |

---

## 5. Figure & Required Qtenv Capture Inventory

All 300-DPI Python figures are exported by `phase2/phase2.ipynb` to `CA2-WiFi-Performance/report/figures/` (excluded from Git via `.gitignore`):

1. `phase2_throughput_airtime_comparison.png` — Per-station throughput ($S_i \pm 95\%$ CI vs. Q4-3 & Q4-4 models) and channel airtime share ($a_i$) across `Uplink`, `DownlinkFifo`, and `DownlinkAirtime`.
2. `phase2_delay_and_lowload.png` — Saturated mean delay vs. Little's Law (Q4-7) and low-load slow-station throughput/delay comparison (Q4-8).
3. `phase2_quantum_sweep.png` — DRR quantum sensitivity ($200\text{--}2000\,\mu\text{s}$) for throughput, $J_a$, and delay.
4. **Required Manual Qtenv Screenshot (`CA2.pdf` p. 14):**
   - **No Sequence Chart / `.elog` captures are required for Phase 2.**
   - **One Qtenv internal module screenshot is required:** Run `[Config DownlinkAirtime]` in Qtenv, inspect `Anomaly.ap.wlan[0].mac.dcf.channelAccess.pendingQueue` (showing `classifier`, `queue[0..3]`, `scheduler`, and `pulled` packet counts differing between `queue[0]` and `queue[1..3]`), and save it as `report/figures/phase2_qtenv_pending_queue.png`.

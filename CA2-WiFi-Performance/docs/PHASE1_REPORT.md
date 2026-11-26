# Computer Assignment 2 — Phase 1 Complete Report
## DCF Contention, Slotted Mathematical Modeling & Hidden Terminals

**Course:** Computer Networks (Fall 2026 / 1405–1406) — University of Tehran  
**Instructor:** Prof. Nasser Yazdani  
**Teaching Assistants:** Sajjad Taghizadeh, Aliakbar Arshadi  
**Students:**  
- Parsa Saeednia (`810102460`)  
- Mohammad Hossein Mazaheri (`810102585`)  
**Assigned Parameters:**  
- Data Payload: $L = 1350\text{ Bytes}$  
- Base Bitrate: $R = 12\text{ Mbps}$ (802.11a OFDM, 5.17 GHz, Channel 8)

---

## 1. Summary of Completed Pre-Simulation Predictions

| Phase / Scenario | Prediction | Theoretical Reasoning | Simulation Result | Cause of Discrepancy / Validation |
|:---|:---|:---|:---:|:---|
| **3.2: Contention ($n=30$)** | Simulation throughput will be **substantially higher** than the model with constant $\tau_0$ ($S_{sim} \approx 7\text{ Mbps} \gg S_{\tau_0} \approx 1.05\text{ Mbps}$). | The $\tau_0$ model assumes constant $CW=15$, leading to a collision probability $>90\%$. In reality, Binary Exponential Backoff (BEB) doubles $CW$ up to $1023$, reducing $\tau$ toward the optimal $1/n$ and keeping throughput high. | **$S_{sim} = 7.1325\text{ Mbps}$** vs. **$S_{\tau_0} = 1.0533\text{ Mbps}$** | **Validated:** BEB dynamically throttles transmission probability ($\hat{\tau} = 0.0244 \approx 1/30$), avoiding collision storms. |
| **3.4: Hidden Terminals (Without RTS/CTS)** | Throughput will degrade severely and fairness will collapse ($J \ll 1.0$). | Stations cannot hear each other ($P_r = -79.7\text{ dBm} < -77\text{ dBm}$), leading to continuous collisions of long $944\,\mu\text{s}$ DATA frames and medium capture by one node. | **$S_{tot} = 6.2737\text{ Mbps}$**, **$J = 0.8322$** (worst run $J = 0.5710$) | **Validated:** 44.1% collision rate, 1324 dropped frames, and severe channel monopolization. |
| **3.4: Hidden Terminals (With RTS/CTS)** | Total throughput will increase and stations will share the channel with near-perfect fairness ($J \approx 1.0$). | RTS frames are short ($36\,\mu\text{s}$), and the AP's CTS ($32\,\mu\text{s}$) reaches both stations, setting the NAV to silence the hidden terminal during the entire DATA+ACK exchange. | **$S_{tot} = 7.9708\text{ Mbps}$**, **$J = 0.9949$** | **Validated:** Throughput increased by $+27.05\%$ and fairness reached $0.995$ ($S_0 \approx 3.89\text{ Mbps}, S_1 \approx 4.08\text{ Mbps}$). |

---

## 2. Part 3.1: Detailed Frame Exchange Timing Analysis

### Question 3-1: Protocol Parameters and Frame Breakdown
- **Physical Layer Parameters (802.11a, 20 MHz channel):**
  - Slot time ($\sigma$): $9\,\mu\text{s}$
  - Short Interframe Space ($SIFS$): $16\,\mu\text{s}$
  - DCF Interframe Space ($DIFS$): $SIFS + 2\sigma = 16 + 2(9) = 34\,\mu\text{s}$
  - Contention Window: $CW_{min} = 15$, $CW_{max} = 1023$

- **Data Frame Header Structure ($L = 1350\text{ B}$):**
  1. **Application Payload:** $L = 1350\text{ Bytes}$ (`cli` generates `10800 bits`).
  2. **LLC Header (`Ieee80211LlcLpd`):** $3\text{ Bytes}$ ($24\text{ bits}$: DSAP = 1B, SSAP = 1B, Control = 1B). Total: $1353\text{ B}$.
  3. **MAC Data Header (`Ieee80211Mac`):** $24\text{ Bytes}$ ($192\text{ bits}$: Frame Control 2B, Duration/ID 2B, Address 1 6B, Address 2 6B, Address 3 6B, Sequence Control 2B).
  4. **FCS (Frame Check Sequence):** $4\text{ Bytes}$ ($32\text{ bits}$).
  - **Total MAC Data Frame Size ($B_{DATA}$):**
    $$B_{DATA} = 1350 + 3 + 24 + 4 = \mathbf{1381\text{ Bytes}} \quad (11048\text{ bits})$$

- **ACK Frame Structure:**
  1. Frame Control: $2\text{ Bytes}$
  2. Duration: $2\text{ Bytes}$
  3. RA (Receiver Address): $6\text{ Bytes}$
  4. FCS: $4\text{ Bytes}$
  - **Total ACK Frame Size ($B_{ACK}$):**
    $$B_{ACK} = 2 + 2 + 6 + 4 = \mathbf{14\text{ Bytes}} \quad (112\text{ bits})$$

---

### Question 3-2: Calculated vs. Measured Timings & ACK Rate Selection

#### 1. Theoretical PPDU Durations
Using the 802.11a OFDM formula ($N_{DBPS} = 4r\text{ bits/symbol}$):
$$T_{PPDU}(B, r) = 20\,\mu\text{s} + 4\,\mu\text{s} \cdot \left\lceil \frac{16 + 8B + 6}{N_{DBPS}} \right\rceil$$

- **DATA PPDU ($B = 1381\text{ B}$, $r = 12\text{ Mbps} \implies N_{DBPS} = 48\text{ bits}$):**
  $$16 + 8(1381) + 6 = 11070\text{ bits} \implies \left\lceil \frac{11070}{48} \right\rceil = 231\text{ symbols}$$
  $$T_{DATA} = 20\,\mu\text{s} + 231 \times 4\,\mu\text{s} = \mathbf{944.0\,\mu\text{s}}$$

- **ACK PPDU ($B = 14\text{ B}$, $r = 12\text{ Mbps} \implies N_{DBPS} = 48\text{ bits}$):**
  $$16 + 8(14) + 6 = 134\text{ bits} \implies \left\lceil \frac{134}{48} \right\rceil = 3\text{ symbols}$$
  $$T_{ACK} = 20\,\mu\text{s} + 3 \times 4\,\mu\text{s} = \mathbf{32.0\,\mu\text{s}}$$

#### 2. Measured vs. Calculated Comparison Table

| Component | Calculated Duration ($\mu\text{s}$) | Measured Duration (`Timing-#0.elog`) ($\mu\text{s}$) | Absolute Difference |
|:---|:---:|:---:|:---:|
| **DIFS** | $34.000$ | $34.000$ | $0.000\,\mu\text{s}$ |
| **Backoff ($k$ slots)** | $k \times 9.000$ | $k \times 9.000$ | $0.000\,\mu\text{s}$ |
| **DATA PPDU** | $944.000$ | $944.000$ | $0.000\,\mu\text{s}$ |
| **SIFS** | $16.000$ | $16.033$ ($16\,\mu\text{s} + \tau_{prop}$) | $+0.033\,\mu\text{s}$ ($10\text{ m}$ speed of light) |
| **ACK PPDU** | $32.000$ | $32.000$ | $0.000\,\mu\text{s}$ |

#### 3. ACK Bitrate and INET Selection Rule
- **Measured ACK Bitrate:** $12\text{ Mbps}$ (transmission time $= 32\,\mu\text{s}$).
- **Selection Rule in INET:** In IEEE 802.11, control response frames (ACK, CTS) must be sent using a mandatory basic rate (`BSSBasicRateSet`) so all stations in the basic service set can decode them. In 802.11a, mandatory basic rates are **6, 12, and 24 Mbps**.  
  In INET (`RateSelection.cc:80-90`):
  ```cpp
  const IIeee80211Mode *RateSelection::computeResponseAckFrameMode(Packet *packet, ...) {
      auto mode = getMode(packet, dataOrMgmtHeader);
      return modeSet->getIsMandatory(mode) ? mode : modeSet->getSlowerMandatoryMode(mode);
  }
  ```
  Since our group's bitrate $R = 12\text{ Mbps}$ is already a member of the mandatory basic rate set, INET transmits the ACK at $12\text{ Mbps}$. If $R$ had been an optional rate (such as 9, 18, 36, or 54 Mbps), INET would downgrade the ACK rate to the highest slower mandatory rate ($6\text{ Mbps}$ or $24\text{ Mbps}$).

#### 4. Extracted Backoff Values ($k$) for 5 Consecutive Exchanges
From `results/Timing-#0.elog`:
- After Exchange 1: $k_1 = 15$ slots ($135.0\,\mu\text{s}$)
- After Exchange 2: $k_2 = 5$ slots ($45.0\,\mu\text{s}$)
- After Exchange 3: $k_3 = 0$ slots ($0.0\,\mu\text{s}$)
- After Exchange 4: $k_4 = 3$ slots ($27.0\,\mu\text{s}$)
- After Exchange 5: $k_5 = 11$ slots ($99.0\,\mu\text{s}$)  
*(All values satisfy $k \in [0, CW_{min}] = [0, 15]$).*

---

### Question 3-3: Single Station Throughput ($S_1$)
For $n=1$, collisions never occur ($P_c = 0$). Before each transmission, the station draws $k \sim \text{Uniform}[0, CW_{min}]$:
$$\bar{k} = \frac{CW_{min}}{2} = \frac{15}{2} = 7.5\text{ slots}$$
$$\bar{k}\sigma = 7.5 \times 9\,\mu\text{s} = 67.5\,\mu\text{s}$$
The total expected cycle time:
$$T_{cycle} = DIFS + \bar{k}\sigma + T_{DATA} + SIFS + T_{ACK}$$
$$T_{cycle} = 34 + 67.5 + 944 + 16 + 32 = 1093.5\,\mu\text{s} = 1.0935\text{ ms}$$

Throughput:
$$S_1 = \frac{8L}{T_{cycle}} = \frac{8 \times 1350\text{ bits}}{1093.5 \times 10^{-6}\text{ s}} = \mathbf{9.8765\text{ Mbps}}$$

- **Simulation Comparison ($n=1$, 10-second measurement interval):**
  - Average received packets: $N_{rx} = 9143.4$ frames
  - Simulation throughput: $S_{sim}(1) = \frac{8 \times 1350 \times 9143.4}{10} = \mathbf{9.8749\text{ Mbps}}$
  - **Relative Error:** $\frac{|9.8765 - 9.8749|}{9.8765} = \mathbf{0.016\%}$ (near perfection).

---

## 3. Part 3.2: Slotted Contention Model Analysis

### Question 3-4: Model Evaluation and Equivalence at $n=1$
Model definitions:
$$\tau_0 = \frac{1}{1 + CW_{min}/2} = \frac{2}{CW_{min}+2} = \frac{2}{17} \approx 0.117647$$
$$T_s = DIFS + T_{DATA} + SIFS + T_{ACK} = 34 + 944 + 16 + 32 = 1026.0\,\mu\text{s}$$
$$T_c = DIFS + T_{DATA} = 34 + 944 = 978.0\,\mu\text{s}$$
$$\sigma = 9.0\,\mu\text{s}$$

$$S(n) = \frac{P_s \cdot 8L}{P_{idle}\sigma + P_s T_s + P_c T_c}$$
where $P_{idle} = (1-\tau)^n$, $P_s = n\tau(1-\tau)^{n-1}$, and $P_c = 1 - P_{idle} - P_s$.

#### Proof of Equivalence at $n=1$:
For $n = 1$:
$$P_{idle} = 1 - \tau_0 = \frac{15}{17}, \quad P_s = \tau_0 = \frac{2}{17}, \quad P_c = 0$$
Denominator:
$$D(1) = \frac{15}{17}\sigma + \frac{2}{17}T_s = \frac{2}{17}\left(\frac{15}{2}\sigma + T_s\right) = \frac{2}{17}(\bar{k}\sigma + T_s)$$
Numerator:
$$N(1) = \frac{2}{17} \cdot 8L$$
Dividing numerator by denominator:
$$S(1) = \frac{8L}{\bar{k}\sigma + T_s} = \frac{8L}{DIFS + \bar{k}\sigma + T_{DATA} + SIFS + T_{ACK}} \equiv S_1$$
Hence, the slotted model mathematically reduces to $S_1$ at $n=1$.

#### Optimal Throughput Station Count & Opposing Effects:
As $n$ grows:
1. **Idle Time Reduction:** $P_{idle} = (1-\tau)^n$ drops rapidly, reducing idle slot waste ($9\,\mu\text{s}$).
2. **Collision Escalation:** $P_c$ surges, wasting $T_c = 978\,\mu\text{s}$ per collision.
Because a collision costs $978\,\mu\text{s}$ (more than 108 times an idle slot!), the collision penalty dwarfs idle savings. Under fixed $\tau_0 = 2/17$, throughput peaks at **$n = 1$ ($9.8765\text{ Mbps}$)** and degrades monotonically ($S(10) = 5.73\text{ Mbps}$, $S(30) = 1.05\text{ Mbps}$, $S(50) = 0.14\text{ Mbps}$).

---

### Question 3-5: Maximizing Slot Success & Role of Exponential Backoff
Slot success probability:
$$P_s(\tau) = n\tau(1 - \tau)^{n-1}$$
Taking the derivative with respect to $\tau$:
$$\frac{dP_s}{d\tau} = n(1-\tau)^{n-1} - n(n-1)\tau(1-\tau)^{n-2} = n(1-\tau)^{n-2}\big[(1-\tau) - (n-1)\tau\big] = n(1-\tau)^{n-2}(1 - n\tau)$$
Setting $\frac{dP_s}{d\tau} = 0$:
$$1 - n\tau = 0 \implies \mathbf{\tau^* = \frac{1}{n}}$$
The second derivative is negative at $\tau^* = 1/n$, confirming that $\tau^* = 1/n$ is the global maximum.

#### Maximum Success Probability for Large $n$:
$$\lim_{n\to\infty} P_s\left(\frac{1}{n}\right) = \lim_{n\to\infty} n \cdot \frac{1}{n}\left(1 - \frac{1}{n}\right)^{n-1} = \lim_{n\to\infty} \left(1 - \frac{1}{n}\right)^{n-1} = \frac{1}{e} \approx \mathbf{0.367879 \quad (36.8\%)}$$

#### Network Impact when $\tau_0 > 1/n$:
$$\tau_0 = \frac{2}{17} > \frac{1}{n} \iff n > \frac{17}{2} = 8.5 \implies \mathbf{n \ge 9}$$
When $n \ge 9$, stations transmit far too aggressively if $\tau$ is fixed, causing collision collapse.

#### How Binary Exponential Backoff (BEB) Compensates:
Stations have no knowledge of $n$. BEB uses frame acknowledgments as implicit feedback:
- When a collision occurs (no ACK received), the station doubles its contention window: $CW \leftarrow \min(2CW+1, CW_{max})$.
- The effective slot transmission probability is $\tau \approx \frac{2}{CW + 1}$.
- In crowded networks, frequent collisions drive $CW$ higher ($15 \to 31 \to 63 \to 127 \dots$), which automatically reduces $\tau$ toward the optimal $1/n$ operating point without any centralized coordination!

---

## 4. Part 3.3: Model vs. Simulation Comparison

### Question 3-6: Estimating Collision Probability $\hat{p}$
$$\hat{p} = \frac{N_{retry} + N_{drop}}{N_{retry} + N_{first}}$$
- **Denominator ($N_{retry} + N_{first}$):** Counts the total number of transmission attempts initiated across all stations. Every frame starts with a first attempt ($N_{first}$), and every subsequent retransmission adds to $N_{retry}$.
- **Numerator ($N_{retry} + N_{drop}$):** Counts the total number of transmission attempts that failed (did not receive an ACK). Every retry is triggered by a failed attempt, and $N_{drop}$ represents the final failed attempt when the retry limit is exhausted.
- **Why $\hat{p}$ is a proper estimator:** By dividing failed attempts by total attempts, $\hat{p}$ directly computes the empirical conditional collision probability per packet transmission attempt.

---

### Question 3-7: Full Sweep Comparison Table & Analysis

From the 50 simulation runs ($10\text{ values of } n \times 5\text{ repetitions}$, 10s measurement interval):

| $n$ | $S_{sim}$ (Mbps) | 95% CI | $\hat{p}$ | $\hat{\tau}$ | $\tau_{opt} = \frac{1}{n}$ | Model $S(\tau_0)$ (Mbps) | Model $S(\hat{\tau})$ (Mbps) | Err ($\tau_0$) | Err ($\hat{\tau}$) |
|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---:|
| **1** | $9.8749$ | $\pm 0.0028$ | $0.0000$ | $0.1176$ | $1.0000$ | $9.8765$ | $9.8765$ | $0.02\%$ | $0.02\%$ |
| **2** | $9.5552$ | $\pm 0.0526$ | $0.1065$ | $0.1065$ | $0.5000$ | $9.6004$ | $9.6254$ | $0.47\%$ | $0.73\%$ |
| **3** | $9.2182$ | $\pm 0.0140$ | $0.1794$ | $0.0942$ | $0.3333$ | $9.1163$ | $9.3100$ | $1.11\%$ | $1.00\%$ |
| **5** | $8.7841$ | $\pm 0.0305$ | $0.2605$ | $0.0727$ | $0.2000$ | $8.0750$ | $8.8909$ | $8.07\%$ | $1.22\%$ |
| **8** | $8.3920$ | $\pm 0.0386$ | $0.3274$ | $0.0551$ | $0.1250$ | $6.6049$ | $8.5044$ | $21.30\%$ | $1.34\%$ |
| **10** | $8.1618$ | $\pm 0.0448$ | $0.3649$ | $0.0492$ | $0.1000$ | $5.7276$ | $8.2722$ | $29.82\%$ | $1.35\%$ |
| **15** | $7.7924$ | $\pm 0.0776$ | $0.4200$ | $0.0382$ | $0.0667$ | $3.9131$ | $7.9086$ | $49.78\%$ | $1.49\%$ |
| **20** | $7.5054$ | $\pm 0.0516$ | $0.4598$ | $0.0319$ | $0.0500$ | $2.5917$ | $7.6286$ | $65.47\%$ | $1.64\%$ |
| **25** | $7.2935$ | $\pm 0.0173$ | $0.4891$ | $0.0276$ | $0.0400$ | $1.6712$ | $7.4126$ | $77.09\%$ | $1.63\%$ |
| **30** | $7.1325$ | $\pm 0.0415$ | $0.5113$ | $0.0244$ | $0.0333$ | $1.0533$ | $7.2429$ | **85.23%** | **1.55%** |

#### Key Analytical Conclusions:
1. **Model with $\hat{\tau}$ is remarkably accurate:** Across all $n$, the slotted model driven by measured $\hat{\tau}$ matches simulation within $0.7\%\text{--}1.6\%$ error.
2. **Fixed $\tau_0$ Model Fails Completely:** At $n = 30$, the model with constant $\tau_0$ predicts $1.0533\text{ Mbps}$ (error **85.23%**), while simulation achieves **$7.1325\text{ Mbps}$**. This massive discrepancy demonstrates that real 802.11 DCF does not operate at constant $CW_{min}$; BEB dramatically reduces $\hat{\tau}$ from $0.1176$ down to $0.0244$ as $n$ reaches 30, stabilizing the network.
3. **Slight Overestimation of Model with $\hat{\tau}$:** As noted in the assignment specification, Model $S(\hat{\tau})$ slightly overestimates simulation throughput ($7.24\text{ Mbps}$ vs. $7.13\text{ Mbps}$ at $n=30$, an overestimate of $\approx 1.5\%$). This minor gap occurs because the analytical model assumes independent memoryless slot transmissions, whereas in actual simulation, backoff timers freeze during busy carrier sensing, inducing slight inter-station backoff correlations.

---

## 5. Part 3.4: Hidden Terminals & RTS/CTS Evaluation

### Question 3-8: Free-Space Path Loss & Hidden Terminal Verification
- **Scenario Parameters:**
  - Carrier Frequency: $f_c = 5010 + 20 \times 8 = 5170\text{ MHz} = 5.17\text{ GHz}$
  - Wavelength: $\lambda = \frac{c}{f_c} = \frac{299,792,458\text{ m/s}}{5.17 \times 10^9\text{ Hz}} \approx 0.057987\text{ m}$
  - Transmitter Power: $P_t = 20\text{ mW} = 13.0103\text{ dBm}$
  - Receiver Sensitivity / Energy Detection: $S_{th} = -77.0\text{ dBm}$
  - Topology: AP at $(200, 200)$, `host[0]` at $(300, 200)$, `host[1]` at $(100, 200)$.
    - Station-to-AP distance: $d_{AP} = 100\text{ m}$
    - Inter-station distance: $d_{inter} = 200\text{ m}$

#### Free-Space Path Loss (Friis Formula):
$$P_r = P_t \left( \frac{\lambda}{4\pi d} \right)^2 \implies P_{r,\text{dBm}} = P_{t,\text{dBm}} - 20 \log_{10}\left( \frac{4\pi d}{\lambda} \right)$$

1. **Received Power from AP at each station ($d = 100\text{ m}$):**
   $$\text{FSPL}(100\text{ m}) = 20 \log_{10}\left( \frac{4\pi \times 100}{0.057987} \right) \approx 86.72\text{ dB}$$
   $$P_r(100\text{ m}) = 13.01 - 86.72 = \mathbf{-73.71\text{ dBm}} > -77.0\text{ dBm} \quad \text{\textbf{(Detectable!)}}$$
   *Both stations can hear the AP.*

2. **Received Power between the two stations ($d = 200\text{ m}$):**
   $$\text{FSPL}(200\text{ m}) = 86.72 + 20\log_{10}(2) \approx 92.74\text{ dB}$$
   $$P_r(200\text{ m}) = 13.01 - 92.74 = \mathbf{-79.73\text{ dBm}} < -77.0\text{ dBm} \quad \text{\textbf{(Undetectable!)}}$$
   *Stations cannot detect each other's transmissions; they are mutually hidden terminals.*

---

### Question 3-9: Simulation Results & Jain's Fairness Index

From the 10 simulation runs (5 repetitions each, 10s measurement interval):

| Config / Metric | Rep 0 | Rep 1 | Rep 2 | Rep 3 | Rep 4 | **Average $\pm$ 95% CI** |
|:---|:---:|:---:|:---:|:---:|:---:|:---:|
| **Without RTS/CTS (`3000B`)** | | | | | | |
| Total Throughput $S_{tot}$ (Mbps) | $6.2716$ | $6.2597$ | $6.2078$ | $6.2932$ | $6.3364$ | **$6.2737 \pm 0.0584$** |
| Host[0] Throughput $S_0$ (Mbps) | $4.4107$ | $4.8881$ | $2.9246$ | $2.6428$ | $5.9141$ | **$4.1561$** |
| Host[1] Throughput $S_1$ (Mbps) | $1.8608$ | $1.3716$ | $3.2821$ | $3.6504$ | $0.4223$ | **$2.1174$** |
| Jain Fairness Index $J$ | $0.8581$ | $0.7601$ | $0.9967$ | $0.9750$ | $0.5710$ | **$0.8322$** |
| Collision Prob $\hat{p}$ | $0.4409$ | $0.4405$ | $0.4475$ | $0.4405$ | $0.4364$ | **$0.4412$ (44.1%)** |
| Packet Drops ($N_{drop}$) | $264$ | $263$ | $274$ | $264$ | $259$ | Total: **$1324$ drops** |
| **With RTS/CTS (`100B`)** | | | | | | |
| Total Throughput $S_{tot}$ (Mbps) | $7.9985$ | $7.8797$ | $7.9196$ | $8.0190$ | $8.0374$ | **$7.9708 \pm 0.0843$** |
| Host[0] Throughput $S_0$ (Mbps) | $4.1780$ | $3.5861$ | $4.0991$ | $3.5262$ | $4.0662$ | **$3.8911$** |
| Host[1] Throughput $S_1$ (Mbps) | $3.8205$ | $4.2935$ | $3.8205$ | $4.4928$ | $3.9712$ | **$4.0797$** |
| Jain Fairness Index $J$ | $0.9980$ | $0.9920$ | $0.9988$ | $0.9857$ | $0.9999$ | **$0.9949$** |
| Collision Prob $\hat{p}$ | $0.0023$ | $0.0017$ | $0.0020$ | $0.0021$ | $0.0024$ | **$0.0021$ (0.2%)** |
| Packet Drops ($N_{drop}$) | $38$ | $29$ | $33$ | $36$ | $40$ | Total: **$176$ drops** |

#### Did RTS/CTS increase total throughput and improve performance?
**Yes, significantly!**
1. **Total Throughput:** Increased from **$6.2737\text{ Mbps}$ to $7.9708\text{ Mbps}$** — an improvement of **$+27.05\%$**.
2. **Fairness:** Increased from **$0.8322$ to $0.9949$**. Without RTS/CTS, one station frequently captures the medium while the other suffers exponential backoff (e.g. Repetition 4: $5.91\text{ Mbps}$ vs. $0.42\text{ Mbps}$, $J = 0.5710$). RTS/CTS enforces complete airtime equity ($S_0 \approx 3.89\text{ Mbps}, S_1 \approx 4.08\text{ Mbps}$).
3. **Collision Elimination:** Dropped from **$44.12\%$ to $0.21\%$**.

---

### Question 3-10: Detailed Overhead vs. Collision Savings Analysis
- RTS frame: $20\text{ Bytes}$, CTS frame: $14\text{ Bytes}$. *(At the base rate $R = 12\text{ Mbps}$, $T_{RTS} = 36.0\,\mu\text{s}$ and $T_{CTS} = 32.0\,\mu\text{s}$; note that in INET's mandatory basic rate set `{6, 12, 24} Mbps`, control frames can also be transmitted at $24\text{ Mbps}$, yielding $T_{RTS} = T_{CTS} = 28.0\,\mu\text{s}$ in trace logs and even higher net savings.)*
- **PPDU Durations (at $R = 12\text{ Mbps}$ conservative baseline):**
  $$T_{RTS} = 20\,\mu\text{s} + 4 \cdot \lceil (16 + 160 + 6)/48 \rceil = 20 + 4(4) = \mathbf{36.0\,\mu\text{s}}$$
  $$T_{CTS} = 20\,\mu\text{s} + 4 \cdot \lceil (16 + 112 + 6)/48 \rceil = 20 + 3(4) = \mathbf{32.0\,\mu\text{s}}$$

- **Overhead added to each successful transmission:**
  $$T_{overhead} = T_{RTS} + SIFS + T_{CTS} + SIFS = 36 + 16 + 32 + 16 = \mathbf{100.0\,\mu\text{s}}$$

- **Time saved on each collision:**
  Without RTS/CTS, a collision locks the channel for $T_{DATA} = 944.0\,\mu\text{s}$. With RTS/CTS, collisions only occur during the RTS frame ($T_{RTS} = 36.0\,\mu\text{s}$).
  $$T_{saving} = T_{DATA} - T_{RTS} = 944.0 - 36.0 = \mathbf{908.0\,\mu\text{s}}$$

- **Collision Frequency and Expected Net Benefit:**
  Across 5 runs of Hidden without RTS/CTS:
  - Total failed attempts: $N_{retry} + N_{drop} = 21605 + 1324 = 22929$.
  - In each collision, both stations fail, so total collision events $\approx 22929 / 2 = 11464.5$.
  - Total successful frames received: $29045$.
  - Collisions per successful transmission:
    $$C = \frac{11464.5}{29045} \approx \mathbf{0.395\text{ collisions per success}}$$
  - **Expected Savings per Transmission:**
    $$\bar{T}_{saving} = C \times T_{saving} = 0.395 \times 908.0\,\mu\text{s} = \mathbf{358.7\,\mu\text{s}}$$
  - **Net Time Saved per Exchange:**
    $$\Delta T = \bar{T}_{saving} - T_{overhead} = 358.7\,\mu\text{s} - 100.0\,\mu\text{s} = \mathbf{+258.7\,\mu\text{s}}$$
  Because the expected savings ($358.7\,\mu\text{s}$) exceed the overhead ($100.0\,\mu\text{s}$) by over $3.5\times$, RTS/CTS saves a net $258.7\,\mu\text{s}$ per packet, directly explaining the observed $+27.05\%$ throughput gain!

- **Impact of Longer Frames or Lower Bitrate:**
  - **Longer frames ($L \uparrow$):** $T_{DATA}$ increases proportionally. Since $T_{saving} = T_{DATA} - T_{RTS}$ grows while $T_{overhead}$ remains fixed at $100\,\mu\text{s}$, the net savings and throughput improvement of RTS/CTS become even greater.
  - **Lower bitrate ($R \downarrow$):** Transmission of DATA frames takes significantly longer (e.g. at 6 Mbps, $T_{DATA} \approx 1850\,\mu\text{s}$). Collisions become catastrophic without RTS/CTS, making RTS/CTS even more essential.

---

## 6. Report Figures & Sequence Chart Coordinates

All figures are stored in `CA2-WiFi-Performance/report/figures/` (excluded from Git via `.gitignore`):

- **Analytical & Simulation Charts (`phase1.ipynb`):**
  - `phase1_slotted_model.png` — Analytical throughput $S(n)$ under fixed $\tau_0 = 2/17$.
  - `phase1_sim_vs_models.png` — 50-run simulation throughput ($95\%$ CI) vs. fixed-$\tau_0$ and measured-$\hat{\tau}(n)$ models.
  - `phase1_cw_trace.png` — `host[0]` Binary Exponential Backoff CW trace ($15 \to 1023$ slots) at $n=10$.
  - `phase1_hidden_rts_comparison.png` — Hidden terminal throughput ($+27.05\%$) and Jain's Fairness ($0.8322 \to 0.9949$) comparison.

- **Sequence Chart Diagrams & OMNeT++ IDE Captures:**
  1. **Single Frame Exchange (`Timing-#0.elog` $\to$ `phase1_seq_timing.png` / `phase1_timing_simulation.png`):**
     - $t = 50.0 \to 142.0\,\mu\text{s}$: DIFS ($34\,\mu\text{s}$) + initial backoff ($58\,\mu\text{s}$).
     - $t = 142.0 \to 1086.0\,\mu\text{s}$: DATA PPDU (`req-78-1`, $944\,\mu\text{s}$).
     - $t = 1086.0 \to 1102.0\,\mu\text{s}$: SIFS ($16\,\mu\text{s}$).
     - $t = 1102.0 \to 1134.1\,\mu\text{s}$: ACK PPDU (`WlanAck`, $32\,\mu\text{s}$).
  2. **Hidden Collision Without RTS/CTS (`HiddenTrace-rtsThreshold=3000B-#0.elog` $\to$ `phase1_seq_hidden_collision.png` / `phase1_hidden_collision_simulation.png`):**
     - `host[0]` transmits `req-79-1` ($142.0 \to 1086.3\,\mu\text{s}$); `host[1]` cannot sense it and transmits `req-118-1` ($169.0 \to 1113.3\,\mu\text{s}$). Overlapping signals corrupt reception at `ap`, yielding no ACK.
  3. **Virtual Carrier Sensing via RTS/CTS & NAV (`HiddenTrace-rtsThreshold=100B-#0.elog` $\to$ `phase1_seq_hidden_rts_cts.png` / `phase1_hidden_rts_cts.png`):**
     - $t = 79.0 \to 107.3\,\mu\text{s}$: `host[0]` sends `RTS`.
     - $t = 123.3 \to 151.7\,\mu\text{s}$: `ap` broadcasts `CTS`; `host[1]` receives CTS at $t = 151.7\,\mu\text{s}$ and sets its **NAV silent interval** until $t = 1160.3\,\mu\text{s}$.
     - $t = 167.7 \to 1112.0\,\mu\text{s}$: `host[0]` completes collision-free DATA ($944\,\mu\text{s}$), followed by `WlanAck` at $1128.0 \to 1160.3\,\mu\text{s}$ ($32\,\mu\text{s}$).

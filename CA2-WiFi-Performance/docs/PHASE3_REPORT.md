# Computer Assignment 2 — Phase 3 Complete Report
## Roaming Between Access Points, Sticky Client Analysis & Hysteresis Handover FSM

**Course:** Computer Networks (Fall 2026 / 1405–1406) — University of Tehran  
**Instructor:** Prof. Nasser Yazdani  
**Teaching Assistants:** Sajjad Taghizadeh, Aliakbar Arshadi  
**Students:**  
- Parsa Saeednia (`810102460`)  
- Mohammad Hossein Mazaheri (`810102585`)  

**Assigned Phase 3 Parameters (`params.json` / `params.ini`):**  
- Access Point Spacing: $D = 51\text{ m}$ (`ap1` on channel `1` at $x=50\text{ m}$, `ap2` on channel `3` at $x=101\text{ m}$, SSID `"CA2"`)  
- Propagation Model (`LogNormalShadowing`): $\alpha = 3.2$, $\sigma = 3\text{ dB}$, $P_t = 20\text{ mW}$ ($13.0103\text{ dBm}$), $S = -85\text{ dBm}$, $N = -110\text{ dBm}$  
- Mobility: $v = 1.75\text{ m/s}$ on $y=102\text{ m}$ ($2\text{ m}$ offset, walk duration $T_{walk} = D/v = 29.143\text{ s}$)  
- Beacon Interval: $BI = 100\text{ ms}$ | Scanned Channels: $n_{ch} = 6$ (channels `0..5`)  
- Voice Stream: `160 B` every `20 ms` (`startTime = 1s`, `sim-time-limit = 120s`)

---

## 1. Summary of Pre-Simulation Predictions vs. Simulation Results

| Scenario / Question | Pre-Simulation Prediction | Theoretical Reasoning | Simulation Result | Cause of Discrepancy / Validation |
|:---|:---|:---|:---:|:---|
| **5-2 (`Walk` Outage)** | Active scan over $n_{ch}=6$ channels ($2$ active) takes $T_{scan} = 1.800\text{ s}$; voice outage $\approx 1.80\text{ s}$ (not $3.5\,BI + T_{scan} = 2.15\text{ s}$). | $6(100) + 4(150) + 2(300) = 1800\text{ ms}$. The station keeps sending voice frames during the $3.5\,BI$ beacon-loss timer before leaving channel `1` to scan. | Scan dur: **$1.7700 \pm 0.1558\text{ s}$**<br>Outage: **$1.7919 \pm 0.1605\text{ s}$** | **Validated:** Outage matches $T_{scan}$ within $0.45\%$, plus $\sim 22\text{ ms}$ for auth/assoc and $20\text{ ms}$ voice framing alignment. |
| **5-3 (`ScanNeighbor` Outage)** | Restricting `channelsToScan = "1 3"` reduces $T_{scan}$ to $2(100 + 300) = \mathbf{0.800\text{ s}}$, cutting outage by $\approx 1.0\text{ s}$. | Scanning only the two neighbor channels eliminates $4 \times 250\text{ ms} = 1000\text{ ms}$ of idle-channel dwells. | Scan dur: **$0.8000 \pm 0.0000\text{ s}$**<br>Outage: **$0.8197 \pm 0.0006\text{ s}$** | **Validated:** Exact match ($0.800\text{ s}$ scan $+ 19.7\text{ ms}$ auth/assoc/framing); outage reduced by **$54.3\%$**. |
| **5-6(a) (FSM: $H=0, N=1$)** | Severe ping-ponging (**$\sim 5\text{--}40$ handovers**) near the midpoint. | With no smoothing or hysteresis, independent $\sigma=3\text{ dB}$ shadowing flips $P_2 - P_1 \sim \mathcal{N}(0, 18)$ across consecutive $100\text{ ms}$ beacons. | **$37.67$ handovers** mean (`[41, 45, 27]` across 3 `Survey` runs) | **Validated:** Instantaneous signal fluctuations trigger rapid back-and-forth switching across $x \in [20, 34]\text{ m}$. |
| **5-6(b) (FSM: $H=6\text{ dB}$, noiseless)** | Handover occurs at $x = \frac{D}{1 + 10^{-H/(10\alpha)}} = \mathbf{30.92\text{ m}}$. | Solving $10\alpha\log_{10}\left(\frac{x}{D-x}\right) = H$ for $D=51\text{ m}, H=6\text{ dB}, \alpha=3.2$ yields $x = 51 / (1 + 10^{-6/32}) = 30.92\text{ m}$. | $N=10, H=6\text{ dB}$: **$30.26\text{ m}$** mean (`[31.25, 31.32, 28.21]`) | **Validated:** Smoothed SMA trace matches the noiseless geometric prediction within $0.66\text{ m}$ ($2.1\%$). |

---

## 2. Part 3.1: Anatomy of a Handover (`Walk` & `ScanNeighbor`)

### Question 5-1: Complete Handover Timeline (`Walk` Repetition 0)

From `Walk-#0.vec` (`ap1.sink`, `ap2.sink`, `host.wlan.agent.{sentRequest, acceptConfirm}`, `host.wlan.radio.radioChannel`):

| Time $t$ (s) | $\Delta t$ from Prev (ms) | Event / Vector Sample | Figure 7 State / Transition |
|:---:|:---:|:---|:---|
| `22.000152` | — | Last voice frame delivered at `ap1.sink` | `Associated` (`ap1`, ch `1`) |
| `22.009385` | $9.23$ | `sentRequest = 1` (Scan) & `radioChannel -> 0` | `Associated -> Scanning` (no beacon for $3.5\,BI$) |
| `22.259385` | $250.00$ | `radioChannel -> 1` | Scanning ch `0` ($100+150=250\text{ ms}$, idle) |
| `22.759385` | $500.00$ | `radioChannel -> 2` | Scanning ch `1` ($500\text{ ms}$ observed vs. $400\text{ ms}$ nominal*) |
| `22.909385` | $150.00$ | `radioChannel -> 3` | Scanning ch `2` ($150\text{ ms}$ observed vs. $250\text{ ms}$ nominal*) |
| `23.309385` | $400.00$ | `radioChannel -> 4` | Scanning ch `3` ($100+300=400\text{ ms}$, active `ap2`) |
| `23.559385` | $250.00$ | `radioChannel -> 5` | Scanning ch `4` ($100+150=250\text{ ms}$, idle) |
| `23.809385` | $250.00$ | `acceptConfirm = 1` (Scan OK), `radioChannel -> 3`, `sentRequest = 2` (Auth) | Scanning ch `5` ($250\text{ ms}$) $\to$ `Scanning -> Authenticating` (`AP found: ap2`) |
| `23.810306` | $0.92$ | `acceptConfirm = 2` (Auth OK), `sentRequest = 4` (Assoc) | `Authenticating -> Associating` (`4 frames, OK`) |
| `23.810740` | $0.43$ | `acceptConfirm = 4` (Assoc OK) | `Associating -> Associated` (`2 frames, OK`) |
| `23.820152` | $9.41$ | First voice frame delivered at `ap2.sink` | `Associated` (`ap2`, ch `3`) |

*\*Note on INET dwell variations:* As noted in `CA2.pdf` (p. 17), `Walk` rep 0 dwelled $500\text{ ms}$ on channel `1` and $150\text{ ms}$ on channel `2` (totaling $650\text{ ms}$, matching $400 + 250\text{ ms}$); across reps `2` and `3`, channel `1` yielded $250\text{ ms}$ (weak/missed `ap1` probe response), shortening scan duration to $1.65\text{ s}$, while rep `4` extended to $1.95\text{ s}$.

---

### Question 5-2: Active Scan Duration Formula & `Walk` Outage

With $n_{ch}$ total scanned channels and $k_{active}$ channels where radio activity is detected:
$$T_{scan} = n_{ch} \cdot \text{probeDelay} + (n_{ch} - k_{active}) \cdot \text{minChannelTime} + k_{active} \cdot \text{maxChannelTime}$$

For our group parameters ($n_{ch} = 6$, channels `0..5`, $\text{probeDelay} = 100\text{ ms}$, $\text{minChannelTime} = 150\text{ ms}$, $\text{maxChannelTime} = 300\text{ ms}$), only channels `1` (`ap1`) and `3` (`ap2`) carry AP traffic ($k_{active} = 2$):
$$T_{scan,Walk} = 6(0.100) + 4(0.150) + 2(0.300) = 0.600 + 0.600 + 0.600 = \mathbf{1.8000\text{ s}}$$

- **5-Run `Walk` Results:**
  - Measured Scan Duration: **$1.7700 \pm 0.1558\text{ s}$** (`[1.80, 1.80, 1.65, 1.65, 1.95] s`)
  - Measured Voice Outage: **$1.7919 \pm 0.1605\text{ s}$** (`[1.8200, 1.8200, 1.6601, 1.6797, 1.9796] s`)
- **Why Outage $\approx T_{scan}$ instead of $3.5\,BI + T_{scan}$:** While the $3.5\,BI = 350\text{ ms}$ beacon-loss timer counts down, the station's radio remains tuned to channel `1` and still transmits its $20\text{ ms}$ voice frames to `ap1` (in rep 0, the last packet reaches `ap1.sink` at $22.000\text{ s}$, just $9.2\text{ ms}$ before scanning begins at $22.009\text{ s}$). Thus, voice connectivity is interrupted only once the station leaves channel `1` to scan, making outage $\approx T_{scan} + \sim 20\text{ ms}$ (auth/assoc + frame alignment).

---

### Question 5-3: `ScanNeighbor` Results (`channelsToScan = "1 3"`)

Restricting `*.host.wlan.agent.channelsToScan = "1 3"` leaves $n_{ch} = 2$ channels (both active, $k_{active} = 2$):
- **Pre-Simulation Prediction:** $T_{scan,nbr} = 2(0.100) + 0(0.150) + 2(0.300) = \mathbf{0.8000\text{ s}}$.
- **5-Run Simulation Results:**
  - Measured Scan Duration: **$0.8000 \pm 0.0000\text{ s}$** across all 5 repetitions (`0.0%` error vs. formula).
  - Measured Voice Outage: **$0.8197 \pm 0.0006\text{ s}$** (`[0.8192, 0.8200, 0.8192, 0.8200, 0.8200] s`).
  - **Improvement over `Walk`:** Outage drops by **$0.9722\text{ s}$ ($-54.26\%$)** simply by eliminating scans of empty channels `0, 2, 4, 5`.

---

## 3. Part 3.2: The Sticky Client (`Survey` & `Walk`)

### Question 5-4: Received Power vs. Log-Distance

From `Walk-#0.sca` / INET `Ieee80211RadioMedium`, background noise power is **$N = -110\text{ dBm}$** (`IsotropicScalarBackgroundNoise.power = -110dBm`), and `minSnir:vector` records linear $\text{SNIR}$. Received power is:
$$P_r\text{ (dBm)} = -110 + 10 \log_{10}(\text{SNIR})$$
With $P_t = 20\text{ mW} = 13.0103\text{ dBm}$ and $PL(d_0=1\text{ m}) = 20\log_{10}(4\pi / \lambda) \approx 46.48\text{ dB}$ (channel 1, $5.03\text{ GHz}$) / $46.55\text{ dB}$ (channel 3, $5.07\text{ GHz}$), the mean log-distance curve is $\bar{P}(d) = 13.01 - 46.48 - 32.0\log_{10}(d)$ (with receiver sensitivity $S = -85\text{ dBm}$ crossed at $d \approx 40.7\text{ m}$).

- **Dispersion at Short vs. Large Distances (`phase3_survey_power_vs_distance.png`):**
  - At **short distances** ($d \in [2, 20]\text{ m}$), $\bar{P}(d) \gg -85\text{ dBm}$, so almost every beacon is received and points scatter symmetrically ($\pm 3\text{ dB}$ Gaussian shadowing) above and below the mean curve.
  - At **large distances** ($d > 35\text{ m}$), $\bar{P}(d)$ approaches and drops below $-85\text{ dBm}$. Any beacon with negative shadowing pushing $P_r < -85\text{ dBm}$ is dropped by sensitivity; consequently, **survivorship/truncation bias** causes recorded samples near $40\text{--}51\text{ m}$ to cluster exclusively above $-85\text{ dBm}$ and above the theoretical mean line.

---

### Question 5-5: Quantifying Sticky Client Behavior

- **(a) Equal Mean Power Midpoint:** By symmetry ($d_1 = d_2$), $\bar{P}_1(x) = \bar{P}_2(x)$ at **$x = D/2 = 25.50\text{ m}$** ($d = \sqrt{25.5^2 + 2^2} = 25.58\text{ m}$, $\bar{P} \approx -78.52\text{ dBm}$). An ideal SNR-optimal station should hand over around $x = 25.5\text{ m}$.
- **(b) Observed Handover Position & Sticky Delay in `Walk`:**
  - Across the 5 `Walk` runs, INET initiated scanning at $x = [38.52, 39.64, 41.07, 42.45, 35.60]\text{ m}$, with mean **$x_{ho} = 39.45 \pm 3.25\text{ m}$**.
  - **Sticky Client Delay:** $\Delta x = 39.45 - 25.50 = \mathbf{13.95\text{ m}}$, corresponding to **$\Delta t = 13.95 / 1.75 = \mathbf{7.97\text{ s}}$** spent connected to the far weaker `ap1` after crossing the midpoint!
- **(c) Why 3 Consecutive Lost Beacons Trigger Disconnect ($3.5\,BI$ Rule):** Every received beacon resets the $3.5\,BI = 350\text{ ms}$ timeout timer at time $t_0$. Subsequent beacons arrive at $t_0 + 100\text{ ms}$, $t_0 + 200\text{ ms}$, $t_0 + 300\text{ ms}$, and $t_0 + 400\text{ ms}$. Because $300\text{ ms} < 350\text{ ms} < 400\text{ ms}$, if **3 consecutive beacons** ($t_0 + 100, 200, 300\text{ ms}$) are lost, the $350\text{ ms}$ timer expires before the 4th beacon arrives at $t_0 + 400\text{ ms}$ — forcing disconnect after an integer count of **3 lost beacons**, not 3.5.

---

## 4. Part 3.3: Custom Roaming State Machine Design & Evaluation

### Question 5-6: FSM Design & Pre-Simulation Predictions

- **State Machine Design (`phase3_fsm_diagram.png`):** Two states (`Associated: ap1` and `Associated: ap2`). Each beacon updates an $N$-sample simple moving average ($\bar{P}_1, \bar{P}_2$) and resets the active AP's $3.5\,BI$ timer. Transition from `cur` to `other` occurs if:
  1. **Hysteresis Condition:** $\bar{P}_{cur} < T \;\land\; \bar{P}_{other} \ge \bar{P}_{cur} + H$, OR
  2. **Link-Loss Fallback:** $\Delta t_{cur} = t - t_{last,cur} > 3.5\,BI$.
- **Pre-Simulation Predictions:**
  - **(a) $H = 0\text{ dB}, N = 1$:** Without smoothing or hysteresis, $P_2 - P_1$ fluctuates randomly around $0\text{ dB}$ ($\sigma_{diff} = 3\sqrt{2} = 4.24\text{ dB}$) across the overlap region, causing dozens of ping-pong handovers.
  - **(b) $H = 6\text{ dB}$ (noiseless, ignoring $2\text{ m}$ offset):**
    $$10\alpha \log_{10}\left(\frac{x}{D - x}\right) = H \implies x = \frac{D}{1 + 10^{-\frac{H}{10\alpha}}} = \frac{51}{1 + 10^{-6/32}} = \mathbf{30.92\text{ m}}$$

---

### Question 5-7: FSM Parameter Sweep on Merged `Survey` Traces

We set initiation threshold **$T = -75.0\text{ dBm}$**: this sits $10\text{ dB}$ above receiver sensitivity ($-85\text{ dBm}$) so handover triggers cleanly before link failure, yet $3.5\text{ dB}$ above the midpoint mean ($-78.52\text{ dBm}$) so handover evaluation activates shortly before reaching $x = 25.5\text{ m}$ (avoiding unnecessary scanning while close to `ap1`).

| $N$ (SMA Window) | $H$ (dB) | Handover Counts `[r0, r1, r2]` | Mean Handovers | Final Handover $x$ (m) `[r0, r1, r2]` | Mean Final $x$ (m) |
|:---:|:---:|:---:|:---:|:---:|:---:|
| **$1$** | $0$ | `[41, 45, 27]` | $37.67$ | `[33.17, 34.37, 30.88]` | $32.81$ |
| **$1$** | $2$ | `[23, 23, 13]` | $19.67$ | `[33.18, 33.85, 28.73]` | $31.92$ |
| **$1$** | $4$ | `[11, 9, 7]` | $9.00$ | `[33.18, 27.65, 28.73]` | $29.85$ |
| **$1$** | $6$ | `[7, 3, 5]` | $5.00$ | `[33.18, 27.65, 28.73]` | $29.85$ |
| **$1$** | $8$ | `[3, 1, 3]` | $2.33$ | `[27.92, 26.67, 28.73]` | $27.78$ |
| **$1$** | $10$ | `[1, 1, 1]` | $1.00$ | `[27.92, 28.17, 28.21]` | $28.10$ |
| **$5$** | $0$ | `[7, 7, 7]` | $7.00$ | `[28.27, 27.72, 26.15]` | $27.38$ |
| **$5$** | $2$ | `[1, 1, 1]` | $1.00$ | `[28.62, 28.25, 26.28]` | $27.72$ |
| **$5$** | $4$ | `[1, 1, 1]` | $1.00$ | `[29.32, 28.87, 28.03]` | $28.74$ |
| **$5$** | $6$ | `[1, 1, 1]` | $1.00$ | `[30.73, 30.35, 28.21]` | $29.76$ |
| **$10$** | $0$ | `[3, 3, 1]` | $2.33$ | `[28.80, 28.07, 26.33]` | $27.73$ |
| **$10$** | $2$ | `[1, 1, 1]` | $1.00$ | `[29.15, 28.77, 26.85]` | $28.26$ |
| **$10$** | **$4$** | **`[1, 1, 1]`** | **$1.00$** | **`[30.55, 30.10, 28.21]`** | **$29.62$** |
| **$10$** | $6$ | `[1, 1, 1]` | $1.00$ | `[31.25, 31.32, 28.21]` | $30.26$ |
| **$10$** | $8$ | `[1, 1, 1]` | $1.00$ | `[31.77, 35.70, 28.21]` | $31.89$ |
| **$10$** | $10$ | `[1, 1, 1]` | $1.00$ | `[36.50, 36.30, 28.21]` | $33.67$ |

- **Where Ping-Pong Handovers Occur & Why:** Ping-pong switching clusters in the central overlap zone ($x \approx 20\text{--}34\text{ m}$), where $\bar{P}_1 \approx \bar{P}_2 \approx -78.5\text{ dBm} < T$ and shadowing variance ($\sigma = 3\text{ dB}$) frequently reverses which AP appears stronger unless damped by $N$ and $H$.
- **Recommended $(H, N)$ Pair & Comparison with INET `Walk`:** Choosing **$H = 4\text{ dB}, N = 10$** (or $H = 4\text{ dB}, N = 5$) yields **exactly 1 handover in all 3 runs** with mean handover location **$x = 29.62\text{ m}$** ($28.74\text{ m}$ for $N=5$) — only $4.12\text{ m}$ ($2.35\text{ s}$) past the $25.5\text{ m}$ midpoint, eliminating **$9.83\text{ m}$ ($5.62\text{ s}$)** of INET's $39.45\text{ m}$ sticky-client lag!
- **Drawback of Excessively Large $H$ and $N$:** Increasing $H$ delays handover deeper into the coverage edge of `ap1`, re-introducing sticky-client degradation and triggering $3.5\,BI$ disconnects before hysteresis is met (notice rep 2 already fell back to `timeout` at $28.21\text{ m}$ for $H \ge 4\text{--}6\text{ dB}$); increasing $N$ introduces moving-average lag ($N \cdot BI = 1\text{ s}$ memory), making a fast-moving station slow to react to steep signal drops.

---

## 5. Figure Inventory & Capture Status

All 300-DPI figures are exported by `phase3/phase3.ipynb` to `CA2-WiFi-Performance/report/figures/` (excluded from Git via `.gitignore`):

1. `phase3_handover_outage.png` — `Walk` rep 0 frame timeline and 5-run outage comparison (`Walk` vs. `ScanNeighbor`, 95% CI).
2. `phase3_survey_power_vs_distance.png` — `Survey` received beacon power vs. log-distance and sticky-client delay trajectory.
3. `phase3_fsm_diagram.png` — Designed state-machine diagram of the proposed hysteresis + moving-average handover FSM.
4. `phase3_fsm_sweep.png` — Ping-pong handover suppression and final handover position across $H \in [0, 10]\text{ dB}$ and $N \in \{1, 5, 10\}$.

- **Manual IDE Screenshot Status:** Per `CA2.pdf` (p. 18), **no Qtenv or `.elog` Sequence Chart screenshots are required for Phase 3** (all required outputs are the designed FSM diagram and analytical/simulation plots above).

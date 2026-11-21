# Prototype Test Results & Bench Verification Records

> [!IMPORTANT]
> **Honesty & Integrity Policy**:
> As specified in the project engineering guidelines, no test data, battery life metrics, or calibration curves are fabricated.
> 
> **Current Status**: **Hardware Bench Validation Not Yet Executed.**
> The tables below are blank verification templates. They will be populated by the author (Vigneshwar S) following physical bench testing on assembled hardware at SRM Institute of Science and Technology.

---

## 1. Zero-Pressure Baseline & Noise Characterization

- **Ambient Temperature**: ________ °C
- **Relative Humidity**: ________ %
- **Barometric Pressure**: ________ hPa
- **Supply Voltage (VDD)**: ________ V

| Parameter | Theoretical / Target | Measured Value (Uncalibrated) | Measured Value (Post-Zero Calibration) | Unit | Notes / Observations |
| :--- | :---: | :---: | :---: | :---: | :--- |
| Zero Offset Voltage ($V_0$) | 1.500 | | | V | Scaled at ADS1115 A0 (nominal 2.5V * 0.6) |
| Peak-to-Peak Noise ($V_{p-p}$) | < 0.010 | | | V | 10-second motionless recording |
| RMS Noise Floor ($\sigma$) | < 0.003 | | | V | Baseline noise standard deviation |
| Zero Pressure Drift (30 min) | < 0.015 | | | V | Drift under motionless open-port condition |

---

## 2. Supervised Tidal Breathing Rate Validation

Validation against manual counting over 60-second intervals while awake and seated.

| Test Run # | Subject Condition | Manual Counted Breaths (60s) | Logger Algorithm Reported BPM | Absolute Error (BPM) | Error Percentage (%) | Notes |
| :---: | :--- | :---: | :---: | :---: | :---: | :--- |
| Run 1 | Normal resting breathing | | | | | |
| Run 2 | Normal resting breathing | | | | | |
| Run 3 | Slow deep breathing (~8 BPM) | | | | | |
| Run 4 | Fast shallow breathing (~24 BPM) | | | | | |
| Run 5 | Post-exercise breathing (~28 BPM) | | | | | |

---

## 3. Breathing Pause Alert (Prototype) Verification

Controlled voluntary breath-holds performed while awake with an observer present. Configured threshold duration: $10.0\text{ s}$.

| Test # | Intended Hold Duration (s) | Stopwatch Duration (s) | Logger Pause Triggered? (Yes/No) | Logger Reported Pause Duration (s) | Delay to Alert (s) | Resume Detection Time (s) |
| :---: | :---: | :---: | :---: | :---: | :---: | :---: |
| Pause 1 | 5.0 s (Sub-threshold) | | | | | |
| Pause 2 | 12.0 s | | | | | |
| Pause 3 | 15.0 s | | | | | |
| Pause 4 | 20.0 s | | | | | |

---

## 4. BLE Wireless Link & Range Evaluation

Receiver device: Google Chrome on Laptop / Android smartphone running Web Bluetooth dashboard.

| Distance (m) | Environment / Obstacles | Packet Reception Rate (%) | Live Graph Smoothness (1-5) | Reconnect Time on Disconnect (s) |
| :---: | :--- | :---: | :---: | :---: |
| 1.0 m | Direct line-of-sight (desk) | | | |
| 3.0 m | Direct line-of-sight | | | |
| 5.0 m | Line-of-sight | | | |
| 5.0 m | Through wooden door | | | |

---

## 5. Battery Discharge & Run-Time Log

Battery under test: 1S LiPo (Nominal 3.7V, Rated Capacity: ________ mAh). BLE active streaming at 20 Hz waveform updates.

| Elapsed Time (hours) | Measured Battery Voltage (V) | Reported Battery % | Status / Operational Check |
| :---: | :---: | :---: | :--- |
| 0.0 h | | 100% | Test start |
| 1.0 h | | | |
| 2.0 h | | | |
| 3.0 h | | | |
| 4.0 h | | | Cutoff reached |

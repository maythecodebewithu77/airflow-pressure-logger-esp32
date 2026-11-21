# Test Plan & Verification Protocol

## Safety Notice
This testing protocol is designed exclusively for educational evaluation on healthy, awake volunteers under direct supervision.
- **Never perform tests while asleep or alone.**
- **Never restrict natural breathing.**
- **Halt testing immediately if any dizziness or resistance is sensed.**

---

## Test Matrix

### Test 1: Zero-Flow Noise Floor & Baseline Stability
- **Objective**: Measure ADC noise floor and baseline drift in stationary room air.
- **Method**: Place sensor on desk with ports open to still air. Record waveform at 50 Hz for 10 minutes.
- **Acceptance Criteria**: Peak-to-peak noise $< 15\text{ Pa}$; drift over 10 minutes $< 25\text{ Pa}$.

### Test 2: Inhale/Exhale Direction & Polarity Verification
- **Objective**: Verify correct sign deflection on inhale vs exhale.
- **Method**: Take 5 gentle, rhythmic breaths through the flow restrictor tube while watching the live dashboard.
- **Acceptance Criteria**: Inhalation deflections are consistently negative; exhalation deflections are consistently positive; signal returns to zero during inter-breath pauses.

### Test 3: Breathing Rate Accuracy vs Manual Counting
- **Objective**: Validate calculated Breaths Per Minute (BPM) against ground truth manual counting.
- **Method**:
  - Test at three cadences:
    1. Normal resting cadence (~12-16 BPM) for 60 seconds.
    2. Slow controlled cadence (~8 BPM) for 60 seconds.
    3. Fast paced breathing (~24-28 BPM) for 60 seconds.
  - A partner counts chest rise-and-fall cycles with a stopwatch while logger records.
- **Acceptance Criteria**: Algorithm reported BPM matches manual count within $\pm 1\text{ BPM}$ at steady cadence.

### Test 4: Breathing Pause Alert (Prototype) Verification
- **Objective**: Verify that cessation of flow triggering exceeds threshold duration and raises an event.
- **Method**:
  - Set pause threshold duration in `config.h` to $10.0\text{ seconds}$.
  - While awake and seated, breathe normally for 30 seconds.
  - Gently hold breath at end-expiration for 12 to 15 seconds under observer supervision.
  - Resume normal breathing.
- **Acceptance Criteria**:
  - Pause event banner activates on dashboard when elapsed time without flow crosses 10.0 seconds.
  - Recorded pause duration matches stopwatch timing within $\pm 0.5\text{ seconds}$.
  - Status immediately clears to "NORMAL" upon the first valid inspiratory or expiratory stroke.

### Test 5: Sensor Disconnection / Flatline Fault Discrimination
- **Objective**: Verify that a disconnected sensor is flagged as a hardware fault and NEVER as a physiological breathing pause.
- **Method**:
  - Disconnect the analog signal jumper between MPXV7002DP and ADS1115 (or unplug ADS1115 I2C).
- **Acceptance Criteria**:
  - Firmware flags `STATUS_SENSOR_FAULT`.
  - Dashboard indicates "SENSOR FAULT: Signal Disconnected / Flatline".
  - Breathing pause alert is suppressed.

### Test 6: BLE Wireless Stability & Reconnection
- **Objective**: Assess Bluetooth Low Energy connection robustness.
- **Method**: Stream live data to web dashboard at distances of 1m, 3m, and 5m. Deliberately toggle computer Bluetooth off and on.
- **Acceptance Criteria**: Zero crash/freeze on ESP32; dashboard detects disconnect and successfully auto-reconnects when prompted.

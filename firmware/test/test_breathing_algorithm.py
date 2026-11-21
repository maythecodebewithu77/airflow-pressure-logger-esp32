"""
Unit Test Suite for Breathing Algorithm Signal Processing
Project: airflow-pressure-logger-esp32
Author: Vigneshwar S

Tests the core algorithm logic:
1. Inhale and exhale phase detection from airflow sign.
2. Breaths-per-minute (BPM) calculation on synthetic waveforms of known cadences (15 BPM and 8 BPM).
3. Breathing pause alert (prototype) trigger on waveforms with a flow gap (>10s).
4. Sensor disconnect / flatline fault detection: verifies that hardware failure is NEVER flagged as a breathing pause.
"""

import math
import sys

# Constants matching firmware/src/config.h
BREATH_START_THRESHOLD_FLOW = 3.0
BREATH_END_HYSTERESIS_FLOW = 1.2
LOW_FLOW_THRESHOLD_FLOW = 2.0
PAUSE_THRESHOLD_FLOW = 1.0
PAUSE_ALERT_THRESHOLD_SEC = 10.0
MIN_BREATH_PERIOD_MS = 1200
MAX_BREATH_PERIOD_MS = 15000
BPM_ROLLING_BUFFER_SIZE = 8

STATUS_BIT_NORMAL = (1 << 0)
STATUS_BIT_LOW_FLOW = (1 << 1)
STATUS_BIT_PAUSE_ALERT = (1 << 2)
STATUS_BIT_SENSOR_FAULT = (1 << 3)
STATUS_BIT_DEMO_MODE = (1 << 4)


class BreathingAnalyzerSim:
    def __init__(self):
        self.reset()

    def reset(self):
        self.bpm = 0.0
        self.peak_insp_flow = 0.0
        self.peak_exp_flow = 0.0
        self.ie_ratio = 1.0
        self.current_pause_duration_s = 0.0
        self.status_flags = STATUS_BIT_NORMAL
        self.pause_event_raised = False
        self.pause_event_completed = False
        self.last_pause_start_ms = 0
        self.last_pause_duration_ms = 0

        self.current_phase = "IDLE"
        self.last_breath_start_ms = 0
        self.insp_start_ms = 0
        self.exp_start_ms = 0
        self.insp_duration_ms = 1500.0
        self.exp_duration_ms = 2500.0

        self.buffer = []
        self.cycle_peak_insp = 0.0
        self.cycle_peak_exp = 0.0
        self.pause_timer_start_ms = 0
        self.pause_alert_latched = False

    def update_bpm(self, cycle_duration_ms):
        if cycle_duration_ms < MIN_BREATH_PERIOD_MS or cycle_duration_ms > MAX_BREATH_PERIOD_MS:
            return
        self.buffer.append(cycle_duration_ms)
        if len(self.buffer) > BPM_ROLLING_BUFFER_SIZE:
            self.buffer.pop(0)
        avg_period_sec = (sum(self.buffer) / len(self.buffer)) / 1000.0
        if avg_period_sec > 0.0:
            self.bpm = 60.0 / avg_period_sec

    def update(self, now_ms, relative_flow, sensor_fault=False):
        self.pause_event_raised = False
        self.pause_event_completed = False

        # 1. Fault Discrimination
        if sensor_fault:
            self.status_flags = STATUS_BIT_SENSOR_FAULT
            self.pause_timer_start_ms = 0
            self.pause_alert_latched = False
            self.current_pause_duration_s = 0.0
            self.current_phase = "IDLE"
            return

        abs_flow = abs(relative_flow)

        # 2. Breathing Pause Alert (prototype) Logic
        if abs_flow < PAUSE_THRESHOLD_FLOW:
            if self.pause_timer_start_ms == 0:
                self.pause_timer_start_ms = now_ms
            pause_sec = (now_ms - self.pause_timer_start_ms) / 1000.0
            self.current_pause_duration_s = pause_sec

            if pause_sec >= PAUSE_ALERT_THRESHOLD_SEC:
                if not self.pause_alert_latched:
                    self.pause_alert_latched = True
                    self.pause_event_raised = True
                    self.last_pause_start_ms = self.pause_timer_start_ms
                self.status_flags |= STATUS_BIT_PAUSE_ALERT
        elif abs_flow >= BREATH_START_THRESHOLD_FLOW:
            if self.pause_alert_latched:
                self.last_pause_duration_ms = now_ms - self.last_pause_start_ms
                self.pause_event_completed = True
                self.pause_alert_latched = False
            self.pause_timer_start_ms = 0
            self.current_pause_duration_s = 0.0
            self.status_flags &= ~STATUS_BIT_PAUSE_ALERT

        # 3. Phase & Peak Detection
        if relative_flow < -BREATH_START_THRESHOLD_FLOW:
            if self.current_phase != "INSPIRATION":
                self.current_phase = "INSPIRATION"
                self.insp_start_ms = now_ms

                if self.exp_start_ms > 0:
                    self.exp_duration_ms = float(now_ms - self.exp_start_ms)

                if self.last_breath_start_ms > 0:
                    self.update_bpm(now_ms - self.last_breath_start_ms)
                self.last_breath_start_ms = now_ms

                self.peak_insp_flow = self.cycle_peak_insp
                self.peak_exp_flow = self.cycle_peak_exp
                self.cycle_peak_insp = 0.0
                self.cycle_peak_exp = 0.0

                if self.insp_duration_ms > 100.0:
                    self.ie_ratio = self.exp_duration_ms / self.insp_duration_ms

            if abs_flow > self.cycle_peak_insp:
                self.cycle_peak_insp = abs_flow

        elif relative_flow > BREATH_START_THRESHOLD_FLOW:
            if self.current_phase != "EXPIRATION":
                self.current_phase = "EXPIRATION"
                self.exp_start_ms = now_ms
                if self.insp_start_ms > 0:
                    self.insp_duration_ms = float(now_ms - self.insp_start_ms)

            if abs_flow > self.cycle_peak_exp:
                self.cycle_peak_exp = abs_flow

        elif abs_flow < BREATH_END_HYSTERESIS_FLOW:
            self.current_phase = "IDLE"

        # 4. Status Flag Assembly
        if not (self.status_flags & STATUS_BIT_PAUSE_ALERT):
            if 0.0 < self.peak_insp_flow < LOW_FLOW_THRESHOLD_FLOW:
                self.status_flags = STATUS_BIT_LOW_FLOW
            else:
                self.status_flags = STATUS_BIT_NORMAL


def run_tests():
    print("==================================================================")
    print("Running Breathing Algorithm Unit Test Suite")
    print("Author: Vigneshwar S (SRM Institute of Science and Technology)")
    print("==================================================================")
    all_passed = True

    # -------------------------------------------------------------------------
    # TEST 1: Synthetic 15 BPM Waveform (Period = 4.0s)
    # -------------------------------------------------------------------------
    analyzer = BreathingAnalyzerSim()
    fs = 50.0  # 50 Hz sampling
    dt = 1.0 / fs
    duration_s = 60.0
    steps = int(duration_s * fs)
    period_s = 4.0  # 15 BPM

    for step in range(steps):
        t = step * dt
        now_ms = int(t * 1000)
        phase_in_cycle = (t % period_s) / period_s

        # Asymmetric breath: 40% inhale, 60% exhale
        if phase_in_cycle < 0.40:
            flow = -22.0 * math.sin(phase_in_cycle / 0.40 * math.pi)
        else:
            flow = 18.0 * math.sin((phase_in_cycle - 0.40) / 0.60 * math.pi)

        analyzer.update(now_ms, flow, sensor_fault=False)

    print(f"[TEST 1] Known 15.0 BPM Cadence Test:")
    print(f"         Expected: 15.0 BPM | Computed: {analyzer.bpm:.1f} BPM")
    error = abs(analyzer.bpm - 15.0)
    if error < 0.2:
        print("         Result: PASS (Within +/- 0.2 BPM error band)")
    else:
        print(f"         Result: FAIL (Error {error:.2f} BPM)")
        all_passed = False

    # -------------------------------------------------------------------------
    # TEST 2: Synthetic Slow 8 BPM Waveform (Period = 7.5s)
    # -------------------------------------------------------------------------
    analyzer.reset()
    period_slow = 7.5  # 8 BPM
    steps_slow = int(60.0 * fs)

    for step in range(steps_slow):
        t = step * dt
        now_ms = int(t * 1000)
        phase = (t % period_slow) / period_slow
        if phase < 0.45:
            flow = -25.0 * math.sin(phase / 0.45 * math.pi)
        else:
            flow = 22.0 * math.sin((phase - 0.45) / 0.55 * math.pi)
        analyzer.update(now_ms, flow, sensor_fault=False)

    print(f"[TEST 2] Slow 8.0 BPM Cadence Test:")
    print(f"         Expected: 8.0 BPM | Computed: {analyzer.bpm:.1f} BPM")
    error = abs(analyzer.bpm - 8.0)
    if error < 0.2:
        print("         Result: PASS (Within +/- 0.2 BPM error band)")
    else:
        print(f"         Result: FAIL (Error {error:.2f} BPM)")
        all_passed = False

    # -------------------------------------------------------------------------
    # TEST 3: Breathing Pause Alert Test (12.0s voluntary gap)
    # -------------------------------------------------------------------------
    analyzer.reset()
    pause_detected = False
    pause_raised_time = 0.0
    pause_completed_received = False
    reported_pause_duration = 0.0

    # 20s breathing -> 12s pause (flow = 0) -> resume breathing
    for step in range(int(45.0 * fs)):
        t = step * dt
        now_ms = int(t * 1000)

        if 15.0 <= t < 27.0:
            flow = 0.0  # 12-second breath hold
        else:
            flow = -20.0 * math.sin((t % 4.0) / 4.0 * math.pi * 2)

        analyzer.update(now_ms, flow, sensor_fault=False)

        if analyzer.pause_event_raised:
            pause_detected = True
            pause_raised_time = t

        if analyzer.pause_event_completed:
            pause_completed_received = True
            reported_pause_duration = analyzer.last_pause_duration_ms / 1000.0

    print(f"[TEST 3] Breathing Pause Alert (prototype) Verification:")
    print(f"         12.0s flow pause injected at t=15.0s.")
    print(f"         Pause alert triggered: {pause_detected} at t={pause_raised_time:.1f}s (delay = {pause_raised_time - 15.0:.1f}s)")
    print(f"         Pause completion detected on resume: {pause_completed_received}")
    print(f"         Reported pause duration: {reported_pause_duration:.1f}s (Expected: 12.0s)")

    if pause_detected and pause_completed_received and abs(reported_pause_duration - 12.0) < 0.5:
        print("         Result: PASS")
    else:
        print("         Result: FAIL")
        all_passed = False

    # -------------------------------------------------------------------------
    # TEST 4: Sensor Disconnected / Flatline Fault Discrimination
    # -------------------------------------------------------------------------
    analyzer.reset()
    # Breathe normally for 10s, then simulate sensor disconnect (fault = True)
    for step in range(int(30.0 * fs)):
        t = step * dt
        now_ms = int(t * 1000)

        if t < 10.0:
            flow = -20.0 * math.sin((t % 4.0) / 4.0 * math.pi * 2)
            fault = False
        else:
            flow = 0.0
            fault = True  # Hardware disconnection

        analyzer.update(now_ms, flow, sensor_fault=fault)

    print(f"[TEST 4] Sensor Disconnect Fault Discrimination:")
    print(f"         Sensor Fault Flag Active: {bool(analyzer.status_flags & STATUS_BIT_SENSOR_FAULT)}")
    print(f"         Breathing Pause Flag Active: {bool(analyzer.status_flags & STATUS_BIT_PAUSE_ALERT)}")

    if (analyzer.status_flags & STATUS_BIT_SENSOR_FAULT) and not (analyzer.status_flags & STATUS_BIT_PAUSE_ALERT):
        print("         Result: PASS (Hardware fault correctly prioritized; NEVER misreported as pause)")
    else:
        print("         Result: FAIL (Disconnected sensor incorrectly triggered pause alert!)")
        all_passed = False

    print("==================================================================")
    if all_passed:
        print("ALL UNIT TESTS PASSED SUCCESSFULLY!")
    else:
        print("SOME TESTS FAILED.")
        sys.exit(1)


if __name__ == "__main__":
    run_tests()

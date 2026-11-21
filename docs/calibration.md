# Calibration & Tuning Guide

## 1. Safety Rules & Precautions
> [!CAUTION]
> - Never apply compressed air, pressurized lines, or hospital wall gas to the sensor ports. The MPXV7002DP maximum overpressure rating is $75\text{ kPa}$ ($10.8\text{ psi}$); exceeding this will permanently rupture the internal silicon diaphragm.
> - Calibration must be performed with the sensing tube open to the room atmosphere. Never create a sealed airtight fixture around the mouth or nose.

---

## 2. Zero-Offset Calibration Procedure

The piezoresistive silicon diaphragm in the MPXV7002DP exhibits mechanical and thermal offset voltage. Although nominal center voltage is $2.50\text{ V}$ ($1.50\text{ V}$ after the 0.60 divider), real units vary between $2.30\text{ V}$ and $2.70\text{ V}$.

### Step-by-Step Zero Routine:
1. **Pneumatic Equilibrium**:
   - Ensure both pressure ports (P1 and P2) on the flow restrictor are disconnected from breathing masks and exposed to calm ambient air.
   - Avoid drafts from room ceiling fans or air conditioners during this step.
2. **Thermal Stabilization**:
   - Allow the ESP32 and MPXV7002DP to warm up for at least $60\text{ seconds}$ so the electronics reach thermal equilibrium.
3. **Execution**:
   - On boot, the firmware enters an automatic 3-second zero routine, collecting 150 consecutive samples at 50 Hz.
   - Alternatively, trigger zero calibration from the web dashboard by pressing the **"Calibrate Zero Offset"** button.
   - The computed average voltage is stored in memory as `v_zero` and subtracted from subsequent measurements.

---

## 3. Port Polarity & Direction Check

Differential pressure is signed: $\Delta P = P_1 - P_2$.
- **Inhalation Convention**: Air drawn into the lungs creates a negative pressure relative to the room or downstream port ($P_1 < P_2 \implies \Delta P < 0$).
- **Exhalation Convention**: Expelled breath creates positive pressure upstream ($P_1 > P_2 \implies \Delta P > 0$).

### Polarity Test:
1. Connect Port 1 (P1) of the MPXV7002DP to the tap closest to the mouth/nose.
2. Connect Port 2 (P2) to the atmospheric/exhaust tap.
3. Observe the live waveform on the web dashboard:
   - Exhaling gently into the restrictor should produce a positive deflection above the zero line.
   - Inhaling gently should produce a negative deflection below the zero line.
4. If deflections are inverted, simply swap the two silicone tube connections at the sensor nipples.

---

## 4. Flow Calibration Approaches

### Method A: Low-Pressure Manometer (Static Calibration)
A simple U-tube water manometer can verify sensor sensitivity ($\text{V/Pa}$):
- $10\text{ mm H}_2\text{O} = 98.07\text{ Pa}$.
- Applying a tiny differential liquid column height verifies that the ADS1115 ADC reading changes in accordance with the theoretical transfer function:
  $$V_{\text{out}} = V_{\text{center}} + 1.0\text{ V/kPa} \times \Delta P$$

### Method B: Known-Volume Syringe (Dynamic Relative Calibration)
1. Use a large plastic veterinary or calibration syringe ($500\text{ mL}$ or $1000\text{ mL}$).
2. Connect the syringe nozzle to the flow restrictor inlet.
3. Empty the syringe steadily over a measured duration (e.g., $1000\text{ mL}$ in $2.0\text{ seconds} = 30\text{ L/min}$ average flow).
4. Integrate the flow curve: $\text{Volume} = \int Q(t) \, dt$.
5. Adjust `FLOW_CALIBRATION_COEFF` in `firmware/src/config.h` until the integrated volume matches the syringe volume.

---

## 5. Honest Limitations of Hobby Calibration
- **Lack of NIST/NABL Calibrated Primary Standard**: In a university or hobby setting, absolute flow accuracy is subject to an estimated $\pm 10\text{ to }15\%$ error band.
- **Turbulent Flow Regimes**: Orifice flow transitions between laminar and turbulent regimes across the breathing cycle, altering the effective discharge coefficient $C_d$.
- **Temperature & Humidity Effects**: Warm, moisture-saturated exhaled air ($34^\circ\text{C}$, $100\%$ RH) has a different density and viscosity than cooler ambient room air ($24^\circ\text{C}$, $50\%$ RH).
- **Labeling Standard**: In all academic write-ups and dashboard metrics, airflow rates must be designated as **`rel. L/min`** (relative liters per minute).

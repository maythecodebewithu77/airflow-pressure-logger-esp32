# Mechanical Flow Restrictor Design & Calibration Guide

## Overview
To measure airflow with a differential pressure sensor, the moving airstream must pass across a known, low-resistance fluid restriction. The resulting pressure drop ($\Delta P = P_1 - P_2$) across the restriction is measured by the sensor and converted into an estimated volumetric flow rate ($Q$).

> [!CAUTION]
> **Safety Critical Requirement**: 
> 1. The sensing channel MUST always allow free, unrestricted natural breathing. The total flow resistance must not exceed $0.15\text{ kPa}$ ($1.5\text{ cm H}_2\text{O}$) at peak inspiratory flow ($60\text{ L/min}$).
> 2. Never construct a seal or tight blockage over the mouth or nose. Use an open-ended tube design positioned near the nasal or oral airway, or a loose open-vent mask cup.

---

## Restrictor Types & Operating Principles

### 1. Orifice Plate Restrictor (Recommended for 3D Printing)
An orifice restrictor consists of a cylindrical breathing tube with an internal circular plate containing a central aperture.

- **Fluid Mechanics Relation**: 
  According to Bernoulli's equation with empirical discharge coefficient $C_d$:
  $$\Delta P = \frac{\rho}{2} \left( \frac{1}{C_d^2 A_o^2} - \frac{1}{A_p^2} \right) Q^2$$
  Where:
  - $\rho \approx 1.204 \text{ kg/m}^3$ (air density at sea level, 20°C)
  - $A_p = \frac{\pi}{4} D^2$ (cross-sectional area of main pipe)
  - $A_o = \frac{\pi}{4} d^2$ (cross-sectional area of orifice)
  - $Q$ = volumetric flow rate ($\text{m}^3\text{/s}$ or $\text{L/min}$)
  - $C_d \approx 0.62$ (discharge coefficient for sharp-edged circular orifice)

- **Flow Estimation Formula**:
  $$Q = k_{\text{cal}} \cdot \text{sgn}(\Delta P) \cdot \sqrt{|\Delta P|}$$

### 2. Mesh Screen (Fleisch / Lilly Pneumotachograph Style)
Instead of a single constriction, a fine stainless steel or nylon mesh (e.g., 200–400 mesh) is placed across the tube diameter.
- **Fluid Mechanics Relation**: At low Reynolds numbers ($Re < 2000$), flow across a fine mesh is laminar and obeys the Darcy-Weisbach / Hagen-Poiseuille relationship:
  $$\Delta P = R_{\text{pneumo}} \cdot Q$$
- The pressure drop is linearly proportional to flow rate. However, mesh screens are sensitive to moisture and saliva droplet buildup, requiring frequent drying.

---

## Mechanical Specifications & Parametric Dimensions

The following dimensions are optimized for resting tidal breathing in healthy young adults ($Q_{\text{peak}} \approx 15\text{ to }35\text{ L/min}$):

| Dimension Parameter | Symbol | Nominal Value | Configurable Range | Engineering Rationale |
| :--- | :---: | :---: | :---: | :--- |
| **Main Tube Inner Diameter** | $D$ | $18.0\text{ mm}$ | $15.0 - 22.0\text{ mm}$ | Matches standard ISO respiratory tubing connectors; low base resistance. |
| **Orifice Constriction Diameter** | $d$ | $8.0\text{ mm}$ | $6.5 - 9.5\text{ mm}$ | Yields $\approx 30 - 80\text{ Pa}$ drop during normal tidal breathing. |
| **Diameter Ratio (Beta)** | $\beta = d/D$| $0.44$ | $0.35 - 0.55$ | Ensures sufficient differential signal without causing noticeable flow resistance. |
| **Upstream Tap Distance** | $L_1$ | $25.0\text{ mm}$ | $20 - 30\text{ mm}$ | Placed $1.4 D$ upstream from orifice to measure undisturbed static pressure. |
| **Downstream Tap Distance** | $L_2$ | $15.0\text{ mm}$ | $10 - 20\text{ mm}$ | Placed near vena contracta zone for maximum pressure differential. |
| **Pressure Port Outer Diameter** | $D_{\text{port}}$ | $4.2\text{ mm}$ | $3.8 - 4.5\text{ mm}$ | Barbed profile to ensure airtight friction fit with $2.5\text{ mm}$ ID silicone tubing. |
| **Pressure Port Inner Bore** | $d_{\text{port}}$ | $2.0\text{ mm}$ | $1.5 - 2.5\text{ mm}$ | Fast pneumatic step response; prevents water droplet capillary blockage. |
| **Total Tube Length** | $L_{\text{total}}$| $75.0\text{ mm}$ | $60 - 90\text{ mm}$ | Compact, lightweight for hand-held or loose harness support. |

---

## 3D Printing & Fabrication Guidelines

1. **Material Selection**:
   - **PETG** is strongly recommended over PLA because it offers superior chemical resistance to isopropyl alcohol (IPA) wipe cleaning and higher heat deflection temperature.
   - Food-safe or medical-grade biocompatible filament is preferred if available.
2. **Slicer Settings**:
   - Layer height: $0.15\text{ mm}$ or $0.20\text{ mm}$ (finer layers ensure smooth internal bore walls and reduce turbulence).
   - Infill: $100\%$ concentric perimeters for the orifice region and tube wall to guarantee zero pneumatic leakage through wall layers.
   - Print orientation: Print vertically along the tube axis to produce round, concentric bores without support material inside the airway.
3. **Pneumatic Port Assembly**:
   - Inspect the $2.0\text{ mm}$ port holes after printing; clear any stray stringing or burrs using a $2.0\text{ mm}$ drill bit spun by hand.
   - Use soft medical silicone tubing ($2.5\text{ mm}$ ID $\times 4.0\text{ mm}$ OD). Push the tubing firmly over the barbed ports. A small zip tie or silicone O-ring provides extra mechanical clamp.

---

## Calibration Approach & Disclaimer

> [!WARNING]
> **Calibration Requirement**: Without calibration against a known physical flow standard, all volumetric readings (in $\text{L/min}$) calculated by the firmware are **relative estimates only**. Uncalibrated values must be reported with the unit `rel. L/min` or arbitrary units (`A.U.`).

To calibrate the assembly:
1. **Zero Point**: With no air moving through the tube, record the ADC voltage over 200 samples ($4\text{ seconds}$ at $50\text{ Hz}$) to determine the resting zero-offset.
2. **Slope Calibration**: Pass a known steady flow through the restrictor (e.g., using a calibrated calibration syringe of $1.0\text{ L}$ or $3.0\text{ L}$ volume delivered over an accurately timed period) and adjust the parameter `FLOW_CALIBRATION_COEFF` in `firmware/src/config.h`.

# Custom PCB Design Guidelines (KiCad Project Notes)

## Overview
This document outlines layout, thermal, pneumatic, and electrical specifications for the custom 2-layer PCB designed by Vigneshwar S in KiCad. It captures best practices for precision analog acquisition, wireless transmission, and low-noise sensor operation.

---

## 1. Board Stackup & Layer Distribution

- **Layer Count**: 2-Layer FR4 ($1.6\text{ mm}$ standard thickness, $1\text{ oz}$ copper).
- **Top Layer**: Component placement, signal routing, power trace distribution, and copper fill tied to GND.
- **Bottom Layer**: Continuous, unbroken ground plane (serving as return path and EMI shield). Minimal signal traces should cross the bottom layer; any trace crossing must have adjacent stitching vias to preserve ground return continuity.

---

## 2. Component Placement & Thermal Management

1. **ESP32 Microcontroller & Antenna Zone**:
   - Place the ESP32-WROOM-32 module on the board edge with the PCB trace antenna overhanging the board or cleared of copper on all layers ($15\text{ mm}$ clearance).
   - The ESP32 2.4 GHz radio transmitter and onboard 3.3V LDO generate localized thermal spikes (board temperature can rise by $5^\circ\text{C}$ to $12^\circ\text{C}$ during active BLE RF bursts).
2. **Sensor Physical Isolation**:
   - Place the **MPXV7002DP** and **BME280** on the opposite end of the PCB, far away from the ESP32 antenna, LDO regulator, and battery charging circuit.
   - Add a thermal relief slot (milled cutout $1.5\text{ mm} \times 15\text{ mm}$) through the PCB between the ESP32/power section and the analog sensor cluster to eliminate conducted thermal drift.
   - Keep sensors isolated from internal enclosure drafts.

---

## 3. Pneumatic Tubing Ports & Mechanical Strain Relief

- Orient the dual pressure ports of the MPXV7002DP facing outwards toward the edge of the enclosure.
- Design two mechanical strain-relief slots or zip-tie anchoring holes ($3.2\text{ mm} \times 1.6\text{ mm}$) on the PCB substrate adjacent to the sensor ports.
- Silicone tubing must be clamped to the PCB before mating with the sensor nipples. This prevents mechanical tugging on the tubing from stressing the sensor solder joints or fracturing the package pins.

---

## 4. Analog Signal Conditioning & ADC Layout

1. **ADS1115 ADC Placement**:
   - Position the ADS1115 directly adjacent to the MPXV7002DP sensor output pin ($< 15\text{ mm}$ trace length).
   - Route the analog trace ($V_{\text{out}}$) with a guard trace or sandwiched between ground pours to avoid picking up high-speed digital transients from the ESP32.
2. **Passive Divider Placement**:
   - Place the $10\text{ k}\Omega$ and $15\text{ k}\Omega$ level-shifting resistors immediately adjacent to ADS1115 pin A0.
   - Use $0.1\%$ or $1\%$ metal film thin-film resistors (0805 or 0603) with a low temperature coefficient ($< 25\text{ ppm/}^\circ\text{C}$).
3. **Decoupling Capacitors**:
   - Place a $100\text{ nF}$ high-frequency ceramic capacitor (X7R) within $2\text{ mm}$ of the ADS1115 VDD pin, backed by a $10\text{ }\mu\text{F}$ tantalum or low-ESR ceramic capacitor.
   - Provide an independent local decoupling capacitor ($100\text{ nF} + 10\text{ }\mu\text{F}$) at the MPXV7002DP power supply pin.

---

## 5. I2C Bus Design & Pull-Up Sizing

- Shared I2C bus routes between ESP32 (GPIO 21 SDA, GPIO 22 SCL), ADS1115 (address `0x48`), and BME280 (address `0x76`).
- Include dedicated $4.7\text{ k}\Omega$ pull-up resistors to the $3.3\text{V}$ rail placed close to the microcontroller.
- Route SDA and SCL as parallel matched-length traces with ground copper isolation to avoid cross-coupling into adjacent analog lines.

---

## 6. Battery Management & Power Supply

1. **TP4056 Subsystem**:
   - Incorporate the TP4056 linear charger with DW01A battery protection IC and dual MOSFET (FS8205A) to guard against cell over-discharge ($< 2.5\text{V}$) and over-charge ($> 4.25\text{V}$).
   - Set charging current to $400\text{ mA}$ ($R_{\text{PROG}} = 3\text{ k}\Omega$) for a typical $800 - 1000\text{ mAh}$ cell to minimize thermal rise during charging.
2. **System LDO Regulator**:
   - Use an ultra-low dropout, low-noise regulator such as the **AP2112K-3.3** ($600\text{ mA}$ rating, $250\text{ mV}$ dropout at full load, $55\text{ }\mu\text{A}$ quiescent current).
   - This ensures regulated $3.3\text{V}$ output down to battery voltage $V_{\text{bat}} = 3.55\text{V}$.
3. **Battery Voltage Sense**:
   - Divider: $R_{\text{bat1}} = 100\text{ k}\Omega$, $R_{\text{bat2}} = 100\text{ k}\Omega$ connected to ESP32 GPIO 34.
   - Add $C_{\text{bat}} = 100\text{ nF}$ ceramic capacitor in parallel with $R_{\text{bat2}}$ to stabilize ADC readings.
4. **Slide Switch**:
   - Include a SPDT mechanical power switch between the battery positive terminal and the LDO input.

---

## 7. Test Points for Validation

Include standard $1.0\text{ mm}$ round SMD test pads for oscilloscope and multimeter probing:
- `TP_VBAT`: Raw battery voltage
- `TP_V3V3`: Regulated 3.3V logic supply
- `TP_V5V`: Sensor 5V boost / USB supply rail
- `TP_GND`: Ground reference loop
- `TP_SDA`: I2C Serial Data
- `TP_SCL`: I2C Serial Clock
- `TP_ANALOG`: Scaled differential sensor voltage at ADS1115 A0
- `TP_TXD0` / `TP_RXD0`: ESP32 UART0 flashing & serial console

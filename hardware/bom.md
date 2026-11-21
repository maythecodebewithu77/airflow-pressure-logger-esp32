# Bill of Materials (BOM) & Component Sourcing

## Overview
This document specifies the electronic and mechanical components required to assemble the Breathing Airflow and Mask-Pressure Logger prototype. It includes estimated market prices in Indian Rupees (INR) and local procurement channels in India (with specific notes for Chennai / Tamil Nadu).

---

## Component Parts List

| Item # | Description / Part Number | Function | Key Specifications | Approx Price (INR) | Recommended Source | Alternatives |
| :---: | :--- | :--- | :--- | :---: | :--- | :--- |
| 1 | **ESP32-WROOM-32 DevKit V1** | Main Microcontroller | Dual-core Xtensa 240MHz, BLE 4.2 / 5.0, 520KB SRAM | ₹380 - ₹450 | Robu.in / Ritchie Street (Supreme / Vega) | NodeMCU ESP32, ESP32-S3 DevKit |
| 2 | **MPXV7002DP** (or breakout) | Primary Differential Pressure Sensor | Dual port, piezoresistive, bidirectional $\pm 2\text{ kPa}$ ($\pm 0.3\text{ psi}$), 0.5V - 4.5V output | ₹1,250 - ₹1,650 | Robu.in / Mouser India / Evelta | Sensirion SDP810 (Digital I2C), MP3V5004DP ($\pm 3.92\text{ kPa}$) |
| 3 | **ADS1115 Breakout Module** | 16-Bit Precision ADC | 4-channel differential/single-ended, programmable gain amp, I2C interface | ₹180 - ₹240 | Robu.in / Amazon.in / Ritchie Street | ADS1015 (12-bit, faster but lower resolution), MCP3424 |
| 4 | **BME280 Module** (I2C/SPI) | Secondary Environmental Sensor | Absolute barometric pressure (300-1100 hPa), Temperature (-40 to 85°C), Relative Humidity (0-100%) | ₹280 - ₹350 | Robu.in / Amazon.in / Ritchie Street | BMP280 (Pressure + Temp only, lacks Humidity; ?120) |
| 5 | **TP4056 Li-Ion Charger Board** | Battery Management | Micro-USB or USB-C input, CC/CV 1A charging, DW01A overcharge/discharge protection | ₹35 - ₹50 | Ritchie Street / Robu.in | IP5306 module, MCP73831 standalone IC on custom PCB |
| 6 | **1S LiPo Battery (3.7V)** | Portable Power Source | 800 mAh to 1200 mAh, 1S pouch cell with JST-PH 2.0 connector | ₹280 - ₹380 | Robu.in / Evelta / Fab.to.lab | 18650 cylindrical cell with holder |
| 7 | **Medical Grade Silicone Tubing** | Pneumatic Routing | 2.5 mm ID / 4.0 mm OD, highly flexible, kink-resistant | ₹100 - ₹150 (per meter) | Amazon.in / Local surgical supply store | Aquarium soft PVC tubing (stiffer, less ideal) |
| 8 | **Passive Components** | Signal conditioning & divider | Resistors: 10k, 15k, 4.7k (x2), 100k (x2); Capacitors: 100nF (x3), 10uF (x2) | ₹40 - ₹60 | Ritchie Street / Local lab stock | 0805 SMD passives for custom PCB |
| 9 | **Mechanical Flow Restrictor** | Differential Pressure Generation | 3D printed PETG/PLA or acrylic tube with 6mm orifice plate and dual barbed taps | ₹120 - ₹200 (printing) | Campus 3D lab / Print3D / DIY pipe | Fleisch pneumotachograph, mesh laminar flow element |
| 10 | **Prototyping Hardware** | Interconnects | Breadboard / Perma-proto board, silicone jumper wires, SPDT slide switch | ₹80 - ₹120 | Local electronics shop | Direct wire soldering |
| **Total** | | | **Estimated Total Cost:** | **₹2,665 - ₹3,600** | | |

---

## Procurement Guide for India

1. **Ritchie Street (Chennai)**:
   - *Supreme Electronics, Vega Electronics, and Kits 'n' Spares* carry ESP32 DevKits, ADS1115 modules, BME280 breakouts, TP4056 boards, batteries, and passives at competitive prices.
   - Note: Analog differential pressure sensors (like the MPXV7002DP) are specialized industrial/automotive components and are rarely stocked on the retail counter at Ritchie Street. Purchase this component online.
2. **Online Indian Distributors**:
   - **Robu.in**: Most reliable stock for ESP32, ADS1115, BME280, TP4056, and differential pressure sensors.
   - **Evelta.com** / **Fab.to.lab**: Excellent for specialized sensors like MPXV7002DP, SDP810, and high-quality LiPo batteries.
   - **Mouser India / Element14 India**: Best for authentic ICs when transitioning to custom PCB manufacturing.
   - **Local Surgical / Medical Supply Shops (Parrys / Central Chennai)**: Excellent source for clean medical silicone tubing and Luer lock fittings.

---

## Technical Sensor Comparison: Why a Differential Pressure Sensor is Essential

### Why the BME280 Alone CANNOT Measure Breath-by-Breath Airflow
A common student mistake is attempting to estimate breathing airflow using an absolute barometric sensor like the BME280. This is physically impossible for responsive breathing flow measurement for the following reasons:

1. **Measurement Domain**:
   - The BME280 measures **absolute atmospheric pressure** ({\text{ambient}} \approx 101,325 \text{ Pa}$).
   - The dynamic pressure drop created by quiet tidal breathing across a safe, low-resistance restriction is only **\text{ to }50\text{ Pa}$** (.005\text{ to }0.05\text{ kPa}$).
   - Detecting a \text{ Pa}$ breath signal on top of a ,325\text{ Pa}$ background requires resolving 1 part in 10,000. Natural room pressure fluctuations from opening doors, fans, or walking past the sensor exceed \text{ to }20\text{ Pa}$, swamping any respiratory signal.
2. **Bandwidth and Response Time**:
   - The BME280 internal IIR filter and digital oversampling pipeline has a typical step response latency of \text{ to }200\text{ ms}$, blurring peak inspiratory and expiratory transitions.
   - In contrast, the piezoresistive MPXV7002DP has an analog step response time of **$< 1.0\text{ ms}$**, enabling genuine 50 Hz waveform capture.
3. **Common-Mode Rejection**:
   - A differential sensor with dual ports pneumatically cancels out ambient barometric shifts: both ports experience identical atmospheric changes, yielding zero net differential offset.

### Primary Sensor Options

| Feature | NXP MPXV7002DP + ADS1115 | Sensirion SDP810-500Pa |
| :--- | :--- | :--- |
| **Output Type** | Analog voltage (0.5V to 4.5V) | Digital I2C (calibrated 16-bit integer) |
| **Measurement Range** | $\pm 2.0\text{ kPa}$ ($\pm 20\text{ cm H}_2\text{O}$) | $\pm 500\text{ Pa}$ ($\pm 5\text{ cm H}_2\text{O}$) |
| **Zero-Point Accuracy** | Factory offset drift $\pm 100\text{ mV}$ (requires software zero calibration) | Thermal-mass flow micro-sensor, exceptional zero-point stability ($< 0.1\text{ Pa}$ offset) |
| **Circuit Complexity** | Requires ADS1115 ADC + voltage divider resistors | Direct connection to I2C bus (3.3V native) |
| **Availability in India** | Readily available as standalone IC or APM airspeed breakout | Available on Mouser / DigiKey; occasionally on Robu.in |
| **Cost** | ~₹1,400 total (sensor + ADC) | ~₹2,500 |

*Conclusion*: For this prototype, the **MPXV7002DP + ADS1115** is the recommended baseline due to widespread availability in India, robust analog signal conditioning learning value, and simple interfacing. Firmware design will maintain an abstraction layer so that digital I2C sensors like the Sensirion SDP810 can be substituted seamlessly.

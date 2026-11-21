# Hardware Wiring Guide & Pin Mapping

## Overview
This document describes the electrical connections between the ESP32 microcontroller, the ADS1115 16-bit ADC, the MPXV7002DP analog differential pressure sensor, the BME280 environmental sensor, the battery monitor, and the status indicator.

> [!CAUTION]
> **Safety Notice**: This is an educational prototype for low-pressure ambient breathing studies only. Never connect this device to CPAP systems, ventilators, or pressurized air sources. Never wear or handle the sensor assembly near your face while connected to mains power or a charging cable.

---

## Pin Connection Table

| Subsystem / Module | Module Pin | ESP32 GPIO / Rail | Description & Signal Type | Notes |
| :--- | :--- | :--- | :--- | :--- |
| **I2C Bus** | SDA | GPIO 21 | I2C Data Line | Shared by ADS1115 and BME280; 4.7k pull-up to 3.3V |
| **I2C Bus** | SCL | GPIO 22 | I2C Clock Line | Shared by ADS1115 and BME280; 4.7k pull-up to 3.3V |
| **ADS1115 ADC** | VDD | 3V3 Rail | Power Supply (+3.3V) | Decouple with 100nF ceramic cap close to IC |
| **ADS1115 ADC** | GND | Common GND | System Ground | Tie to analog ground plane if using custom PCB |
| **ADS1115 ADC** | ADDR | GND | I2C Address Select | Sets I2C address to 0x48 |
| **ADS1115 ADC** | A0 | Divider Out | Analog Input 0 | Reads scaled output of MPXV7002DP |
| **MPXV7002DP** | Pin 1 (N/C) | - | Not connected | - |
| **MPXV7002DP** | Pin 2 (Vs) | 5V Rail / VBUS | Sensor Power Supply (+5.0V) | Required for 5V analog output span (0.5V - 4.5V) |
| **MPXV7002DP** | Pin 3 (GND) | Common GND | Sensor Ground | Connect directly to star ground |
| **MPXV7002DP** | Pin 4 (Vout)| Divider In | Analog Voltage Output | Feeds 2:3 voltage divider to protect 3.3V ADS1115 |
| **Voltage Divider**| Top | MPXV7002DP Vout| R1 = 10 kOhm | Scales 0.5V - 4.5V down to 0.3V - 2.7V |
| **Voltage Divider**| Center | ADS1115 A0 | Divided signal | Safe for ADS1115 on 3.3V VDD (0.256V to 4.096V range) |
| **Voltage Divider**| Bottom | Common GND | R2 = 15 kOhm | Scale factor: 15 / (10 + 15) = 0.60 |
| **BME280** | VIN | 3V3 Rail | Power Supply (+3.3V) | Secondary sensor (ambient pressure/temp/humidity) |
| **BME280** | GND | Common GND | Sensor Ground | - |
| **BME280** | SDO | GND (or VDD) | I2C Address Select | GND = 0x76, VDD = 0x77 (Default firmware: 0x76) |
| **Battery Sense** | Divider Top | VBAT (LiPo +) | 1S LiPo Positive (3.0V - 4.2V) | Through R_bat1 = 100 kOhm |
| **Battery Sense** | Divider Mid | GPIO 34 (ADC1_CH6)| Divided Battery Voltage | High-impedance ADC pin (Input Only) |
| **Battery Sense** | Divider Bot | Common GND | R_bat2 = 100 kOhm + 100nF cap | Factor: 0.50 (Max input: 2.1V for 4.2V cell) |
| **Status LED** | Anode (+) | GPIO 2 | Digital Output | Built-in ESP32 LED; pulses during breathing |
| **Power Input** | TP4056 OUT+ | 5V/VIN or LDO IN | Regulated 3.3V system power | Via onboard LDO or custom PCB regulator |
| **Power Input** | TP4056 OUT- | Common GND | System Ground | Battery protection cutoff managed by DW01A |

---

## Wiring Diagram

\\\mermaid
flowchart TD
    subgraph Power["Power Subsystem (1S LiPo + TP4056)"]
        BAT["1S LiPo Battery\n3.7V nominal"] --> TP["TP4056 Charger\n+ Protection"]
        TP -->|"VBAT (3.0 - 4.2V)"| VDIV["Battery Divider\n100k / 100k + 100nF"]
        TP -->|"VOUT"| LDO["3.3V LDO Regulator\n(AP2112K / Onboard)"]
        USB["5V USB Input"] --> TP
        USB -->|"5V Rail"| MPX_PWR["5V Sensor Rail"]
    end

    subgraph MCU["ESP32 DevKit V1 (ESP32-WROOM-32)"]
        GPIO34["GPIO 34 (ADC1)"]
        I2C_SDA["GPIO 21 (SDA)"]
        I2C_SCL["GPIO 22 (SCL)"]
        GPIO2["GPIO 2 (LED)"]
        RAIL_3V3["3.3V Rail"]
        SYS_GND["Common GND"]
    end

    subgraph FlowSens["Airflow Sensing Subsystem"]
        RESTRICT["Flow Restrictor Tube\nDual Pressure Ports"] -.->|"Silicone Tube P1"| MPX["MPXV7002DP\nDifferential Pressure\n(+-2 kPa)"]
        RESTRICT -.->|"Silicone Tube P2"| MPX
        MPX_PWR --> MPX
        MPX -->|"Vout (0.5V - 4.5V)"| LEVEL["Passive Divider\n10k / 15k (Factor 0.60)"]
        LEVEL -->|"Scaled (0.3V - 2.7V)"| ADS_A0["ADS1115 A0"]
        ADS["ADS1115 16-bit ADC\n(I2C: 0x48)"]
        ADS_A0 --- ADS
    end

    subgraph EnvSens["Environmental Secondary Subsystem"]
        BME["BME280 Module\n(I2C: 0x76)\nTemp, Humidity, Baro"]
    end

    VDIV -->|"0 - 2.1V"| GPIO34
    LDO --> RAIL_3V3
    RAIL_3V3 --> ADS
    RAIL_3V3 --> BME
    SYS_GND --- ADS
    SYS_GND --- BME
    SYS_GND --- MPX
    SYS_GND --- VDIV

    I2C_SDA ===|"I2C SDA"| ADS
    I2C_SDA ===|"I2C SDA"| BME
    I2C_SCL ===|"I2C SCL"| ADS
    I2C_SCL ===|"I2C SCL"| BME
    GPIO2 -->|"PWM/Blink"| LED["Status LED"]
\\\

---

## Engineering Details on Signal Conditioning

### 1. MPXV7002DP Level Shifting
The MPXV7002DP is powered by a 5.0V source to ensure standard sensitivity (.0 \text{ V/kPa}$) and zero-pressure center point (.5 \text{ V}$). Because the ESP32 and ADS1115 run on 3.3V:
- Direct 4.5V output would exceed the maximum absolute rating of an ADC powered by 3.3V ({DD} + 0.3\text{V} = 3.6\text{V}$).
- A passive voltage divider of  = 10\text{ k}\Omega$ and  = 15\text{ k}\Omega$ scales the voltage by  / (10 + 15) = 0.60$.
- The zero-pressure voltage at ADS1115 A0 becomes .5\text{ V} \times 0.60 = 1.50\text{ V}$.
- The full scale ($\pm 2\text{ kPa}$) spans from .30\text{ V}$ to .70\text{ V}$, perfectly matching the ADS1115 gain setting  = 1$ ($\text{Full Scale} = \pm 4.096\text{ V}$).

### 2. Battery Divider
- Resistors: \text{ k}\Omega$ high side, \text{ k}\Omega$ low side.
- Filter capacitor: \text{ nF}$ ceramic in parallel with the low-side resistor suppresses high-frequency ADC switching noise.
- Current draw from battery: .2\text{ V} / 200\text{ k}\Omega = 21\text{ }\mu\text{A}$, causing negligible battery drain during standby.
- The mid-point voltage for a fully charged .2\text{ V}$ cell is .10\text{ V}$, safely below the ESP32 ADC1 ceiling (.1\text{ V}$).

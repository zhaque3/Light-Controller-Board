# Light Controller Board

**Project:** Light Controller Board, the lighting controller for the University of Alberta EcoCar prototype vehicle (Sally).

**Purpose:** This board takes the vehicle's 12 V and 7 V supply rails and drives up to four external LED light panels (brake, signal and running lights). Each panel channel has its own 12 V to 5 V buck converter, an eFuse with current monitoring, an addressable LED data line, and a temperature sensor readback. An STM32G4 MCU handles light patterns, panel monitoring, a local UI (OLED + light mode buttons), USB, and CAN bus communication with the other boards on the vehicle.

<!-- Add a render or photo of the board here, e.g.
<p align="center">
  <img src="..." width="400">
</p>
-->

---

## Table of contents

1. [Repository layout](#repository-layout)
2. [Key features](#key-features)
3. [Hardware details](#hardware-details)
4. [Light panel daughter board](#light-panel-daughter-board)
5. [Manufacturing files](#manufacturing-files)
6. [Firmware overview](#firmware-overview)

---

## Repository layout

```
Light-Board Firmware/
└── LIGHT_BOARD_Firmware/               # STM32CubeIDE project
    ├── LIGHT_BOARD.ioc                 # STM32CubeMX pin / peripheral configuration
    ├── Core/Src/main.c                 # Application code
    ├── Core/Src/ssd1306*.c             # OLED driver
    └── Drivers/                        # STM32G4 HAL + CMSIS

Light-Board Hardware/
└── Rev 1/
    ├── ECO Car Brakes.kicad_pro        # Main KiCad project (open this)
    ├── ECO Car Brakes.kicad_sch        # Top-level schematic
    ├── ECO Car Brakes.kicad_pcb        # 4-layer controller board layout
    ├── PowerSheet.kicad_sch            # Power input, 3.3 V regulator, panel channels
    ├── protection_circuitry.kicad_sch  # 12 V / 7 V input eFuses
    ├── panel_power.kicad_sch           # Per-panel buck + eFuse + output connector (x4)
    ├── Microcontroller.kicad_sch       # MCU, USB and CAN sub-sheets
    ├── MCU.kicad_sch                   # STM32G474, programming header, OLED, buttons
    ├── VBUS.kicad_sch                  # USB-C + ESD protection
    ├── CAN Bus.kicad_sch               # CAN FD transceiver + vehicle connector
    ├── Light Panel/                    # LED panel daughter board (KiCad + Gerbers)
    ├── bom/ibom.html                   # Interactive BOM
    ├── production/                     # JLCPCB fabrication outputs (Gerbers, BOM, CPL)
    └── ECO Car Brakes-backups/         # KiCad automatic project backups
```

> The KiCad project is named `ECO Car Brakes` for historical reasons; it is the Light Controller Board.

---

## Key features

* **STM32G474RET6** microcontroller (LQFP-64) as the central control unit.
* **Four independent light panel channels**, each with:
  * **AP62250** synchronous buck converter (12 V to 5 V) to power the panel LEDs.
  * **TPS259631 eFuse** for overcurrent protection, with the ILM current-monitor output fed back to an MCU ADC.
  * Addressable LED data line and temperature sensor data line routed to the MCU.
  * 6-pin Molex output connector to the light panel daughter board.
* **Input power protection**: TPS259631 eFuses on both the 12 V and 7 V input rails, with current monitoring and fault test points.
* **CAN FD** communication through an MCP2542FD transceiver with ESD2CAN24 protection.
* **USB-C** (USB 2.0) with USBLC6-2P6 ESD protection.
* **Local UI**: I2C OLED header, light mode push buttons, and status LEDs.
* **4-layer PCB** (F.Cu / In1.Cu / In2.Cu / B.Cu) with ground poured on all layers.

---

## Hardware details

### Power input and protection

* The board is powered from the vehicle's 12 V and 7 V rails, brought in through the DB7W2 vehicle connector alongside CAN.
* Each input rail passes through a TPS259631 eFuse (`protection_circuitry.kicad_sch`). The ILM pins set the current limit and give an analog current readout (`v_ilm_12V`, `v_ilm_7V`) to the MCU.
* Fault outputs are broken out to test points (`7V_ILM_fault`, `12V_ILM_fault`) with indicator LEDs.
* An AZ1117-3.3 LDO regulates the protected 7 V rail down to 3.3 V for the MCU and logic.

### Light panel channels

Each of the four channels (`panel_power.kicad_sch`, instantiated four times) contains:

* **Buck converter**: AP62250 converting 12 V to 5 V (52.3 kΩ / 10 kΩ feedback divider), with 22 µF ceramic and 100 µF electrolytic output capacitance.
* **eFuse**: TPS259631 on the 5 V output to protect each panel. A solder jumper allows the eFuse to be bypassed for testing.
* **Monitoring**: the eFuse ILM voltage (`v_ilm_1..4`) is read by the MCU ADC to measure per-panel current and detect faults.
* **Output connector**: 6-pin Molex connector carrying 5 V, GND, LED data (`led_data_1..4`) and temperature data (`temp_sens_data_1..4`).

### Microcontroller

* STM32G474RET6 with an external Abracon oscillator, VDDA ferrite bead filtering, and BOOT / reset buttons.
* Programmed and debugged through a 2x5 STM32 / ST-Link SWD header.
* Peripherals in use: ADC (eFuse current monitors), timer/GPIO outputs for the four LED data lines, temperature sensor inputs, I2C1 for the OLED, FDCAN, and USB.

### Communication

* **CAN bus**: MCP2542FD CAN FD transceiver with standby control, ESD2CAN24 TVS protection, and an optional 120 Ω termination resistor.
* **USB**: USB-C receptacle with 5.1 kΩ CC pull-downs and USBLC6-2P6 ESD protection on the data lines.

---

## Light panel daughter board

The `Light Panel/` folder contains the LED panel that plugs into each controller channel.

* **12x WS2812B** addressable RGB LEDs (5050 package).
* **MCP9700A** analog temperature sensor for panel temperature monitoring.
* **Molex 43045** 6-pin connector to the controller board.
* Gerber files are in `Light Panel/LED Gerber/` (also zipped as `LED Gerber.zip`).

Connector wiring:

| Pin | Signal           | Wire colour |
|-----|------------------|-------------|
| 1   | 5 V              | Red         |
| 2   | GND              | Black       |
| 3   | LED data         | Yellow      |
| 4   | Temperature data | Green       |

---

## Manufacturing files

Fabrication outputs in `production/` were generated with the KiCad JLCPCB Fabrication Toolkit:

* `ECO_Car_Brakes.zip`: Gerbers and drill files
* `bom.csv`: bill of materials
* `positions.csv`: pick-and-place (CPL)
* `designators.csv`, `netlist.ipc`

An interactive BOM for assembly is in `bom/ibom.html`.

---

## Firmware overview

The firmware is an STM32CubeIDE project for the STM32G474RET6, found in `Light-Board Firmware/LIGHT_BOARD_Firmware/`. It is a bare-metal (no RTOS) super-loop using the STM32 HAL, with the application code in `Core/Src/main.c`.

To build: open the folder in STM32CubeIDE (File > Import > Existing Projects into Workspace), build the `Debug` configuration, and flash through the ST-Link header with `LIGHT_BOARD Debug.launch`.

### WS2812B LED driver

- The LED data line for panel 1 is driven by **TIM3 CH2 PWM with DMA**.
- Each colour bit is encoded as one PWM period: a duty of 5 counts for a `0` and 10 counts for a `1`, packed in GRB order (24 bits per LED).
- The DMA buffer starts with a 300 µs low reset period (which also absorbs the spurious first pulse) and ends with a short trailing pad so the last bit completes cleanly.
- Transfers are non-blocking: `HAL_TIM_PWM_PulseFinishedCallback` stops the timer and clears a busy flag when the frame is done.
- The strip is only refreshed when the selected colour changes.

### Light mode buttons

- On startup all LEDs are set to white.
- The on-board buttons latch a colour: **SW4** red, **SW5** green, **SW2** blue.

### Panel temperature monitoring

- Each panel's MCP9700A sensor is read on its own ADC channel:

  | Panel | Pin | ADC channel |
  |-------|-----|-------------|
  | 1     | PA6 | ADC2_IN3    |
  | 2     | PA7 | ADC2_IN4    |
  | 3     | PA8 | ADC5_IN1    |
  | 4     | PA9 | ADC5_IN2    |

- ADC2 and ADC5 are calibrated at startup. Each reading uses a 640.5-cycle sampling time (the sensor has a high output impedance and no filter cap) and averages 8 samples.
- The voltage is converted using the MCP9700 transfer function: 500 mV at 0 °C, 10 mV/°C.

### OLED display

- An SSD1306 OLED on I2C1 is refreshed once per second with the temperature of each panel.
- A panel reading below 18 °C is shown as "Panel N is off" (the sensor isn't connected or powered).

### Current status

- Only panel 1's LED output is driven so far; panels 2–4 are monitored for temperature only.
- The FDCAN, USB and eFuse ILM ADC peripherals are configured in CubeMX but not yet used by the application.

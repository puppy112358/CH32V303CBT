# Feature Research

**Domain:** Programmable DC Electronic Load (Firmware)
**Researched:** 2026-06-02
**Confidence:** HIGH

## Feature Landscape

### Table Stakes (Users Expect These)

Features users assume exist. Missing these = product feels incomplete or unsafe.

| Feature | Why Expected | Complexity | Notes |
|---------|--------------|------------|-------|
| **CC (Constant Current) Mode** | Fundamental operating mode for PSU testing, battery discharge, LED driver testing | MEDIUM | Hardware control loop (DAC8571 → op-amp → MOSFET). Firmware computes DAC setpoint. Fast regulation due to analog loop. |
| **CV (Constant Voltage) Mode** | Required for testing current-source supplies, solar panels, battery chargers | MEDIUM | Same hardware loop as CC, different feedback point. DAC reference drives voltage error amplifier. |
| **Per-MOSFET Hardware OCP (Over-Current Protection)** | Each of 4 parallel MOSFETs must be independently protected from exceeding SOA. Single-point current monitoring is insufficient for parallel design. | MEDIUM | INA226 ALARM pin per MOSFET. Instant hardware interrupt response — no software polling lag. Threshold = rated current/4 x 1.3 per MOSFET. |
| **Total Current/Voltage/Power Measurement** | Users need aggregate load data for test validation, data logging, and closed-loop control | LOW | Summary INA226 provides V/I/P via I2C1 readout. INA226 does power multiplication internally. |
| **10Hz+ Telemetry Data Reporting** | Real-time monitoring is essential for any test instrument. <5Hz update rate feels laggy and unresponsive. | LOW | cJSON-packaged UART0 output at 10Hz. Include: 4x MOS currents, total V/I/P, temperature, fan RPM, status flags. |
| **Over-Temperature Protection (OTP)** | MOSFETs dissipate all power as heat. Thermal runaway destroys hardware in seconds without protection. | MEDIUM | NTC thermistor on heatsink + ADC. Two thresholds: warning (derate current) and critical (immediate shutdown). Must have hardware-level override. |
| **Over-Power Protection (OPP)** | Total power limit is a hard constraint of the 4-MOS parallel design. Exceeding rated power damages MOSFETs. | LOW | Software check: P_total > rated_W triggers load reduction or shutdown. INA226 summary channel provides power directly. |
| **Fan Cooling with Thermal Control** | MOSFET heatsinks require forced air. Fixed-speed fan is either too loud at idle or insufficient at full load. | MEDIUM | PWM-driven fan with PID control loop. Target temperature setpoint. Fan RPM tachometer feedback for fault detection (stall/locked rotor). |
| **Remote Control via UART Command Protocol** | Electronic loads are typically controlled remotely (ATE systems, PC automation). Front-panel-only operation is a nonstarter for programmable loads. | MEDIUM | UART0 at >115200 bps, odd parity. cJSON text protocol: set mode (CV/CC), set target value, read measurements, enable/disable load. |
| **Load Enable/Disable** | Must be able to engage/disengage the load remotely and immediately. Hardware enable pin preferred for fast response. | LOW | DAC output gating or MOSFET gate pulldown. Emergency disable must not depend on firmware loop — use GPIO or hardware comparator to cut gate drive. |
| **Status Indication** | Users need at-a-glance visual feedback: Is the load active? What mode? Any faults? | LOW | WS2812 LED strip with color-coded states: green=idle, blue=CC active, cyan=CV active, yellow=warning, red=fault, flashing=protection tripped. |
| **Reverse Polarity Protection** | Misconnecting DUT polarity destroys MOSFET body diodes and shunt resistors instantly. | LOW | Typically handled in hardware (diode + fuse or MOSFET reverse-blocking). Firmware can add detection: negative voltage reading on INA226 = alarm + disable. |

### Differentiators (Competitive Advantage)

Features that set this design apart from basic commercial loads. Not required, but valuable.

| Feature | Value Proposition | Complexity | Notes |
|---------|-------------------|------------|-------|
| **Per-MOSFET Current Balance Monitoring** | 4 independent INA226 channels reveal dangerous current imbalance before a single MOSFET overheats. Basic loads only measure total current — they cannot detect that one MOSFET is carrying 60% of the load. | MEDIUM | I2C1 read all 4 MOS INA226 channels each control cycle. Detect imbalance >25% deviation from mean. Log as warning, trip if >40%. This is the hardware's primary architectural advantage. |
| **Per-MOSFET Independent Hardware OCP** | Each MOSFET has its own INA226 with ALARM pin wired to MCU interrupt. If one MOSFET enters thermal runaway, it's disconnected before the others fail. Commercial parallel loads usually have single-point OCP. | MEDIUM | 4 separate EXTI interrupt lines from INA226 ALARM outputs. Interrupt handler identifies which MOSFET(s) tripped, disables load, logs fault channel. |
| **CR (Constant Resistance) Mode** | Software-computed CR mode extends use cases to: battery internal resistance measurement, fuel cell testing, thermistor characterization. Not present in basic loads. | MEDIUM | Firmware reads V_total, computes I_setpoint = V_total / R_target, writes to DAC. Regulation speed limited by loop rate (100ms). Not as fast as analog CC/CV, but functional for DC/R_load testing. |
| **CP (Constant Power) Mode** | Essential for testing power supplies at their rated wattage, simulating constant-power loads like motors and computing equipment. | MEDIUM | Firmware reads V_total and I_total, computes P_current, adjusts I_setpoint = P_target / V_measured. Requires tighter V/I measurement correlation than CR mode. |
| **Programmable Soft-Start Ramp** | Prevents inrush current spikes when engaging load. Especially important for battery testing where a sudden CC load can drop terminal voltage below cutoff. | LOW | DAC setpoint ramps from 0 to target over configurable time (default 100ms). Linear ramp in firmware timer ISR at 10ms intervals. |
| **Battery Discharge Test with Ah Counting** | Charges and evaluates battery capacity (mAh or Ah). Terminates on configurable under-voltage cutoff + optional time limit. Stores results for readback. | MEDIUM | Operates in CC mode. Firmware integrates I_total over time (Ah = SUM(I * Δt)). Stop conditions: V_bat < V_cutoff OR t > t_max. Log final Ah, end voltage, discharge duration. |
| **USB-CDC Debug Channel (Independent of Control UART)** | Control UART (UART0) handles cJSON commands. Separate USB-CDC provides printf debug output without interfering with the command protocol. Enables real-time debug tracing during operation. | LOW | printf() redirected to USB-CDC via `_write()` syscall override. UART0 remains dedicated to cJSON protocol. No hardware conflict — USB-CDC uses USB peripheral, not UART. |
| **cJSON Text Protocol for Easy Integration** | Binary protocols require parsing libraries on the PC side. cJSON is trivially parseable by any language: Python `json.loads()`, Node `JSON.parse()`, etc. Debuggable with any serial terminal. | LOW | Command format: `{"cmd":"set_mode","mode":"CC","value":1.5}`. Response: `{"status":"ok","mode":"CC","v":12.3,"i":1.5,"p":18.5}`. Human-readable in terminal. |
| **WS2812 Multi-Color Status with Animation** | Unlike a single bi-color LED, WS2812 provides rich state communication: pulsing=active, solid=fault, color gradient=temperature, brightness=load percentage. | LOW | TIM PWM DMA to WS2812 data line (PA0). Pre-computed color lookup table. States: IDLE (green slow pulse), ACTIVE_CC (blue breathing), ACTIVE_CV (cyan breathing), WARNING (yellow blink), FAULT_OCP (red fast flash), FAULT_OTP (red solid). |
| **MOSFET Thermal Derating (Adaptive Current Limit)** | As heatsink temperature rises, firmware progressively reduces max allowed current to keep MOSFETs within SOA. Extends operating envelope beyond what a simple OTP trip point allows. | MEDIUM | Temperature zones: T<60C: 100% rated, T=60-80C: linear derate to 70%, T=80-90C: linear derate to 40%, T>90C: shutdown. Publish derate factor in telemetry. |
| **Von Latch (Voltage-Triggered Load On/Off)** | Load automatically engages when input voltage crosses a threshold (e.g., battery connected) and disengages when voltage drops. Useful for automated battery cycling and production testing. | LOW | Configurable Von/Voff thresholds. Firmware monitors V_total. On V_in > Von: engage load. On V_in < Voff: disengage. State machine with hysteresis. |

### Planned Future Features (Beyond v1)

Features recognized as valuable but deferred due to complexity or dependency on v1 foundations.

| Feature | Deferral Reason | Target Phase |
|---------|----------------|-------------|
| **Transient/Dynamic Mode (A/B Switching)** | Requires stable CC baseline + timer infrastructure. Adds complexity to setpoint management. | v1.1 or v2 |
| **Multi-Step List/Sequence Mode** | Requires storage for step definitions, sequencer state machine, and reliable timing. Builds on transient mode foundations. | v2 |
| **SCPI Command Subset** | Standard protocol for ATE integration, but cJSON covers the same needs with less complexity. Would be an alternative protocol, not a replacement. | v2 (if ATE integration needed) |
| **OCP/OPP Trip Characterization** | Automated power supply protection testing requires precise ramp control and trip-point capture. Advanced feature for PSU validation. | v2 |
| **CR-LED Simulation Mode** | Non-linear I-V curve emulation for LED driver testing. Complex lookup-table or equation-based control. | v2 (niche use case) |

### Anti-Features (Deliberately NOT Built)

Features that seem good but create problems — scope creep, complexity, or conflict with core architecture.

| Feature | Why Requested | Why Problematic | Alternative |
|---------|---------------|-----------------|-------------|
| **On-Device LCD/Touchscreen UI** | "Real instruments have screens" | Adds 20%+ to firmware complexity (display driver, touch driver, UI framework). CH32V303 has 32K SRAM — adding a UI framework and framebuffer would consume significant memory. Out of scope per PROJECT.md. | WS2812 status LED provides essential state feedback. Full UI belongs on the PC-side application (not in scope). |
| **RTOS (FreeRTOS/RT-Thread)** | "RTOS makes multitasking easier" | This controller has exactly 3 concurrent concerns (control loop, comms, monitoring) at deterministic rates. An RTOS adds scheduler overhead, stack-per-task memory cost, and synchronization complexity for a problem that a super-loop handles cleanly. Out of scope per PROJECT.md. | Bare-metal super-loop with interrupt-driven I/O. Timer ISRs handle periodic tasks (PID, telemetry). Main loop handles cJSON parsing and state machine. |
| **WiFi / Bluetooth Connectivity** | "Wireless monitoring would be convenient" | Requires additional hardware (ESP32 module or BLE chip), additional protocol stack, RF certification concerns. CH32V303 has no built-in wireless. Adds major scope to a v1 firmware project. | USB-CDC provides PC connectivity. UART can connect to external wireless bridge if user needs it — no firmware changes required. |
| **Binary Protocol (e.g., Modbus)** | "Binary is more efficient than JSON" | cJSON text protocol is already designed and validated for this project. Switching to binary saves bytes but loses debuggability and easy integration. Premature optimization. | cJSON text protocol is sufficient for 115200 bps control channel. If throughput becomes an issue, add a compact binary telemetry mode as a second channel — keep cJSON for commands. |
| **Independent Per-MOSFET Control** | "Control each MOSFET individually for better balance" | The 4 MOSFETs share a common heatsink, common DAC reference, and common control loop. Independent gate control would require 4 DACs and 4 control loops — a fundamentally different hardware architecture. The current parallel design with per-MOSFET monitoring is the right balance. | Per-MOSFET monitoring + aggregate control achieves 90% of the benefit with 25% of the complexity. |
| **High-Frequency Transient Mode (>100Hz)** | "Simulate CPU load transients" | Bare-metal super-loop at ~100ms/cycle cannot generate sub-10ms transients reliably. Fast transients require hardware timer-based DMA to DAC — possible but adds significant complexity for v1. | Defer to v2. If implemented, use TIM-triggered DMA to DAC8571 for sub-millisecond setpoint changes. |
| **Calibration Data Stored in Flash** | "Factory calibration improves accuracy" | In-system calibration requires a calibrated reference instrument + calibration UI. For a DIY/hobbyist project, this adds complexity with questionable ROI — users calibrate against their own meters if needed. Flash wear from calibration cycles is also a concern (128K Flash, no EEPROM emulation layer implemented). | Default to INA226 factory calibration (0.1% typical accuracy). Provide calibration offset/gain parameters as cJSON-settable config (stored in backup registers, not Flash). Let users who need calibration do it via the command interface. |

## Feature Dependencies

```
CC Mode ──required by──> Battery Discharge Test
CC Mode ──required by──> Transient/Dynamic Mode
CC Mode ──required by──> Soft-Start Ramp
CC Mode ──required by──> MOSFET Thermal Derating

CV Mode ──independent──> (no v1 features depend on CV)

CR Mode ──depends on──> V/I Measurement (INA226 summary)
CP Mode ──depends on──> V/I Measurement (INA226 summary)

Per-MOSFET OCP ──depends on──> INA226 ALARM Interrupt (per-channel)
Per-MOSFET OCP ──enhances──> Telemetry Reporting (adds per-channel fault flags)

Fan PID Control ──depends on──> Temperature Sensing (ADC/NTC)
Fan PID Control ──depends on──> PWM Output (TIM)

Battery Discharge Test ──depends on──> CC Mode
Battery Discharge Test ──depends on──> V/I Measurement (for Ah integration)
Battery Discharge Test ──depends on──> Load Enable/Disable (for termination)

Soft-Start Ramp ──depends on──> DAC Setpoint Control
Soft-Start Ramp ──depends on──> Timer Infrastructure

WS2812 Status ──depends on──> TIM PWM + DMA

USB-CDC Debug ──independent──> (no functional dependencies)

Transient Mode ──depends on──> Timer-triggered DAC Update
Transient Mode ──depends on──> CC Mode baseline

List/Sequence Mode ──depends on──> Transient Mode
List/Sequence Mode ──depends on──> Config Storage (sequence definitions)

Von Latch ──depends on──> V/I Measurement
Von Latch ──depends on──> Load Enable/Disable
```

### Dependency Notes

- **Battery Discharge requires CC Mode**: Ah integration operates in CC discharge. CV or CP discharge testing requires separate implementation — CC is the standard and MVP path.
- **Transient Mode requires timer-triggered DAC update**: Software-loop setpoint changes are too slow for >10Hz transients. Must use TIM DMA to DAC8571 via I2C for sub-ms transitions.
- **CR and CP are independent of CC/CV**: They use the same DAC output and control loop, but compute their setpoint differently (V/R or P/V). They can be added after CC/CV are stable.
- **Fan PID is independent of load control**: Fan control loop reads temperature, writes PWM. No dependency on load state except for feed-forward (high power → pre-spin fan).
- **Per-MOSFET OCP failure affects all features**: If INA226 ALARM interrupts are unreliable, every protection feature degrades to software-only (slower). This is the single highest-reliability requirement.

## MVP Definition

### Launch With (v1)

Minimum viable product — what's needed to validate the concept and be usable as a basic electronic load.

- [ ] **CC Mode with PID Control** — The most-used operating mode, foundation for all others. Must regulate within 1% of setpoint.
- [ ] **CV Mode with PID Control** — Second essential mode. Required for power supply testing.
- [ ] **Per-MOSFET INA226 Monitoring (4 channels)** — Core safety architecture. Without per-MOSFET monitoring, the parallel design has no advantage over single-MOSFET.
- [ ] **Per-MOSFET Hardware OCP via INA226 ALARM Interrupt** — Cannot ship without hardware overcurrent protection. MOSFET destruction is permanent and expensive.
- [ ] **Summary INA226 V/I/P Measurement** — Required for CV/CC feedback, telemetry, and OPP.
- [ ] **10Hz Telemetry via UART0 (cJSON)** — Required for remote monitoring. Without telemetry, the load is a black box.
- [ ] **cJSON Command Protocol (mode set, value set, load on/off)** — Required for remote control. Without commands, it's not a programmable load.
- [ ] **Load Enable/Disable (hardware-gated)** — Safety-critical. Must be able to kill load instantly on fault.
- [ ] **Over-Temperature Protection (two-level)** — Warning + shutdown. MOSFETs dissipate 100W+ — no OTP = fire risk.
- [ ] **Over-Power Protection** — Hard power limit. Enforced per hardware rating.
- [ ] **Fan PWM Control with RPM Feedback** — Cooling is not optional. Must detect fan failure.
- [ ] **WS2812 Status Indication** — Minimum viable user feedback: active/inactive/mode/fault.
- [ ] **USB-CDC Debug Output** — Required for development and troubleshooting. printf() redirection.
- [ ] **Soft-Start Ramp** — Prevents inrush when engaging load. Simple linear ramp. Low complexity, high safety value.

### Add After Validation (v1.x)

Features to add once core CC/CV modes are proven stable and measurement pipeline is reliable.

- [ ] **CR (Constant Resistance) Mode** — Trigger: CC and CV modes validated as stable. Software-computed setpoint, reuse existing control loop.
- [ ] **CP (Constant Power) Mode** — Trigger: CR mode validated (same computational approach). Add once CR works.
- [ ] **Battery Discharge Test with Ah Counting** — Trigger: CC mode + telemetry + load on/off all stable. Adds value for battery testing users.
- [ ] **MOSFET Thermal Derating** — Trigger: temperature sensing + fan control validated. Adds safety margin.
- [ ] **Von Latch** — Trigger: measurement pipeline stable. Simple state machine addition.

### Future Consideration (v2+)

Features to defer until v1 is mature and in-use feedback guides priorities.

- [ ] **Transient/Dynamic Mode (A/B Switching)** — Defer: needs timer-based DAC DMA for fast transitions. Users must validate that static CC/CV already meets their needs.
- [ ] **Multi-Step List/Sequence Mode** — Defer: builds on transient mode. Value depends on whether users need automated test sequences.
- [ ] **SCPI Command Subset (alternative protocol)** — Defer: cJSON covers v1 needs. Only add if ATE system integration is demanded.
- [ ] **CR-LED Simulation Mode** — Defer: niche use case (LED driver testing). Low priority vs. general-purpose features.

## Feature Prioritization Matrix

| Feature | User Value | Implementation Cost | Priority |
|---------|------------|---------------------|----------|
| CC Mode | CRITICAL | MEDIUM | P1 |
| CV Mode | CRITICAL | MEDIUM | P1 |
| Per-MOSFET Hardware OCP | CRITICAL | MEDIUM | P1 |
| Total V/I/P Measurement | CRITICAL | LOW | P1 |
| 10Hz Telemetry | CRITICAL | LOW | P1 |
| Load Enable/Disable | CRITICAL | LOW | P1 |
| Over-Temperature Protection | CRITICAL | MEDIUM | P1 |
| Over-Power Protection | CRITICAL | LOW | P1 |
| Fan PWM with RPM | CRITICAL | MEDIUM | P1 |
| WS2812 Status | HIGH | LOW | P1 |
| USB-CDC Debug | HIGH | LOW | P1 |
| cJSON Command Protocol | CRITICAL | MEDIUM | P1 |
| Soft-Start Ramp | MEDIUM | LOW | P1 |
| CR Mode | MEDIUM | MEDIUM | P2 |
| CP Mode | MEDIUM | MEDIUM | P2 |
| Battery Discharge Test | HIGH | MEDIUM | P2 |
| MOSFET Thermal Derating | MEDIUM | MEDIUM | P2 |
| Von Latch | LOW | LOW | P2 |
| Transient/Dynamic Mode | MEDIUM | HIGH | P3 |
| List/Sequence Mode | MEDIUM | HIGH | P3 |
| SCPI Protocol | LOW | HIGH | P3 |
| CR-LED Mode | LOW | HIGH | P3 |

**Priority key:**
- P1: Must have for launch (v1)
- P2: Should have, add when core is stable (v1.x)
- P3: Nice to have, future consideration (v2+)

## Competitor Feature Analysis

Comparing against representative products at each tier.

| Feature | Entry-Level (Atorch DL24) | Mid-Range (BK 8600) | High-End (GW Instek PEL-3111) | Our v1 Plan |
|---------|--------------------------|---------------------|-------------------------------|-------------|
| **CC/CV Modes** | CC, CV | CC, CV, CR, CP | CC, CV, CR, CP, CC+CV, CR+CV | CC, CV (v1); CR, CP (v1.x) |
| **Current Monitoring** | Single shunt, software | Single shunt, 16-bit ADC | Single shunt, 16-bit ADC | 4x per-MOSFET + 1x summary, 16-bit INA226 |
| **OCP** | Software threshold | Hardware comparator | Hardware comparator | Hardware per-MOSFET (INA226 ALARM) |
| **Transient Mode** | None | 25 kHz, A/B pulse/continuous | 20 kHz, 16 A/us slew rate | Deferred to v2 |
| **Battery Test** | Ah counting, V cutoff | Ah + time + capacity cutoff | Full battery test suite | Ah counting + V cutoff (v1.x) |
| **Interface** | Bluetooth LE, USB, PC app | USB, RS-232, GPIB (SCPI) | USB, RS-232, GPIB, analog control | UART0 cJSON + USB-CDC debug |
| **Display** | 2.4" TFT LCD | VFD display | TFT LCD | WS2812 LED strip only (PC for full UI) |
| **Setup Memory** | 10 groups | 100 setups | 120 setups | In-scope via cJSON (PC stores configs) |
| **Soft-Start** | No | Via ramp control | Yes, programmable 1-200ms | Yes, programmable (v1) |
| **MOSFET SOA Protection** | None (single OTP trip) | Power derating curve | Full SOA profiling | Thermal derating (v1.x) |

## Sources

- **Commercial Products**: BK Precision 8600 Series, GW Instek PEL-3000 Series, Keithley 2380 Series, Rigol DL3000 Series, ITECH IT8500G+ Series — datasheets and user manuals accessed via WebSearch
- **Open-Source Projects**: DCL8010/EL-Load (fietser28), ZPB30A1 custom firmware (herm), Jasper's Electronic Load R2/R3, Dominik-Workshop Electronic Load, ATtiny85 TinyLoad — GitHub and Hackaday project pages accessed via WebSearch
- **Industry Standards**: SCPI command set conventions, 4-quadrant load operating modes (CC/CV/CR/CP), MOSFET SOA derating practices
- **This Project's Hardware**: CH32V303CBT6 MCU (128K/32K), DAC8571 16-bit I2C DAC, 5x INA226 I2C current/power monitors, 4x parallel MOSFETs, WS2812 LED strip, PWM fan — PROJECT.md

**Confidence Notes:**
- HIGH: CC/CV/CR/CP operating modes, OCP/OVP/OPP/OTP protection features, battery test capabilities, telemetry requirements. Well-established in both commercial and open-source domains.
- MEDIUM: Open-source-specific implementation details (WebSearch-based rather than direct source code review). Priority ordering for this specific hardware — informed by architecture understanding but not validated on actual hardware.
- LOW: Specific timing constraints for software-based CR/CP mode regulation accuracy relative to user expectations.

---
*Feature research for: Programmable DC Electronic Load (Firmware)*
*Researched: 2026-06-02*


# Project Research Summary

**Project:** Programmable DC Electronic Load (Firmware)
**Domain:** Bare-metal embedded firmware (RISC-V MCU, real-time control)
**Researched:** 2026-06-02
**Confidence:** HIGH

## Executive Summary

This is a bare-metal firmware controller for a 4-MOSFET parallel programmable DC electronic load. Experts build this class of instrument using a **cooperative time-triggered super-loop** architecture with interrupt-driven I/O -- no RTOS. The firmware runs on a CH32V303CBT6 RISC-V MCU (128K Flash / 32K SRAM) and manages CC/CV control loops, per-MOSFET current monitoring via 5x INA226 sensors, a cJSON-based UART command protocol, WS2812 status indication, and fan-cooled thermal management.

The recommended approach is a **layered architecture** with strict isolation: the application layer (mode management, command parsing, telemetry) never touches hardware registers; a thin HAL wraps the CH32V30x SPL; device drivers for INA226, DAC8571, and WS2812 are self-implemented in clean C over the I2C bus manager and TIM+DMA infrastructure. cJSON v1.7.19 with a static bump-allocator pool is mandatory -- default malloc/free on 32K SRAM guarantees fragmentation failure within hours of operation. All WS2812 output must use TIM PWM + DMA, never bit-banging, because disabling interrupts to bit-bang LED timing misses INA226 ALARM events and causes UART RX overruns.

The **dominant risk** is MOSFET thermal runaway (Spirito effect) in parallel linear-region operation -- this is a hardware-level problem that firmware cannot fully compensate. Each MOSFET MUST have its own dedicated op-amp current control loop with per-channel INA226 monitoring and source ballast resistors. Secondary risks include I2C bus hang with 6 devices on a single bus (mitigated by timeout/recovery, appropriate pull-up resistors, and verified address allocation), and INA226 calibration register silent reset under supply-voltage droop (mitigated by periodic calibration re-write and zero-reading detection). All critical pitfalls and their prevention strategies are documented in PITFALLS.md.

## Key Findings

### Recommended Stack

Bare-metal super-loop on CH32V303CBT6 (RV32IMAC, 96MHz, 128K/32K). The SPL V2.9 provides the hardware abstraction -- no additional HAL layer is warranted. cJSON v1.7.19 is the single JSON library (covers both command parsing and telemetry generation) configured with cJSON_InitHooks() for a static memory pool to eliminate malloc fragmentation. All device drivers (INA226, DAC8571, WS2812, PID) are self-implemented in C -- the protocols are simple enough that external libraries add complexity without value. Full details: STACK.md.

**Core technologies:**
- **CH32V30x SPL V2.9** -- peripheral hardware abstraction -- WCH-provided, well-tested, covers all needed peripherals (I2C, USART, TIM, DMA, GPIO, EXTI)
- **cJSON v1.7.19 + static pool** -- JSON parse and generate -- must be exactly v1.7.19 (older versions have known CVEs); static pool prevents heap fragmentation on 32K SRAM
- **Bare-metal super-loop** -- runtime architecture -- no RTOS (32K SRAM too tight for RTOS + cJSON pool + WS2812 DMA buffer); cooperative time-triggered scheduler at 1ms SysTick granularity
- **Self-implemented drivers** -- INA226, DAC8571, WS2812, PID -- all simple protocols; external libraries add ROM overhead and abstraction without value
- **TIM PWM + DMA for WS2812** -- non-blocking LED control -- bit-banging blocks CPU and disables interrupts, missing INA226 alarm events; hardware TIM+DMA transmits autonomously

### Expected Features

Thirteen table-stakes features are required for v1 launch. The core differentiator over basic commercial loads is **per-MOSFET current monitoring** with independent hardware OCP per channel -- this detects dangerous current imbalance that single-shunt designs cannot see. Full analysis: FEATURES.md.

**Must have (table stakes, P1):**
- CC Mode with PID control -- the foundational operating mode; all other mode features build on it
- CV Mode with PID control -- second essential mode for power supply testing
- Per-MOSFET INA226 monitoring (4 channels) + Summary INA226 V/I/P -- core safety architecture
- Per-MOSFET Hardware OCP via INA226 ALARM interrupt -- cannot ship without hardware overcurrent protection
- 10Hz Telemetry via UART0 (cJSON) -- real-time monitoring is essential for any test instrument
- cJSON Command Protocol -- remote control is necessary for a programmable load
- Load Enable/Disable (hardware-gated) -- safety-critical instant shutdown
- Over-Temperature Protection (two-level) + Over-Power Protection -- thermal safety
- Fan PWM Control with RPM Feedback -- cooling is not optional; must detect fan failure
- WS2812 Status Indication -- minimum viable user feedback
- USB-CDC Debug Output -- required for development and troubleshooting
- Soft-Start Ramp -- prevents inrush current spikes when engaging load

**Should have (competitive, P2):**
- CR (Constant Resistance) Mode and CP (Constant Power) Mode -- software-computed modes reusing the same control loop
- Battery Discharge Test with Ah Counting -- adds battery-testing use case
- MOSFET Thermal Derating -- adaptive current limit based on heatsink temperature
- Von Latch -- voltage-triggered automatic load engage/disengage

**Defer (v2+):**
- Transient/Dynamic Mode (A/B Switching) -- requires timer-based DAC DMA for fast transitions
- Multi-Step List/Sequence Mode -- builds on transient mode
- SCPI Command Subset -- cJSON covers v1 needs
- CR-LED Simulation Mode -- niche use case

### Architecture Approach

A **four-layer bare-metal architecture**: Application (mode manager, command parser, reporter) -> Control (CV/CC PID, Fan PID) -> Device Driver (INA226, DAC8571, WS2812) -> HAL (I2C bus manager, UART ring buffer, SysTick). The super-loop uses a cooperative time-triggered scheduler driven by a 1ms SysTick counter. Operating modes use the Strategy pattern (function-pointer vtables per mode), enabling clean addition of CR/CP modes later. INA226 faults use latch-mode alerts wired to EXTI -- the ISR sets a volatile flag and returns immediately; the main loop performs heavy processing (DAC zeroing, mode transition, UART notification). All state is static globals; ISRs may only write to volatile flags. Full architecture: ARCHITECTURE.md.

**Major components:**
1. **Cooperative Scheduler** -- SysTick-driven 1ms counter; main loop runs tasks at their cadence (100ms control, 100ms telemetry, 500ms fan PID) via elapsed-time checks. No RTOS, zero task-stack overhead, deterministic behavior.
2. **Mode Manager (Strategy Pattern)** -- CV, CC, Idle, and Fault modes as vtables. Mode transitions swap the active strategy pointer. Adding CR/CP modes requires only writing three new functions.
3. **INA226 Driver (5 instances)** -- I2C register read/write per device, calibration management, alert threshold configuration, zero-reading detection with auto-recovery.
4. **PID Controller (Position Form)** -- Float32 PID with back-calculation anti-windup clamping. Separate gain sets for CV and CC loops. Output clamped to DAC range.
5. **UART RX Ring Buffer + cJSON Parser** -- ISR-driven byte capture into static ring buffer; newline-delimited frame detection; cJSON parses from static arena with bump allocator (cJSON_InitHooks). JSON response built into preallocated buffer for 10Hz UART TX DMA.
6. **Protection Monitor** -- EXTI ISR receives INA226 ALARM edges, sets fault flags; main loop reads INA226 Mask/Enable register to identify culprit, zeroes DAC, transitions to Fault mode, logs event.
7. **WS2812 TIM+PWM+DMA Driver** -- Pre-computed bitstream buffer; DMA feeds TIM CCR register at 800kHz PWM frequency; CPU is free during transmission.
8. **Fan Controller** -- TIM3 PWM output for speed control; TIM4 input capture for tachometer RPM measurement; PID loop with anti-windup, minimum-duty-cycle enforcement, kick-start on transition from OFF, and stall detection.

### Critical Pitfalls

The full catalog of 11 pitfalls with prevention strategies, warning signs, and recovery plans is in PITFALLS.md. The top five that must be addressed in Phase 1-2:

1. **MOSFET Thermal Runaway (Spirito Effect)** -- Parallel MOSFETs in linear region exhibit positive-feedback thermal runaway. Prevention: each MOSFET must have its own dedicated op-amp current control loop; use linear-rated MOSFETs; add source ballast resistors. This is the only pitfall that **cannot be fixed in firmware** -- it is a hardware architecture requirement.

2. **I2C Bus Hang with 6 Devices** -- A single hung slave stalls all sensing (5x INA226) and control (DAC8571). Prevention: 2.2k pull-up resistors, I2C timeout/recovery (9 SCL pulses to free stuck slave), 10-50ms POR delay before first transaction, repeated-start for register-pointer-then-read sequences, verify all addresses with logic analyzer.

3. **INA226 Calibration Register Silent Reset** -- Supply-voltage droop or accidental Config Register bit-15 write clears CAL to zero, causing all current/power readings to silently report zero with no fault flag. Prevention: periodic calibration re-write (every 100 reads), mask off bit 15 on all Config Register writes, detect zero-current-while-voltage-present and trigger device re-init.

4. **PID Integral Windup** -- During startup ramp, saturated DAC output causes integral term to accumulate without bound, producing severe overshoot when error crosses zero. Prevention: back-calculation anti-windup (compute what integral should have been at saturation limit), conditional integration (freeze when output is rail), output hard-clamp before DAC write. Must be designed in from the start.

5. **cJSON Memory Fragmentation on 32K SRAM** -- Default cJSON malloc-per-node causes irreversible heap fragmentation within hours. Prevention: cJSON_InitHooks() with static bump allocator from a preallocated 4KB arena; reset arena offset after each parse-use-discard cycle. This is a solved problem with the documented pattern in STACK.md, but must be implemented before the first cJSON parse.

## Implications for Roadmap

Based on combined research, a 5-phase build order is recommended. This closely follows the ARCHITECTURE.md build order while grouping phases for practical development flow and incorporating the pitfall prevention schedule from PITFALLS.md.

### Phase 1: Foundation and Hardware Validation

**Rationale:** All other phases depend on verified hardware communication. The I2C bus with 6 devices is the critical shared resource -- if it is unreliable, nothing else works. MOSFET thermal stability must be validated on the actual PCB before any control loop runs. DAC power-on glitch behavior must be characterized. This phase establishes the debug channel and timing infrastructure used by every subsequent phase.

**Delivers:**
- SysTick 1ms timer with cooperative scheduler skeleton
- UART0 printf (skeleton) and USB-CDC debug channel (via _write() syscall override)
- I2C1 bus manager with timeout/recovery, validated against all 6 devices
- INA226 driver: per-channel reads, calibration init and periodic re-write, zero-reading detection
- DAC8571 driver: manual setpoint write, POR glitch characterization
- GPIO heartbeat LED

**Addresses features:** USB-CDC Debug (P1), Total V/I/P Measurement (P1), Per-MOSFET INA226 Monitoring (P1)

**Avoids pitfalls:** P1 (MOSFET thermal runaway -- verify hardware), P2 (I2C bus reliability -- validate with logic analyzer), P7 (DAC power-on glitch -- characterize and mitigate), P3 (INA226 calibration loss -- build periodic re-write from day one)

### Phase 2: Closed-Loop Control and Hardware Protection

**Rationale:** The control loop and protection system are inseparable for safety. You cannot operate the control loop without hardware OCP (risk of MOSFET destruction), and you cannot test protection without the control loop running. This phase delivers a safe, regulated electronic load that can operate in CC and CV modes with all hardware protection active -- even without remote control, it is a functional instrument.

**Delivers:**
- PID library with back-calculation anti-windup (separate gain sets for CV and CC)
- CV Mode control loop (100ms): INA226 voltage reading -> PID -> DAC output
- CC Mode control loop (100ms): INA226 current reading -> PID -> DAC output
- Mode Manager (Strategy pattern): Idle -> CV -> CC -> Fault state machine
- Soft-Start Ramp: linear DAC ramp from 0 to target over configurable time
- Load Enable/Disable: hardware-gated DAC output, emergency kill path
- EXTI configuration for 4x INA226 ALARM pins (level-triggered interrupt)
- Fault ISR -> main loop deferred processing: alarm -> fault flag -> DAC zero -> mode Fault
- Over-Temperature Protection: NTC thermistor on heatsink, two-level (warning + shutdown)
- Over-Power Protection: software check against rated wattage

**Addresses features:** CC Mode (P1), CV Mode (P1), Per-MOSFET Hardware OCP (P1), Load Enable/Disable (P1), Over-Temperature Protection (P1), Over-Power Protection (P1), Soft-Start Ramp (P1)

**Avoids pitfalls:** P4 (PID integral windup -- anti-windup from day one), P10 (INA226 shared alert race condition -- level-triggered ISR with full device scan)

### Phase 3: Command Protocol and Telemetry

**Rationale:** With a safe, regulated control loop in place, the command interface transforms the load from a manually-configured circuit to a remotely controllable instrument. The cJSON protocol and 10Hz telemetry are the primary user interface. This phase adds no safety-critical code (control and protection are already solid from Phase 2), so it can be developed with confidence that bugs here cannot destroy hardware.

**Delivers:**
- UART0 RX interrupt-driven ring buffer with newline-delimited frame detection
- cJSON static pool allocator (cJSON_InitHooks with bump allocator from 4KB arena)
- Command parser: JSON deserialization, command table dispatch (set_mode, set_value, load_on, load_off, get_status)
- Setpoint validation (range check, mode check) before applying to control loop
- 10Hz Status Reporter: build cJSON packet (4ch MOS V/I + summary V/I/P + mode + temperature + faults + fan RPM) -> UART0 TX DMA
- Full protocol integration: command -> parse -> validate -> apply -> acknowledge; periodic telemetry stream

**Addresses features:** cJSON Command Protocol (P1), 10Hz Telemetry (P1)

**Avoids pitfalls:** P5 (cJSON memory fragmentation -- static pool from day one), P11 (UART RX buffer overrun -- double-buffer or handshake rate-limiting)

### Phase 4: Peripherals and Cooling

**Rationale:** WS2812 LED status and fan PID control are independent of the command protocol but depend on the timer and DMA infrastructure from Phase 1-2. The fan is a safety-critical cooling system that must be operational for sustained high-power operation. The WS2812 provides essential at-a-glance user feedback. These can be developed in parallel with Phase 3 if resources allow.

**Delivers:**
- WS2812 TIM2 CH1 PWM + DMA1 driver: RGB color buffer -> DMA to TIM CCR -> LED strip
- LED state mapping: IDLE (green pulse), CC active (blue breathing), CV active (cyan breathing), WARNING (yellow blink), FAULT (red flash/solid)
- Fan PWM driver (TIM3 CH1, 25kHz): duty cycle control with minimum-duty enforcement
- Fan tachometer input capture (TIM4 CH1): RPM measurement from open-collector pulse train
- Fan PID controller (500ms loop): temperature setpoint -> PID -> PWM duty
- Fan stall detection: PWM > 20% and RPM = 0 for N samples -> fault flag
- Fan kick-start: 100% duty for 200ms when transitioning from OFF to ON

**Addresses features:** WS2812 Status Indication (P1), Fan PWM with RPM Feedback (P1)

**Avoids pitfalls:** P6 (WS2812 timing -- use TIM+DMA, never bit-bang), P9 (Fan PID stall -- minimum duty enforcement + kick-start + stall detection), P8 (super-loop jitter -- ensure total loop time under 70ms)

### Phase 5: Advanced Modes and Polish

**Rationale:** With the core instrument complete (control + protection + command + telemetry + cooling + status), advanced operating modes add competitive value. All P2 features are software-computed setpoint transformations that reuse the existing control loop and measurement pipeline. They add no new hardware dependencies. Battery discharge testing and thermal derating are the highest-value additions for real-world use.

**Delivers:**
- CR (Constant Resistance) Mode: V_measured / R_target = I_setpoint, applied to CC control loop
- CP (Constant Power) Mode: P_target / V_measured = I_setpoint, applied to CC control loop
- Battery Discharge Test: CC discharge mode with Ah integration (SUM I * delta_t), under-voltage cutoff, optional time limit, result logging
- MOSFET Thermal Derating: adaptive current limit based on heatsink temperature zones (100% at under 60C, linear derate to 40% by 90C, shutdown above 90C)
- Von Latch: voltage-triggered load engage/disengage with configurable Von/Voff thresholds and hysteresis
- Production hardening: watchdog (IWDG ~1s), stack watermark monitoring, heap watermark monitoring

**Addresses features:** CR Mode (P2), CP Mode (P2), Battery Discharge Test (P2), MOSFET Thermal Derating (P2), Von Latch (P2)

**Avoids pitfalls:** These are standard computational features with no new safety-critical pitfalls. Validate CR/CP regulation accuracy against user expectations (LOW confidence from research -- test on actual hardware).

### Phase Ordering Rationale

- **Hardware before software:** Phase 1 validates the physical layer (I2C bus, sensor readings, DAC output, MOSFET thermal behavior) before any control logic is built. This is non-negotiable for safety-critical hardware.
- **Protection before remote control:** Phase 2 establishes hardware OCP, OTP, and OPP before Phase 3 exposes the instrument to remote commands. Accepting UART commands without active protection is unsafe.
- **Control before telemetry:** The control loop must be stable and validated before building the reporting layer on top. Telemetry reflects control state -- garbage-in-garbage-out applies.
- **Independent streams in parallel:** Phase 3 (command/telemetry) and Phase 4 (peripherals/cooling) share no dependencies beyond Phase 2 and can be developed concurrently if resources allow.
- **Advanced modes last:** CR/CP/Battery/Thermal-Derating are software-only setpoint transformations. They depend on stable CC/CV modes from Phase 2 but are self-contained additions with no new hardware paths.

### Research Flags

Phases likely needing deeper research during planning:

- **Phase 1 -- NEEDS research:** Hardware-specific validation. I2C bus with 6 devices must be verified on the actual PCB (signal integrity, rise times, address conflicts -- note STACK.md and ARCHITECTURE.md show slightly different INA226 address maps). DMA channel mapping for TIM2_UP must be verified against the CH32V303 Reference Manual (STACK.md notes STM32F103 assumption that may not hold). MOSFET thermal stability must be tested under load. DAC power-on glitch behavior is topology-dependent.
- **Phase 2 -- NEEDS research:** PID tuning is hardware-plant-dependent. CV and CC loop dynamics differ -- separate gain sets must be empirically determined. INA226 ALARM EXTI integration on CH32V303: pin assignments, interrupt priority levels, simultaneous multi-channel alert testing. ADC/NTC temperature calibration curve for OTP thresholds.
- **Phase 3 -- STANDARD patterns:** cJSON parsing from UART ring buffer is a well-documented embedded pattern. JSON telemetry packet building is straightforward. The static pool allocator from STACK.md is battle-tested. Skip research-phase for planning.
- **Phase 4 -- MAY need research:** WS2812 TIM PWM + DMA on CH32V303 has CH32V003 reference (fabian-bxr) but not CH32V303-specific verification. TIM resource allocation (TIM2 for WS2812, TIM3 for fan PWM, TIM4 for tachometer) must be validated against SPL API. Fan PID is standard -- skip research-phase.
- **Phase 5 -- STANDARD patterns:** CR/CP setpoint computation, Ah integration, thermal derating zone logic, and Von Latch state machine are all straightforward algorithms with well-understood implementations. Skip research-phase.

## Confidence Assessment

| Area | Confidence | Notes |
|------|------------|-------|
| Stack | HIGH | Core technologies verified against project codebase (SPL headers, system init, interrupt vectors). cJSON v1.7.19 requirement confirmed by multiple CVE sources. I2C addresses from official TI datasheets. DMA channel mapping caveat explicitly flagged (verify against CH32V303 RM). |
| Features | HIGH | CC/CV/CR/CP operating modes and protection features are well-established across commercial products (BK Precision, GW Instek, Keithley) and open-source projects. Feature priority ordering is informed by architecture understanding but not validated on actual hardware. |
| Architecture | HIGH | Layered bare-metal super-loop with cooperative scheduler is the dominant pattern in open-source electronic load firmware and consistent with Michael J. Pont's time-triggered patterns. Strategy pattern for modes documented in element14 community project. INA226 latch-mode alert pattern verified against Linux kernel ina2xx.c driver. |
| Pitfalls | HIGH | MOSFET thermal runaway (Spirito effect) is well-documented by Nexperia and Infineon app notes. INA226 calibration reset is a known failure mode on TI E2E forums. cJSON memory fragmentation is a standard embedded concern. WS2812 interrupt-disable starvation is documented in Zephyr RTOS commit history. I2C bus reliability with multi-device buses is a universal embedded concern. |

**Overall confidence:** HIGH -- all research areas are supported by multiple sources including official datasheets, manufacturer app notes, open-source reference implementations, and verified codebase inspection.

### Gaps to Address

These items could not be definitively resolved during research and must be validated during implementation:

- **INA226 I2C address map inconsistency:** STACK.md lists addresses 0x40-0x44 (A0/A1 pin configs specified), while ARCHITECTURE.md shows 0x44 for CH3 and 0x45 for CH4. STACK.md's version has specific pin configurations and is likely more accurate, but the actual address map must be verified against the physical PCB pin strapping during Phase 1 bring-up.
- **CH32V303 DMA channel mapping for TIM2_UP:** STACK.md assumes STM32F103-compatible DMA1 Ch5 assignment but explicitly flags that CH32V303 has DMA2 with 11 channels (vs STM32F103's DMA1-only 7 channels). The actual TIM2_UP DMA channel must be verified against CH32V303 Reference Manual Chapter 7 before WS2812 implementation.
- **Control loop regulation accuracy for software-computed modes:** CR and CP modes compute setpoints in firmware rather than hardware analog loop. Regulation speed is limited to 100ms loop period. Whether this is sufficient for users depends on DUT characteristics and use case -- LOW confidence from research. Validate during Phase 5 with real loads.
- **Op-amp control loop topology:** The PROJECT.md mentions DAC output to op-amp comparator to MOSFET gate but the specific topology (inverting vs non-inverting, per-MOSFET vs shared op-amp) determines DAC power-on safety behavior and whether P1 (MOSFET thermal runaway) is a hardware or firmware concern. Must be verified on actual PCB before Phase 1 conclusion.
- **32K SRAM headroom:** Estimated usage is ~16K of 32K, but this depends on final stack depth, cJSON pool sizing, WS2812 buffer size, and driver state struct sizes. If actual usage exceeds 24K, the cJSON pool may need reduction (4096 to 2048 bytes) or WS2812 buffer optimization (uint16_t HalfWord encoding). Build-time map file analysis in Phase 3 will confirm.

## Sources

### Primary (HIGH confidence)
- CH32V303CBT6 Reference Manual and SPL V2.9 -- verified in project codebase at Peripheral/inc/
- INA226 Datasheet (TI SBOS547A) -- registers, latch mode, alert configuration, calibration formula
- DAC8571 Datasheet (TI SLAS373A) -- I2C address 0x4C, control byte, write sequence, POR to zero
- cJSON v1.7.19 GitHub release and CVE advisories (CVE-2025-57052, CVE-2023-53154, CVE-2024-31755)
- Linux kernel ina2xx.c hwmon driver -- verified latch-mode alert handling pattern
- Zephyr RTOS WS2812 driver commit bdde886 -- verified TIM+DMA approach and bit-bang starvation analysis
- Nexperia IAN50005: Paralleling Power MOSFETs in High Power Applications
- WCH CH32V003 WS2812 driver (fabian-bxr) -- CH32-specific porting reference

### Secondary (MEDIUM confidence)
- element14 Programmable Electronic Load community project -- strategy pattern for operating modes
- Michael J. Pont, "Patterns for Time-Triggered Embedded Systems" -- cooperative scheduler design
- PX4 autopilot firmware INA226 driver -- interrupt-driven alert handling pattern
- STM32 WS2812 PWM+DMA drivers (MaJerle, ErniW) -- verified TIM+PWM+DMA pattern
- TI E2E forums -- INA226 calibration reset, I2C read errors, DAC8571 output issues (community responses)
- CSDN embedded cJSON guide -- static memory pool and embedded configuration patterns

### Tertiary (LOW confidence, needs validation)
- Arduino Forum cJSON memory management on STM32 -- blog post, matches known patterns
- EEVblog WS2812B DMX flickering debug -- real-world narrative, not formally verified
- CSDN embedded fan PID library -- community project, standard textbook algorithm

---
*Research completed: 2026-06-02*
*Ready for roadmap: yes*

# Roadmap: Electronic Load Controller (电子负载控制器)

## Overview

Four-phase build delivering a safe, remotely-controllable electronic load: hardware validation of the I2C sensor/actuator bus first, then closed-loop CV/CC control with hardware over-current protection, then cJSON-based remote command and 10Hz telemetry, and finally WS2812 status indication with fan PID cooling. Each phase produces a verifiable, end-to-end user-visible capability.

## Phases

- [ ] **Phase 1: Hardware Foundation** - I2C1 bus validated against all 6 devices, USB-CDC debug output operational
- [ ] **Phase 2: Control Loop + Protection** - CV/CC PID control with per-MOSFET over-current protection and soft-start
- [ ] **Phase 3: Communication** - UART0 cJSON remote command protocol and 10Hz telemetry reporting
- [ ] **Phase 4: Status Display + Cooling** - WS2812 LED mode indication and PID-controlled fan cooling

## Phase Details

### Phase 1: Hardware Foundation

**Goal**: All I2C1 devices respond correctly at their assigned addresses, I2C bus recovers from faults, and USB-CDC provides debug output
**Mode**: mvp
**Depends on**: Nothing (first phase)
**Requirements**: I2C-01, I2C-02, I2C-03, COMM-03
**Success Criteria** (what must be TRUE):

  1. Developer can read bus voltage, shunt voltage, current, and power from all 5 INA226 devices at their assigned I2C addresses (0x40-0x44) via debug output
  2. Developer can write a 16-bit DAC value to DAC8571 at address 0x4C and observe the corresponding analog output voltage change
  3. I2C bus automatically recovers (9-clock-pulse reset sequence) when a slave device holds SDA low; a stuck device never hangs the system beyond its configured timeout period
  4. Debug log messages appear on the USB-CDC virtual serial port (printf redirected via `_write()` syscall) when the MCU is connected to a PC via USB

**Plans**: 3 plans (3 waves, sequential due to User/main.c shared ownership)
Plans:
**Wave 1**

- [x] 01-01-PLAN.md — i2c_util wrapper + one INA226 end-to-end read via USART1 (I2C-01 partial, I2C-03)

**Wave 2** *(blocked on Wave 1 completion)*

- [x] 01-02-PLAN.md — INA226 expansion (all devices, all getters) + DAC8571 driver (I2C-01 complete, I2C-02)

**Wave 3** *(blocked on Wave 2 completion)*

- [ ] 01-03-PLAN.md — USB-CDC debug output adaptation from WCH SimulateCDC (COMM-03)

### Phase 2: Control Loop + Protection

**Goal**: The electronic load maintains a set voltage or current with all hardware over-current protection active and a soft-start ramp preventing inrush
**Mode**: mvp
**Depends on**: Phase 1
**Requirements**: CTRL-01, CTRL-02, CTRL-03, PROT-01, PROT-02, PROT-03, PROT-04
**Success Criteria** (what must be TRUE):

  1. In CV mode, the DAC output is adjusted via PID such that the summary INA226 voltage reading converges to the target within 5 control cycles (~500ms) without overshoot
  2. In CC mode, the DAC output is adjusted via PID such that the summary INA226 current reading converges to the target within 5 control cycles (~500ms) without overshoot
  3. When any single MOS current exceeds its INA226 alarm threshold OR total power exceeds the rated wattage, the EXTI ISR fires, the DAC output is zeroed, and the system transitions to Fault mode within one interrupt latency
  4. The system starts from an idle state into CV or CC mode with a soft-start linear DAC ramp — no output spike is observed at engage
  5. INA226 calibration registers are periodically re-written; if a zero-valued calibration register is detected while bus voltage is present, the affected device is automatically re-initialized and the correct calibration value is restored

**Plans**: TBD

### Phase 3: Communication

**Goal**: The load is remotely controllable via cJSON commands over UART0 and reports real-time telemetry at 10Hz
**Mode**: mvp
**Depends on**: Phase 2
**Requirements**: COMM-01, COMM-02
**Success Criteria** (what must be TRUE):

  1. Sending a cJSON command (e.g., `{"cmd":"set_mode","mode":"CV","value":5.0}`) via UART0 at >115200bps with odd parity causes the load to enter the specified mode at the specified setpoint and respond with an acknowledgment JSON
  2. Sending a cJSON command (e.g., `{"cmd":"set_mode","mode":"CC","value":2.0}`) via UART0 causes the load to enter CC mode at the specified current
  3. A cJSON telemetry packet containing 4-channel MOS voltage/current, summary voltage/current/power, active mode, fault flags, and temperature is transmitted via UART0 every ~100ms without blocking the control loop
  4. Malformed cJSON commands, out-of-range setpoints, and invalid mode transitions are rejected with a descriptive error JSON response; the control loop continues operating unaffected

**Plans**: TBD

### Phase 4: Status Display + Cooling

**Goal**: WS2812 LED strip displays operating mode at a glance and the fan is automatically PID-controlled with stall detection
**Mode**: mvp
**Depends on**: Phase 1 (I2C and timer infrastructure)
**Requirements**: LED-01, LED-02, FAN-01, FAN-02, FAN-03
**Success Criteria** (what must be TRUE):

  1. The WS2812 LED strip displays a distinct color for each operating state — standby (green), CV active (cyan), CC active (blue), fault (red) — and changes immediately on state transition
  2. The fan speed increases automatically as heatsink temperature or total power rises, and decreases as they fall — the PID controller maintains the target temperature without oscillation
  3. Actual fan RPM is measured via tachometer input capture and reported; if the fan stalls (RPM = 0 while PWM duty > minimum threshold), a fault flag is set within the detection window
  4. LED strip updates are DMA-driven from a pre-computed bitstream buffer — the CPU is free during the entire WS2812 transmission and no interrupts are disabled

**Plans**: TBD
**UI hint**: yes

## Progress

| Phase | Plans Complete | Status | Completed |
|-------|----------------|--------|-----------|
| 1. Hardware Foundation | 2/3 | In Progress|  |
| 2. Control Loop + Protection | 0/0 | Not started | - |
| 3. Communication | 0/0 | Not started | - |
| 4. Status Display + Cooling | 0/0 | Not started | - |

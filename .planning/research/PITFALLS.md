# Pitfalls Research

**Domain:** Electronic Load Controller (DC Load Firmware)
**Researched:** 2026-06-02
**Confidence:** HIGH

## Critical Pitfalls

### Pitfall 1: MOSFET Thermal Runaway (Spirito Effect) in Parallel Linear Region Operation

**What goes wrong:**
Multiple MOSFETs operating in parallel in the linear/saturation region exhibit thermal runaway. One MOSFET carries disproportionately more current, heats up faster, its Vgs(th) drops (negative temperature coefficient in the linear region), which causes it to conduct even more current -- a positive feedback loop. Within microseconds, that MOSFET fails short-circuit. The current then redistributes to the remaining MOSFETs, and they fail in rapid cascade. Often there is no visible external heating before failure.

**Why it happens:**
- Standard switching MOSFETs have tight cell pitch (<10 um) making them susceptible to the Spirito Effect when operated in the linear region below their Zero Temperature Coefficient (ZTC) point.
- Even MOSFETs from the same batch can have 0.3V-2V variation in Vgs(th). The device with the lowest threshold turns on first and hoggs current.
- Electronic loads inherently operate MOSFETs in the linear/saturation region (not full enhancement), which is the worst case for thermal stability.
- The project's "DAC output -> op-amp comparator -> MOSFET gate" topology (from PROJECT.md) operates the MOSFETs as a linear pass element.

**How to avoid:**
1. **Each MOSFET MUST have its own dedicated op-amp current control loop** -- this is non-negotiable. Commercial loads (Chroma, Kikusui, etc.) parallel complete control circuits, not individual MOSFETs. Each INA226 senses one MOSFET's current, and a per-channel op-amp compares it against the setpoint derived from the DAC8571 reference.
2. **Use linear-rated MOSFETs** (e.g., IXYS IXTA80N075L2, Nexperia ASFETs) rather than standard switching MOSFETs (IRF540, IRFZ44, etc.). Linear-rated parts have lower transconductance, softer transfer curves, and lower ZTC points.
3. **Source ballast resistors** (10-100 milliohm, 1% tolerance) in series with each MOSFET source as negative feedback -- increases voltage drop with current, reducing effective Vgs.
4. **Thermally couple all MOSFETs** on the same heatsink so they track each other's temperature.
5. **Per-MOSFET INA226 monitoring** with hardware ALARM interrupt on overcurrent (already planned in INA-02, INA-04).
6. **Gate resistors** (5-100 Ohm per MOSFET) to suppress parasitic oscillation.

**Warning signs:**
- Poor current sharing (one INA226 channel reads significantly higher than others under same load)
- One MOSFET runs noticeably hotter than others (thermal imaging or touch test)
- Unexplained MOSFET failure during load step changes or at high power
- Oscillation visible on gate waveform (probing needed)

**Phase to address:**
Phase 1 (core control loop) -- this is fundamental to the hardware architecture. The op-amp current-loop topology must be correct before any firmware can function safely. If hardware does not have per-MOSFET op-amp control, firmware CANNOT compensate.

---

### Pitfall 2: I2C Bus Reliability with 6 Devices on a Single Bus

**What goes wrong:**
I2C1 bus hangs, data corruption, or intermittent communication failures. With 1 DAC8571 + 5 INA226 on one bus, the combined bus capacitance can exceed I2C specifications. A single hung device can stall the entire bus -- and since all sensing and control flow through this bus, the system is blind and unable to adjust DAC output.

**Why it happens:**
- Six devices on one bus with typical 10 pF per pin input capacitance plus PCB trace capacitance can exceed the 400 pF I2C spec at standard pull-up values.
- Weak or absent pull-up resistors cause slow rise times, violating timing.
- The CH32V303 SPL I2C driver may not handle NACK/arbitration-loss recovery gracefully in a super-loop.
- INA226 power-on-reset can be slower than MCU boot; first I2C transactions may NACK if attempted too early.
- Bit-banged I2C or HAL-level I2C without proper timeout handling can deadlock.

**How to avoid:**
1. **Use 2.2k Ohm pull-up resistors** on SDA and SCL (not the typical 4.7k or 10k for single-device setups). For 6 devices at standard mode, calculate: R_pullup_min = (Vcc - 0.4V) / 3mA. For 3.3V: ~1k Ohm minimum. Use 1.5k-2.2k Ohm.
2. **Add I2C bus timeout and recovery** in firmware: if a transaction does not complete within N milliseconds, issue 9 SCL pulses to free a stuck slave, then reinitialize the I2C peripheral.
3. **Verify INA226 addresses do not conflict** -- A0/A1 pin strapping must be unique across all 5 INA226 chips. Document the address map explicitly. The DAC8571 has a fixed slave address, but verify it does not collide with any INA226 address.
4. **Add a short power-on delay** (10-50 ms) before first I2C access to let all slaves complete their POR sequence.
5. **Use I2C repeated-start** for register-pointer-then-read sequences. The CH32V303 I2C peripheral supports this; confirm the SPL HAL exposes it correctly. Without repeated start, the register pointer may not stick, returning garbage from the wrong register.
6. **Test with a logic analyzer** on the actual hardware to verify rise times, fall times, and ACK/NACK behavior for all 6 devices.
7. **Consider I2C bus isolation** (e.g., PCA9548A I2C mux, or split into two I2C peripherals if CH32V303 has I2C2 available) if signal integrity is marginal.

**Warning signs:**
- Occasional NACK on specific devices at specific times
- Data reads that are mostly correct but occasionally wrong (register pointer drift)
- Bus hangs requiring power cycle to recover
- Works on bench but fails after temperature changes or EMC events

**Phase to address:**
Phase 1 (I2C driver initialization and bus validation). Must be verified before any INA226 or DAC8571 code is built on top.

---

### Pitfall 3: INA226 Calibration Register Silent Reset (Zero Current Reading)

**What goes wrong:**
INA226 current and power readings suddenly report zero while bus voltage continues to read correctly. The system loses all current monitoring, including overcurrent protection. The load may continue operating with no current feedback.

**Why it happens:**
The INA226 Calibration Register (0x05) can be silently reset by two mechanisms:
1. **Bit 15 of Configuration Register (0x00) set to 1**: Any write that accidentally sets this bit triggers a full device reset, clearing the calibration value to zero. This can happen from I2C bus glitches or firmware bugs in config register writes.
2. **Supply voltage droop below ~2V**: During load switching, the VCC rail can transient droop, causing a brown-out reset of the INA226 without the MCU resetting.

When CAL=0, shunt voltage and bus voltage remain readable, but current and power require the calibration value for scaling -- so they report zero. There is NO fault flag or status bit that indicates this condition.

**How to avoid:**
1. **Scope the INA226 VCC rail** during load transients -- add bulk capacitance or a Zener clamp if droops are observed.
2. **Periodically re-write the Calibration Register** from firmware -- e.g., after every N reads or on a watchdog timer. This is not wasteful; it ensures recovery from silent resets.
3. **Use read-modify-write** when updating the Configuration Register -- never write a full register value blindly. Mask off bit 15 explicitly.
4. **Validate readings**: If current reads exactly zero for multiple samples while bus voltage > 0, flag it as a potential calibration loss and re-initialize the device.
5. **Consider the INA228** (20-bit, pin-compatible) for future revisions -- it stores calibration in non-volatile memory.

**Warning signs:**
- All current channels simultaneously report zero (suggests shared VCC droop)
- Single channel reports zero while others work (suggests config register write glitch)
- Intermittent zero readings that self-correct (suggests marginal VCC stability)

**Phase to address:**
Phase 2 (INA226 driver and monitoring). Build calibration re-write and validation into the INA226 read loop from the start.

---

### Pitfall 4: PID Integral Windup Causing Output Saturation and Overshoot

**What goes wrong:**
The CV/CC PID controller accumulates a large integral error term while the DAC output is saturated (e.g., during startup when the load ramps from 0 to target). When the error finally crosses zero, the large integral term keeps driving the output past the setpoint, causing severe overshoot. In a CV electronic load controlling voltage on a power supply under test, this overshoot can damage the DUT or the load itself. Recovery from windup can take seconds (or require a full PID reset), during which the load is not controlling.

**Why it happens:**
- The DAC8571 output range is bounded (e.g., 0V to Vref). When the PID demands a voltage outside this range, the output saturates but the integral term continues to accumulate.
- In fixed-point integer implementations, the integral accumulator can also overflow silently (wrap-around), producing nonsensical output.
- Startup transients (from power-on 0V to target setpoint) are the largest-error periods and prime windup scenarios.
- The 100 ms PID period means it takes ~10 samples per second -- recovery from deep windup can take many seconds.

**How to avoid:**
1. **Conditional integration (clamping)**: Only accumulate integral error when the output is not saturated. If DAC output == 0 or DAC output == Vref_max, freeze the integral term.
2. **Back-calculation anti-windup**: When the output saturates, compute what the integral term SHOULD have been to produce exactly the saturation limit, and set it to that value. This is the most effective method.
3. **Integral term output limiting**: Clamp the integral term to a defined range (e.g., no more than 20% of total output contribution).
4. **Fixed-point overflow protection**: Use saturating arithmetic for integral accumulation, not wrapping. Check for overflow before adding.
5. **PID output hard clamp** before writing to DAC to prevent out-of-range DAC values.
6. **Separate tuning for CV and CC modes** -- the plant characteristics differ, so one PID parameter set will not work well for both.

**Warning signs:**
- Large overshoot on startup or setpoint change
- Long settling time after transient
- Oscillation around setpoint that doesn't dampen
- DAC output stuck at rail (0V or Vref) for extended periods during normal operation

**Phase to address:**
Phase 2 (PID control implementation). Anti-windup must be designed in from the start, not retrofitted.

---

### Pitfall 5: cJSON Memory Exhaustion and Heap Fragmentation on 32K SRAM

**What goes wrong:**
After hours/days of operation, `cJSON_Parse()` returns NULL or the system hard-faults. The MCU has 32K SRAM total, of which the stack, global variables, and peripheral buffers already consume a significant portion. cJSON's malloc-per-node model creates many small allocations, and on embedded heap managers without strong coalescing, the heap fragments irreversibly. Even though total free memory is sufficient, no single contiguous block is large enough for the next allocation.

**Why it happens:**
- Each cJSON node struct is ~20-40 bytes plus a deep-copied string for the key/value. A modest JSON command like `{"cmd":"CV","voltage":3.300,"current":2.000}` creates 6+ nodes with string copies.
- `cJSON_Print()` allocates a separate buffer for the serialized string, even though the caller may not need the raw text.
- If the main loop calls `cJSON_Parse()` then `cJSON_Delete()` on every cycle, small fragments accumulate because free'd blocks are not always adjacent and cannot be merged.
- UART receive buffers for cJSON must be large enough to hold the largest expected message. At >115200 baud, a 512-byte receive buffer + cJSON node overhead can easily consume 2-4K of heap per parse cycle.

**How to avoid:**
1. **Static memory pool for cJSON**: Override `cJSON_InitHooks()` with a bump allocator backed by a static `uint8_t cjson_pool[2048]` array. On `cJSON_Delete()`, simply reset the pool offset to zero. This eliminates fragmentation entirely. The tradeoff is that JSON objects live as a group and cannot be partially freed -- but this is acceptable for a parse-use-discard cycle.
2. **If static pool is insufficient**, pre-allocate a fixed-size node pool (e.g., 32 cJSON nodes) and recycle them.
3. **Define a maximum message size** (e.g., 256 bytes) and reject anything larger before parsing. The UART receive buffer should be exactly this size.
4. **Disable `CJSON_ENABLE_FLOATS`** if you can parse floats manually from string values -- this reduces per-node memory.
5. **Consider replacing cJSON with a simpler protocol** for the inner loop. For example: use cJSON for command reception only (infrequent), but use a compact binary format for the 10Hz data reports. Or use a streaming parser that doesn't build an in-memory tree.
6. **Monitor heap watermark**: Track the largest contiguous free block periodically. If it drops below a threshold, log a warning or force a heap reset.

**Warning signs:**
- `cJSON_Parse()` returns NULL for messages that previously parsed correctly
- System runs fine for hours then crashes (classic fragmentation timeline)
- Increasing parse latency as fragmentation worsens
- HardFault in `malloc()` (CH32V303 default HardFault handler just calls `NVIC_SystemReset()`)

**Phase to address:**
Phase 3 (UART protocol and cJSON integration). Memory strategy must be decided before protocol implementation.

---

### Pitfall 6: WS2812 Bit-Bang Interrupt Disable Starves Real-Time Tasks

**What goes wrong:**
To bit-bang WS2812 timing (350 ns "0" bit, 900 ns "1" bit, 300 ns inter-bit), interrupts must be disabled for the entire LED update sequence. For N LEDs, the critical section is N x 30 us. For even a modest 8-LED strip, that is ~240 us of interrupts-off. During this window:
- UART receive ISR cannot fire, risking RX buffer overrun at >115200 baud (every ~87 us at 115200).
- INA226 ALARM interrupt cannot fire, missing overcurrent events.
- SysTick is lost, causing `Delay_Init()` and all `Delay_Ms()` timing to drift.
- Fan tachometer pulse counting may miss edges.

**Why it happens:**
The CH32V303 at 72 MHz has ~25 cycles per WS2812 bit slot. The fast interrupt attribute (`WCH-Interrupt-fast`) uses hardware stacking, but the ISR entry + exit still costs ~10-15 cycles. A UART RX ISR arriving mid-bit would cause the bit timing to exceed the 50 us RESET threshold, latching garbage into the LED strip.

**How to avoid:**
1. **Use TIM PWM + DMA** instead of bit-banging. Configure a timer channel to generate the WS2812 waveform, use a DMA channel to feed the bit pattern from a lookup table in memory. The CH32V303 has 4 general-purpose timers and 2 advanced timers, with DMA support. This approach requires NO CPU involvement during transmission and NO interrupt disabling.
2. **If TIM+DMA is not feasible**, use SPI at a specific baud rate (e.g., 5.25 MHz where 3 SPI bits = 1 WS2812 bit). The SPI peripheral handles timing in hardware, and DMA feeds the data. Memory cost: 8 bytes of SPI data per WS2812 LED (24 color bits x 8-bit SPI word = need ~72 SPI bytes per LED).
3. **As a last resort** (bit-bang): Keep the LED count small (<10), call `show()` at the END of the super-loop cycle when all sensor reads are complete, and re-enable interrupts immediately after. Accept that SysTick will lose ticks and correct it post-update.
4. **Consider APA102/DotStar LEDs** (standard SPI + clock, no timing constraints) for future hardware revisions if LED animation is a key feature.

**Warning signs:**
- LED flicker or wrong colors, especially on later LEDs in the strip
- UART data corruption coinciding with LED updates
- Missed INA226 alarm events during LED update
- `Delay_Ms()` timing gradually drifting over hours of operation

**Phase to address:**
Phase 3 (peripheral integration). PWM+DMA approach should be evaluated early since it affects timer resource allocation.

---

### Pitfall 7: DAC8571 Power-On 0V Glitch at MCU Boot

**What goes wrong:**
The DAC8571 powers up with output at 0V (the datasheet's "Power-On Reset to Zero" feature). If the control op-amp loop is already active, 0V on the DAC output means 0V on the op-amp setpoint, which means the MOSFETs receive maximum gate drive (inverting topology) or zero gate drive (non-inverting topology). Depending on the op-amp topology, this could mean:
- **Inverting topology (common)**: DAC=0V means maximum MOSFET conduction -- the load pulls maximum current on startup, potentially exceeding safe limits.
- **Non-inverting topology**: DAC=0V means MOSFETs are off -- the load presents an open circuit, which may be safe but denies control.

The duration of this glitch is from MCU power-on until the first I2C write to DAC8571 completes. On a CH32V303 booting from Flash, this could be 1-50+ ms.

**Why it happens:**
- DAC8571 POR state is documented as 0V, not midscale.
- The MCU boot sequence (clock init, peripheral init, I2C init) happens before any application code runs.
- If the MOSFET gate drive circuit powers up before the MCU finishes booting, the op-amp sees DAC=0V and drives the MOSFETs accordingly.
- There is no hardware mechanism to "hold off" the op-amp until DAC is configured.

**How to avoid:**
1. **Hardware: Add an enable/shutdown pin** on the op-amp power rail or MOSFET gate drive, controlled by a GPIO that defaults to the "safe" state (MOSFETs off). Assert the enable pin only after DAC configuration is complete.
2. **Hardware: Pull-up on DAC output**: A weak pull-up to Vref/2 at the DAC output node can provide a safe midpoint voltage during the POR window. This is a compromise -- it adds an error source during normal operation.
3. **Firmware: Make DAC initialization the FIRST thing in main()**, before any other peripheral init that could enable the power stage. Minimize the time between POR and first DAC write.
4. **Firmware: Boot the DAC to a known safe value** (e.g., a voltage that commands minimum current), then ramp slowly to the target setpoint.

**Warning signs:**
- Current surge on power-up visible on external current meter
- Brief full-load condition at every MCU reset
- Power supply under test trips overcurrent protection during boot

**Phase to address:**
Phase 1 (system initialization order). DAC init sequence is critical path.

---

## Moderate Pitfalls

### Pitfall 8: Super-Loop I2C Blocking Causes 10Hz Reporting Jitter

**What goes wrong:**
The super-loop reads 5 INA226 chips + writes DAC8571 in sequence over I2C1. Each INA226 read takes ~500 us to 1 ms (register pointer write + 2-byte read, at 400 kHz). 5 reads + 1 DAC write = ~3-5 ms of I2C bus time per cycle. If the super-loop does this synchronously, the rest of the loop (cJSON parse, PID compute, WS2812 update, fan check) must fit between I2C windows. The 10Hz reporting cadence (100 ms period) may jitter by +/-5 ms depending on I2C timing, affecting the PID sample rate if the PID shares the INA226 read cycle.

**How to avoid:**
1. **DMA-based I2C** if the CH32V303 SPL supports it -- initiate all 6 transfers via DMA, let them complete in hardware while the CPU does PID math and protocol processing.
2. **Stagger INA226 reads** across multiple loop iterations: read 1-2 channels per cycle, maintain a circular state machine. This spreads I2C load evenly at the cost of slightly stale data for slower channels.
3. **Use a hardware timer** to trigger the PID computation at a fixed 100 ms cadence, decoupled from the I2C read schedule. The PID uses the most recent available readings.
4. **If I2C timing is the bottleneck**, increase bus speed to 1 MHz (Fast Mode Plus) if all devices support it (check INA226 and DAC8571 datasheets -- INA226 supports up to 2.94 MHz in High-Speed mode).

**Warning signs:**
- 10Hz data timestamps show significant jitter (> +/-10 ms)
- PID response changes depending on system load
- Occasional missed data report cycles

**Phase to address:**
Phase 4 (system integration and timing tuning).

---

### Pitfall 9: Fan PID Stalling at Low PWM Duty Cycles

**What goes wrong:**
At low PWM duty cycles (<20%), the fan may stall or not start reliably. The PID sees that RPM is below setpoint and keeps increasing the integral term, winding up without effect. When the fan finally unsticks (perhaps due to a thermal event), the accumulated integral term slams the fan to full speed, causing audible annoyance and temperature oscillation.

**How to avoid:**
1. **Define a minimum PWM duty cycle** (e.g., 20-30%) below which the fan is either OFF (0%) or at the minimum-start threshold -- never in between.
2. **Implement "kick-start"**: When transitioning from OFF to ON, apply 100% duty for ~100-200 ms to overcome stiction, then reduce to the PID-calculated value.
3. **Detect stall**: If PWM > 20% and RPM = 0 for N consecutive samples, flag a fan fault and either retry with kick-start or alert.
4. **Anti-windup for fan PID too**: Freeze the integral term when PWM is at minimum or maximum limits.

**Warning signs:**
- Fan oscillates between off and full speed
- Audible "clicking" as fan tries to start but stalls
- Temperature overshoot because fan starts too late

**Phase to address:**
Phase 5 (fan control and thermal management).

---

### Pitfall 10: INA226 Shared Alert Pin Race Condition

**What goes wrong:**
All 4 (or 5) INA226 ALARM pins are wired-AND to a single MCU EXTI interrupt line. When an overcurrent event occurs, the ISR fires but cannot immediately determine WHICH INA226 triggered. If the ISR only reads the first device and clears its flag, but multiple devices triggered simultaneously, the remaining triggers are lost because the alert line is already asserted and the edge-triggered EXTI won't fire again.

**How to avoid:**
1. **Level-triggered interrupt** on the ALARM pin, NOT edge-triggered. The pin stays low as long as any device has an active alert. The ISR loops through all INA226 devices, reads their Mask/Enable registers (0x06), handles each active alert, and only exits when the pin is high again (all cleared).
2. **ISR flags a deferred handler** in the super-loop. Do NOT do I2C reads inside the ISR -- I2C transactions block and can cause nested interrupt issues. The ISR sets a volatile flag; the super-loop detects it and performs the I2C scan.
3. **Scan ALL devices on every alert**: Even if you find one with an active alert, continue scanning the rest. Multiple channels can trip simultaneously.

**Warning signs:**
- Some overcurrent events are missed (not logged, no protection action)
- ALARM pin stays low forever (stuck alert not being cleared)
- Intermittent protection failures

**Phase to address:**
Phase 3 (INA226 alarm integration with interrupt system).

---

### Pitfall 11: UART RX Buffer Overrun During cJSON Parsing

**What goes wrong:**
The UART0 ISR receives bytes into a ring buffer at >115200 baud. The super-loop calls `cJSON_Parse()` on the accumulated buffer when a complete frame is detected (e.g., by newline delimiter). However, cJSON parsing itself is CPU-intensive and runs in the super-loop context. If a second command arrives while the first is being parsed, the UART RX buffer overruns and bytes are lost.

With a 256-byte RX buffer at 115200 baud, a full buffer of new data arrives in ~22 ms. cJSON parsing of a moderately complex object can take 1-10 ms on a 72 MHz RISC-V core, so the window for overrun is real.

**How to avoid:**
1. **Double-buffered UART receive**: Two buffers, alternating. While one is being parsed, the other receives. Swap on frame-complete.
2. **Implement a simple state machine for frame detection in the ISR** to detect end-of-frame, then set a flag. The super-loop processes the complete frame.
3. **Limit command rate on the host side**: Do not send a new command until the previous command's response is received (handshake style).
4. **Increase UART RX buffer to the maximum expected frame size** x 2 (e.g., 512 bytes for cJSON messages).
5. **Use UART hardware flow control** (RTS/CTS) if available on the CH32V303 and connected to the host.

**Warning signs:**
- Partial/corrupted JSON causing parse failures
- Commands intermittently dropped
- Error rate increases under heavy command traffic

**Phase to address:**
Phase 3 (UART protocol).

---

## Technical Debt Patterns

Shortcuts that seem reasonable but create long-term problems.

| Shortcut | Immediate Benefit | Long-term Cost | When Acceptable |
|----------|-------------------|----------------|-----------------|
| Hardcode INA226 calibration values | Fewer lines of code | Silent wrong readings after shunt resistor rework or device replacement | Never -- compute from actual shunt values in init |
| Disable all interrupts for WS2812 updates | Simpler LED driver | Missed INA226 alarms, UART overruns, SysTick drift | Only if LED count < 5 and using TIM+DMA is impossible |
| Use `Delay_Ms()` for I2C timing | Quick to write, no timer needed | Blocks super-loop, adds jitter, misses events | Debug-only; never in production |
| Single PID parameter set for CV and CC | Less tuning work | Poor regulation in one or both modes, possible instability | Only if validated with actual hardware and load range |
| Parse cJSON in ISR context to "save time" | Lower perceived latency | ISR too long, misses other interrupts, stack overflow risk | Never |
| Store DAC target as float | Natural math | CH32V303 has no FPU -- float ops are software-emulated and SLOW (~100 cycles per add) | Never for control loops; use fixed-point |
| Skip I2C error checking to keep code simple | Cleaner-looking code | Silent data corruption, impossible to debug field issues | Never |
| Ignore INA226 conversion-ready timing | One fewer state to track | Reading mid-conversion returns previous value; stale data in control loop | Only if conversion time is shorter than read interval |

---

## Integration Gotchas

Common mistakes when connecting to external services/peripherals.

| Integration | Common Mistake | Correct Approach |
|-------------|----------------|------------------|
| INA226 I2C address | Using 8-bit address (e.g., 0x80) when HAL expects 7-bit (0x40), or vice versa | Verify which format the CH32V303 SPL I2C API uses. The datasheet lists INA226 addresses as 7-bit (0x40-0x4F range). |
| INA226 register read | Issuing STOP after register pointer write, then START for read | Use I2C repeated start: Write [Addr+W, RegPtr] then Read [Addr+R, DataHi, DataLo] without intermediate STOP |
| DAC8571 update mode | Accidentally writing to the temporary register (Control Byte 0x10) instead of direct update (0x00) | The DAC8571 has two write modes. 0x00 = direct update to DAC output. 0x10 = write to temporary register (requires separate load command 0x20). Always use 0x00 for immediate update. |
| WS2812 GPIO speed | Using GPIO output speed too slow for 350 ns pulses | Set PA0 (and PA1) GPIO output speed to 50 MHz mode. At 72 MHz CPU clock, a bit-bang needs ~6 CPU cycles per toggle. |
| Fan tachometer | Reading tach pulses with polling instead of input capture | Fan tach is an open-collector pulse train (2 pulses/rev). Use TIM input capture mode for accurate RPM measurement without CPU polling. |
| cJSON + UART delimiter | Assuming newline-terminated JSON, but not handling partial/multiple frames | UART ring buffer must detect frame boundaries (e.g., '\n' delimiter), handle partial frames (incomplete), and handle multiple frames in one buffer. |

---

## Performance Traps

Patterns that work at small scale but fail as usage grows.

| Trap | Symptoms | Prevention | When It Breaks |
|------|----------|------------|----------------|
| Super-loop iteration time exceeds 100 ms budget | 10Hz reporting becomes 5Hz or irregular; PID timing drifts | Measure loop iteration time with a spare GPIO toggle + scope. Keep total loop time < 70 ms to leave 30% margin. | When I2C reads + cJSON parse + WS2812 update + PID compute + fan check > 100 ms combined |
| cJSON parse time grows with message complexity | UART overrun during parse of complex commands | Limit message size. Test with worst-case message. Consider streaming parser. | Messages > 200 bytes on 72 MHz MCU |
| I2C bus contention at 400 kHz with 6 devices | Reads take longer than expected, loop time grows | Profile each I2C transaction. Move to 1 MHz Fast Mode+ if all devices support it. | When total I2C time > 10 ms per loop (5 reads + 1 write) |
| INA226 conversion time too slow for control requirements | PID receives stale data, overcorrects | Set conversion time to 140 us (fastest for shunt+bus), averaging to 1. Read at 10 Hz. | When PID response time must be < 50 ms |
| Stack overflow in nested ISR or deep cJSON parse tree | HardFault (NVIC_SystemReset loop) | Measure stack high-water mark. Increase stack in linker script if < 4K. Keep ISR logic minimal. | When cJSON parse tree depth > 10 levels or recursive function calls in ISR |

---

## Security Mistakes

Domain-specific embedded security issues.

| Mistake | Risk | Prevention |
|---------|------|------------|
| No command authentication (any UART sender can control the load) | Malicious or accidental commands could destroy the DUT or load hardware | If the electronic load is connected to a PC, add a simple checksum or CRC to each command frame. For multi-user setups, consider command source validation. |
| cJSON parse of unbounded input | Buffer overflow, stack smash, arbitrary code execution (via crafted JSON) | Bound the UART input buffer. Validate JSON depth/nesting before parse. Reject messages that exceed limits. |
| I2C bus accessible via debug header | An attacker with physical access could inject arbitrary I2C commands, overriding DAC or INA226 | Not typically a concern for benchtop equipment. If deployed, disable SWD/JTAG in production via option bytes. |
| Firmware update over UART without authentication | Malicious firmware could be loaded, bypassing all protections | Implement a simple firmware update authentication (signature check) if remote updates are planned. Not critical for initial release. |
| No watchdog timer in production | Firmware hang (e.g., I2C deadlock) leaves load in unsafe state, potentially destroying DUT | Enable independent watchdog (IWDG) with a ~1s timeout. Feed it in the super-loop. On watchdog reset, the DAC powers up at 0V (safe if op-amp topology is correct). |

---

## "Looks Done But Isn't" Checklist

Things that appear complete but are missing critical pieces.

- [ ] **INA226 reads:** Often missing calibration register re-write after anomaly. Verify: does the read loop validate current != 0 when bus voltage > 0?
- [ ] **PID controller:** Often missing anti-windup. Verify: does integral term freeze when DAC output is at rail (0 or Vref)?
- [ ] **I2C bus:** Often missing timeout/recovery. Verify: if SDA is held low by a glitched slave, does firmware detect and recover (9 clock pulses)?
- [ ] **WS2812 output:** Often tested with LEDs disconnected. Verify: connect actual LED strip and confirm colors match expected state mapping (CV=green, CC=blue, fault=red, etc.)
- [ ] **Fan control:** Often missing stall detection. Verify: if fan is mechanically blocked, does firmware detect 0 RPM despite PWM > 30% and flag a fault?
- [ ] **DAC8571 init:** Often missing POR delay. Verify: is there a 10ms delay before first I2C access to DAC8571?
- [ ] **UART protocol:** Often missing frame boundary handling. Verify: can the parser handle a message split across two UART interrupt bursts?
- [ ] **Overcurrent protection:** Often tested with a single channel. Verify: what happens when ALL 4 channels trip simultaneously? Does the ISR handle all 4 alerts?
- [ ] **Memory:** Often no heap watermark monitoring. Verify: track `malloc` failures or add a `mallinfo()` equivalent check in the super-loop for diagnostics.
- [ ] **10Hz reporting:** Often timestamped at report-time, not sample-time. Verify: are INA226 readings timestamped when they were taken, not when they are serialized to cJSON?

---

## Recovery Strategies

When pitfalls occur despite prevention, how to recover.

| Pitfall | Recovery Cost | Recovery Steps |
|---------|---------------|----------------|
| MOSFET thermal runaway (P1) | HIGH | MOSFETs and possibly PCB traces destroyed. Requires hardware redesign (add per-channel op-amp control, linear-rated MOSFETs). Cannot be fixed in firmware alone. |
| I2C bus hang (P2) | LOW | Add bus recovery routine (9 SCL pulses) in watchdog path. If persistent, reduce pull-up resistance, check PCB layout for capacitance. |
| INA226 calibration loss (P3) | LOW | Firmware fix: add periodic calibration re-write and zero-reading detection. Can be patched without hardware changes. |
| PID windup (P4) | LOW | Firmware fix: add anti-windup logic. Parameter-only change in most cases. |
| cJSON memory fragmentation (P5) | MEDIUM | Switch to static pool allocator. Requires cJSON API hook changes and buffer sizing, but no hardware changes. |
| WS2812 timing (P6) | MEDIUM | Switch from bit-bang to TIM+DMA. Requires timer/DMA resource reallocation, possible pin reassignment. |
| DAC power-on glitch (P7) | MEDIUM | If no hardware enable pin exists, add external circuit or accept firmware-only mitigation (fast DAC init). |
| Fan PID stall (P9) | LOW | Firmware fix: add minimum duty cycle and kick-start. |
| INA226 alert race (P10) | LOW | Firmware fix: change to level-triggered interrupt and add full scan in handler. |

---

## Pitfall-to-Phase Mapping

How roadmap phases should address these pitfalls.

| Pitfall | Prevention Phase | Verification |
|---------|------------------|--------------|
| P1: MOSFET Thermal Runaway | Phase 1 (Hardware bring-up) | Per-channel INA226 shows <5% current imbalance across all 4 MOSFETs at 50% rated load. Thermal image confirms uniform heating. |
| P2: I2C Bus Reliability | Phase 1 (Hardware bring-up) | All 6 I2C devices ACK to address probes. Logic analyzer shows clean rise times < 300 ns at 400 kHz. No NACK in 1000 consecutive transactions. |
| P7: DAC Power-On Glitch | Phase 1 (Hardware bring-up) | Scope DAC output during MCU reset. Confirm load does not pull current during boot. If using enable pin, verify GPIO defaults to disabled. |
| P3: INA226 Calibration Loss | Phase 2 (INA226 driver) | INA226 read loop includes calibration re-write every 100 reads. Zero-current detection triggers device re-init. |
| P4: PID Integral Windup | Phase 2 (PID control) | Step response test: setpoint change from 0 to 50% shows <5% overshoot and settles within 2 seconds. No DAC output rail-sticking. |
| P10: INA226 Alert Race | Phase 3 (INA226 alarm integration) | Test with simultaneous overcurrent on 2 channels. Verify both alerts are detected and handled. |
| P5: cJSON Memory | Phase 3 (UART protocol) | Run 10,000 parse/delete cycles with heap watermark monitoring. Memory usage must be flat (no downward trend). |
| P11: UART RX Overrun | Phase 3 (UART protocol) | Send 10 back-to-back commands. All 10 parsed correctly. No dropped bytes. |
| P6: WS2812 Timing | Phase 3 (WS2812 driver) | Use TIM+DMA approach. Verify no UART data loss during LED update. Colors correct end-to-end. |
| P9: Fan PID Stall | Phase 5 (Fan control) | Start fan from 0 RPM. Verify it reaches setpoint without stall. Apply mechanical resistance -- verify stall detection triggers. |
| P8: Super-Loop Jitter | Phase 4 (System integration) | Toggle GPIO at start of each loop iteration. Measure period jitter on scope -- must be < +/-10 ms from 100 ms target. |

---

## Sources

- [INA226 Datasheet (TI SBOS547A) -- Calibration Register, Alert Pin, Power-On Reset](https://www.ti.com/lit/ds/symlink/ina226.pdf) -- HIGH confidence
- [DAC8571 Datasheet (TI SLAS373A) -- Power-On Reset to Zero, Control Byte, Update Modes](https://www.ti.com/lit/ds/symlink/dac8571.pdf) -- HIGH confidence
- [TI E2E: INA226 Output Current Reporting Issue (Calibration Reset)](https://e2e.ti.com/support/amplifiers-group/amplifiers/f/amplifiers-forum/772930/ina226-output-current-reporting-issue) -- MEDIUM confidence (community forum)
- [TI E2E: INA226 I2C Read Errors (Repeated Start Required)](https://e2e.ti.com/support/amplifiers-group/amplifiers/f/amplifiers-forum/540417/ina226-i2c-read-errors/) -- MEDIUM confidence (community forum)
- [Stack Exchange: INA226 I2C Communication Issues on STM32F4](https://electronics.stackexchange.com/questions/731229/weird-i2c-communication-issue-with-ina226-tested-on-arduino-uno-and-stm32f4) -- MEDIUM confidence
- [Hackaday.io: Mastering MOSFET Paralleling -- Current Sharing & Thermal Management](https://hackaday.io/page/399508-mastering-mosfet-paralleling-a-deep-dive-into-current-sharing-amp-thermal-management-for-high-power-circuits) -- MEDIUM confidence
- [TI E2E: DAC8571 Output Fixed at 0V (Power-Down Mode Misconfiguration)](https://e2e.ti.com/support/data-converters-group/data-converters/f/data-converters-forum/1500783/dac8571-the-output-is-fixed-at-0v) -- MEDIUM confidence (community forum)
- [Nexperia IAN50005: Paralleling Power MOSFETs in High Power Applications](https://www.nexperia.cn/applications/interactive-app-notes/IAN50005_paralleling_MOSFETs_in_high_power_applications.html) -- HIGH confidence (manufacturer app note)
- [Infineon: Parallelization of PROFET Devices (Current Sharing Principles)](https://community.infineon.com/t5/Knowledge-Base-Articles/Parallelization-of-PROFET-devices/ta-p/961642) -- MEDIUM confidence
- [Arduino Forum: cJSON Memory Management on STM32](https://www.cnblogs.com/SuperCodeCat/p/18840094) -- LOW confidence (blog, but content matches known patterns)
- [cJSON GitHub: Custom Allocator Hooks (cJSON_InitHooks)](https://github.com/DaveGamble/cJSON) -- HIGH confidence (official source)
- [Zephyr RTOS: WS2812 Driver Commit bdde886 (SPI Overhead and Interrupt Disable)](https://github.com/zephyrproject-rtos/zephyr/commit/bdde886ed59e993b6839780e57f6ed961d1613aa) -- HIGH confidence (official RTOS commit)
- [EEVblog Forum: WS2812B DMX Flickering Debug (Real-World Interrupt Disable Testing)](https://www.eevblog.com/forum/projects/ws2812b-dmx-project/10/) -- LOW confidence (forum, but real-world debug narrative)
- [OpenBMC Mailing List: phosphor-pid-control Scaling Issue (Fan PID + Sensor Scaling Bug)](https://lists.ozlabs.org/pipermail/openbmc/2019-March/015648.html) -- MEDIUM confidence
- [CH32V303CBT6 Datasheet and Reference Manual (WCH)](https://www.wch.cn/products/CH32V303.html) -- HIGH confidence (official MCU documentation)
- [Stack Overflow: AVR Timer PWM WS2812 -- Cycle Budget Analysis Proving ISR Approaches Infeasible](https://stackoverflow.com/questions/78136834) -- MEDIUM confidence

---
*Pitfalls research for: 电子负载控制器 (Electronic Load Controller)*
*Researched: 2026-06-02*

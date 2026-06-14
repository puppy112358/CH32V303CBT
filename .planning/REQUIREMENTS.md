# Requirements: 电子负载控制器

**Defined:** 2026-06-02
**Core Value:** 精确、安全的电子负载控制 — 可靠的CV/CC双模式运行，每路MOS独立过流保护，实时数据监控和上报

## v1 Requirements

Requirements for initial release. Each maps to roadmap phases.

### 通信与协议 (COMM)

- [x] **COMM-01**: 上位机通过UART6发送cJSON格式指令，设置CV恒压值或CC恒流值，波特率>115200bps，奇校验，其中使用引脚PB8和PB9重映射为uart6的TX和RX引脚
- [x] **COMM-02**: 系统通过UART6以cJSON格式上报测量数据（4路MOS电流/电压+汇总电流/电压/功率），频率10Hz
- [x] **COMM-03**: USB-CDC虚拟串口提供调试日志输出（printf重定向），不影响UART0指令通道

### I2C设备驱动 (I2C)

- [x] **I2C-01**: I2C1总线驱动5个INA226（地址0x40-0x44），读取总线电压、分流电压、电流、功率寄存器
- [x] **I2C-02**: I2C1总线驱动DAC8571IDGK（地址0x4C），16位DAC输出参考电压
- [x] **I2C-03**: I2C总线超时恢复机制 — 每次I2C操作带tick-count超时，从设备卡死时执行9时钟脉冲总线复位

### 控制回路 (CTRL)

- [x] **CTRL-01**: CV恒压模式 — 软件PID调节DAC输出，控制运放比较器驱动MOS管线性区，维持设定电压
- [x] **CTRL-02**: CC恒流模式 — 软件PID调节DAC输出，控制运放比较器驱动MOS管线性区，维持设定电流
- [x] **CTRL-03**: PID带anti-windup（反积分饱和）和输出钳位，~100ms调节周期，软启动抑制上电冲击

### INA226监测与保护 (PROT)

- [x] **PROT-01**: 4路MOS的INA226配置为过流报警模式（Latch模式），单MOS限流 = 额定电流/4 × 1.3
- [x] **PROT-02**: INA226 ALARM引脚EXTI中断触发，ISR读取Alert寄存器清除Latch后置flag，主循环执行关断DAC+模式切换+故障记录
- [x] **PROT-03**: 汇总INA226读取总电流/电压/功率，用于数据上报和总功率保护(OPP)
- [x] **PROT-04**: INA226校准寄存器定期重写验证，防止校准值静默归零导致电流读数为0

### WS2812状态指示 (LED)

- [x] **LED-01**: TIM2 CH1 PWM + DMA驱动WS2812灯带，800kHz位速率，零CPU开销
- [x] **LED-02**: 灯带颜色指示工作状态（恒压模式/恒流模式/故障保护/待机等）

### 风扇控制 (FAN)

- [x] **FAN-01**: TIM PWM输出驱动风扇调速
- [x] **FAN-02**: 风扇转速反馈（测速信号输入捕获）采集RPM
- [x] **FAN-03**: 位置式PID控制风扇转速（根据温度/功率调节），带anti-windup

## v2 Requirements

Deferred to future release. Tracked but not in current roadmap.

### 高级模式 (ADV)

- **ADV-01**: CR恒阻模式 — 软件计算R=V/I，PID调节维持设定电阻值
- **ADV-02**: CP恒功率模式 — 软件计算P=V×I，PID调节维持设定功率值
- **ADV-03**: 电池放电测试 — CC放电 + Ah容量积分 + 可配置截止条件（电压/容量/时间）
- **ADV-04**: Von Latch — 输入电压低于阈值时自动关断，防电池过放

### 高级保护 (ADV-PROT)
- **ADV-PROT-01**: 过温保护(OTP) — NTC温度传感器ADC采样 + 温度阈值关断 + 两级保护（警告降额 + 临界关断）
- **ADV-PROT-02**: MOSFET热降额 — 温度升高时自动降低电流限制

## Out of Scope

| Feature | Reason |
|---------|--------|
| Transient/List/Sequence模式 | 需要定时器触发DMA到DAC，架构与静态PID不同，v2+ |
| SCPI协议 | cJSON已满足需求，SCPI增加复杂度但无增量价值 |
| 上位机控制软件 | 仅固件项目 |
| LCD/OLED屏幕显示 | 当前仅WS2812灯带指示 |
| RTOS | 裸机super-loop已满足确定性要求，32K SRAM不足以支持RTOS |
| PA1 LED控制 | 预留IO，本次不实现 |

## Traceability

Which phases cover which requirements. Updated during roadmap creation.

| Requirement | Phase | Status |
|-------------|-------|--------|
| COMM-01 | Phase 3 | Complete |
| COMM-02 | Phase 3 | Complete |
| COMM-03 | Phase 1 | Complete |
| I2C-01 | Phase 1 | Complete |
| I2C-02 | Phase 1 | Complete |
| I2C-03 | Phase 1 | Complete |
| CTRL-01 | Phase 2 | Complete |
| CTRL-02 | Phase 2 | Complete |
| CTRL-03 | Phase 2 | Complete |
| PROT-01 | Phase 2 | Complete |
| PROT-02 | Phase 2 | Complete |
| PROT-03 | Phase 2 | Complete |
| PROT-04 | Phase 2 | Complete |
| LED-01 | Phase 4 | Complete |
| LED-02 | Phase 4 | Complete |
| FAN-01 | Phase 4 | Complete |
| FAN-02 | Phase 4 | Complete |
| FAN-03 | Phase 4 | Complete |

**Coverage:**
- v1 requirements: 18 total
- Mapped to phases: 18
- Unmapped: 0

---
*Requirements defined: 2026-06-02*
*Last updated: 2026-06-02 after roadmap creation*

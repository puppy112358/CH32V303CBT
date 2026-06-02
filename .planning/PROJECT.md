# 电子负载控制器 (Electronic Load Controller)

## What This Is

基于CH32V303CBT6 RISC-V MCU的电子负载控制固件。通过UART接收cJSON格式的恒压(CV)/恒流(CC)指令，控制外置DAC8571输出参考电压驱动恒压恒流环路。I2C1总线上挂载5个INA226（4路MOS电流检测+1路汇总电流检测）实时监测，数据以10Hz频率通过UART上报。WS2812灯带显示负载状态，风扇PID控速散热。USB-CDC提供调试日志输出。

## Core Value

**精确、安全的电子负载控制** —— 可靠的CV/CC双模式运行，每路MOS独立过流保护，实时数据监控和上报。

## Requirements

### Validated

- CH32V303CBT6 RISC-V MCU (128K Flash, 32K SRAM, RV32IMAC) with Standard Peripheral Library V2.9
- Bare-metal super-loop架构，中断驱动外设访问
- `riscv-none-embed-gcc` 交叉编译工具链，MounRiver Studio IDE
- UART/USART/I2C/SPI/TIM/GPIO 外设驱动已就绪（SPL提供）
- 通过WCH-Link调试下载
- 参考RTOS port可选（FreeRTOS, RT-Thread, TencentOS）

### Active

#### 通信与协议
- [ ] **COMM-01**: UART0 cJSON协议通信，波特率>115200，奇校验
- [ ] **COMM-02**: USB-CDC虚拟串口，用于调试日志输出（printf重定向）
- [ ] **COMM-03**: cJSON指令解析：CV模式设置恒压值，CC模式设置恒流值
- [ ] **COMM-04**: cJSON数据上报：4路MOS电流/电压+汇总电流/电压，10Hz频率

#### DAC控制
- [ ] **DAC-01**: I2C1驱动DAC8571IDGK，16位DAC输出参考电压
- [ ] **DAC-02**: 软件PID控制回路，~100ms调节周期，实现CV/CC恒压恒流

#### INA226检测
- [ ] **INA-01**: I2C1驱动5个INA226（地址分配：4路MOS + 1路汇总）
- [ ] **INA-02**: 4路MOS的INA226配置为过流报警模式，单MOS限流 = 额定电流/4 × 1.3
- [ ] **INA-03**: 汇总INA226读取总电流/电压/功率
- [ ] **INA-04**: INA226 ALARM引脚中断处理，过流时触发保护动作

#### WS2812状态指示
- [ ] **LED-01**: 驱动WS2812灯带，通过颜色显示负载工作状态
- [ ] **LED-02**: TIM PWM输出PA0控制（PA1预留）

#### 风扇控制
- [ ] **FAN-01**: PWM驱动风扇调速
- [ ] **FAN-02**: 风扇转速反馈（测速）采集
- [ ] **FAN-03**: 简易PID控制风扇转速，根据温度/功率调节

### Out of Scope

- 上位机/PC端控制软件 — 仅固件
- PA1 LED控制 — 预留，本次不实现
- 触摸屏/LCD显示 — 仅WS2812灯带指示
- RTOS — 裸机实现
- 程序验证 — 由用户自行完成（固件烧录和硬件测试）

## Context

- **现有代码库**: CH32V303CBT6 SPL裸机工程，40+外设示例（GPIO, USART, I2C, SPI, ADC, TIM等），编译环境已就绪
- **硬件平台**: CH32V303CBT6 + DAC8571 + 5×INA226 + WS2812灯带 + MOS功率管 + 风扇
- **I2C1总线设备**: DAC8571(1个) + INA226(5个)，共6个I2C从设备
- **控制原理**: DAC输出参考电压 → 运放比较器 → MOS管线性区控制 → 恒压/恒流
- **用户角色**: 开发者自行验证固件烧录和硬件功能，需要我在关键节点提醒验证

## Constraints

- **MCU**: CH32V303CBT6 — 128K Flash, 32K SRAM，需注意内存使用
- **I2C总线**: I2C1单总线挂载6个设备，需合理分配地址，注意总线速率和冲突
- **实时性**: 10Hz数据上报 + 100ms级控制回路 + 风扇PID，需注意时序调度
- **保护**: 4路INA226 ALARM中断需可靠响应，过流保护不能有遗漏
- **UART0**: cJSON协议 >115200bps + 奇校验，注意cJSON内存分配和解析性能
- **内存**: cJSON库在32K SRAM下的使用需注意动态内存碎片

## Key Decisions

| Decision | Rationale | Outcome |
|----------|-----------|---------|
| cJSON文本协议 vs 二进制协议 | 易于调试和上位机对接，cJSON在嵌入式领域广泛使用 | — Pending |
| 外置DAC8571 vs 内置DAC | 16位分辨率，I2C控制灵活，满足精度要求 | — Pending |
| 裸机super-loop vs RTOS | 控制逻辑简单，裸机降低复杂度和内存占用 | — Pending |
| INA226 ALARM硬件保护 vs 软件比较 | 硬件级过流响应更快更可靠，不依赖软件轮询周期 | — Pending |
| USB-CDC debug vs UART debug | USB-CDC不占用UART0指令通道，虚拟串口方便PC查看 | — Pending |

## Evolution

This document evolves at phase transitions and milestone boundaries.

**After each phase transition** (via `/gsd-transition`):
1. Requirements invalidated? → Move to Out of Scope with reason
2. Requirements validated? → Move to Validated with phase reference
3. New requirements emerged? → Add to Active
4. Decisions to log? → Add to Key Decisions
5. "What This Is" still accurate? → Update if drifted

**After each milestone** (via `/gsd:complete-milestone`):
1. Full review of all sections
2. Core Value check — still the right priority?
3. Audit Out of Scope — reasons still valid?
4. Update Context with current state

---
*Last updated: 2026-06-02 after initialization*

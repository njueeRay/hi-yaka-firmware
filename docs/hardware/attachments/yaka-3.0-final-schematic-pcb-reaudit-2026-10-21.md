# YaKa 3.0 封存前原理图、PCB 与 PRD 全链路复审

> 文档状态：封存前复审版 `R1.6`  
> 复审日期：2026-10-21  
> 活体工程：`YaKa v3.0`  
> 项目 / 原理图 / PCB UUID：`8f58125ad0d64436afbfeeeb03d3b159` / `23291d2ced634dd6` / `db0afd3e4013441a9add6d64b7979887`  
> 适用范围：当前 EasyEDA Pro 在线工程；本文件覆盖并更新 2026-10-14 及本文件 R1.0 的状态结论。  
> 审计目标：验证 `PRD 目的 → 功能 → 模块 → 引脚/电源 → PCB → 固件契约` 是否闭环；不以“有器件、DRC 为 0”代替功能成立。

## 0. 封存前状态清单

### 0.1 已闭环项与能力边界

| ID | 级别 | 位置 | 活体事实 | 明确处置 | 验收 |
|---|---|---|---|---|---|
| `YAKA-FINAL-P0-001` | 已关闭 | IO 页 `MOTOR/U14/Q9/R80-R83` | MOTOR 已更换为 3 V ERM `LCM0720A3176F/C2942347`；原理图为两个逻辑脚，PCB 为两个焊盘；`1/+→SYS_3V3`、`2/-→Q9/$5N7` | 保持当前设计；EVT 仅验证启动电流、震感、Q9 VDS 和 3.3 V 纹波 | 器件身份、两脚/两焊盘、极性、端点和装配属性 0 差异；PCB DRC=0 |
| `YAKA-FINAL-P0-002` | 已转为 Owner 接受的能力边界 | POWER 页 `U5 LGS4056H` | 充电芯片能自治判满、终止和再充；`CHRG` 只驱动板载灯，`DONE` 悬空，无 VBUS_DET，因此 MCU 不能唯一识别完整充电状态 | 首板不改硬件；PRD/固件只承诺板载充电灯和 BAT ADC 电量估算，不承诺完整充电状态 UI | EVT 验证充电电流、终止、再充、异常温升和 LED 行为；文档不得超出硬件能力 |

说明：当前不存在未关闭的电气 P0。`YAKA-FINAL-P0-002` 的功能边界仍必须写入 PRD/固件规格，但不再要求工程师为 EVT1 改图。`C22` 也已在原理图和 PCB 双侧 DNP。

### 0.2 当前发布结论

**当前结论：`GO TO PROTOTYPE`；制造发布为 `CONDITIONAL GO`。** 原理图、PCB、MOTOR、BOM 与 DRC 的电气门禁均已关闭。下单前只剩从冻结版本导出生产文件、人工预览和库存/元件匹配页签核。

边界：本版支持振动反馈；电池支持硬件自治充电、板载充电灯和电压估算，但不支持 MCU 读取完整充电状态。该边界必须进入 PRD/固件，不得在 UI 中虚构“已充满”等不可观测状态。

## 1. 当前活体基线与自动检查

| 核验项 | 当前结果 | 判定 |
|---|---:|---|
| EasyEDA Pro / 连接器 / daemon | `4.1.71 / 1.10.0 / 1.10.0` | 版本健康 |
| 原理图 / PCB 绑定 | `YaKa-SCH ↔ YaKa-PCB`，UUID 正确 | 通过 |
| 原理图 ↔ PCB 器件身份 | 292/292 唯一 ID、位号、精确料号 0 差异 | 通过 |
| 原理图 ↔ PCB 装配属性 | C22、R66、SCREW1–4 为 DNP/非贴装；其余同步 | 通过 |
| 原理图引脚 ↔ PCB 焊盘网络端点 | 全部器件端点 0 差异；MOTOR 两脚/两焊盘一致 | 通过 |
| 业务 Must 相关未连接引脚 | 未发现无故悬空；U5 `DONE` 属已接受能力边界 | 通过静态签核 |
| PCB 层数 / 网络数 | 4 层 / 182 nets | 已读取 |
| PCB 原生 DRC | `passed=true`，0 violations | 通过 |
| 原理图原生 DRC | `0 error / 0 fatal / 14 warning` | 警告为离网格、属性、RGB 链尾悬空；无原生电气错误 |
| 最新生产 BOM | 97 行、286 个贴装位号；6 个 DNP/结构对象排除；空商城编号 0 | 通过工程侧复核；下单页仍需库存/旋转签核 |
| 重构式 `schematic.check/bridgeCheck` | 本次 SDK 返回“引脚几何不可用” | 工具覆盖受限；已用全页语义网表、原生 DRC 和 PCB 端点一致性补足，不宣称该检查通过 |

## 2. PRD—模块—固件追踪矩阵

| PRD 目的 | 硬件实现与关键连接 | 固件必须遵守的契约 | 当前判定 |
|---|---|---|---|
| 完全断电、正常启动 | `MASTER_SW` 直接串 `VIN→VOUT`；U7 生成 SYS_3V3；C6 `CHIP_PU` 有 10 kΩ/1 µF RC | 主开关接通即启动；文件系统必须按任意时刻硬切设计 | 满足 Must；SW7 50 mA 额定属于 EVT/寿命风险 |
| 烧录、恢复、日志 | USB-C→CH340K→UART0；DTR/RTS 自动下载；独立 BOOT/RESET | 固件保留 UART0 下载/日志；量产验证自动与手工 BOOT+RESET 两条路径 | `C22` DNP 后满足；当前原理图已 DNP |
| 应用、OTA 与资源 | C6 QFN40 + `PY25Q128HA` 16 MiB 外部 Flash，专用 SPI0/1 引脚 | 按 16 MiB 生成分区表；OTA 至少双应用槽、校验与回滚；首次点亮先保守频率/DIO | 硬件满足 |
| 2.4 GHz Wi-Fi | C6 RF、40 MHz ±10 ppm 晶振、CLCCL/天线匹配、短顶层馈线 | 使用 ESP-IDF C6；射频功率/共存参数按实测定 | 静态满足；VNA/RSSI/吞吐仍是 EVT |
| 非触控显示 | H0160S002 V1，ST7789，200×320；VDD/IOVCC 均 3.3 V；FPC 12 Pin | **实际契约**：GPIO6→FPC pin9 `SDA/MOSI`，GPIO7→pin10 `SCL/SCLK`；PCA9557 控制 CS/DC/RST/BL。不得照当前网名 `GPIO6=CLK/GPIO7=DIN` 配置 | 电气可用；名称写反是集成风险，不是铜线断路 |
| 屏幕可读反馈 | LEDA=SYS_3V3；LEDK 经 10 Ω 与 Q10 低侧开关；BL 由 PCA9557 | 启动先关闭背光，LCD 初始化后再开；若仅开关不做软 PWM，亮度只有开/关 | 可点亮但不保证批次亮度；列 P1 实测 |
| 四向导航 | 六个方向/功能按键中的 DPAD 通过 GPIO4 ADC 电阻梯 | ADC1_CH4；按实板标定中心值与窗口；禁止把多键同时按下解释为单键 | 满足 |
| 两动作键 | KEYA→GPIO22，KEYB→GPIO5，经 Q6/Q7 反相拉低 | 输入上拉、去抖；ACT_A/ACT_B 行为由 PRD/UI 冻结 | 满足 |
| 软电源/关机态录音 | D5/D6/D7 的 A 端均在 `SW_EN`，K 端在 PWR_A/PWR_B/PWR_EN；按键高电平不能正向送入 SW_EN | 当前只能把 PWR_A/B 当作开机后的按键，不能从 MASTER_SW=OFF 唤醒 | `YAKA-REC-002` 仍是 TBD，不能判通过；与“完全硬断电”语义冲突，见 5.1 |
| 双麦录音 | 两颗 ZTS6216→ES7210 CH1/CH2；I2S SDOUT1 经 R65→GPIO13；R66 为备用且 DNP | ES7210 使用 I2C + I2S；固定主从、采样率、slot 和 MCLK；启动时关闭未用通道 | 满足 |
| 回声参考/满双工 | ES8311 输出同时进入 ES7210 CH3 差分参考 | AEC 使用实际播放参考通道；不可把 CH3 当第三只环境麦 | 硬件基础满足；算法效果 EVT |
| 扬声器播放 | ES8311→HT8691R→8 Ω/1 W 扬声器；PA_EN=GPIO23；升压约 7.56 V | 上电先静音再使能；限制数字增益/最大音量，避免 1 W 扬声器过载；关断顺序反向 | 满足；声压/失真取决于腔体与限幅 |
| 本地录音存储 | TF 卡 SDSPI：GPIO18 CMD、19 CLK、20 DAT0、15 CS；DAT1/2 不用；CD 未接 | SDSPI 1-bit；无卡检测采用挂载/轮询；写入需分段落盘、同步与掉电恢复 | 满足 Must |
| NFC 只读业务 | PN532 I2C 模式绑带已为 `I1:I0=10`；I2C 连通；IRQ 未接；RES&OFF 固定上拉 | 使用轮询；设超时；总线异常可先 I2C 恢复，极端情况需整机复位 | 满足只读核心；低功耗/独立硬复位能力受限 |
| IMU | QMI8658A，I2C 地址绑带为 0x6A；IRQ 未接 | 当前只允许轮询；摇晃唤醒若后续进入 Must，需另审 IRQ/低功耗唤醒 | 满足 Could，不支持硬件中断唤醒 |
| 独立 RGB + 五灯阵列 | GPIO8 单灯；GPIO21 五灯链；六颗均 `WS2812B-2020-V6/C52917434`，各有去耦 | 用 RMT/等价确定时序；上电默认灭；限制总亮度和全白占空比 | 满足 |
| 振动反馈 | 3 V ERM `LCM0720A3176F` + Q9 低侧开关 | PCA9557 控制开/关或低频脉冲；不按 LRA 共振驱动 | 满足短促确认/提醒；震感强度进入 EVT |
| 电池电量 | BAT+ 经 1 MΩ/2 MΩ 与 100 nF 到 GPIO2 ADC | ADC 12 dB 衰减、校准、多次采样；换算比例 3:2；显示百分比需实测曲线 | 硬件满足；高阻源需增加采样时间 |
| 电池充电 | LGS4056H，RPROG=5.1 kΩ，约 176 mA；TEMP 经 R89=0 Ω 接地；3.7 V/650 mAh 两线保护电池 | 充电电流/终止/再充由硬件；固件不能声称读取电芯温度或完整充电状态 | 充电可用；UI 仅显示电量估算，不显示不可观测的精确充电状态 |
| 版本、诊断、恢复 | UART 日志、屏幕、Flash/NVS、按键均具备 | 显示版本、复位原因、存储/网络/外设自检；不得显示明文凭据 | 硬件基础满足 |

### 2.1 模块目的、启动默认态与固件控制闭环

| 模块 | 在产品中的唯一目的 | 上电/复位安全态 | 固件控制闭环 | 结论 |
|---|---|---|---|---|
| ESP32-C6 + 40 MHz + 16 MiB Flash | 承载 UI、录音、联网、存储、诊断和 OTA | `CHIP_PU` RC 释放后启动；GPIO9 上拉且 C22 DNP | ROM/UART 下载 → Flash ID → 分区校验 → 应用启动 | 闭环；首板先 DIO/保守频率，再资格化 QIO |
| CH340K + BOOT/RESET | 保证开发、救砖和量产烧录不依赖应用固件 | MASTER_SW 必须 ON；DTR/RTS 自动下载，按键为手工后备 | 自动下载失败时执行“按住 BOOT→点按 RESET→释放 BOOT” | 闭环；两条路径都须产测 |
| MASTER_SW + 两路 3.3 V LDO | 提供硬断电、数字/模拟电源域 | OFF 真正断开 VIN→VOUT；ON 后 SYS_3V3/AU_3V3 常开 | 固件不能阻止任意时刻硬切，只能在 UI 中引导安全关机 | 功能成立；低电量压差和并发峰值列 EVT |
| 电池 ADC + LGS4056H | 电量估算与单节电池充电 | 充电器硬件自治；ADC 高阻分压常接 | ADC 只估电量；当前版本不实现完整充电状态 UI | 闭环于已接受首板范围 |
| LCD + PCA9557 | 承担主要信息、菜单和错误表达 | 扩展器先设 RST/CS/DC/BL 安全态，背光最后开启 | 先 I2C/PCA9557，再 ST7789，GPIO6=MOSI、GPIO7=SCLK | 闭环；网名写反必须固件显式覆盖 |
| 四向键 + ACT_A/B + PWR_A/B | 本地导航、确认和快捷入口 | GPIO 输入上拉；ADC 梯在复位阶段不应被按压 | ADC 实板标定、独立键去抖、动作表由 PRD/UI 冻结 | 开机态闭环；硬关机唤醒不成立 |
| ES7210 + 双麦 | 对话/录音采集 | 初始化前保持 PA 关闭，未用 ADC 通道关闭 | I2C 配置 + I2S 时钟/slot 固定；监测溢出和掉帧 | 闭环；声学质量进入 EVT |
| ES8311 + HT8691R + 扬声器 | 语音/提示音播放和 AEC 参考 | PA_EN 默认关闭；无 I2S 时不得使能功放 | DAC 稳定后开 PA；停播先关 PA；限制最大增益 | 闭环；1 W 扬声器必须限幅 |
| TF 卡 | 长录音、本地缓存和离线延续 | 上电先低速挂载；无卡不能阻塞启动 | SDSPI、分段写入、周期同步、掉电恢复、挂载轮询 | 闭环；无 CD 是已接受能力边界 |
| PN532 | 只读 NFC 情境入口 | I2C 模式固定；无 IRQ/独立复位 | 轮询、超时、I2C 总线恢复；失败不得阻塞首页 | 只读核心闭环；低功耗/独立恢复受限 |
| QMI8658A | 可选动作感知 | I2C、无中断唤醒 | 轮询读取；未进入 Must 前不依赖深睡唤醒 | 满足 Could |
| 两路 RGB | 弱注意力状态补充 | 上电默认灭、全局亮度为 0 | GPIO8 单灯、GPIO21 五灯链，RMT/等价外设驱动 | 闭环；必须限总电流 |
| 振动 | 短促确认/提醒 | 当前默认关闭 | 3 V ERM 由 Q9 低侧开关驱动，可做开/关与有限脉冲模式 | 闭环；强度与噪声进入 EVT |

## 3. ESP32-C6 引脚健康表

| GPIO/专用脚 | 当前用途 | 启动/复用判断 | 判定 |
|---:|---|---|---|
| GPIO0 | I2S MCLK | 可经 GPIO matrix 映射；放弃外部 32 kHz 晶振 | 正常 |
| GPIO1 | I2S DO | 同上 | 正常 |
| GPIO2 | BAT ADC | ADC1_CH2；最高电芯 4.25 V 经分压约 2.83 V | 正常；需 ADC 衰减与校准 |
| GPIO3 | I2S BCLK | 无同时复用 | 正常 |
| GPIO4/MTMS | ADC 按键梯 | 绑带/JTAG 复用；复位采样影响 SDIO Slave 时钟边沿，不改变 SPI Boot；本产品未用 SDIO Slave | 可用；仍应把“复位时不按方向键”写入产测规范 |
| GPIO5/MTDI | KEYB | 绑带/JTAG 复用；复位采样影响 SDIO Slave 时钟边沿，不改变 SPI Boot | 可用；同上 |
| GPIO6/MTCK | LCD pin9 数据 | 当前网名误写为 `LCD_CLK` | 电气正常；固件按 MOSI 配置 |
| GPIO7/MTDO | LCD pin10 时钟 | 当前网名误写为 `LCD_DIN` | 电气正常；固件按 SCLK 配置 |
| GPIO8 | 单颗 RGB 数据 | 启动绑带脚；10 kΩ 上拉，下载模式组合要求满足 | 正常 |
| GPIO9 | BOOT | 10 kΩ 上拉、按键及自动下载；C22 已设 DNP | 原理图、PCB 与 BOM 属性已同步关闭 |
| GPIO10 | I2C SDA | 全局共享总线 | 正常 |
| GPIO11 | I2C SCL | 全局共享总线 | 正常 |
| GPIO12 | I2S WS | 与原生 USB D− 复用；本机使用 CH340K USB-UART | 无产品冲突 |
| GPIO13 | I2S DI | 与原生 USB D+ 复用；同上 | 无产品冲突 |
| GPIO15 | TF CS | 绑带脚；51 kΩ 上拉 | 可用；改变 eFuse/JTAG 策略后复核 |
| GPIO16/U0TXD | CH340 RXD | 串联 R1 | 正常 |
| GPIO17/U0RXD | CH340 TXD | UART0 | 正常 |
| GPIO18 | TF CMD/MOSI | SDIO_CMD 复用为 GPIO/SPI | 正常 |
| GPIO19 | TF CLK | SDIO_CLK 复用 | 正常 |
| GPIO20 | TF DAT0/MISO | SDIO_DATA0 复用 | 正常 |
| GPIO21 | 五颗 RGB 数据 | 无同时复用 | 正常 |
| GPIO22 | KEYA | 无同时复用 | 正常 |
| GPIO23 | PA_EN | 功放控制 | 正常；启动默认关闭/下拉由功放内部实现 |
| SPI0/1 专用脚 20–26 | 16 MiB Flash | 未与业务 GPIO 共用 | 正常 |
| CHIP_PU | RESET/自动下载 | 10 kΩ + 1 µF RC，独立 RESET 键 | 正常 |
| ANT/XTAL/电源/EP | RF、40 MHz、3.3 V、GND | 专用连接完整 | 正常；布局与 RF 需实测 |

结论：未发现两个“必须同时工作”的业务外设争用同一 GPIO。实际集成风险集中在 LCD 名称与物理功能相反、绑带脚初始化顺序和未冻结的软电源语义，不是 GPIO 数量不足。

## 4. 总线、地址与 PCB 时序健康

### 4.1 共享总线

| 总线 | 设备/端点 | 固件初始参数 | 判定 |
|---|---|---|---|
| I2C | PCA9557 `0x19`、ES8311 `0x18`、ES7210 `0x41`、QMI8658A `0x6A`、PN532 `0x24` | 首板先 100 kHz；分设备超时和总线恢复 | 地址无冲突 |
| I2S | ES8311 播放 + ES7210 采集 | 同一 MCLK/BCLK/WS 规划；明确 slot/位宽/主从 | 引脚完整 |
| LCD 4SPI | GPIO6/7 + PCA9557 CS/DC/RST | MOSI=GPIO6、SCLK=GPIO7；CS/DC/RST 先初始化扩展器 | 引脚完整 |
| TF SDSPI | GPIO18/19/20/15 | 先低速初始化，再提升频率；无 CD | 引脚完整 |
| Flash SPI0/1 | C6 专用引脚 | 启动 ROM/IDF 管理 | 引脚完整 |

### 4.2 未连接引脚逐项判定

| 对象 | 未连接引脚 | 判定与功能边界 |
|---|---|---|
| LED1、五灯链尾 LED3 | `DO` | 地址灯链末端，预期悬空；原理图 warning 可接受 |
| U3 PCA9557 | IO3、IO5 | 当前为真实空闲口；可优先用于 `CHRG/DONE` 状态输入 |
| U5 LGS4056H | `DONE` | 首板按 Owner 决策保持未接；充电器仍可自治判满，但 MCU 不具备完整状态可观测性 |
| USB-C | SBU1/SBU2 | 本产品只使用 USB 2.0/供电，预期不接 |
| U6/U7 LDO | 器件 `NC` | 预期不接 |
| U9 CH340K | CTS# | 不使用硬件流控，预期不接 |
| 六个 DPAD/ACT 与 PWR_A/B | 开关同侧重复触点 | 当前另一对触点已形成开关功能；未用脚不构成开路 |
| MASTER_SW | pin 3 | 已冻结为两档硬开关，另一掷位 NC；正常 |
| ES8311 | ASDOUT、MIC1N/MIC1P | 当前只承担播放 DAC，不使用其 ADC/数字麦输出；正常 |
| ES7210 | INT、DMIC_CLK | 当前使用模拟麦与轮询/寄存器控制；不使用数字麦时钟；正常 |
| PN532 | IRQ、UART/SPI/辅助 GPIO 等 | 当前冻结 I2C 轮询，只读核心可用；无 IRQ、无受控 RES&OFF 会限制低功耗与独立故障恢复，列 P1 |
| QMI8658A | INT1/INT2、AUX | 当前仅轮询；满足 Could，不支持深睡手势唤醒 |
| TF 卡座 | CD | 无卡检测；固件必须通过挂载/轮询判卡 |
| LCD FPC | TE | 不使用撕裂同步；刷新策略必须避免依赖 TE |

未发现新的“业务 Must 引脚无故悬空”。U5.DONE 是已记录的产品能力边界，不是未知断网。这张表只证明连接意图可解释，不替代相应的 EVT。

### 4.3 当前 PCB 铜线实测

| 网络组 | 当前长度/偏差 | 判断 |
|---|---:|---|
| USBIN D+/D− | 552.121 / 552.391 mil，差 0.270 mil | 优良 |
| USBOUT D+/D− | 421.107 / 423.341 mil，差 2.234 mil | 优良 |
| TF CMD/CLK/DAT0 | 最大差 4.845 mil（约 0.12 mm） | 满足 SDSPI |
| LCD 数据/时钟/CS | 最大差 3.099 mil（约 0.079 mm） | 满足 4SPI |
| I2S 五线 | 最大差 6.694 mil（约 0.17 mm） | 满足板内音频 |
| Flash 五线 | 当前报告组内最大差 24.029 mil；全部顶层、无过孔 | 满足当前连接；实板先保守频率验证 |
| 模拟麦差分 | 各对内 0–3.082 mil；四路组间差 5.019 mil | 优良 |
| 功放差分输出 | 差 1.827 mil | 优良 |
| I2C SCL/SDA | 不要求等长；4.7 kΩ 上拉 | 首板测上升时间 |
| RF `LAN_IN` | 约 3.53 mm，顶层无过孔 | 拓扑短；50 Ω/S11 不能由长度单独证明 |

## 5. 需要明确的跨层边界

### 5.1 MASTER_SW 与“关机态快速录音”不能同时按字面成立

当前 `MASTER_SW` 直接断开 `VIN→VOUT`，这是 PRD 冻结的“完全硬断电”。D5/D6/D7 方向又使软启动/保持路径无效，因此 MASTER_SW=OFF 时 PWR_A/PWR_B 不能唤醒设备。

这不是固件可修问题。当前优先级应为：

1. 保留冻结的硬断电 Must；
2. 将 PWR_A/PWR_B 定义为开机后的快捷键；
3. 把“关机态快速录音”改为“MASTER_SW 已 ON、系统处于低功耗待机时快速进入录音”，或延期；
4. 不要简单翻转 D5-D7 后宣称修复，因为那会让软按键绕过 MASTER_SW，破坏完全断电语义。

### 5.2 充电状态能力边界

BAT ADC 只能估算电芯电压，不能可靠区分 USB 未插、恒流/恒压充电、充满、无电池、温度/输入异常。当前首板不增加 `CHRG/DONE/VBUS_DET` 可编程链路，因此固件只能提供电量估算；板载 CHRG 灯可直观显示充电动作，LGS4056H 自身仍能终止并再充。完整状态 UI 若将来重新进入 Must，才需要硬件 ECO。

### 5.3 ERM 替换已经关闭驱动类型问题

原 1.8 V LRA 确实不能由单向低侧开关正确驱动；当前已更换为 3 V 两线 ERM，适配 `SYS_3V3→MOTOR→Q9→GND` 拓扑。DRC、端点和精确料号均已通过；剩余任务是首板测启动电流、震感、Q9 VDS 和电源扰动，不再是封版前 ECO。

## 6. 非阻断但必须进入 EVT 的 P1

1. **整机电源裕量**：Wi-Fi TX、LCD 背光、PN532、TF 写入、功放与 RGB 并发时测 SYS_3V3/VOUT 压降、纹波、LDO 与 SW7 温升；首版限制 RGB 全白亮度。
2. **SW7 额定**：`MSK12C02/C431540` 标称 50 mA，低于整机峰值；首板可验证，但量产前必须换成有明确持续电流和寿命裕量的同结构器件。
3. **LCD 背光**：规格书要求三颗并联 LED 每颗 15–20 mA 并推荐恒流；现 3.3 V + 10 Ω 受 Vf 离散影响大。首板验证最低电量、冷热与批次亮度；偏暗或不一致时改恒流驱动。
4. **音频输出限幅**：HT8691R 升压约 7.56 V，而扬声器为 8 Ω/1 W；固件必须设置音量上限并在腔体中测 THD、温升和声压。
5. **I2C**：先 100 kHz；若升至 400 kHz，最远器件测 30%–70% 上升时间，超过约 300 ns 再调整 4.7 kΩ 上拉。
6. **NFC**：IRQ 未接、RES&OFF 不受控；只读轮询可工作，但应验证连续读卡、无卡超时、I2C 异常恢复、金属/电池装配后的读距。
7. **IMU**：IRQ 未接，只能轮询；当前属于 Could。若后续要求深睡手势唤醒，必须改硬件。
8. **TF 掉电**：无 CD；录音采用分段、周期同步、临时文件/原子改名与随机掉电恢复测试。
9. **RF**：按实际四层叠层确认馈线 50 Ω，并以 VNA/S11、吞吐与 RSSI 实测；此前“85.7%”不能代表整机天线效率。
10. **闲置 1.8 V 电源**：U8/C32/C33 已无 LCD 负载，属于 BOM/功耗优化；不影响功能，可在本轮 ECO 顺手删除，也可首板保留。
11. **边充边用与低电量供电**：LGS4056H 的 BAT 端同时承担电池和系统输入，当前没有独立 power-path/load-sharing；整机运行电流会参与充电终止判断。ME6217 为 3.3 V LDO，电池电压接近 3.3 V 时输出会进入压差区。若产品允许开机充电，首板必须覆盖“Wi-Fi + 录音/播放 + 背光”并发下的充满判定、反复再充、SYS_3V3 最低值和复位；若不能稳定通过，再增加电源路径管理或改用升降压架构。

## 7. 首板固件初始化顺序

1. 最小启动：保持 PA、背光、RGB、马达关闭；输出复位原因和 Flash ID。
2. 初始化 I2C 100 kHz，扫描并核对五个固定地址；PCA9557 先配置安全默认态。
3. 初始化 LCD：按真实 pinout 配置 GPIO6=MOSI、GPIO7=SCLK；复位、发初始化表、最后开背光。
4. 初始化 TF：低速 SDSPI 挂载、读写/同步/重挂载测试；无卡时不得阻塞启动。
5. 初始化音频：先时钟和 ES7210/ES8311，再使能 PA；停止时先关 PA。
6. 初始化 NFC/IMU：均采用带超时的轮询，单个器件失败不得阻塞首页与录音。
7. 初始化 ADC：电池 ADC 使用衰减/校准/多采样；按键梯使用实板阈值表。
8. 初始化 RGB：两条链分别用确定时序外设，默认灭，设置全局亮度上限。
9. 初始化振动控制为关闭；使用当前 ERM 做短促开/关或有限脉冲测试，先从短脉宽开始并监测 3.3 V 纹波。
10. 进入业务前完成最小自检，并把失败模块映射到屏幕/日志，不以无限重启代替恢复。

## 8. 封存退出条件

- [x] `YAKA-FINAL-P0-001` 已关闭：3 V ERM 与两脚/两焊盘低侧开关拓扑一致。
- [x] `YAKA-FINAL-P0-002` 已由 Owner 选择首板能力边界；PRD/固件文案仍需同步。
- [x] `YAKA-FINAL-P0-003` 已关闭：C22 原理图/PCB/BOM 同为 DNP；PCB DRC 为 0。
- [x] 当前原理图原生 DRC 为 0 error / 0 fatal；14 条 warning 已分类，无新增功能悬空。
- [x] 当前 292/292 器件、供应商料号、装配属性与网络端点比较为 0 差异。
- [ ] 导出 Gerber、钻孔、坐标、装配图、BOM、钢网设置并记录哈希。
- [ ] 冻结固件引脚表、I2C 地址表、LCD 真实 pinout、I2S 契约与 16 MiB 分区表。
- [ ] 准备自动下载、手工 BOOT+RESET、Flash、TF 连续写入、LCD、按键、录放音、NFC、IMU、RGB、振动、充电状态与电源压力测试。

## 9. 证据、置信度与未验证边界

| 结论 | 证据 | 置信度 |
|---|---|---|
| C22 原理图/PCB 均 DNP | 活体原理图与 PCB 组件属性；292 项逐一比较 | 高 |
| 当前 PCB 原生 DRC 为 0 | EasyEDA 原生 PCB DRC | 高 |
| 原理图引脚 ↔ PCB 焊盘网络 0 差异 | 292 个器件逐 pin/pad 端点比较 | 高 |
| 3 V ERM 与现驱动拓扑一致 | 活体精确料号、两脚/两焊盘端点 + LCM0720A3176F 规格书 | 高 |
| 完整充电状态不可由 MCU 观测 | U5 活体引脚：DONE 悬空、CHRG 仅 LED、无 VBUS_DET | 高；已作为产品边界接受 |
| IO 无同时占用冲突 | 全页活体网表 + C6 官方 GPIO/绑带资料 | 高 |
| 电池 ADC 为 1 MΩ / 2 MΩ / 100 nF | 活体 R27/R30/C30 精确料号与网络；4.25 V 输入换算约 2.83 V | 高 |
| LCD 电平与逻辑接口可用 | 活体 FPC 端点 + H0160S002 V1 规格书 | 高 |
| LCD 亮度满足最终产品 | 电路估算 | 低；必须实板测量 |
| Wi-Fi/NFC/音频/整机功耗达到产品指标 | 静态原理图与 PCB | 中/低；取决于外壳、装配和 EVT |

静态审计不能替代：焊接良率、器件真值、RF 阻抗、天线效率、麦克风声孔、扬声器腔体、显示亮度、热、充电完整周期、机械装配和随机掉电测试。

## 10. 参考资料

- [YaKa 3.0 PRD](yaka-3.0-prd.md)
- [Espressif ESP32-C6 原理图检查清单](https://docs.espressif.com/projects/esp-hardware-design-guidelines/en/latest/esp32c6/schematic-checklist.html)
- [ESP32-C6 下载设计指南](https://docs.espressif.com/projects/esp-hardware-design-guidelines/en/latest/esp32c6/download-guidelines.html)
- [ESP32-C6 PCB Layout Design](https://docs.espressif.com/projects/esp-hardware-design-guidelines/en/latest/esp32c6/pcb-layout-design.html)
- [ESP32-C6 Datasheet](https://documentation.espressif.com/esp32-c6_datasheet_en.html)
- [LGS4056H 厂商页面](https://www.legend-si.com/productinfo/1562504.html)
- [ME6217C33M5G 数据手册](https://www.lcsc.com/datasheet/C427602.pdf)
- [HT8691R 厂商页面](https://www.heroic.com.cn/en/product/audio/audio3/ht8691r-127.html)
- [LD0832AA-0126F / C2942349](https://jlcpcb.com/partdetail/LEADER-LD0832AA0126F/C2942349)
- [3 V ERM 首选 LCM0720A3176F / C2942347](https://jlcpcb.com/partdetail/LEADER-LCM0720A3176F/C2942347)
- [3 V ERM 引线式备选 LCM0830A1097F / C2759982](https://jlcpcb.com/partdetail/LEADER-LCM0830A1097F/C2759982)
- [H0160S002 V1 LCD 规格书](assets/specs/lcd-h0160s002-v1-spec.pdf)
- [303260-650mAh 电池规格书](assets/specs/battery-303260-650mah-spec.doc)

## 11. 修订记录

| 版本 | 日期 | 变更 |
|---|---|---|
| R1.5 | 2026-10-21 | 识别旧 LRA 与单向低侧开关不匹配、MOTOR 错误三脚符号，以及完整充电状态缺少可编程证据。 |
| R1.6 | 2026-10-21 | 重新加载最新版活体工程：确认 3 V ERM 已按正确两脚/两焊盘同步；292/292 器件与全部端点 0 差异；PCB DRC=0；BOM 97 行/286 个贴装位号；按 Owner 决策将充电状态改列为首板能力边界，发布结论更新为 `GO TO PROTOTYPE / 制造文件 CONDITIONAL GO`。 |

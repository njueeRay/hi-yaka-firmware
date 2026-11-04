# HiYaKa 首板 LCD、PN532 与麦克风专项诊断

> 2026-11-04；当前样机已拆 L7、功放 GPIO23 强制低、未接扬声器。结论只适用于这块受控开发样机。板级三项仍为 FAIL，不能关闭 M2/M3/M5。U11 异常发热另见 [`HIYAKA_U11_THERMAL_INCIDENT_AND_RECOVERY.md`](../hardware/HIYAKA_U11_THERMAL_INCIDENT_AND_RECOVERY.md)。

## 已核实的现象

| 链路 | 实板证据 | 当前判定 |
|---|---|---|
| 主程序 | `hi-yaka-lab` 启动，持续输出内存日志；RGB 顺序闪烁、马达短振已由操作者确认；TF 可写 WAV，IMU 返回动态六轴值 | 主循环运行，不代表整机功能正常 |
| LCD | ST7789 写入调用无报错；PCA9557 地址 `0x19` 应答；背光 IO0 在延时测试中由 0 切到 1，回读 Output=`0xC3`（TDM 版）或 `0x83`（标准双麦版），Configuration=`0x38`；操作者确认仍完全无背光；R84 两端对地均约 1.6–1.7 V，R85 两端均约 0 V | NXP PCA9557 的 IO0 是开漏输出；板上 `LCD_BL` 缺上拉、R86 反而下拉。输出锁存为 1 仅使 IO0 高阻，Q10 栅极仍为 0 V。已定位硬件设计遗漏；屏幕控制器无读回线，不能称 LCD 已应答 |
| PN532 | 冷启动全地址扫描仅见 `0x18/0x19/0x41/0x6B`；`0x24` 开机和延时探测均 NACK，固件版本请求失败；无屏幕版本仍 NACK；C108/VREF 与 `RES&OFF` 上电均约 3 V | 芯片未进入可通信状态，尚未到标签/天线业务层；无供电、硬复位及屏幕初始化三种简单解释已被实测削弱 |
| 麦克风 | ES7210 `0x41` 应答，寄存器可读；三麦 TDM 的原始 I²S 与录音全零；改为双麦标准 I²S 单声道后仍全零；双声道原始采样也全零。芯片状态读回“正常”、双麦选通、未静音、SDOUT1 未设高阻，MCU 侧三路时钟活动，但 GPIO13 数据线无翻转。ACT_B 红灯与 WAV 开关/关档多次通过，峰值均为 0；操作者报告红灯亮时 R42/R45 附近约 2.83 V，另有一次 0 V 读数未能定位到明确测点与时刻 | TF/WAV 控制链成立，音频数据链仍失败；偏置尚需逐端、同一时刻复测；下一步查 U12 焊盘时钟、SDOUT1/R65 两侧及供电 |

日志：本地未纳入 Git 的 `.tmp/lab-diag-current/serial-boot.log` 与 `serial-std.log`。TDM 诊断代码提交 `8d1e5c7`，烧录文件 SHA-256 `A7E7450A8BE4BB652BC62A06C87BA4BE9DA58D0398D582FA2C946C073FF77264`；标准双麦对照代码提交 `62ff9f7`，烧录文件 SHA-256 `9CA6D19700B57F5B7A8BE52DAFBB6ACCEB9CD13A8CF2A5BD7EF484514CFB0AA0`。两版均由 `aliyun_raynode` 上 ESP-IDF v6.1 容器构建，并在 COM5 烧录校验成功。前者改正 `BoxAudioCodec::Read()` 把读取错误伪装为成功的返回值；原始读仍全零，说明该错误不是本次零音频的唯一原因。

双声道对照提交 `a8edac9`，`aliyun_raynode` 上 ESP-IDF v6.1 构建成功，合并烧录文件 SHA-256 `63D37C5A8285A4D63752D91DABFC3EC2079F5FF318871A12153FC624E2D4ED6C`；GitHub Actions 的板级契约、产品核心和板型矩阵三项均成功。首次以 460800 baud 写入时串口中断，不能算烧录成功；在操作者确认温度正常、MASTER ON 后以 115200 baud 重试，写入与哈希校验成功。重新冷启动后的完整日志在本地 `.tmp/lab-diag-stereo/serial-stereo.log`；其中 RX 明确为 `STEREO, slot_mask=0x3`，原始 1024 字节 `nonzero=0`，TF WAV 16 万采样 `peak=0`。这次测试未使用扬声器，L7 仍断开、PA 仍禁用。

2026-11-04 实验室按键版在服务器构建；应用镜像 SHA-256 `433519B51AEC5E793ACDE53E46DDC6EC9F5145F0C8B3516DBDE2A12EB40EADB8`，COM5 上以 115200 baud 写入 OTA0 的 `0x20000`，随后 `verify-flash` 比对 2,610,544 字节，摘要一致。启动日志显示实验室功能已加载、TF 挂载、扬声器 PA 禁用。旧按键映射测试中，板上 `PWR_B` 两次启停 WAV，首段 `samples=411520 peak=0 wav_closed=1`，第二段 `samples=2991360 peak=0 wav_closed=1`；RGB 和五灯阵列在录音期间红色常亮，操作者已目视确认。两次录音的原始 I²S 读取均为 `nonzero=0`。中途出现 PCA9557 写失败、I²C 总线复位失败及 LCD 写失败，记录为并发观察，尚不能证明与录音全零同因。

**按键名纠错与复验：**该实验固件最初把内部网 `KEYB/GPIO5` 当作板上 `ACT_B`，实际它来自 `PWR_B → Q7`。活体原理图导出网表显示 `ACT_B → R39 后的 ADC 梯 → GPIO4`；操作者短按 `ACT_B` 时实板 ADC 约 709 mV、固件事件 `BACK`，与网表一致。修订提交 `89d3a01` 将实验室录音改绑板上 `ACT_B`，由 `aliyun_raynode` 的 ESP-IDF 6.1 构建，镜像 SHA-256 `B9E60839059F9E292B02020284677D73A69F421738F45E9A0115D070BBCF2368`，以 115200 baud 写入 OTA0 并由 esptool 报告哈希校验成功。冷启动日志确认为 `ACT_B` 录音版；两次短按分别触发开始与停止，操作者确认 RGB/五灯阵列先红色常亮、后熄灭；`act-b-001.wav` 关闭成功，`samples=124800 peak=0 wav_closed=1`。**按键与灯效通过短时 L1 验证，麦克风采样仍失败。**本地日志 `.tmp/lab-act-record/serial-act-record.log`。

**引脚活动计数复验：**提交 `7de88d6` 的实验室镜像 SHA-256 `B26D5EA03C88EB3CB1A694D7C19FAD59226EC6253F2672684C9F9FB79864D42A`，由 `aliyun_raynode` 上 ESP-IDF 6.1 编译、14 项主机测试通过，COM5 以 115200 baud 写入 OTA0，esptool 报告写入哈希校验成功。`ACT_B` 两次短按后操作者再次确认红灯正常亮灭，`act-b-001.wav` 关闭成功，`samples=152960 peak=0 wav_closed=1`。录音开启时，ESP32-C6 PCNT 在 **MCU 端引脚** 测得 MCLK/GPIO0：4144 次/1001 µs，BCLK/GPIO3：5122 次/10001 µs，WS/GPIO12：320 次/20001 µs，符合约 4.096 MHz、512 kHz、16 kHz；SDOUT1/GPIO13：**0 次/20001 µs**，同时原始 I²S RX 1024 字节全零。这排除了“MCU 根本没输出三路时钟”的解释，证明 MCU 侧数据引脚没有可见翻转；**尚不能证明这些时钟已到达 U12 焊盘，也不能区分 U12 无输出、R65/布线故障或驱动/供电状态问题**。本地日志 `.tmp/lab-i2s-edges/serial-i2s-edges.log`。

同一启动中，录音已在约 38.7 s 停止，但约 139.7 s 仍出现一次 PCA9557 寄存器写入 `ESP_ERR_INVALID_RESPONSE` 重试；不能把 I²C 间歇异常简单归因于录音。第一段串口日志在约 142.5 s 后没有新增输出，Windows 某次 `Win32_SerialPort` 查询遗漏 COM5；操作者确认未断电、无发热、LED 仍亮。随后 Windows PnP 列表可见 COM5，被动重新打开串口取得约 798–815 s 的日志，固件计时未归零。**不能将端口列表的瞬时缺项等同 MCU 断电或死机**；串口链路曾否短时重连仍不确定。

**无屏幕对照：**同一代码基线的 `hi-yaka-lab-headless`（不启用 ST7789/PARLIO，但仍执行一次 PCA9557 安全态和背光诊断）镜像 SHA-256 `8A140D79FDFF64EEDCB0F0434746C4AB9000881F9C0FB6D0E6036FE2FD1FFBA7` 由 ESP-IDF 6.1 编译，COM5 写入 OTA0 并通过 esptool 哈希校验。重新启动后，PN532 在冷启动扫描、固件版本请求和延时探测中仍不应答，其他四个固定地址应答；IMU 六轴与温度读取成功。240 秒串口观察中未见 PCA9557 重试或总线清除失败，系统内存日志持续。此单次 A/B 对照说明 **PN532 NACK 不依赖 LCD 初始化**；I²C 间歇异常可能与显示流量或其他运行时差异有关，尚需重复实验才能归因。日志 `.tmp/lab-i2s-edges/serial-headless.log`。

**ES7210 内部状态复验：**提交 `3ebf55b` 的实验室有屏幕镜像 SHA-256 `09F22B97522642B52583CF4CD284C19F42788BF87A52BAEF6CC07C1C47FB226A` 由 ESP-IDF 6.1 编译，COM5 写入 OTA0 并通过 esptool 哈希校验。操作者按 `ACT_B` 启停短录音，再次确认红灯正常亮灭；WAV `samples=162560 peak=0 wav_closed=1`。采集中 ES7210 `0x0B=0x02`，按手册低两位代表 **正常状态**；`0x12=0x00` 表示 SDOUT1 未设高阻、标准 I²S；`0x15=0x00` 表示 ADC1/2 未静音；`0x43/0x44=0x18` 选通双麦，`0x4B=0x00` 未关闭其模拟/ADC 路径。MCU 端 MCLK/BCLK/WS 再次约 4.1 MHz/512 kHz/16 kHz，SDOUT1/GPIO13 仍为 0 次上升沿/20 ms，原始 RX 全零。**配置读回削弱“芯片仍处于内部关机或软件静音”假设，但不能证明 U12 所有供电、焊盘时钟和实际输出均正常。**本轮约 22.7 s 出现 PCA9557 一次 `ESP_ERR_TIMEOUT` 重试，约 175.6 s 又有一次 I²C transaction timeout/PCA9557 重试，之后内存日志仍持续。无屏幕 240 秒对照未见这类错误，提高了显示流量相关假设的优先级，但运行时间和操作条件不同，仍不足以判定根因。日志 `.tmp/lab-i2s-edges/serial-regstate.log`。

## PN532：供电拓扑待评估，NACK 根因未定

当前 EasyEDA Pro 活体工程逐页语义回读以及 PCB **U13 焊盘网络**回读一致：U13.40/VBAT=`SYS_3V3`；U13.23/PVDD 与 U13.39/DVDD 同属 `VREF`。`VREF` 只连 U13.23、U13.39、C97/C108/C109/C113 和 L10/L11，并没有主机电源输入。U13.38/RSTPD_N 的 `RES&OFF` 是 R71 10 kΩ 上拉、R75 100 kΩ 下拉；I1:I0 由 R74/R73 设为 `10`，与 PN532/C1 的 I²C 模式相符。

NXP《PN532/C1》§8.4 说明：PVDD 通常接主机电源并负责宿主接口；VBAT 为内部稳压器供电，生成 DVDD。现板把 DVDD 接回 PVDD，并非手册中的推荐供电图。但操作者在 MASTER ON 时测得 C108 非接地端约 **3 V**，`RES&OFF` 也约 **3 V**；即 `VREF`/PVDD/DVDD 网确有电压，RSTPD_N 未被拉在硬复位低电平。不能再据连接图断言 PN532 未供电，也不能把此非标准连接直接判为本次 NACK 的根因。该拓扑仍需硬件评估；历史审计的“NFC 电气闭环”结论缺少实板证据，当前应记为未通过、原因待定。

EDA 原理图 U13.27/P50_SCL=`IO11_I2C_SCL`、U13.28/SDA=`IO10_I2C_SDA`，PCB 两脚网络名一致，各有已布铜线；这排除了显性的 SDA/SCL 对调或焊盘未布线，但不等于证明两脚在实板可靠导通。驱动已发送地址探测唤醒并延时 20 ms，另做了开机与延时探测，仍为 NACK。下一步优先查两脚实板波形、晶振活动、模式绑带实际电压和器件焊接。不要仅凭焊盘网络图短接 `VREF` 到 `SYS_3V3`：DVDD 是稳压器输出，直接并入 3.3 V 可能反向供电。是否修改 PVDD 接法，须在通讯根因明确后制定方案。

## LCD：读回与物理显示的边界

屏幕 FPC1 的 LEDA(5) 接 `SYS_3V3`；LEDK(6) 经 R84=10 Ω 到 Q10 漏极，Q10 源极接地，栅极经 R85 由 PCA9557 IO0/LCD_BL 驱动、R86 下拉。IO0 高电平和寄存器回读成立，但不能证明栅极、电阻、FPC 接点或屏内白光 LED 有电流。原厂背光为三只并联白光 LED，总额定 60 mA，Vf 2.8–3.3 V；3.3 V 供电加 10 Ω 电阻存在亮度裕量不足的已知风险。

操作者实测 R84 两端均约 1.6–1.7 V，对 10 Ω 电阻而言压差接近零，因此背光电流接近零；R85 输入侧和 Q10 栅极侧均约 0 V。进一步交叉核对 NXP 原厂 PCA9557 手册：**IO0（TSSOP pin 6）是高阻开漏输出，只能下拉，不能源出高电平**。当前活体原理图及 PCB 的 `LCD_BL` 只有 U3.IO0、R85=1 kΩ 和 Q10 栅极/R86=10 kΩ 下拉，没有任何上拉。固件把 IO0 锁存位置 1 时，它只是释放这条网；R86 使栅极维持 0 V，Q10 不可能正常导通。这是确定的设计缺陷，与量测完全一致；不是 LCD 初始化序列或屏幕无应答造成。

**当前样机临时验证**可以在断电后，用一颗约 2.2 kΩ 电阻把 `LCD_BL`（R85 的 PCA 侧）上拉到 `SYS_3V3`；高阻态下预计 Q10 栅极约 `3.3×10/(2.2+1+10)=2.50 V`，满足所选 Si2302CDS 在 2.5 V 下的导通规格；IO0 拉低时约 1.5 mA 流经上拉电阻，低于其灌电流能力。**不能用裸导线直接连接 3.3 V**，因为固件安全初始化期间 IO0 会主动拉低。此临时上拉会使 IO0 上电默认高阻阶段的背光短暂自动开启，不能作为量产方案。下一版硬件建议改由空闲的推挽 IO3/IO5 控制 Q10 栅极，保持上电外部下拉和默认关闭；当前板若采用飞线返修，也必须同步修改 PCA 安全输出与方向配置，并验证 IO0 不会与新驱动对拉。背光恢复后再用全屏测试色块核验 LCD 数据与排线方向。不要直接触碰 0.5 mm FPC 相邻脚。

## 麦克风：硬件与时钟分层

活体 CODEC 页显示两颗 ZTS6216 模拟麦的 VDD 经 R42/R45 接 ES7210 `MICBIAS12`；U12 的 VDDP/VDDD 经 R63 供电，VDDA/VDDM 经 R64；U12.11/SDOUT1 经 **已纳入 BOM 的 R65=51 Ω** 到 MCU GPIO13，R66/SDOUT2 备用且 DNP。官方 ES7210 驱动在三路选择时设 `0x12=0x02`（TDM I²S），只选两路时设 `0x12=0x00`（标准 I²S）；两个实板对照版本已分别读回这两个值。

录音版在 `ES7210 0x41=0x70`、输入启用且红灯亮时，操作者报告 R42/R45 一带对地约 2.83 V，说明至少所接触测点不再是先前录音关闭时的 0 V；另一次回复又出现 0 V，未指明 R42/R45 的哪一端、是否在红灯仍亮时，因此**不能**据此认定两个麦克风 VDD 均已确认正常，更不能将偏置作为零音频的排除项。下一轮应在同一录音窗口逐一标记四个焊盘并记录读数；没有安全测点就使用示波器/测试夹具，不要求操作者对微小焊盘飞线。

MCU 侧三路时钟已由 PCNT 证实活动，SDOUT1/GPIO13 无翻转。下一步优先用示波器或逻辑分析仪确认采样期间 U12.5/MCLK、U12.9/BCLK、U12.10/LRCK 和 R65 **两端** 的电平活动；再测 U12 的 VDDP/VDDD、VDDA/VDDM 和 MICBIAS12、两颗麦 VDD。ES7210 的 I²C 可应答只说明控制接口可访问，不能代替全部供电引脚与音频输出确认。若 U12 时钟存在、SDOUT1 仍无翻转，重点查 U12 供电、配置、焊接和芯片输出；若 U12 一侧有数据而 GPIO13/原始 DMA 全零，重点查 R65、布线和 GPIO13 输入。持续全零不是正常的安静环境底噪。

现场目前只有万用表。操作者按要求在 **MASTER OFF、USB/电池完全断开** 后跨 R65 两端测得约 **51 Ω**，R63/R64 两端各约 **0 Ω**，均与活体原理图和器件值一致，三颗电阻本体明显开路的假设被削弱；不再逐颗追查这组电阻。跨件读数不能证明 U12 到 R65 或 R65 到 MCU 焊盘均导通，也不能代替复电时的 U12 供电测量。**万用表直流读数不能替代 512 kHz I²S 数据波形判断，也不能据约 1.6 V 均值宣称有正确数字音频。**后续由能稳定接触 0402/QFN 的维修台测 U12 焊盘时钟、供电和 SDOUT1/R65 两侧波形；当前样机保持断电，待准备好测量条件再恢复验证。

## 参考

- [NXP PN532/C1 数据手册](https://www.nxp.com/docs/en/nxp/data-sheets/PN532_C1.pdf)，§8.3 宿主接口、§8.4.1 电源；
- [NXP PCA9557 数据手册](https://www.nxp.com/docs/en/data-sheet/PCA9557.pdf)，§1、§5.1：IO0 是开漏输出；
- [Vishay Si2302CDS 数据手册](https://www.vishay.com/docs/68645/si2302cds.pdf)，2.5 V 栅压下的导通电阻；
- [Everest ES7210 数据手册](https://files.waveshare.com/wiki/common/ES7210_DS.pdf)，寄存器 `0x01/0x11/0x12/0x41–0x4C`；
- [Espressif ESP32-C6 I²S 驱动文档](https://docs.espressif.com/projects/esp-idf/en/v6.0.3/esp32c6/api-reference/peripherals/i2s.html)，全双工共享 BCLK/WS；
- 当前工程 `managed_components/espressif__esp_codec_dev/device/es7210/es7210.c` 与 `managed_components/jef-sure__pn532/src/pn532-bus-i2c.c`。

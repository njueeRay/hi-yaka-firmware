# HiYaKa 首板 LCD、PN532 与麦克风专项诊断

> 2026-11-04；当前样机已拆 L7、功放 GPIO23 强制低、未接扬声器。结论只适用于这块受控开发样机。板级三项仍为 FAIL，不能关闭 M2/M3/M5。U11 异常发热另见 [`HIYAKA_U11_THERMAL_INCIDENT_AND_RECOVERY.md`](../hardware/HIYAKA_U11_THERMAL_INCIDENT_AND_RECOVERY.md)。

## 已核实的现象

| 链路 | 实板证据 | 当前判定 |
|---|---|---|
| 主程序 | `hi-yaka-lab` 启动，持续输出内存日志；RGB 顺序闪烁、马达短振已由操作者确认；TF 可写 WAV，IMU 返回动态六轴值 | 主循环运行，不代表整机功能正常 |
| LCD | ST7789 写入调用无报错；PCA9557 地址 `0x19` 应答；背光 IO0 在延时测试中由 0 切到 1，回读 Output=`0xC3`（TDM 版）或 `0x83`（标准双麦版），Configuration=`0x38`；操作者确认仍完全无背光；R84 两端对地均约 1.6–1.7 V，R85 两端均约 0 V | NXP PCA9557 的 IO0 是开漏输出；板上 `LCD_BL` 缺上拉、R86 反而下拉。输出锁存为 1 仅使 IO0 高阻，Q10 栅极仍为 0 V。已定位硬件设计遗漏；屏幕控制器无读回线，不能称 LCD 已应答 |
| PN532 | 两次启动扫描均只见 `0x18/0x19/0x41/0x6B`；`0x24` 开机和延时探测均 NACK，固件版本请求失败 | 芯片未进入可通信状态，尚未到标签/天线业务层 |
| 麦克风 | ES7210 `0x41` 应答，寄存器可读；TDM 模式寄存器 `0x12=0x02`，原始 I²S 1024 字节全零，16 万采样全零；改为双麦标准 I²S 后 `0x12=0x00`，原始 I²S 仍全零，8 万采样全零 | 零值不是 WAV 写卡或上层录音格式造成；单独归咎于 TDM 配置的解释已被对照试验削弱 |

日志：本地未纳入 Git 的 `.tmp/lab-diag-current/serial-boot.log` 与 `serial-std.log`。TDM 诊断代码提交 `8d1e5c7`，烧录文件 SHA-256 `A7E7450A8BE4BB652BC62A06C87BA4BE9DA58D0398D582FA2C946C073FF77264`；标准双麦对照代码提交 `62ff9f7`，烧录文件 SHA-256 `9CA6D19700B57F5B7A8BE52DAFBB6ACCEB9CD13A8CF2A5BD7EF484514CFB0AA0`。两版均由 `aliyun_raynode` 上 ESP-IDF v6.1 容器构建，并在 COM5 烧录校验成功。前者改正 `BoxAudioCodec::Read()` 把读取错误伪装为成功的返回值；原始读仍全零，说明该错误不是本次零音频的唯一原因。

## PN532：新的硬件 P0

当前 EasyEDA Pro 活体工程逐页语义回读以及 PCB **U13 焊盘网络**回读一致：U13.40/VBAT=`SYS_3V3`；U13.23/PVDD 与 U13.39/DVDD 同属 `VREF`。`VREF` 只连 U13.23、U13.39、C97/C108/C109/C113 和 L10/L11，并没有主机电源输入。U13.38/RSTPD_N 的 `RES&OFF` 是 R71 10 kΩ 上拉、R75 100 kΩ 下拉；I1:I0 由 R74/R73 设为 `10`，与 PN532/C1 的 I²C 模式相符。

NXP《PN532/C1》§8.4.1 明确：**PVDD 由主机电源供电，内部稳压器产生 DVDD，且其工作条件包括 PVDD 高于 1.6 V**。现有连接把所需的稳压器输出接回其启动所依赖的 PVDD，缺少独立供电。这是可从原理图和 PCB 确定的设计缺陷；它对本样机 NACK 的直接因果仍需量测 `VREF` 电压确认。历史审计把 NFC 标为“电气闭环”，未查出该供电依赖，须以本记录更正。

下一步先测上电时 `VREF` 对 GND 电压（可选 C108 非接地端），以及 `RES&OFF` 对 GND 电压。不要仅凭焊盘网络图短接 `VREF` 到 `SYS_3V3`：DVDD 是稳压器输出，直接并入 3.3 V 可能反向供电。返修应先按 NXP 推荐拓扑分开 PVDD 与 DVDD、确认可切断的铜线和电流，再经限流上电复测。

## LCD：读回与物理显示的边界

屏幕 FPC1 的 LEDA(5) 接 `SYS_3V3`；LEDK(6) 经 R84=10 Ω 到 Q10 漏极，Q10 源极接地，栅极经 R85 由 PCA9557 IO0/LCD_BL 驱动、R86 下拉。IO0 高电平和寄存器回读成立，但不能证明栅极、电阻、FPC 接点或屏内白光 LED 有电流。原厂背光为三只并联白光 LED，总额定 60 mA，Vf 2.8–3.3 V；3.3 V 供电加 10 Ω 电阻存在亮度裕量不足的已知风险。

操作者实测 R84 两端均约 1.6–1.7 V，对 10 Ω 电阻而言压差接近零，因此背光电流接近零；R85 输入侧和 Q10 栅极侧均约 0 V。进一步交叉核对 NXP 原厂 PCA9557 手册：**IO0（TSSOP pin 6）是高阻开漏输出，只能下拉，不能源出高电平**。当前活体原理图及 PCB 的 `LCD_BL` 只有 U3.IO0、R85=1 kΩ 和 Q10 栅极/R86=10 kΩ 下拉，没有任何上拉。固件把 IO0 锁存位置 1 时，它只是释放这条网；R86 使栅极维持 0 V，Q10 不可能正常导通。这是确定的设计缺陷，与量测完全一致；不是 LCD 初始化序列或屏幕无应答造成。

**当前样机临时验证**可以在断电后，用一颗约 2.2 kΩ 电阻把 `LCD_BL`（R85 的 PCA 侧）上拉到 `SYS_3V3`；高阻态下预计 Q10 栅极约 `3.3×10/(2.2+1+10)=2.50 V`，满足所选 Si2302CDS 在 2.5 V 下的导通规格；IO0 拉低时约 1.5 mA 流经上拉电阻，低于其灌电流能力。**不能用裸导线直接连接 3.3 V**，因为固件安全初始化期间 IO0 会主动拉低。此临时上拉会使 IO0 上电默认高阻阶段的背光短暂自动开启，不能作为量产方案。下一版硬件建议改由空闲的推挽 IO3/IO5 控制 Q10 栅极，保持上电外部下拉和默认关闭；当前板若采用飞线返修，也必须同步修改 PCA 安全输出与方向配置，并验证 IO0 不会与新驱动对拉。背光恢复后再用全屏测试色块核验 LCD 数据与排线方向。不要直接触碰 0.5 mm FPC 相邻脚。

## 麦克风：硬件与时钟分层

活体 CODEC 页显示两颗 ZTS6216 模拟麦的 VDD 经 R42/R45 接 ES7210 `MICBIAS12`；U12 的 VDDP/VDDD 经 R63 供电，VDDA/VDDM 经 R64；U12.11/SDOUT1 经 **已纳入 BOM 的 R65=51 Ω** 到 MCU GPIO13，R66/SDOUT2 备用且 DNP。官方 ES7210 驱动在三路选择时设 `0x12=0x02`（TDM I²S），只选两路时设 `0x12=0x00`（标准 I²S）；两个实板对照版本已分别读回这两个值。

下一步优先用示波器或逻辑分析仪确认采样期间 U12.5/MCLK、U12.9/BCLK、U12.10/LRCK 和 R65 两端/MCU GPIO13 的电平活动；再测 U12 模拟电源与 MICBIAS12、两颗麦 VDD。若时钟存在、SDOUT1 始终低，重点查 U12 供电、焊接、R65 和芯片输出；若 SDOUT1 有数据而 GPIO13/原始 DMA 全零，重点查 R65、GPIO13 路由和 I²S 输入配置。持续全零不是正常的安静环境底噪。

## 参考

- [NXP PN532/C1 数据手册](https://www.nxp.com/docs/en/nxp/data-sheets/PN532_C1.pdf)，§8.3 宿主接口、§8.4.1 电源；
- [NXP PCA9557 数据手册](https://www.nxp.com/docs/en/data-sheet/PCA9557.pdf)，§1、§5.1：IO0 是开漏输出；
- [Vishay Si2302CDS 数据手册](https://www.vishay.com/docs/68645/si2302cds.pdf)，2.5 V 栅压下的导通电阻；
- [Everest ES7210 数据手册](https://files.waveshare.com/wiki/common/ES7210_DS.pdf)，寄存器 `0x01/0x11/0x12/0x41–0x4C`；
- [Espressif ESP32-C6 I²S 驱动文档](https://docs.espressif.com/projects/esp-idf/en/v6.0.3/esp32c6/api-reference/peripherals/i2s.html)，全双工共享 BCLK/WS；
- 当前工程 `managed_components/espressif__esp_codec_dev/device/es7210/es7210.c` 与 `managed_components/jef-sure__pn532/src/pn532-bus-i2c.c`。

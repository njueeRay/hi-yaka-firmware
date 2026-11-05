# 40 验证与证据

## 证据等级

- L0：静态检查、host tests、可复现构建和产物哈希；
- L1：目标板启动、总线/器件探测和基础驱动日志；
- L2：功能链路、故障注入和重复性实板记录；
- L3：整机场景、功耗/声学/温升、长稳和发布证据。

结论不得高于证据等级。当前软件基线完成 L0；实板已取得 U11/BOOST 异常发热的 `FAIL` 证据。拆除 L7 后，RGB、ACT_B 录音灯开关、短振动、TF 基础写入及 IMU 基础六轴/温度读数具有单项 L1 证据；IMU 校准和长稳尚未验证。LCD 完全无背光，PN532 对 `0x24` NACK，麦克风无有效数据。2026-11-05 实板 A/B 证明 ESP32-C6 I²S 内部回环可读已知数据；ES7210 单 RX 与独立双工的 GPIO13 数据码值由内部弱上下拉决定，PCNT 无数据边沿，MCU 侧 MCLK/BCLK/WS 正常。当前样机的有效主动数据输出仍为 `FAIL`，不能把 MCU 端观测等同于 U12 引脚状态。ES7210 状态在不同条件下出现 0/2 的冲突；R65 实测约 51 Ω，R63/R64 实测约 0 Ω，仅证明电阻本体与设计值一致。各项仍未形成可关闭 M1–M7 的完整 L1/L2 证据。三项缺陷各自的入口为 `docs/bringup/HIYAKA_LCD_BACKLIGHT_INCIDENT.md`、`docs/bringup/HIYAKA_NFC_NO_ACK_INCIDENT.md`、`docs/bringup/HIYAKA_MICROPHONE_DATA_PATH_INCIDENT.md`；原始排查及麦克风 A/B 结果见 `docs/bringup/2026-11-04-lcd-nfc-mic-investigation.md`、`docs/bringup/2026-11-05-audio-ab-results.md`。

## 最小记录字段

日期、操作者、Git SHA、上游 SHA、SDK/镜像 digest、硬件 revision/样机编号、配置、步骤、原始日志路径、产物 SHA-256、期望/实测、结论、缺陷 ID、复测条件。

资源门禁至少记录内部堆最小值、最大连续块、任务栈高水位、队列峰值、音频丢帧和 UI 刷新数据。发布结论只允许 `GO / CONDITIONAL GO / NO-GO`，并列明跳过或降级检查。

## 首板证据工具

首板和后续实板测试先读 `docs/bringup/README.md`、`FIRST_BOARD_VALIDATION.md`、`MODULE_TEST_MATRIX.md` 与 `EVIDENCE_LEVEL_MATRIX.md`。使用 `scripts/hi_yaka_evidence.py` 创建证据包，把样机编号、硬件 revision、产品/上游 SHA、构建容器 digest 和烧录产物 SHA-256 固定在 `run.json`。

模块记录只能从 `NOT_RUN` 写入一次；`PASS` 必须引用当前 run 内非空原始证据，`FAIL/BLOCKED` 必须绑定缺陷或阻塞项 ID。复测创建新 run，不覆盖旧结论。工具的 `valid: true` 只说明结构和引用完整，不自动产生功能通过或发布 `GO`。

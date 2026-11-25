# 40 验证与证据

## 证据等级

- L0：静态检查、host tests、可复现构建和产物哈希；
- L1：目标板启动、总线/器件探测和基础驱动日志；
- L2：功能链路、故障注入和重复性实板记录；
- L3：整机场景、功耗/声学/温升、长稳和发布证据。

结论不得高于证据等级。软件构建属于 L0；实板曾取得 U11/BOOST 异常发热的 `FAIL` 证据。功放隔离条件下，RGB、ACT_B 录音灯、短振动、TF 基础写入及 IMU 基础六轴/温度读数有单项 L1 证据；IMU 校准和长稳未验。LCD 的原始无背光设计缺陷已定位，**一块样机飞线后背光亮，低速 GPIO 横屏曾显示颜色与文字且雪花带消失**；但同镜像首次只有背光、复位后有画面，PCA9557 重试仍多，显示稳定性和全功能验收继续 `FAIL`，不能再写“LCD 完全无背光”。PN532 原板 `0x24` 仍 NACK，见独立专项。

麦克风截至 2026-11-21 已完成 C1–C4 启动控制面及 D1–D5 自动录音对照：C4 五次均构造音频设备，不等于录音通过；D1、D2、D4、D5 的录音窗口中双声道 PCM 固定全 `-1`，GPIO13 无有效数据边沿。D3 弱下拉双路全 0、弱上拉双路全 `-1`，高置信仅证明 MCU GPIO13 端缺有效强驱动；D4 的真实身份值窗口支持录音期 ES7210 `0x0B=0x00`，D5 数字静音输出前序仍为阴性。MCU 侧时钟不等于 U12 焊盘时钟；U12 本体与 R65/走线尚未分离，麦克风保持 `FAIL / NO-GO`。下一步转实体供电、时钟和 SDOUT 分段测量，具体一次性清单与原始日志/镜像哈希以 `docs/bringup/HIYAKA_MICROPHONE_DATA_PATH_INCIDENT.md` 为准。

各项仍未形成关闭 M1–M7 的完整 L1/L2 证据。LCD、NFC、麦克风权威专项入口分别为 `docs/bringup/HIYAKA_LCD_BACKLIGHT_INCIDENT.md`、`docs/bringup/HIYAKA_NFC_NO_ACK_INCIDENT.md`、`docs/bringup/HIYAKA_MICROPHONE_DATA_PATH_INCIDENT.md`；早期原始排查见 `docs/bringup/2026-11-04-lcd-nfc-mic-investigation.md`、`docs/bringup/2026-11-05-audio-ab-results.md`，其历史状态不得覆盖上述当前结论。

## 最小记录字段

日期、操作者、Git SHA、上游 SHA、SDK/镜像 digest、硬件 revision/样机编号、配置、步骤、原始日志路径、产物 SHA-256、期望/实测、结论、缺陷 ID、复测条件。

资源门禁至少记录内部堆最小值、最大连续块、任务栈高水位、队列峰值、音频丢帧和 UI 刷新数据。发布结论只允许 `GO / CONDITIONAL GO / NO-GO`，并列明跳过或降级检查。

## 首板证据工具

首板和后续实板测试先读 `docs/bringup/README.md`、`FIRST_BOARD_VALIDATION.md`、`MODULE_TEST_MATRIX.md` 与 `EVIDENCE_LEVEL_MATRIX.md`。使用 `scripts/hi_yaka_evidence.py` 创建证据包，把样机编号、硬件 revision、产品/上游 SHA、构建容器 digest 和烧录产物 SHA-256 固定在 `run.json`。

模块记录只能从 `NOT_RUN` 写入一次；`PASS` 必须引用当前 run 内非空原始证据，`FAIL/BLOCKED` 必须绑定缺陷或阻塞项 ID。复测创建新 run，不覆盖旧结论。工具的 `valid: true` 只说明结构和引用完整，不自动产生功能通过或发布 `GO`。

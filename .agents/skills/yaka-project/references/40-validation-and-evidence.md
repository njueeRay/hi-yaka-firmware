# 40 验证与证据

## 证据等级

- L0：静态检查、host tests、可复现构建和产物哈希；
- L1：目标板启动、总线/器件探测和基础驱动日志；
- L2：功能链路、故障注入和重复性实板记录；
- L3：整机场景、功耗/声学/温升、长稳和发布证据。

结论不得高于证据等级。当前 M1 完成 L0，首板尚未接入，因此不宣称硬件适配完成。

## 最小记录字段

日期、操作者、Git SHA、上游 SHA、SDK/镜像 digest、硬件 revision/样机编号、配置、步骤、原始日志路径、产物 SHA-256、期望/实测、结论、缺陷 ID、复测条件。

资源门禁至少记录内部堆最小值、最大连续块、任务栈高水位、队列峰值、音频丢帧和 UI 刷新数据。发布结论只允许 `GO / CONDITIONAL GO / NO-GO`，并列明跳过或降级检查。

## 首板证据工具

首板和后续实板测试先读 `docs/bringup/README.md`、`FIRST_BOARD_VALIDATION.md`、`MODULE_TEST_MATRIX.md` 与 `EVIDENCE_LEVEL_MATRIX.md`。使用 `scripts/hi_yaka_evidence.py` 创建证据包，把样机编号、硬件 revision、产品/上游 SHA、构建容器 digest 和烧录产物 SHA-256 固定在 `run.json`。

模块记录只能从 `NOT_RUN` 写入一次；`PASS` 必须引用当前 run 内非空原始证据，`FAIL/BLOCKED` 必须绑定缺陷或阻塞项 ID。复测创建新 run，不覆盖旧结论。工具的 `valid: true` 只说明结构和引用完整，不自动产生功能通过或发布 `GO`。

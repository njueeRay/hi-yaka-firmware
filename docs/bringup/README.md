# HiYaKa 首板验证与证据包

> 状态：PH5 软件准备基线
> 适用对象：ChatChat HiYaKa / ESP32-C6
> 证据边界：本目录提供操作顺序、判定门槛和记录工具，不代表任何实板项目已经通过

## 1. 目的

首板验证必须同时回答三个问题：烧录的究竟是哪一个候选产物、测试针对哪一块硬件、结论能否由原始证据复核。只保存一张照片、一次口头确认或一段被截断的串口输出，不构成可追溯结论。

本验证包包含：

- [首板上电与基础验证手册](FIRST_BOARD_VALIDATION.md)：限流上电、下载、串口、Flash、I²C 和 PCA9557；
- [模块测试矩阵](MODULE_TEST_MATRIX.md)：显示、输入、音频、TF、NFC、IMU、反馈、电量、网络、OTA 和长稳；
- [证据等级与里程碑矩阵](EVIDENCE_LEVEL_MATRIX.md)：L0–L3 与 M1–M7 的关闭条件；
- [实板前适配状态与首板启用顺序](PREBOARD_ADAPTER_STATUS.md)：安全构建、实验构建、M2–M5 L0 适配与逐项启用顺序；
- `scripts/hi_yaka_evidence.py`：生成、记录和校验样机证据包。

## 2. 不可跳过的身份绑定

创建证据包前，必须从 CI 候选产物及其 provenance 中取得：

1. 产品仓库源提交 SHA；
2. 上游小智提交 SHA；
3. ESP-IDF 版本和构建容器 digest；
4. 烧录产物文件名和 SHA-256；
5. 样机编号、硬件 revision、操作者和实验环境。

缺少任一项时可以做故障探索，但不得形成 `PASS` 或里程碑关闭结论。

## 3. 创建证据包

证据目录应位于实验室受控存储，不要把用户录音、长视频或大体积原始日志直接提交到 Git。

```powershell
python scripts/hi_yaka_evidence.py init `
  --output-root D:\lab-evidence\hi-yaka `
  --sample-id HYK-001 `
  --hardware-revision R1.6 `
  --firmware-sha <40位产品提交SHA> `
  --upstream-sha <40位上游提交SHA> `
  --firmware-artifact-name merged-binary.bin `
  --firmware-artifact-sha256 <64位SHA-256> `
  --idf-version "ESP-IDF v6.1" `
  --container-digest sha256:<64位digest> `
  --operator <操作者> `
  --environment "实验台/温度/供电与仪表"
```

工具创建 15 个模块记录、串口日志入口、电源测量 CSV、媒体目录和缺陷索引。初始状态一律为 `UNVERIFIED/NOT_RUN`。

## 4. 记录和校验

原始证据必须先放入本次 run 目录，再写结论。例如：

```powershell
python scripts/hi_yaka_evidence.py record `
  --run D:\lab-evidence\hi-yaka\<run-id> `
  --module power `
  --result PASS `
  --actual "3.3 V 轨实测 3.30 V，三次冷启动无持续限流" `
  --raw-evidence measurements/power.csv `
  --raw-evidence photos/power_cold_boot.mp4 `
  --tester <操作者>

python scripts/hi_yaka_evidence.py validate `
  --run D:\lab-evidence\hi-yaka\<run-id> `
  --require-module power `
  --require-module flash `
  --require-module i2c `
  --require-module pca9557
```

规则是确定性的：

- `PASS` 必须有实测描述、测试人、时间和至少一个非空原始证据文件；
- `FAIL` 还必须绑定缺陷 ID；
- `BLOCKED` 必须绑定缺陷或阻塞项 ID；
- `NOT_RUN` 不算通过；
- 原始证据只能引用当前 run 内的相对路径，不能逃逸到其他目录；
- 模块结果只能从 `NOT_RUN` 写入一次，工具拒绝覆盖已经记录的 `PASS/FAIL/BLOCKED`；
- 修改旧证据不等于复测，复测必须创建新 run 并保留旧版本。

## 5. 阶段关闭

工具的 `valid: true` 只表示记录结构和引用完整，不自动证明硬件功能正确。里程碑关闭仍须由对应矩阵规定的 L1/L2/L3 证据、缺陷状态和人工复核共同决定。

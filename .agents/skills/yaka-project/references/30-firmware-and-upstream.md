# 30 固件与上游

## 固定基线

- 产品仓库：`hi-yaka-firmware`；
- 上游：`78/xiaozhi-esp32`；
- 当前基线：tag `v2.5.2` / commit `49b4380`；
- ESP-IDF：`v6.1`；
- board：`chatchat/hi-yaka-v3`，ESP32-C6、DIO、16 MiB、无 PSRAM。

## 架构边界

复用小智构建、Wi-Fi、协议、AudioService、Opus、LVGL、NVS 和 OTA transport。YaKa 自行维护 board/BSP、capability health、产品状态协调、本地录音、TF 恢复、实体输入、NFC 业务、反馈、电源边界与 OTA 产品策略。

完整设计读 `docs/yaka/firmware-architecture.md`。所有产品状态只有一个协调者；ISR、驱动和后台任务只发送有界事件。I²S RX 只有一个所有者，本地录音在采集适配层分流。

## 上游同步

产品开发固定 release/tag，不追上游 `main`。`origin` 指向产品私有仓库，`upstream` 只拉取；同步在 `sync/xiaozhi-vX.Y.Z` 分支单独 PR，不混入产品功能。细节见 `UPSTREAM.md`。

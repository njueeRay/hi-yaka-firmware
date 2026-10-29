# 30 固件与上游

## 固定基线

- 产品仓库：`hi-yaka-firmware`；
- 上游：`78/xiaozhi-esp32`；
- 当前基线：tag `v2.5.2` / commit `49b4380`；
- ESP-IDF：`v6.1`；
- board：`chatchat/hi-yaka-v3`，ESP32-C6、DIO、16 MiB、无 PSRAM。
- 公开发布：`ChatChatTech/yaka/firmware`，只接收经过凭据、许可证和内部资料审查的快照，不承载私有开发历史。

## 架构边界

复用小智构建、Wi-Fi、协议、AudioService、Opus、LVGL、NVS 和 OTA transport。YaKa 自行维护 board/BSP、capability health、产品状态协调、本地录音、TF 恢复、实体输入、NFC 业务、反馈、电源边界与 OTA 产品策略。

完整设计读 `docs/yaka/firmware-architecture.md`。所有产品状态只有一个协调者；ISR、驱动和后台任务只发送有界事件。I²S RX 只有一个所有者，本地录音在采集适配层分流。

## 上游同步

产品开发固定 release/tag，不追上游 `main`。`origin` 指向产品私有仓库，`upstream` 只拉取；同步在 `sync/xiaozhi-vX.Y.Z` 分支单独 PR，不混入产品功能。细节见 `UPSTREAM.md`。

公开发布不得把私有仓库设置为 subtree 或 mirror。每次快照记录私有来源 SHA、上游 SHA、SDK、验证等级和排除范围；应用代码位于公开仓库其他顶层目录，不与固件共用内部 PRD。

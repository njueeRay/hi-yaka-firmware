# YaKa 3.0 固件 M1 基线

## 1. 版本契约

| 对象 | 固定值 |
|---|---|
| 产品仓库 | `njueeRay/hi-yaka-firmware`，private |
| 上游 | `78/xiaozhi-esp32` |
| 上游发布 | `v2.5.2` |
| 上游提交 | `49b4380b3faa0ceadd28a3e78aaf0f311c46c57e` |
| ESP-IDF | `v6.1` |
| 构建镜像 | `espressif/idf:v6.1@sha256:81893c71bb5e570088901f21def8684c25cd2a9020281bd01b843a7655edb18c` |
| target | `esp32c6` |
| board type/name | `hi-yaka-v3` |
| manufacturer | `chatchat` |
| Flash | 16 MiB，M1 使用 DIO |
| PSRAM | 不支持、禁用 |

## 2. M1 目标

M1 只建立正式固件仓库和可运行的板级最小入口：

1. 固定上游和 ESP-IDF 版本；
2. 建立唯一 YaKa 板型与构建选择链；
3. 输出复位原因、Flash ID/容量和内部堆信息；
4. 初始化共享 I2C 并探测五个固定地址；
5. 按“先安全值、后输出方向”的顺序初始化 PCA9557；
6. 在远程 Linux/Raynode 完成构建并记录制品；
7. 保持 LCD、存储、音频和产品行为未启用，避免把编译成功误报为实板通过。

## 3. 无 PSRAM 约束

ESP32-C6 不支持外部 PSRAM，因此该 target 的 Kconfig 不提供可启用的 `CONFIG_SPIRAM` 选项。M1 不伪造这个无效配置，而是同时以 ESP32-C6 target 约束和板级编译期防护保证无 PSRAM。后续阶段必须：

- 使用 LVGL 局部行缓冲，禁止全屏双缓冲；
- 音频和存储采用固定容量队列与流式处理；
- 记录内部堆空闲量和最大连续块；
- 对 Wi-Fi、屏幕、音频和 TF 并发执行压力测试；
- 不把 S3/P4 的 AFE/FD AEC 能力默认为 C6 已具备。

## 4. 分支与上游同步

- `origin`：私有产品仓库；
- `upstream`：`78/xiaozhi-esp32`，只拉取；
- `main`：受保护的可构建产品主线；
- 功能工作使用 `feature/*`；
- 上游同步使用 `sync/xiaozhi-vX.Y.Z`，通过独立 PR 合入；
- 上游版本升级使用 merge commit，产品功能 PR 默认 squash；
- 不在同步 PR 中混入产品功能修改。

## 5. 验收边界

| 检查 | M1 结果含义 |
|---|---|
| host tests | 板型选择链和静态硬件契约未漂移 |
| IDF target build | ESP32-C6/IDF 6.1 编译和链接成功 |
| merged binary | 可供后续烧录，但未证明硬件可工作 |
| 实板 UART 日志 | 尚需连接首板验证 |
| Flash ID/全片读写 | 尚需首板验证 |
| I2C 地址和 PCA9557 readback | 尚需首板验证 |

M1 的阶段结论只能是“软件基线可进入首板 bring-up”，不能写成“硬件适配完成”。

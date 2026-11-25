# 30 固件与上游

## 固定基线

- 产品仓库：`hi-yaka-firmware`；
- 上游：`78/xiaozhi-esp32`；
- 当前基线：tag `v2.5.2` / commit `49b4380`；
- ESP-IDF：`v6.1`；
- board 对外语义名：ChatChat HiYaKa；构建标识：`chatchat/hi-yaka`，ESP32-C6、DIO、16 MiB、无 PSRAM。
- 公开发布：`ChatChatTech/yaka/firmware`，只接收经过凭据、许可证和内部资料审查的快照，不承载私有开发历史。

## 命名与实现标识

| 用途 | 统一值 | 规则 |
|---|---|---|
| 对外/语义板名 | `HiYaKa` | 首次需要表达归属时写“ChatChat HiYaKa”；不追加代际或版本后缀 |
| 全小写短横线形式 | `hi-yaka` | 用于新建的仓库、构建名或其他 kebab-case 标识；不追加版本后缀 |
| 全大写下划线形式 | `HI_YAKA` | 用于新建的宏、常量前缀或其他 UPPER_SNAKE_CASE 标识 |
| 产品/系列语义 | `HiYaKa` | 不追加代际或版本后缀 |
| board/build ID | `chatchat/hi-yaka`、`hi-yaka` | 用于构建命令、配置和代码追踪 |
| 类/宏/文件 | `HiYaKaBoard`、`BOARD_TYPE_HI_YAKA`、`hi_yaka_board.cc` | 代码、测试、CI 与架构图保持一致 |

页面、日志、诊断字段、发布标题和用户可见字符串只能使用 HiYaKa；机器标识按 `hi-yaka` / `HI_YAKA` 选取相应形式。内容和路径均不得残留代际后缀。

## 架构边界

复用小智构建、Wi-Fi、协议、AudioService、Opus、LVGL、NVS 和 OTA transport。HiYaKa 自行维护 board/BSP、capability health、产品状态协调、本地录音、TF 恢复、实体输入、NFC 业务、反馈、电源边界与 OTA 产品策略。

完整设计读 `docs/yaka/firmware-architecture.md`。所有产品状态只有一个协调者；ISR、驱动和后台任务只发送有界事件。I²S RX 只有一个所有者，本地录音在采集适配层分流。

## 当前实施状态（2026-11-04）

- `hi-yaka` 是首板安全构建，只启用启动诊断、I²C 探测和 PCA9557 安全态；
- `hi-yaka-lab` 编译 M2–M5 的显示、输入、反馈、电池、音频、TF/FatFs 录音、产品运行时，以及 PN532/QMI8658A L0 驱动；
- ESP32-C6 只有一个应用侧通用 SPI 控制器，资源合同固定为 TF 独占 SPI2、LCD 使用 PARLIO 候选实现；
- 功放始终软件硬禁用；实验室构建可把 5 秒麦克风采集和最多 30 秒首次数字解码输出保存为标准 PCM WAV，以便在扬声器不可用时离线核验数字音频链路；
- 实板已验证 ACT_B 录音灯开关、RGB、短振动、TF 基础写入和 QMI8658A 六轴/温度基础读数；这些是相应单项 L1 证据，不代表整项里程碑通过。IMU 坐标、零偏和稳定性仍待验证。
- ES7210 的寄存器读回显示正常工作态、I²S 格式、ADC12 未静音和 MIC1/MIC2 路由；ESP32-C6 输出 MCLK/BCLK/WS，在 MCU 的 GPIO13 输入端未计到 SDOUT1 跳变，原始 DMA 与录音 WAV 全零。时钟尚未在 U12 焊盘核验，也未直接观测 U12 的 SDOUT1 引脚，不能判定芯片内部或板级线路哪一段失效。
- LCD 背光因 PCA9557 IO0 开漏、LCD_BL 缺上拉而无法由现有固件点亮；PN532 在无屏构建仍对 `0x24` NACK。功放继续硬禁用，产品 Release 暂不可创建。

## 执行环境边界

- Windows 本地机只承担 USB/串口、烧录、实板调试和日志采集，不作为编译或全仓测试环境；
- ESP-IDF 6.1 实际构建、host tests、板级/产品合同、资源门禁和候选产物在 `aliyun_raynode` 或 GitHub Actions 的 Linux 环境执行；
- Windows 上的编码、临时目录文件锁、编译器或 sanitizer 依赖差异不进入 HiYaKa 缺陷结论，除非它直接影响烧录、串口或实板调试链路。

## 上游同步

产品开发固定 release/tag，不追上游 `main`。`origin` 指向产品私有仓库，`upstream` 只拉取；同步在 `sync/xiaozhi-vX.Y.Z` 分支单独 PR，不混入产品功能。细节见 `UPSTREAM.md`。

公开发布不得把私有仓库设置为 subtree 或 mirror。每次快照记录私有来源 SHA、上游 SHA、SDK、验证等级和排除范围；应用代码位于公开仓库其他顶层目录，不与固件共用内部 PRD。

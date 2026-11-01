# HiYaKa OTA、资源预算与构建追溯合同

> 状态：PH4 L0 合同实现；真实掉电、首启确认和回滚等待首板 L1/L2 证据。
> 固定环境：xiaozhi-esp32 `v2.5.2 / 49b4380`、ESP-IDF 6.1、ESP32-C6、16 MiB Flash、无 PSRAM。

## 1. 结论边界

PH4 建立的不是“OTA 已在产品上通过”，而是以下可重复检查的工程合同：

- HiYaKa 独占分区表，双 OTA 槽严格等长；
- 候选包必须匹配板型、硬件修订范围、最低已安装版本和严格递增的语义版本；
- 摘要格式与签名验证接口是强制项，摘要不等于身份认证；生产签名密钥尚未生成或烧录；
- ESP-IDF 6.1 回滚开启，新固件只有在板级安全态和核心服务初始化成功后才确认；确认不依赖网络或云端；
- 配置迁移以快照—应用—校验—提交执行，任何一步失败都恢复旧配置；迁移接口没有录音存储权限；
- CI 超过 Flash/内部 RAM/队列/新增任务预算会失败；每个候选产物同时归档来源、上游、SDK、配置、分区和 SHA-256；
- 上述结论均为 L0。真实电源中断、Flash 磨损、TF 文件系统、射频和量产密钥属于实板或生产验证。

## 2. 分区与容量

唯一产品分区表为 [`partitions/hi-yaka-16m.csv`](../../partitions/hi-yaka-16m.csv)。

| 区域 | 起始地址 | 大小 | 用途 |
|---|---:|---:|---|
| NVS | `0x9000` | 16 KiB | 配置与系统键值 |
| OTA data | `0xD000` | 8 KiB | 双扇区 OTA 选择状态 |
| PHY init | `0xF000` | 4 KiB | PHY 初始化数据 |
| OTA 0 | `0x20000` | 3.9375 MiB | 当前或候选应用 |
| OTA 1 | `0x410000` | 3.9375 MiB | 当前或候选应用 |
| assets | `0x800000` | 3.90625 MiB | 资源分区 |

每个应用槽为 `0x3F0000`（4,128,768 字节）。CI 将应用镜像上限设为 3,866,624 字节，固定保留 256 KiB 余量。Flash 尾部 4,292,608 字节暂不分配；这不是可随意挪用的“空闲容量”，修改前必须重新评估 ESP32-C6 映射限制、资源升级策略和回滚对称性。

## 3. 候选包合同

[`main/yaka/update_assurance.*`](../../main/yaka/update_assurance.h) 在不依赖网络栈的纯 C++ 层验证：

1. `board == hi-yaka`；
2. 候选版本、当前版本、最低兼容版本都是三段式语义版本；
3. 候选版本必须高于当前版本，当前版本不得低于包声明的最低版本；
4. 当前硬件修订号落在包声明范围内；
5. 镜像非空且不超过冻结上限；
6. SHA-256 使用 64 位小写十六进制；
7. 签名字段非空且验证器返回成功。

`PackageSignatureVerifier` 是生产签名接入点。当前宿主测试只使用可控假验证器验证拒绝路径，不能据此宣称已有量产签名。正式发布前必须确定密钥托管、轮换、吊销和烧录方案，并用 ESP-IDF 的签名镜像能力完成 L2/L3 验证。

## 4. 回滚确认点

ESP-IDF 6.1 使用 `CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE=y`。新 OTA 应用首启处于 `ESP_OTA_IMG_PENDING_VERIFY`：

1. 板对象先建立 I²C，并把 PCA9557 输出锁存器写入安全值，再改变方向；
2. 显示抽象和音频服务完成初始化；
3. HiYaKa 自检确认 I²C 总线存在、PCA9557 安全态已回读、内部最大连续堆块至少 16 KiB；
4. 成功后调用 `esp_ota_mark_app_valid_cancel_rollback()`；
5. 自检失败时调用 `esp_ota_mark_app_invalid_rollback_and_reboot()`；
6. 网络连接、云端版本检查、激活和非关键外设不进入确认条件，避免“离线即回滚”。

这符合 ESP-IDF 6.1 的显式应用确认模型。较新 ESP-IDF 主线新增了不同 Kconfig 名称和自动/应用确认选项，不能复制到本项目的 6.1 基线。

## 5. 故障状态与配置迁移

纯逻辑状态机覆盖：清单拒绝、下载中、摘要失败、签名失败、空间不足、网络中断、配置迁移失败、等待重启、等待首启确认、自检失败、确认超时、确认成功和请求回滚。

配置迁移事务只允许访问 `ConfigurationMigrationStore`。宿主测试先提交一段录音，再注入配置校验失败，确认旧配置被恢复且已提交录音的文件和字节数不变。这个结果证明接口隔离与宿主逻辑成立，不证明 TF/FAT 在掉电时安全。

## 6. CI 门禁与候选产物

策略真值位于 [`config/hi-yaka-assurance.json`](../../config/hi-yaka-assurance.json)，执行器为 [`scripts/hi_yaka_assurance.py`](../../scripts/hi_yaka_assurance.py)。CI 执行：

- 分区无重叠、双槽对称、资源区位置和总容量检查；
- 应用镜像硬上限；
- ESP-IDF size JSON 中每个内部 RAM 区域使用率不超过 90%；
- 产品事件队列 ≤ 32、音频描述符队列 ≤ 12、存储写队列 ≤ 8；
- `main/yaka` 不得私自新增 FreeRTOS task；
- 生成 `hi-yaka-resource-report.json`；
- 生成 `hi-yaka-provenance.json`，记录源码 SHA、固定上游 SHA、ESP-IDF 版本、CI run、分区和全部关键文件 SHA-256；
- 生成 `SHA256SUMS` 并与应用、合并镜像、size 报告、资源报告、追溯清单、sdkconfig、分区表共同归档。

90% RAM 门槛是工程阻断线，不是性能通过线。首板到位后要根据峰值堆、任务栈高水位、音频并发、弱网重传和长录音实测收紧预算。

## 7. 证据与外部依据

- [ESP-IDF 6.1 OTA 与回滚](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/api-reference/system/ota.html)：双扇区 OTA data、`PENDING_VERIFY`、显式确认和失败回滚语义；
- [ESP-IDF 6.1 HTTPS OTA](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/api-reference/system/esp_https_ota.html)：HTTPS OTA 与签名验证能力；
- [ESP-IDF Secure Boot v2](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/security/secure-boot-v2.html)：签名镜像、密钥摘要和 OTA 更新安全边界；
- [Espressif OTA 示例索引](https://github.com/espressif/esp-idf/blob/v6.1/examples/system/ota/README.md)：官方示例和验证路径。

## 8. PH5 必须补充的实板证据

- 候选镜像第一次启动时的 OTA state、确认点和日志；
- 在下载、写入、切换和未确认首启阶段分别断电；
- 坏摘要、坏签名、错板型、错硬件修订和容量超限拒绝；
- 自检失败回滚与无可回滚镜像的安全终态；
- 配置迁移失败后旧配置和已提交 TF 录音均可读；
- 长录音、弱网和音频播放并发时的 heap、栈高水位和 watchdog 记录。

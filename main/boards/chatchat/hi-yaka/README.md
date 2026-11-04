# ChatChat HiYaKa 板型

状态：`M1 安全基线 + M2–M5 实板前 L0 适配`。本目录不是完整产品固件，也不代表任何外设已经通过实板验证。

命名说明：板子的对外名称统一为 HiYaKa，所属组织为 ChatChat；全小写短横线形式用 `hi-yaka`，全大写下划线形式用 `HI_YAKA`。

## M1 已接入

- ESP32-C6，无 PSRAM；
- 16 MiB Flash 构建配置，首板使用 DIO；
- UART0/ROM 与 ESP-IDF 启动日志；
- 复位原因、Flash ID/容量和内部堆基线日志；
- I2C0：SDA=GPIO10、SCL=GPIO11、100 kHz；
- 固定地址探测：ES8311 `0x18`、PCA9557 `0x19`、PN532 `0x24`、ES7210 `0x41`、QMI8658A `0x6B`；
- PCA9557 先写安全输出锁存值，再切换方向，并读回校验。

## PCA9557 M1 安全态

| IO | 当前用途 | M1 方向 | M1 锁存值 |
|---:|---|---|---:|
| 0 | LCD_BL | 输出 | 0，背光关闭 |
| 1 | LCD_RST | 输出 | 0，LCD 保持复位 |
| 2 | MOTOR_CTRL | 输出 | 0，马达关闭 |
| 3 | 空闲 | 输入 | 不驱动 |
| 4 | PWR_EN/历史软电源链路 | 输入 | 不驱动 |
| 5 | 空闲 | 输入 | 不驱动 |
| 6 | LCD_CS | 输出 | 1，片选无效 |
| 7 | LCD_DC | 输出 | 0 |

对应寄存器：Output Port=`0x40`，Configuration=`0x38`。这些值来自 2026-10-21 最终硬件复审；如果活体原理图再次修改，必须重新审计后再改代码。

## 实板前 L0 适配

- `hi-yaka`：保持上述 M1 安全基线，所有未验证外设默认关闭；
- `hi-yaka-lab`：编译 ST7789/PARLIO、ADC/GPIO 输入、电池、RGB/灯阵/马达、ES7210/ES8311、TF/FatFs 和产品运行时；
- LCD 使用 PARLIO GPIO6/7，TF 独占 SPI2 GPIO15/18/19/20，避免争用 ESP32-C6 唯一用户 SPI 控制器；
- 本地录音复用 AudioService 单一 I²S RX 所有者，经容量 12 的有界队列交给独立写卡任务；
- PN532 已具备固件版本校验、SAM 初始化、有界 ISO14443A 轮询、UID/ATQA/SAK
  读取、超时分类和 RF 关闭；日志仅输出 UID 指纹，不输出完整 UID；
- QMI8658A 已具备身份校验、软复位、约 112 Hz 六轴初始化、数据就绪轮询、
  温度/加速度/角速度读取与物理量换算；板级轴向仍须通过六面实测冻结；
- 两者都没有硬件 IRQ；PN532 也没有 MCU 可控复位，因此不声明深睡唤醒或
  独立硬复位恢复能力。具体 NFC 业务映射仍属于 M6 产品决策；
- TF 卡可用时，上电诊断会生成 `diagnostics/microphone-capture.wav`（5 秒、
  16-bit PCM）；首次数字播放另存 `diagnostics/decoded-output.wav`（最多 30 秒）。
  这两份文件用于在功放断电时离线核验采集、解码和 I²S 数字链路；
- 全部参数仍须按 `docs/bringup/PREBOARD_ADAPTER_STATUS.md` 逐项取得 L1/L2 证据。

## 构建

```bash
python3 scripts/build.py chatchat/hi-yaka \
  --name hi-yaka \
  --language zh-CN \
  --wake-word disabled
```

构建成功只证明软件链接和配置链成立，不证明 PUYA Flash、I2C 地址或安全电平已经通过实板验证。

全适配编译使用 `--name hi-yaka-lab`。该产物只用于 CI 和受控实验室调试，不应跳过 M1 直接烧录到未知状态首板。

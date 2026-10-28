# Hi YaKa V3 板型

状态：`M1 最小板级基线`。本目录不是完整产品固件，也不代表实板外设已经通过。

## M1 已接入

- ESP32-C6，无 PSRAM；
- 16 MiB Flash 构建配置，首板使用 DIO；
- UART0/ROM 与 ESP-IDF 启动日志；
- 复位原因、Flash ID/容量和内部堆基线日志；
- I2C0：SDA=GPIO10、SCL=GPIO11、100 kHz；
- 固定地址探测：ES8311 `0x18`、PCA9557 `0x19`、PN532 `0x24`、ES7210 `0x41`、QMI8658A `0x6A`；
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

## M1 明确未接入

- 真实 ES7210/ES8311 音频链路；当前使用 `DummyAudioCodec`；
- ST7789 显示与按键交互；
- TF/SDSPI；
- PN532/QMI8658A 轮询业务；
- RGB、振动控制和电池百分比；
- 产品状态机、录音和 OTA 策略。

## 构建

```bash
python3 scripts/build.py chatchat/hi-yaka-v3 \
  --name hi-yaka-v3 \
  --language zh-CN \
  --wake-word disabled
```

构建成功只证明软件链接和配置链成立，不证明 PUYA Flash、I2C 地址或安全电平已经通过实板验证。

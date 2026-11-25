# 微雪 ESP32-C6-LCD-1.47 的 PARLIO 屏幕对照程序

**范围：**此独立 ESP-IDF 工程只适用于微雪 `ESP32-C6-LCD-1.47` 开发板及其 172 × 320 ST7789 系列屏幕。它保留了 2026-11-25 实板显示成功的源码，不是 HiYaKa 产品固件，也不是 HiYaKa 屏幕验收结果。

## 接线与运行方式

| 信号 | 微雪板 ESP32-C6 GPIO |
| --- | ---: |
| LCD MOSI / SDA | 6 |
| LCD SCLK | 7 |
| LCD CS | 14 |
| LCD D/C | 15 |
| LCD RESET | 21 |
| LCD 背光 | 22 |
| TF 卡 CS | 4，程序保持高电平且不挂载 TF 卡 |

程序使用 RGB565、172 × 320 可见区域、X 偏移 34、Y 偏移 0。启动后先运行低速 GPIO、高速 GPIO 和 SPI2 DMA 对照；约 35 秒后进入最终 PARLIO 阶段。最终应看到**整屏绿色、黑色 `PARLIO` 字样完整、没有横向分界**。代码向 GPIO22 背光 PWM 请求 384/1024（37.5%）占空比；日志出现过 `GPIO 22 is not usable` 提示，实际引脚波形和占空比未测。

PARLIO 阶段设置为 1 MHz、1 位数据宽度、下降沿移位；每次发送 160 行，即 55,040 字节，两笔共覆盖 320 行。GPIO14 由 PARLIO 的反相 `valid` 信号接管，使 LCD CS 只在每笔有效传输期间为低；发送后调用 `parlio_tx_unit_wait_all_done()`，再复用静态缓冲区。两笔合计约 890 ms 是此次实板串口记录值，不是产品帧率承诺。

本程序在 **ESP-IDF v6.1 的 ESP32-C6** 上将 `bit_pack_order` 设为 `PARLIO_BIT_PACK_ORDER_LSB`。这是一项针对该版本实际输出的工作绕行：IDF 枚举值直接写入 C6 寄存器，而枚举文字与 C6 技术参考手册对寄存器 0/1 的位序说明相反。不得仅凭枚举名称推定线上的实际位序；升级 SDK 后须重新核对手册、驱动源码和实屏颜色。

## 实板 A/B 证据

四个条件保持相同的面板初始化、像素数据、1 MHz 和 160＋160 行传输；只改变 CS 控制及位打包配置。

| CS 控制 | 位打包配置 | 2026-11-25 微雪板观察 |
| --- | --- | --- |
| 手动 GPIO，跨两笔保持低 | MSB | 颜色和文字错误，并出现分界 |
| PARLIO 反相 valid | MSB | 两笔完成、无 160 行分界，文字顺序正确；目标绿色显示为红粉色 |
| 手动 GPIO，跨两笔保持低 | LSB | 二次重烧后仍是上半绿、下半红，文字错乱 |
| PARLIO 反相 valid | LSB | 两轮复位后整屏绿色、文字完整、无分界 |

这些结果支持 **CS 有效窗口和位序均影响本次微雪板显示**。它们尚未区分手动 CS 方案中究竟是 EOF 后时钟、首位同步还是其他交易边界时序造成错行；没有示波器波形，不能把推断写成芯片级根因。该矩阵只证明这块微雪板在上述配置下的结果。

## 构建与烧录

在 Linux 上、从本目录运行；Windows 仅用于 USB 烧录和串口观察：

```sh
docker run --rm -v "$PWD:/project" -w /project espressif/idf:v6.1 \
  idf.py set-target esp32c6 build
sha256sum build/waveshare_c6_lcd_benchmark.bin
```

`sdkconfig.defaults` 指定 ESP32-C6 与 8 MB Flash。全新刷写时，把 Linux 构建得到的 `build/` 产物复制到连接开发板的电脑，再从本工程目录运行：

```sh
python -m esptool --chip esp32c6 -p COM7 -b 115200 \
  --before default-reset --after hard-reset write-flash \
  --flash-mode dio --flash-size 8MB --flash-freq 80m \
  0x0 build/bootloader/bootloader.bin \
  0x8000 build/partition_table/partition-table.bin \
  0x10000 build/waveshare_c6_lcd_benchmark.bin
```

`COM7` 是当次测试端口，应以实际识别结果替换。若板上已有相同构建配置的 bootloader 和分区表，对照实验只需写入 `0x10000` 的应用镜像，并校验烧录结果。此次成功的应用镜像 SHA-256 为 `b1e0395914c7d3eec78ee01cdcd06fe2b14a8799a0c56f0b30433d8eea0d70a9`；新构建的哈希可能因构建元数据不同而变化，需记录自己的产物哈希。源码与测试经过见[微雪屏幕传输对照记录](../../docs/bringup/2026-11-25-waveshare-c6-lcd-transport-comparison.md)。

## 与 HiYaKa 的边界

微雪板的 LCD MOSI=GPIO6、SCLK=GPIO7、CS=GPIO14，且 Flash 为 8 MB。HiYaKa 的板级引脚、16 MB Flash 和屏幕接口合同不同；当前 HiYaKa 的 LCD CS 经过 PCA9557 I²C 扩展器，不能直接充当 PARLIO 的 `valid_gpio_num`。因此本例不能复制到 HiYaKa 板级代码，也不能据此宣布 HiYaKa 的 PARLIO 或 M2 屏幕验收通过。移植前须重新核对 HiYaKa 的精确屏幕料号、PCB 铜线、控制脚可达性、构建配置和同板实测。

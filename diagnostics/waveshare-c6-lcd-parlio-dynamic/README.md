# 微雪 ESP32-C6-LCD-1.47：PARLIO 动态刷新诊断

本目录是独立的 ESP-IDF 6.1 实验工程，用于复现微雪 `ESP32-C6-LCD-1.47` 上的整屏刷新、区域刷新和动态界面。它保存了 2026-11-25 在 10 MHz 档实屏验证所用的 **v2 源码**；同一源码的 1 MHz v2 也已构建，但此次实屏观察的 1 MHz 档使用较早镜像。实验板当时接在 Windows 的 `COM7`；端口号只标识那次测试，不是设备的固定属性。本工程不属于 HiYaKa 产品固件。

## 硬件和实验路径

| 信号 | 微雪板 GPIO |
| --- | ---: |
| LCD MOSI、SCLK | 6、7 |
| LCD CS、D/C、RESET | 14、15、21 |
| 背光 | 22 |
| TF 卡 CS | 4；保持高电平，不挂载 TF 卡 |

面板可见区域为 172 × 320，窗口 X 偏移 34、Y 偏移 0，像素格式为 RGB565。代码沿用微雪面板初始化及 MX+BGR 方向设置。PARLIO 使用一位数据宽度、下降沿移位、`PARLIO_BIT_PACK_ORDER_LSB`，并用反相 `valid` 输出直接控制 GPIO14 的 LCD CS。该位序枚举是针对 ESP-IDF 6.1 与 ESP32-C6 实测结果采用的配置；更换 SDK 后应重新核对线上位序。

短命令与像素采用两条路径：每次设置窗口前等待 PARLIO 完成并释放单元，显式将 GPIO6/7/14 的输出矩阵交还普通 GPIO，由 GPIO 位绑发送 CASET、RASET、RAMWR；随后重新创建 PARLIO 单元，并通过硬件 `valid` 片选发送像素。单笔整屏数据为 160 行、55,040 字节；两笔覆盖 320 行。这个切换过程的耗时已计入日志，不应只用像素传输时间代表一次刷新。

## 画面与计时口径

启动后程序依次显示红、绿、蓝、白四象限，刷新黑底和四角 24 × 24 色块，最后显示 `PARLIO`、`menu`、`UPTIME`、48 × 48 循环动画以及刷新耗时。`UPTIME` 是从设备启动算起的时间；动画是 8 帧预先转换的 RGB565 数组，运行时没有 GIF 解码器。

串口 `FULL_PATTERN`、`CORNER`、`PARTIAL` 分别记录整屏、四角和动态局刷。`release_us` 是等待并释放 PARLIO、恢复 GPIO 输出矩阵的时间；`command_us` 是 GPIO 发送窗口命令的时间；`acquire_us` 是重建 PARLIO 的时间；`pixel_us` 包含像素发送及完成等待；`total_us` 是该次操作的端到端耗时。屏幕 `FULL US`、`ANM`、`CLK` 分别显示最近一次整屏、动画区域和时钟区域的总耗时。`actual_fps_milli` 用完成的动画帧数除以刷新完成后重新读取的实际时间窗口；它不是预设的调度频率，也不代表整屏帧率。出现 `STOP` 时程序停留并打印阶段和错误，不自动重启。

## 构建

在 Linux 上从本目录构建；工程默认目标为 ESP32-C6、8 MB Flash。两档必须用不同的构建目录，以免把时钟设置与产物混淆：

```sh
docker run --rm -v "$PWD:/project" -w /project espressif/idf:v6.1 \
  idf.py -B build-1mhz -DWS_PARLIO_HZ=1000000 build

docker run --rm -v "$PWD:/project" -w /project espressif/idf:v6.1 \
  idf.py -B build-10mhz -DWS_PARLIO_HZ=10000000 build
```

两个目录各自产生 `bootloader/bootloader.bin`、`partition_table/partition-table.bin` 和 `waveshare_c6_lcd_parlio_menu.bin`。若要复现实板实验，先核对连接的确实是微雪板及实际串口，再按 ESP-IDF 打印的对应构建目录烧录参数写入；此次 Windows 测试采用 `COM7`、115200 bit/s。不要把本工程镜像烧入 HiYaKa。

## 2026-11-25 实板证据

同一块微雪板上，1 MHz 的早期镜像与 10 MHz 的 v2 镜像均观察到完整四象限、四角局刷、菜单、`UPTIME` 和动画；10 MHz 运行时温度正常。10 MHz 的 v2 镜像烧入后，约 30 秒串口采集中未出现程序报告的刷新失败或 `STOP`。下面是**单次日志样本**，不是性能保证：

| 请求时钟 | 整屏数据量 | 像素发送完成耗时 | 整屏端到端耗时 | 证据口径 |
| --- | ---: | ---: | ---: | --- |
| 1 MHz | 110,080 B | 880,753 µs | 895,308 µs | 早期同源码实验日志，帧率窗口修订前 |
| 10 MHz | 110,080 B | 88,181 µs | 102,769 µs | v2 实验日志 |

10 MHz 的一条动态日志记录了 48 × 48 动画区域 `anim_total_us=5,375`、时钟区域 `clock_total_us=7,007`、本窗口 7 帧 / 1,000,000 µs，即 `actual_fps_milli=7000`。计时中包含释放、命令、重建和像素传输；不同轮次会波动。留存的 v2 构建产物 SHA-256 分别为：1 MHz `eabd759e390cd357470789790cd81bbcd139a206ac72f656bd20728348e98aa0`（仅构建）、10 MHz `8b1585a9387415f98604af496d383a7d1941266227639edd27429116fe3163e0`（已实屏验证）。镜像未纳入仓库；重新构建时应记录新产物哈希。

## 限制与后续判断

运行日志仍有重复的 `gpio: conflict found for GPIO[6/7/14]` 警告；它与这轮成功显示并存，但本实验没有测量引脚波形或证明长期稳定性。背光代码请求 384/1024（37.5%）PWM，占空比和亮度未用仪器实测；日志曾出现 `ledc: GPIO 22 is not usable, maybe conflict with others`，不能把 API 返回成功等同于物理输出已验证。

本例依赖微雪板的 LCD CS 直连 MCU。HiYaKa 当前 LCD CS 经 PCA9557 扩展器，不能直接作为 PARLIO 的 `valid_gpio_num`；两块板的引脚、供电、Flash 配置和屏幕链路也不同。微雪板结果仅证明这条实验路径在微雪硬件上可运行，不能据此宣布 HiYaKa 的 PARLIO 路径或 M2 屏幕验收通过。相关静态 A/B 实验见[微雪 PARLIO 屏幕对照程序](../waveshare-c6-lcd-parlio/README.md)。

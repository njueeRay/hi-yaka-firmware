# 微雪 ESP32-C6 屏幕传输对照（2026-11-25）

## 目的与边界

在独立的微雪 `ESP32-C6-LCD-1.47` 开发板上，比较 ESP32-C6 用慢速 GPIO、快速 GPIO、硬件 SPI 与 PARLIO 驱动同一块屏幕的显示结果与整屏传输时间。这是 **LCD 数据传输方式** 的对照；它不包含 HiYaKa 的 PCA9557、共享 I²C 总线或背光飞线，不能据此判定 HiYaKa 的 I²C 重试根因。HiYaKa M2 屏幕验收仍未完成。

## 板卡与可复现条件

| 项目 | 本次条件 |
|---|---|
| 板卡 | 微雪 `ESP32-C6-LCD-1.47`，Windows 串口 `COM7`；本次 `esptool` 识别主控为 ESP32-C6FH4，`flash_id` 检出 Flash 容量 8 MB。主控型号与检出的 Flash 容量是两项独立读数，不能把它合写成“C6FH8”。 |
| LCD | ST7789，172 × 320，RGB565，显示窗口 X 偏移 34、Y 偏移 0。 |
| 接线 | MOSI GPIO6、SCLK GPIO7、CS GPIO14、DC GPIO15、RST GPIO21、背光 GPIO22；TF 卡 CS GPIO4 保持高电平。 |
| 共同条件 | 三模式共用微雪官方示例的完整面板初始化序列、显示窗口、五色整屏图案及 37.5% 背光；每模式三轮，每轮五色。无 TF、Wi-Fi、I²C 任务。 |
| 编译 | Raynode `/data/hi-yaka-build/waveshare-c6-lcd-benchmark-20261125`，Docker `espressif/idf:v6.1`，独立工程；不改 HiYaKa 源码。 |

官方依据：[板卡资料](https://docs.waveshare.com/ESP32-C6-LCD-1.47)、[原理图](https://files.waveshare.com/wiki/ESP32-C6-LCD-1.47/ESP32-C6-LCD-1.47_schemetics.pdf)、[面板资料](https://files.waveshare.com/wiki/ESP32-C6-LCD-1.47/1.47inch_LCD_Datasheet.pdf)。微雪建议背光不超过 50%；本次设置低于该值。

烧录前完整读取了开发板 8,388,608 字节出厂 Flash，保存在工作区 `.tmp/waveshare-c6-lcd-147-20261125/factory-flash-8m.bin`，SHA-256 `6989391AB9849476283DCA1EE6EA8932CB1DDF55EAA05BE270C84D1FC654FF73`。首版三模式应用镜像 SHA-256 `568B92E5D04552C34CECCACBA9FCAF590298D1B91F949B5EA1BA22BFFAF4ED40`；bootloader／partition／app 按 `0x0`／`0x8000`／`0x10000` 写入，esptool 对三段均报告写后哈希验证通过。

## 实板结果

每次整屏发送 **110,080 字节**。以下时间由设备端 `esp_timer_get_time()` 包围整屏命令和像素发送得到，**不含**初始化、屏间等待和人眼观察时间。两次独立复位，每次每模式 15 次，均记录 `err=ESP_OK`、`BENCH_DONE`；下面各模式平均值以两次日志核对，单轮数值相同。

| 模式 | 实现 | 整屏平均耗时 | 等效整屏速率 | 两轮记录 |
|---|---|---:|---:|---|
| `GPIO_SLOW` | `gpio_set_level`，每半周期延时 1 µs | 3.134449 s | 0.319 次/秒 | 30/30 API 成功 |
| `GPIO_FAST` | GPIO 寄存器置位／清位，无额外半周期延时 | 0.708515 s | 1.411 次/秒 | 30/30 API 成功 |
| `SPI2_12MHZ` | SPI2，模式 0，时钟 12 MHz | 0.083791 s | 11.934 次/秒 | 30/30 API 成功 |

12 MHz SPI 发送 110,080 字节像素的纯线速下界约 73.4 ms；实测 83.8 ms 包含指令及软件开销，数量级合理。第二次复位的 `bench-run2.log` SHA-256 为 `7A9F5E3104DA71D94AD04F2538CFC85CBE3047CC797678C7264005396BBCC1F9`；第一次 `bench-run1.log` SHA-256 为 `E4F9BA505B5DE926B732BB5389F9EAF56DE979C799866BF399810B663B00D1BC`。两份原始日志位于工作区 `.tmp/waveshare-c6-lcd-147-20261125/`。

ST7789 为只写屏幕接口，`ESP_OK` 只能证明主控提交传输。用户在第二次复位时目视确认三个模式结束标记依次为**完整红屏、绿屏、蓝屏**，没有观察到裁切、雪花或异常发热；首次结束后的蓝屏也得到目视确认。因此本次三种方式的**基础显示与整屏传输**均通过。该观察未覆盖长期运行、复杂界面或高于 12 MHz 的 SPI 时钟。

## PARLIO 补充实验：设备端完成，但实屏显示失败

独立工程加入第四种 `PARLIO_TX` 传输路径后，四模式应用镜像 SHA-256 为 `E5C529E0393CA09B2D04E65CB444F573C8A438388E18EBA354FBE493B68CB202`。首轮原始日志 `bench-parlio-run1.log` 位于同一工作区 `.tmp/waveshare-c6-lcd-147-20261125/`，SHA-256 为 `9D36FDF6E748F39ED90D18CA74B149CE82ACB46EE0EB9DFBE0349034CE4F87B9`。日志到达 `BENCH_DONE`，PARLIO 的 15 次整屏传输均返回 `ESP_OK`，平均 **0.916311 s/110,080 字节**，等效约 **1.091 次/秒**。该镜像内其他三种方式的平均值为慢 GPIO **3.133851 s**、快 GPIO **0.708521 s**、SPI2 12 MHz **0.083851 s**，与原三模式镜像接近。

上述首轮数值只证明主控侧传输调用完成。后续加入屏幕模式字样的实板实验中，慢 GPIO、快 GPIO 和 SPI DMA 阶段能显示；PARLIO 阶段却未完整显示。把 PARLIO 限制为仅发送像素后，屏幕出现彩色横带与残缺文字，而不是目标整屏画面。用户提供的实屏照片分别记录了多条横带，以及原生一位数据模式下上半部蓝色、下半部绿色、`PARLIO` 字样位置错乱。这证明 PARLIO 信号确实影响显示，却**未形成正确的 RGB565 像素流**。视觉状态已从“待验证”更新为“失败”，不得用 `ESP_OK` 或耗时数据替代实屏验收。

为区分传输分段与位序问题，实验曾把缓冲从逐行改为每 40 行一次传输，花屏仍在；改为原生一位传输、每 160 行一次后，画面呈现约半屏蓝、半屏绿。试图单次发送全部 320 行时，驱动在传输前报 `invalid transfer size, max transfer size should be less than 65535` 并重启，所以该轮没有有效屏幕结果。随后改为合法的 190 行与 130 行两笔传输，串口显示两笔已完成；用户新照片确认颜色分界从原来的约 160 行移到约 190 行，画面变成上部绿色、下部红色，字母仍然错序。**分界随 DMA 交易边界移动**有高置信度实屏证据；但具体是首笔/后续交易的位相、缓冲描述符、CS 接管时序，还是几项叠加，尚未定位。

另一条独立嫌疑是 GPIO/SPI 到 PARLIO 接管 SCLK、MOSI 时，LCD 的 CS 仍保持选中。若 GPIO 矩阵切换产生额外边沿，屏幕可能把它当作数据。已烧录仅在接管期间拉高 CS 的对照镜像；日志 `run12-csguard190.log` 记录 GPIO 与 SPI DMA 正常阶段后，PARLIO 像素阶段返回完成（`STAGE_PARLIO_PIXELS_ONLY_DONE us=890203`），但用户照片显示上部亮蓝色、下部暗蓝色与字样残缺，**CS 接管保护未修复显示**。其日志 SHA-256 为 `B6D88EF032191DB95404D37FDAD7290BE6DDBE107EF5E4105C940F710F6E1C55`。

随后只调整 PARLIO DMA burst 为驱动默认值，仍按 190／130 行分笔；`run13-burstdefault190.log` 到达 `STAGE_PARLIO_PIXELS_ONLY_DONE us=890193`，用户实屏照片仍显示上部蓝色、下部绿色及错序字样，**默认 burst 也未修复显示**。日志 SHA-256 为 `B31F2189242017073B6D236FC9436F5D1744A60886D936BDC7AC21F7BD003D4F`。此前原生一位模式从约 160 行改为 190／130 行分笔，用户照片见颜色分界移至约 190 行；这表明异常与传输分界相关，但不足以断言 DMA 描述符、首/后续交易位相或像素字节序哪一项是根因。

三张用户实屏照片分别记录了约 190 行绿／红分界、CS 保护后的双蓝分界、默认 burst 后的蓝／绿分界；其 SHA-256 依次为 `58A48FF0AA97EC3E3567243B350D83C2EDB24D30AC44D0A975B2B8E18CD43ADC`、`D7EAF53204CE4C9F2DEBD7717F9F8BE1E344334E22F91F9F60D943321DD95715`、`5EC0CC936A0DD2EAC760835D5C5343BA78A7134F0E933EEBFDDF607D685FE273`。照片仅用于确认画面异常及分界，无法单独测出总线波形。

测试后已将首版三模式基线镜像以 115200、no-stub 烧回 COM7，写后哈希校验通过。`run14-restored-spi.log` 前约 65 秒已记录慢 GPIO 并进行到快 GPIO 中途；日志 SHA-256 为 `E9FD169D07F124BAD20893CAEBF44C36E5B2C455585EE7F17B3E74E5AF07CF21`。本轮采集未覆盖最终蓝屏，也没有本轮恢复后的目视确认，不能把此前三模式的目视结果冒充本轮恢复结果。

ESP-IDF 6.1 的通用 PARLIO TX 支持 C6，而官方 `esp_lcd` PARLIO 屏幕封装由 `SOC_PARLIO_LCD_SUPPORTED` 控制，C6 未启用该封装；不能把未链接成功的官方封装实验当作显示失败。参考：[ESP-IDF PARLIO TX 文档](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/api-reference/peripherals/parlio/parlio_tx.html)、[官方 esp_lcd 构建条件](https://github.com/espressif/esp-idf/blob/v6.1/components/esp_lcd/CMakeLists.txt)。

## SPI DMA 菜单与动画局刷：独立外部对照

为观察比整屏纯色更接近界面的刷新，另在 COM7 烧录隔离工程 `.tmp/waveshare-c6-lcd-menu-demo-20261125/dist/waveshare_c6_lcd_menu_demo.bin`，SHA-256 `F03C14FBD10103D2C22BFBC76634E1F138E9AC2C2152B901BC2895530E7F38E6`；烧录使用 115200、no-stub，写后哈希校验通过。该程序用 SPI2 12 MHz 驱动微雪 ST7789，包含 `MENU`、`UPTIME`、24×24 动画区域及 `GIF`／`CLK` 数值区域。屏上时间是**设备上电时长 UPTIME**，并非实时时钟日期。

原始 `run1-menu.log` SHA-256 `0C2E0E22E19D08E27E20F2B4AF36EF3A4AB1796C26289AB1F1EC007094702780`。65 条逐秒 `PARTIAL` 样本均返回 `ESP_OK`；24×24 区域每次发送 576 像素／1,152 字节，传输耗时 **1,005–1,010 µs，均值 1,005.6 µs**；时钟区域每次发送 3,432 像素／6,864 字节，传输耗时 **5,329–5,332 µs，均值 5,329.8 µs**。日志末 `gif_frames=427` 是约 65 秒观察窗口内的累计计数。以上计时只包围显示命令与像素传输，**不含绘制、动画帧生成或 GIF 解码**，不得把局刷耗时直接当产品界面帧率。

用户本次一次性目视确认 `MENU`、`UPTIME`、24×24 动画及 `GIF`／`CLK` 数值均正常显示，没有裁切、花屏或异常发热。这证明微雪板的 **SPI DMA 局部刷新**可正确呈现该演示界面；一次目视与 65 秒日志不构成长期稳定性验证，也不改变上述 PARLIO 实屏失败结论。

## 同画面 48×48 菜单传输 A/B/C：串口与轮换版实屏已验证

后续隔离工程把动画区域放大到 **48×48 像素／4,608 字节**，保留同一菜单、`UPTIME` 和 **3,432 像素／6,864 字节**时钟区域，分别以低速 GPIO、高速 GPIO、SPI2 DMA 12 MHz 驱动。八帧 GIF 已预解码为数组，设备端不解码 GIF。`PARTIAL` 计时包含 CASET、RASET、RAMWR 与像素传输，不含绘制和帧生成；动画目标每 150 ms 换帧，实际频率受传输速度影响，不能仅用局刷微秒数推定界面帧率。

| 模式 | 串口逐秒样本 | 48×48 区域传输耗时 | 时钟区域传输耗时 | 日志末累计动画帧 | 实屏证据 |
|---|---:|---:|---:|---:|---|
| 低速 GPIO | 58 条 | 131,046–131,062 µs，均值 131,052.4 µs | 194,935–195,056 µs，均值 195,007.9 µs | 119 | 单版与轮换版均完整、温度正常；轮换对比中最卡 |
| 高速 GPIO | 59 条 | 29,226–29,254 µs，均值 29,239.5 µs | 43,252–43,285 µs，均值 43,269.8 µs | 412 | 单版未目视；**轮换版**完整、温度正常，主观比低速稍好 |
| SPI2 DMA | 64 条 | 3,628–3,647 µs，均值 3,633.8 µs | 5,329–5,334 µs，均值 5,330.6 µs | 427 | 单版与轮换版均目视完整、温度正常 |

三份日志依次为 `.tmp/waveshare-c6-lcd-menu-demo-20261125/run-gpio-slow-big.log`、`run-gpio-fast-big.log`、`run-spi-big.log`，SHA-256 分别为 `3FA1EACB4DD8E31938A0E1048F44DD6EA6D3D00170C089BAADCD682061582D74`、`B7A0F1F6207FAD79E9C124B1F0DF6C8D3B44A0D96F04A439D34945F62547A985`、`4AC77CBF622365AFD7106935AD1241F1A06C94397CA829ECEB9F4F81FAC50D29`。对应应用镜像 SHA-256 为低速 GPIO `E1590AD559D0E46E82E880776D59996107AE2A20C699912CF4D29FCCBCC9EC48`、高速 GPIO `E33252D6FB4AEA7C3F6BDA790F708E09F133A6194AB20ED93BEF861B97C3F6A9`、SPI2 DMA `71A26D3FA13433D46C69C20034CEAF1E5086519702D0EE5D4AE153476965EF26`。

三版串口均给出 `ESP_OK`，它本身只证明调用完成。逐版烧录阶段，用户起初只观察了 SPI，随后补充确认低速 GPIO 单版显示完整、主观流畅且温度正常；高速 GPIO 单版没有单独目视。三种方式的同镜像轮换对照见下节，该次用户均按模式确认完整显示。

### 同一镜像每 10 秒轮换：串口两轮与实屏均完成

为排除逐版烧录时错过 GPIO 显示窗口的问题，另用单一镜像按低速 GPIO → 高速 GPIO → SPI2 DMA 顺序，每种模式运行约 10 秒后自动轮换。应用镜像 SHA-256 `1E68B2FACD8031BBED9831EBA36FA4CB392B491BCC981B7CFF04CAAB92D5862D`，源码 `main.c` SHA-256 `51F700364310937D2A9FA187B3A2FE9C1F1A509CCCF25101003E761DDC99586E`。原始 `.tmp/waveshare-c6-lcd-menu-cycle-20261125/run-cycle-1.log` SHA-256 `4A35851EA549D9CEB48B05383E5815871544EEB142E55A41216353317EA708B7`。

| 模式 | 首轮时长／完成动画帧 | 第二轮时长／完成动画帧 | 串口与视觉状态 |
|---|---:|---:|---|
| 低速 GPIO | 10.169216 s／21 帧 | 10.175558 s／21 帧 | 两次 `MODE_EXIT`；轮换版用户目视完整 |
| 高速 GPIO | 10.038204 s／65 帧 | 10.038126 s／65 帧 | 两次 `MODE_EXIT`；轮换版用户目视完整 |
| SPI2 DMA | 10.007421 s／67 帧 | 10.007477 s／67 帧 | 两次 `MODE_EXIT`；轮换版用户目视完整 |

约 65 秒串口窗口内完成两轮三模式，并进入第三轮低速 GPIO；`PARTIAL` 样本均返回 `ESP_OK`，未见 `MODE_SWITCH_STOP` 或重启。用户对**这版轮换镜像**确认三段按序出现，`MENU`、`UPTIME` 时钟及 48×48 动画完整，温度正常。这给三种传输方式各自增加了对应阶段的实屏证据；它仍是约 65 秒的单次演示，不等于长期稳定性或 HiYaKa 板上验收。

低速 GPIO 首轮在 **10.169216 秒完成 21 帧，约 2.1 帧／秒**；高速 GPIO 为 65 帧／10.038204 秒，SPI2 DMA 为 67 帧／10.007421 秒。用户最初认为三段“速度差别不明显”，并曾把低速单版描述为“完整显示且流畅”；连续观察轮换版后，进一步明确反馈 **“GPIO SLOW 确实最卡，GPIO FAST 稍好”**，而高速 GPIO 与 SPI 的主观差别较小。以后一次更具体的观察作为当前主观结论。低速模式能完整绘制菜单，与其动画仅约 2.1 帧／秒并不矛盾；高速 GPIO 和 SPI 的完成帧数接近，还受动画目标 **150 ms/帧**的节拍限制，不能从这一演示推断两者传输性能相同或极限帧率相同。

## 对 HiYaKa 的解释与待测项

微雪板的 CS、DC、RST、背光直接连接 ESP32-C6；HiYaKa c0 样机则由 PCA9557 控制其中若干脚。微雪三模式通过，支持“ESP32-C6 能用 GPIO 和 SPI2 驱动这一类串行 ST7789 屏”的结论，不能排除 HiYaKa 的 PCA9557 控制失联、共享 I²C 电气问题或 HiYaKa PARLIO 编码问题。两板面板版本和显示窗口亦不同，速度数值不应直接当作 HiYaKa 产品帧率。

**PARLIO：主控侧交易完成，微雪实屏已确认异常。** 当前不能用于 HiYaKa 的显示基线。已验证可正确显示的慢速 GPIO、高速 GPIO 与 SPI DMA 菜单路径，仍只属于微雪外部对照；它们不能关闭 HiYaKa 的 PCA9557、共享 I²C 或 PARLIO 问题，也不能作为 HiYaKa M2 完成证据。

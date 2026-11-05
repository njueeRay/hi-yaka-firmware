# HiYaKa 麦克风全零：固件 A/B 判别实验

> 2026-11-05。当前样机 MASTER OFF；本文件只定义实验与软件构建证据，不声明任何新实板结论。U11 功放支路仍按 L7 隔离、GPIO23 强制低的受控样机边界处理。

## 已知事实与待分辨点

- 已验收的旧板和当前板均用 ES7210 与两颗 ZTS6216，实体麦分别进入 MIC1/MIC2，SDOUT1 经 51 Ω 到主控。旧板固件 GPIO 配置与其 EDA 主控页一致。
- 当前双麦标准 I²S 的 C6 端 MCLK/BCLK/WS 活动分别约 4.096 MHz、512 kHz、16 kHz；原始 RX 1024 字节全零，GPIO13 在 20 ms 内无上升沿。ES7210 可经 I²C 读寄存器，报告正常、双麦选通且未静音。现有万用表结果不能说明 U12 焊盘时钟或 SDOUT1 波形。
- 待分辨：C6 I²S 接收/DMA 是否能采到已知数据；配对 TX/RX 的双工设置是否妨碍 ES7210 采样；C6 GPIO13 是否确实路由到 I²S 输入；ES7210 是否在实物焊盘输出数据。

## 实验构建

`hi-yaka-lab-audio-ab` 是从无屏幕实验配置派生的独立变体，开机在普通音频初始化之前依次执行两项一次性测试；普通 `hi-yaka-lab` 与正式 `hi-yaka` 的行为不变。

| 步骤 | 保持不变 | 唯一有意改变 | 观测与解释 |
|---|---|---|---|
| A：内部回环 | C6 I²S0、16 kHz、双槽 16 位标准帧、相同 DMA/时钟族 | 暂时把 TX 输出和 RX 输入同时接到 GPIO1；不接 GPIO13，也不启用 ES7210/功放 | 按 Espressif 官方 I²S 回环方式发送固定 `0x1357/0x2468`；接收端命中表示 C6 I²S 收发、DMA 和基本时钟链可工作。未命中记为“不确定”，需先检查自测配置，不能直接判主控损坏。 |
| B：ES7210 仅接收 | 与当前失败样本相同的 16 kHz、双槽 16 位、MIC1/MIC2、DIN=GPIO13、ES7210 驱动和 I²C 地址 | 不分配 TX 通道，也不初始化 ES8311；直接从 RX DMA 读取 | 若得到非零，优先检查原双工配对/重配置；若仍全零，双工独占冲突的解释被削弱，但 GPIO13、U12 实际输出与供电仍不能排除。 |
| C：原双工基线 | 沿用现行无屏幕实验代码与实板连接 | A/B 临时通道销毁后，回到常规 `BoxAudioCodec` 配对 TX/RX | `ACT_B` 录音、原始 I²S 与 GPIO12/13 路由日志用于和此前双工全零结果对照；若 C 与之前结果不同，先查前两项测试的状态残留，不能直接归因。 |

GPIO1 连接的是 ES8311 数字输入；A 不驱动与 ES7210 SDOUT1 相连的 GPIO13，避免输出对打。实验仅在功放物理隔离、GPIO23 保持低电平的现有样机上使用。若没有实板上电窗口，则只能完成源码审计和 Linux 构建，不能将 A/B 表格中的预期当作结果。

## 结果判读优先级

1. **A 通过、B 非零、C 全零：**优先定位双工通道创建、启用和 `esp_codec_dev_open` 的重配置顺序。再做“配对 RX/TX 但不预先 enable”的单变量复验。
2. **A 通过、B/C 均全零：**I²S DMA 主体已证明可采样，下一步检查 GPIO13 路由读回；若路由正确，需维修台在 U12 与 R65 两侧测波形和各电源脚。不能只凭软件将芯片判坏。
3. **A 不确定：**先修正内部回环测试的时序/配置，再解释 B/C。A 的阴性本身不证明硬件故障。
4. **C 非零：**优先重复独立的原基线构建，确认不是 A/B 测试清除了某个状态；随后检查启动顺序差异。

原始依据：[Espressif ESP-IDF 6.1 I²S 文档](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/api-reference/peripherals/i2s.html)明确允许 DIN/DOUT 同 GPIO 内部回环；[官方标准 I²S 示例](https://github.com/espressif/esp-idf/blob/master/examples/peripherals/i2s/i2s_basic/i2s_std/main/i2s_std_example_main.c)使用该方式；旧版与当前板的 EDA/固件对照见[首板专项诊断](2026-11-04-lcd-nfc-mic-investigation.md)。

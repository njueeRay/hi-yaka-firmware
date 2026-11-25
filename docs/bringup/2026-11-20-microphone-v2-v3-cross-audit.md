# HiYaKa 麦克风链路 v2.x/v3.0 交叉审计

> 附件用途：记录 `HIYAKA-AUDIO-INC-001` 在 2026-11-20 的详细交叉核查。主结论以 YAKA-0006 摘要文档为准；本文保留版本差异、证据边界和后续判别流程。

## 1. 结论

1. 麦克风功能仍为 `FAIL / NO-GO`；已确认主控没有收到有效 PCM，但最终根因未定。
2. 当前前置阻断是 ES7210 控制寄存器写 NACK，完整初始化未稳定完成。这只能证明“事务形状/调用顺序/器件前序状态/总线电气条件仍未分离”，不能直接写成 PCB I²C 物理层已损坏。
3. 历史全 0/全 `-1` 和 GPIO13 弱拉对照证明 MCU 端未收到有效主动数据驱动。目前仍不能区分“U12.11 未输出”与“U12.11 → R65 → GPIO13 实体链中断”。
4. YaKa v2.x 与 v3.0 的 ES7210 采集拓扑高度一致，暂未找到只存在于 v3.0 的明显原理图断链。两代 MCU、引脚复用、驱动版本、采样率和 slot 配置不同，旧板可用不能替代当前板的分段实测。
5. 功放从 HT8691R 改为 NS4150B 不会直接修复麦克风；但迁移时必须保留 ES8311 OUTP/OUTN 在 PA 前送往 ES7210 MIC3 的播放参考网络，否则 AEC 参考会丢失。

## 2. 已确认现象

| 证据 | 能支持的结论 | 不能支持的结论 |
|---|---|---|
| ACT_B、录音状态、TF 建档与 WAV 关闭曾正常 | 业务控制链和文件流程可运行 | 不代表 PCM 有效 |
| 原始 DMA/WAV 固定全 0 或全 `-1` | 没有有效声音数据 | 不能单独定位芯片、走线或麦克风 |
| GPIO13 弱下拉后全 0，弱上拉后全 `-1`；20 ms 无数据边沿 | MCU 端没有看到外部强驱动 | 不能区分 U12.11 本身 Hi-Z 与中间链路断点 |
| C6 内部 I²S 回环通过 | I²S 接收/DMA 核心没有完全失效 | 不代表 ES7210、GPIO13 或外部走线正常 |
| MCU 端 MCLK/BCLK/WS 存在 | MCU 时钟引脚有输出 | 不代表时钟已到达 U12 焊盘 |
| 历史曾读到 `0x0B=2`、`0x12=0`，但 GPIO13 仍无边沿 | 不能用“U12 始终 power-down”解释全部现象 | 不能证明 U12.11 实际已输出 |
| 当前镜像的 ES7210 初始化写入 NACK | 当前有控制平面前置阻断 | 不能解释所有历史无数据现象 |

## 3. v2.x/v3.0 交叉复核

### 3.1 项目身份

- 在线 `YaKa v2.0`：project UUID `61d2076a056e4cb4b9c5b734f0f793e9`。
- 当前 `YaKa v3.0`：project UUID `8f58125ad0d64436afbfeeeb03d3b159`。

### 3.2 硬件对照

| 项目 | YaKa v2.x | YaKa v3.0 | 判断 |
|---|---|---|---|
| 音频 ADC | ES7210 | ES7210 | 一致 |
| 实体麦克风 | ZTS6216 ×2 | ZTS6216 ×2 | 一致 |
| 主数据输出 | U7.11 → R48=51 Ω → S3 GPIO12 | U12.11 → R65=51 Ω → C6 GPIO13 | 功能拓扑一致，引脚不同 |
| 备用 SDOUT2/TDMIN | R49=0 Ω，DNP | R66=0 Ω，DNP | 一致 |
| MCLK/BCLK/WS/DIN | GPIO38/14/13/12 | GPIO0/3/12/13 | MCU 与 pin mux 风险不同 |

### 3.3 固件对照

- 旧 2.x：24 kHz、4-slot 16-bit TDM、MIC1–MIC4、ESP-IDF 5.4.2 / `esp_codec_dev ~1.4`。
- 当前 3.0 诊断：16 kHz、2-slot 16-bit 标准 I²S，MIC1/MIC2。
- 当前 3.0 生产候选：16 kHz、4-slot TDM，MIC1/MIC2/MIC3，ESP-IDF 6.1 / `esp_codec_dev 1.6.2`。
- 2.x 旧源码同样采用“标准 I²S TX + 4-slot TDM RX”；这是继承架构风险，不是 3.0 新增错误。
- `esp_codec_dev 1.6.2` 在 `open()` 时会经数据接口重配格式、时钟并启用通道。因此不能用构造期的静态配置将 STD/TDM 写成已证实根因，必须以 `open()` 成功后的实际帧时序为准。
- 历史 512 kHz BCLK 来自 16 kHz、双槽、16-bit 标准 I²S 诊断，数值正确；不能倒推生产 4-slot TDM 已配错。

## 4. 第二个固件契约风险

生产候选选中 `MIC1 | MIC2 | MIC3`，意图是两颗实体麦克风加一路播放参考；但上层 `input_channels_` 在开启参考通道时仍是 2，而 ES7210 TDM 顺序为 `MIC1, MIC3, MIC2, MIC4`。

因此当前有中高风险实际只向上层交付：

```text
MIC1 + MIC3 播放参考
```

而 MIC2 虽被 ADC 选中，没有进入产品算法链。这不解释当前 GPIO13 无边沿，但 PCM 恢复后仍必须明确产品契约：

- `MR`：单麦 + 播放参考；或
- `MMR`：双实体麦 + 播放参考。

## 5. 根因分支与优先级

| 优先级 | 分支 | 置信度 | 说明 |
|---:|---|---|---|
| 1 | ES7210 控制事务/初始化顺序未稳定 | 高，当前阻断 | 最新实验尚未进入 PCM |
| 2 | U12 电源域、芯片脚时钟或输出状态不完整 | 中高 | MCU 端有时钟不等于 U12 焊盘有时钟 |
| 3 | U12.11/R65/焊盘/过孔/走线到 GPIO13 存在断点 | 中 | 网表正确，但缺分段波形和端到端证据 |
| 4 | GPIO matrix、pin mux、TDM slot/数据布局 | 中 | 不能解释所有 Hi-Z 现象，但影响最终通道契约 |
| 5 | MICBIAS、模拟输入、麦克风本体 | 中低 | 待数字输出恢复后查；不足以单独解释 SDOUT 随 GPIO 弱拉变化 |
| 6 | U12 芯片或装配缺陷 | 未知 | 只能在供电、时钟、寄存器和 U12.11 均已分层后归因 |

## 6. 最短验证流程

### Gate 1：控制平面

- 使用独立 ES7210 输入程序，不初始化 LCD、PN532、PCA9557、ES8311。
- PA_EN 保持低，I²C 100 kHz。
- 记录每笔初始化写入、ACK/NACK、重复起始与 STOP。
- 读回 `0x00/01/02/06/08/0B/11/12/40–4C`。
- 连续三次冷启动初始化成功且 `0x0B=2` 后再进入 Gate 2。

### Gate 2：U12 芯片脚供电与时钟

同一录音窗口测量：

- U12.6/VDDP、U12.7/VDDD、U12.22/VDDA、U12.23/VDDM；
- U12.5/MCLK、U12.9/BCLK、U12.10/LRCK。

禁止以 MCU 端波形代替 U12 焊盘波形。

### Gate 3：SDOUT 分段

```text
U12.11 → R65 U12侧 → R65 MCU侧 → GPIO13
```

- U12.11 有波形、R65 后无：查 R65、焊盘、过孔、走线。
- R65 两侧有波形、DMA 仍固定：查 GPIO matrix、pin mux、I²S RX 配置。
- U12.11 无波形，但供电、时钟、状态均正确：查 U12 输出配置、焊接或芯片。

### Gate 4：模拟前端

数字输出恢复后再测 MICBIAS12、两颗麦克风 VDD、MIC1/MIC2 输入端、单麦启用、增益和通道静音。

### Gate 5：产品通道契约

1. 双槽标准 I²S：MIC1/MIC2 分别随已知声源变化。
2. 四槽 TDM：逐槽验证真实排列。
3. 验证 `MIC1 + MIC2 + MIC3参考` 的内存顺序。
4. 验证上层 AFE 的 `MR/MMR` 契约。
5. 完成 AEC、录音、唤醒、TF WAV 可回放与并发稳定性。

## 7. 关闭标准

- 完整初始化连续冷启动通过，寄存器读回稳定。
- U12 电源、时钟、U12.11→R65→GPIO13 分段波形有完整证据。
- MIC1/MIC2 原始 PCM 分别随声源变化，静音时有合理底噪。
- MIC3 参考与播放内容相关，通道顺序符合固件契约。
- ACT_B 录音 WAV 可回放，连续录音、关闭、重启后仍正常。
- 保存样机编号、固件 SHA、串口日志、逻辑分析仪波形和返修记录。

## 8. 证据入口

- [YAKA-0006｜麦克风无有效数据](https://scnlb1lk96sb.feishu.cn/docx/Q1u5df5QWomTj0x0xkfcSoz4nvh)
- [ESP32-C6 I²S 官方文档](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/api-reference/peripherals/i2s.html)
- [ESP32-C6 GPIO 官方文档](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/api-reference/peripherals/gpio.html)
- [`esp_codec_dev 1.6.2`](https://components.espressif.com/components/espressif/esp_codec_dev/versions/1.6.2)
- [ES7210 数据手册](https://files.waveshare.com/wiki/common/ES7210_DS.pdf)
- [2026-11-05 软件 A/B 实板结果](2026-11-05-audio-ab-results.md)
- [2026-11-12 PCA9557 与麦克风共享 I²C 同板对照](2026-11-12-pca-audio-shared-i2c-ab-evidence.md)

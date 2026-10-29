# Velxio 用于 YaKa 3.0 固件前置仿真的评估

> 评估日期：2026-10-29  
> 评估对象：`davidmonterocrespo24/velxio`，本地审阅 SHA `6d1bf83e90186ca96c70dc986e7ed7f042dc0328`  
> 结论置信度：总体结论高；ESP32-C6 无线能力的线上实现细节为中，需以受控试验复核

## 1. 结论

Velxio 可以作为 YaKa 的**可选 L0.5 虚拟行为测试通道**，适合硬件到位前验证 ESP32-C6 基础启动、UART、部分 GPIO/I²C/SPI 行为、UI/状态机和故障注入。它不能替代 YaKa 的 ESP-IDF 6.1 构建真值，也不能替代音频、射频、电源、NFC 场强、显示电气和整机验证。

最合理的使用方式不是把 YaKa 工程迁入 Velxio，也不是维护一套“仿真专用固件”，而是：继续在 Raynode 用固定 ESP-IDF 6.1 构建；把同一份 merged flash image 送入 Velxio；只为外设行为建立薄模型；任何为了仿真而产生的条件分支都必须能证明不会污染实机路径。

当前不建议直接将 Velxio 作为正式依赖。先做一次有明确退出条件的短周期 spike；若同一固件镜像、I²C/PCA9557 安全顺序或故障注入无法稳定工作，就停止投入，把资源转向 host-native HAL mock 与实板自动化。

## 2. 它是什么

Velxio 是一个开源多开发板仿真平台，项目以 AGPL-3.0 发布。它提供浏览器编辑器、开发板 CPU 仿真、虚拟外设/连线、自定义芯片和 CI 场景。公开资料列出 ESP32、ESP32-S3、ESP32-C3、ESP32-C6 等板卡；不同芯片使用的后端并不完全相同。

对 YaKa 最关键的事实：

- ESP32-C6 在线仿真使用 `esp32c6js` 路径；官方 C6 页面声明可运行 Arduino、MicroPython 和 ESP-IDF，并覆盖 GPIO/PWM/ADC/I²C/SPI/UART；
- 公开自托管镜像不包含 ESP32-C6；C6 依赖在线/托管能力；
- 官方 ESP32 编译说明当前列出的 ESP-IDF 是 5.5.4，而 YaKa 固定为 ESP-IDF 6.1；
- 自定义芯片可用 C/WASM 建立 GPIO、I²C、SPI、UART 和 framebuffer 行为模型；公开 API 未提供等价的 I²S codec 从设备模型；
- 平台提供 microSD、PN532 示例或部件线索，但未找到 YaKa 使用的 ES7210、ES8311、QMI8658A、PCA9557 的现成精确模型；
- 仓库中的模拟电路说明定位于准静态/毫秒级行为，不足以证明音频速率模拟反馈和真实声学链路。

## 3. 已做验证

### 3.1 源码与文档审阅

审阅了仓库 README、架构、ESP32/C6 仿真、CI、自定义芯片 API、部件目录和编译后端。当前公开源码中的 RISC-V 板型表主要覆盖 C3；C6 板型通过在线能力注册，意味着“仓库可自建”不能推导出“C6 可完全离线自建”。

编译后端会生成/改写 ESP-IDF 工程结构、sdkconfig 和分区，适合平台模板，但不能等同于原样构建复杂的 xiaozhi 多组件工程。YaKa 应以 Raynode 的真实构建产物为准。

### 3.2 在线烟雾测试

- `c6-blink` 示例在在线平台完成编译和运行；日志显示 ESP32-C6 ROM 启动、UART 输出和 LED 周期变化，证明当前在线 C6 基础启动/UART/GPIO 路径可用；
- `c6-oled-i2c` 示例存在并展示 C6 与 SSD1306 的 I²C 连线，但本次执行因自动化会话超时未取得运行完成证据，因此不计为通过。

上述结果只证明平台最小样例，不证明 YaKa 镜像或复杂外设可运行。

## 4. 与 YaKa 的能力匹配

| YaKa 能力 | Velxio 可承担的前置验证 | 不能由它证明 | 评价 |
|---|---|---|---|
| C6 启动/UART | 启动状态机、串口诊断、复位原因路径 | 实板 Flash 模式、绑带、电源完整性 | 可用 |
| GPIO/按键/RGB/马达 | 数字输入、输出语义、事件流、限时保护 | 实际阈值、电流、时序裕量、掉压和触感 | 可用作行为测试 |
| I²C/PCA9557 | 自定义寄存器模型、地址探测、output-before-config、readback、NACK/卡总线注入 | 电平、上拉、边沿、器件上电时序 | 高价值 spike 项 |
| ST7789 UI | framebuffer 或兼容显示模型可验证页面/状态 | 真实面板初始化、颜色/方向、电气时序、亮度和撕裂 | 有条件可用 |
| TF/SDSPI | 文件语义、无卡/满卡/错误路径、部分 SPI 行为 | 卡兼容性、信号完整性、写入抖动和随机掉电 | 有条件可用 |
| PN532 | 内置示例/行为模型可验证轮询、超时和标签事件 | RF 场、读距、天线、金属影响和标签兼容性 | 仅协议行为 |
| QMI8658A | 自定义寄存器/轨迹模型、轮询与异常路径 | 真实噪声、零偏、轴向、机械耦合 | 仅行为模型 |
| ES7210/ES8311/I²S | 可测试上层状态机和假 PCM 数据 | I²S 时序、slot、模拟麦、播放参考、爆音、AEC/声学 | 不适合硬件链验证 |
| Wi-Fi/AI/OTA | 可对上层网络状态机做 mock 或有限 bench test | C6 无线栈、射频、真实网络长稳和 OTA 可靠性 | 不作为验收依据 |
| 电池/充电/功耗 | 注入 ADC 数值和断电事件 | 电池曲线、充电状态、功耗、温升、brownout | 只能测逻辑 |

## 5. 关键不兼容与成本

### 5.1 ESP-IDF 版本不一致

Velxio 官方资料当前以 ESP-IDF 5.5.4 为主要编译基线，YaKa 使用 6.1。若用 Velxio 自带编译器编译 YaKa，会同时改变 SDK、项目生成逻辑和部分配置，失败时无法判断是 YaKa 缺陷还是仿真平台差异。

**处理原则：**只接受 Raynode IDF 6.1 构建的 merged image 作为候选；平台编译器只用于最小模型和示例，不作为发布构建。

### 5.2 C6 依赖在线服务

公开 README 明确区分了自托管镜像和在线板型，C6 不在公开自托管镜像内。采用它意味着仿真通道受在线服务、账户计划和平台版本更新影响，不能成为唯一 CI 或长期可复现证据源。

### 5.3 外设模型缺口

PCA9557、QMI8658A 可通过自定义 I²C 寄存器模型补足；ST7789 可能通过 framebuffer 模型补足；PN532 可复用示例再验证。ES7210/ES8311 的关键价值在 I²S、模拟采播和时钟关系，而公开自定义芯片 API 没有等价 I²S 模型，投入自研后仍无法逼近真实音频链路。

### 5.4 无线说明存在口径冲突

通用 Wi-Fi 文档描述 ESP32 家族可使用 Wi-Fi；C6 专页又把 Wi-Fi 6/Zigbee/Thread 描述为 bench-only，并重点承诺有线外设。更具体的 C6 页面应优先，直到用 YaKa 镜像完成可重复测试前，不把 C6 网络能力列为可用仿真覆盖。

## 6. 推荐验证架构

```text
                  ┌─ host tests / HAL mock：领域状态机、队列、文件恢复、协议
YaKa 同一源码 ───┼─ Raynode ESP-IDF 6.1：发布构建真值与 merged image
                  ├─ Velxio L0.5：C6 启动、虚拟 IO、UI、I²C 行为、故障注入
                  └─ 实板 L1–L3：电气、音频、射频、电源、交互、长稳
```

Velxio 和 host mock 是互补关系：前者提供可视化连线和开发板运行体验，后者更适合稳定、快速、可完全控制的业务逻辑回归。核心状态机不应依赖 Velxio 才能测试。

## 7. 建议的限时 spike

目标只验证“是否值得接入”，不先建设完整虚拟样机。

1. 在 Raynode 用当前固定容器/ESP-IDF 6.1 构建 YaKa 16 MiB merged flash image；
2. 在独立 `feature/velxio-spike` 分支创建 `simulation/velxio/velxio.toml`、`diagram.json` 和最小自定义 PCA9557 模型；
3. 启动同一镜像，验证 ROM/应用启动、UART 诊断和复位路径；
4. 验证五个 I²C 地址的存在/缺失矩阵，重点观察 PCA9557 先 Output 后 Configuration 及 readback；
5. 注入 NACK、器件缺失、错误 readback 和总线超时，确认系统降级且不重启循环；
6. 若前述稳定，再尝试 LCD framebuffer 与按键交互；音频仅接假 PCM/上层 mock，不建设 codec 电气模型。

### 通过条件

- 不修改发布固件逻辑即可启动同一 merged image；
- I²C/PCA9557 顺序和故障可被确定性观察；
- 场景能在 CI 或脚本中重复执行；
- 单次完整场景稳定、日志可导出并能关联 Git SHA；
- 不产生长期维护的仿真专用业务分支。

### 停止条件

- IDF 6.1 镜像无法运行且只能改用平台 5.5.4 重编；
- C6 在线运行或项目格式无法自动化、频繁受平台更新破坏；
- 自定义 I²C/LCD 模型维护量超过 host mock；
- 需要在生产代码中加入不可接受的 simulator-only 条件分支；
- 团队把仿真结果误当成 L1/L2 验收证据。

## 8. 决策

**建议：有条件试用，暂不纳入主工程依赖。**先做上述 spike，时间预算控制在 1–2 个工程日。通过后，把 Velxio 固定为非阻断的 L0.5 可视化/故障注入通道；未通过则保留本评估和最小实验分支，不继续投入。

置信度：

- Velxio 可用于 C6 基础启动、UART/GPIO 与部分外设行为：**高**；
- 可用自定义模型覆盖 PCA9557/QMI8658A 业务行为：**高**；
- 能无修改运行 YaKa 当前 IDF 6.1 merged image：**中，必须 spike**；
- 能有效验证 YaKa 实际音频、无线、电源与整机体验：**低/否定**；
- 能替代实板 L1–L3：**高置信否定**。

## 9. 主要来源

- [Velxio GitHub 仓库](https://github.com/davidmonterocrespo24/velxio)
- [ESP32-C6 Simulator](https://velxio.dev/esp32-c6-simulator/)
- [ESP32 Emulation](https://github.com/davidmonterocrespo24/velxio/blob/master/docs/ESP32_EMULATION.md)
- [Custom Chips Overview](https://velxio.dev/docs/custom-chips/overview/)
- [Custom Chips API](https://velxio.dev/docs/custom-chips/api/)
- [Velxio Examples](https://velxio.dev/examples/)
- [CI Configuration](https://velxio.dev/docs/ci/velxio-toml/)
- [Plans and Limits](https://velxio.dev/docs/getting-started/plans/)

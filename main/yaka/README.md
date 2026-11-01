# HiYaKa 产品内核

本目录承载 HiYaKa 独有的产品协调与领域服务，不复制小智协议、音频引擎或网络栈。当前 PH2 只建立可宿主测试的产品内核和端口合同；真实硬件适配仍由板级目录负责，产品功能仍是 L0 状态。

## 1. 单写入者状态模型

`ProductController` 是产品状态的唯一写入者。驱动、ISR、网络回调和模拟端点只投递 `ProductEvent`，不能直接修改页面或产品状态。

状态采用正交维度，避免把所有组合膨胀成一个枚举：

- `PowerPhase`：启动、运行、受控收尾、等待硬关断、恢复；
- `ForegroundMode`：启动、首页、配网、对话、录音、NFC、设置、OTA、错误；
- `Connectivity`：离线、配网、连接中、在线、服务异常；
- `DialoguePhase`：只读映射小智会话阶段；
- `UpdatePhase`：等待确认、下载、待重启和失败。

相同 `correlation_id` 的事件只处理一次；当前状态下重复到达的幂等事件是 no-op。超时事件和非法转换有稳定错误码，错误页必须能够通过 `Recover` 或 `Cancel` 回到首页。

## 2. 可替换端口

`product_ports.h` 定义显示、输入、音频、存储、网络、NFC、IMU、反馈、电源和 OTA 十类端口。产品内核只依赖这些接口，不包含 GPIO、I²C 地址或具体器件类。

`simulated_product_ports.h` 提供无实板模拟实现和调用计数，可注入音频、存储、网络、NFC、电源或 OTA 拒绝，用于验证异常和回滚路径。模拟通过不代表真实外设通过。

## 3. 有界队列合同

| 队列 | 容量 | 满载策略 | 超时策略 | 观测字段 |
|---|---:|---|---|---|
| 产品事件 | 32 | 拒绝最新事件，由生产者获知失败 | 事件携带绝对 deadline；过期后丢弃 | 当前/峰值深度、接受、弹出、拒绝 |
| 音频块描述符 | 12 | 录音分流默认丢弃最旧块，保持采集前进 | 由序号和时间戳检测间断 | 丢弃最旧块、峰值深度 |
| 写盘请求 | 8 | 拒绝最新请求，录音服务必须进入显式错误/收尾 | 服务层设置操作超时 | 拒绝数、峰值深度 |

`BoundedQueue` 使用调用方可确定的 `std::array` 容量，不以 `deque`/`vector` 扩容积压。当前实现用 mutex 支持多生产者；接入 ESP-IDF 任务时仍需验证延迟和栈，不允许从 ISR 直接调用该 C++ 队列。

## 4. 能力健康

每项能力使用同一套状态：`unavailable / initializing / ready / degraded / failed`。`degraded` 仍可由产品层有条件使用；`failed` 只在当前前台任务依赖该能力时进入错误页。IMU 等非关键能力单独失败不得让首页失去返回路径。

能力集合：显示、输入、采集音频、播放音频、存储、网络、NFC、IMU、反馈、电源、OTA。

## 5. 首版错误码

| 范围 | 错误码 | 含义 |
|---|---:|---|
| 队列 | 1001 | 产品事件队列已满 |
| 队列 | 1002 | 事件超过 deadline |
| 状态 | 1101 | 当前状态不允许该转换 |
| 状态 | 1102 | 当前操作超时 |
| 能力 | 1201 | 所需能力不可用 |
| 能力 | 1202 | 运行中能力失败 |
| 端口 | 1203 | 端口拒绝启动或提交 |
| 网络 | 2001 | 网络能力不可用 |
| 音频 | 3001 | 音频采集或播放不可用 |
| 存储 | 4001 | 存储不可用或提交失败 |
| NFC | 5001 | NFC 不可用 |
| OTA | 6001 | OTA 能力不可用 |

错误码只描述稳定类别，不把 ESP-IDF 返回值、器件寄存器值或用户数据编码进公开诊断。

## 6. 诊断字段

| 字段 | 来源 | 用途 |
|---|---|---|
| `snapshot.generation` | ProductController | 判断状态是否实际变化 |
| 五个正交状态维度 | ProductController | UI、日志和测试共享产品真值 |
| `active_error` / `last_error` | Controller | 当前用户可见错误与最近内部错误 |
| handled/duplicate/expired/rejected | ControllerMetrics | 幂等、过期和非法事件审计 |
| capability_failures | ControllerMetrics | 所需能力缺失次数 |
| depth/peak/accepted/popped/rejected/dropped | QueueMetrics | 队列容量与满载行为 |
| capability state/error/changed_at | CapabilityRegistry | 外设健康与降级依据 |

后续实板诊断还必须增加内部堆最小值、最大连续块、任务栈高水位、音频丢帧、UI 刷新和复位原因；PH2 不伪造这些数据。

## 7. 测试

```bash
cmake -S tests/yaka_host -B /tmp/hi-yaka-host
cmake --build /tmp/hi-yaka-host
ctest --test-dir /tmp/hi-yaka-host --output-on-failure
```

覆盖固定容量满载策略、多生产者、启动、对话、录音提交、取消、超时、重复事件、OTA 确认、受控关机和非关键能力失败。

## 8. 设计证据

- [ESP-IDF 6.1 FreeRTOS 文档](https://docs.espressif.com/projects/esp-idf/en/v6.1/esp32c6/api-reference/system/freertos_idf.html)：确认任务/同步原语属于 IDF 运行时边界；
- [FreeRTOS Kernel `queue.h`](https://github.com/FreeRTOS/FreeRTOS-Kernel/blob/main/include/queue.h)：静态队列由调用方提供存储、容量固定，满载返回明确结果；
- [Zephyr State Machine Framework](https://docs.zephyrproject.org/latest/services/smf/index.html)：参考集中状态执行、转换和退出动作的组织方式；
- [小智 DeviceStateMachine](https://github.com/78/xiaozhi-esp32/blob/main/main/device_state_machine.cc)：保持上游会话状态独立，并继承重复转换 no-op 的行为。

这些资料只支持软件结构决策；HiYaKa 的硬件接口仍以活体 EDA、封存审计和本仓库硬件契约为准。

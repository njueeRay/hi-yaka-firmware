# HiYaKa ES7210 控制面与麦克风采样 A/B 实板证据

> 所属问题：`YAKA-0006 / HIYAKA-AUDIO-INC-001`<br>
> 记录日期：2026-11-20<br>
> 状态：A/B、C1 热启动、C2–C4、D/E、历史 I²C 隔离重放、身份核验、首次 probe、no-PCA-write、25 kHz 窗口、极简首笔读、bus reset、顺序互换、安全写门禁、25/10 kHz 突发只读、旧版 I²C 驱动、GPIO 软件 I²C、受限 MCLK 候选、`0x12` 可逆写回、官方驱动控制序列、MCU 时钟/DMA 预读、PCNT 时钟自检、TX+RX 静音、16→24 kHz、24 kHz 的 16→32 bit 时隙、RX TDM4、16 kHz 启用后 90 秒状态轮询、启用前 90 秒预供时、24 dB 增益单因素、I²C 两种只读路径交叉、关闭硬件 I²C 内部上拉及 input-only wrapper 路径包均已实测。U12 曾应答并完成多寄存器写入与读回，但状态仍为 power-down；MCU GPIO 时钟存在，U12 芯片脚未测，DMA 预读全为 `-1`，后续 PCM 窗口因门禁失败跳过，麦克风功能尚未通过。

## 1. 本轮要回答的问题

1. ES7210 的 `0x41` 地址应答之后，初始化寄存器写入和关键状态读回能否稳定完成？
2. 若完整初始化成功，GPIO13 是否收到有效的 I²S 数据，原始 PCM 是否随声音变化？
3. 若 A/B 结果不同，差异来自事务格式、器件前序状态、总线共享负载，还是其他因素？只记录实际可分离的结论。

## 2. 测试边界与身份

| 项目 | 本轮记录 |
| --- | --- |
| 样机编号、返修状态 | 样机编号待补；主 agent 确认 L7 已隔离、温度正常。以遮蔽的 BASE MAC 区分：历史样机 `…:d6:b4`，本轮样机 `…:d6:c0`；后者与 2026-11-12 屏幕/麦克风日志中的样机相同 |
| MASTER、USB、电池、扬声器连接状态 | USB 串口 COM5 已连接；其余接线状态待补 |
| L7 是否物理隔离、PA_EN 默认状态 | L7 已隔离；诊断镜像启动日志 `PA_EN=0`；ROM/烧录期默认状态未由本轮日志证明 |
| `AU_3V3` 实测 | 用户在 MASTER ON 时量得 C25/C26 非地端对 GND 约 3.3 V；U12 近端各供电脚未测 |
| U12 附近去耦点实测 | 用户确认 C83.2 与 C87/C88.2 两处均约 3.3 V；仅证明测点有电压，不能代替 U12 每个焊脚接触、电源完整性或上电时序验证 |
| 地址绑带与地桥实测 | 用户在上电时测得 R61.2/AD0=3.3 V、R62.2/AD1=0 V；完全断电后测得 R60 两端 0 Ω。三个测点符合当前设计值，尚未证明 U12 焊脚接触或 I²C 事务健康 |
| 测量后板卡状态 | R60 断电电阻测量后曾主动断电；随后重新上电完成本轮诊断。本文不代表实时 MASTER/COM 状态 |
| ES7210 / I²C / I²S 引脚 | 本轮日志：MCLK=GPIO0、BCLK=GPIO3、WS=GPIO12、DIN=GPIO13；先后测试 16/24 kHz、16-bit 有效样本及 16/32-bit 时隙；I²C 以 `0x41`（日志写地址 `0x82`）访问，GPIO10/11 待按镜像配置复核 |
| 编译环境、Git commit/工作树 | 启动日志报告 ESP-IDF v6.1；独立诊断工程的 commit/工作树待记录 |
| 烧录镜像名称、SHA-256、Flash 偏移 | 第一版诊断镜像 SHA-256 `0C0E3665B1EF8C0C4CA93BC8ADD2995A5B6E960E56A3D345949A89CAB63EBB22`；`ota_0` 偏移 `0x20000`，烧录哈希校验成功；镜像文件名待记录 |
| 当前与恢复镜像名称、SHA-256 | 烧录前 `ota_0` 备份 `ota0-before.bin`，SHA-256 `2B2BF4939DB4BA60A72465792C55F1B37BAB1D51ABBC65ABEBFFB8F2CE505719`；恢复是否执行待记录 |

### 2.1 样机身份与历史状态的边界

| 原始记录 | 样机标识（BASE MAC 后两字节） | ES7210 状态及数据 | 解释范围 |
| --- | --- | --- | --- |
| 早期 `lab-i2s-edges/serial-regstate.log`：110、165、185 行 | `…:d6:b4` | `0x0B=0x02`（normal）；I²S RX 1024 B 全 0 | 与本轮 `…:d6:c0` 是**不同物理板**，不能用作本轮同板 A/B；只能说明另一块板曾在该固件流程进入 normal，但仍无有效 PCM |
| 2026-11-12 `lcd-20261112-full-boot.log`：144、184–229 行 | `…:d6:c0` | ACT_B 启动后 `0x0B=0x02`；RX 1024 B 全 0，GPIO13 20 ms 窗口 0 个上升沿；16 kHz、16-bit 数据、16-bit 时隙、双声道，RX/TX 均启用 | **同一物理板**曾进入 normal，但数据面仍不工作；与本轮固件/访问顺序不同，不能据当前 `0x0B=0x00` 判为该板永久无法进入 normal |
| 本轮 Z/AA/AB 独立诊断及当前 esptool `current-board-read-mac.log` | `…:d6:c0` | 完整控制调用和关键寄存器读回后，Z/AA/AB 两次复位均 `0x0B=0x00`；Z/AA DMA 512 词全 `-1`，AB TDM DMA 1024 词全 `-1`；本轮 MCU 时钟有计数 | 本轮 esptool BASE MAC 与 11/12 同板串口标识一致，身份有独立日志；当前状态失败是真实观察，但单凭状态差异无法区分固件时序、寄存器配置、电气状态或样机期间变化 |

11/12 日志在约 4.32 秒启用输出、21.301 秒记录输出 enable 关闭，直到 88.041 秒 ACT_B 启动录音才重新配置；这**不能证明时钟在此前连续驻留约 84 秒**。本轮 enable 后 90 秒轮询不是历史上电/录音时序复刻，“先预时钟再 enable”需独立试验。11/12 同板记录的 `0x00/01/02/06/08/11/12` 与本轮读值一致；其 `0x43/44=0x18`，本轮官方 enable 最终读值为 `0x10`。因此两次**并非完全相同配置的严格 A/B**。两块板都曾读到 normal 且都无有效 PCM，使“历史 `0x0B=0x02` 已证明麦克风功能正常”不成立；同板 `…:d6:c0` 曾到 normal，也削弱“当前板 U12 永久硬件损坏导致必然 power-down”的直接推断。物理时钟/数据线与具体根因仍未验证。公开文档仅保留遮蔽标识，完整 MAC 留在不发布的原始日志。

## 3. 控制面 Gate

**判据：**地址 ACK 仅证明总线存在应答；必须记录 ES7210 实际写入、关键寄存器读回，以及初始化结束时的状态。单次成功不足以证明冷启动稳定。

| 测试 | 前置状态和事务格式 | 地址探测 | 首个失败寄存器/错误码 | 初始化完成 | `0x0B` 读回 | 重复次数 | 证据文件 |
| --- | --- | --- | --- | --- | --- | --- | --- |
| A：现有驱动初始化 | 第一版独立镜像，PA_EN=0；无 LCD/PCA/PN532/ES8311 访问，寄存器写日志为 `single-stop` | 本轮未单独计数，不能把先前 16/16 套用到本次 | 首笔 `reg=0x00 value=0xFF`，`ret=269`；九笔写入全部失败 | 否；`ES7210: Open fail` | 读回 `ESP_ERR_INVALID_RESPONSE`，值 `0x7C` 不能当作寄存器有效值 | 第一轮日志 1 次；第二轮日志文件目前为空 | `serial-gate1-run1.log` |
| B：仅移除初始化前的三次寄存器预读 | 与 A 同镜像主流程、相同 PA_EN 和 I²S 时钟；启动标记 `no-preread` | 本轮未单独计数 | 首笔同为 `reg=0x00 value=0xFF ret=269`；九笔写入全部失败 | 否；`ES7210: Open fail` | `0x0B` 读回仍为 `ESP_ERR_INVALID_RESPONSE` | 第一轮日志 1 次 | `serial-gate1-B-run1.log` |
| C1：驱动外原始写事务，热启动 | 已烧录；PA_EN=0，I²S RX 时钟已启动；不运行编解码驱动初始化或 DMA；单笔 `addr7=0x41 payload=[00 FF] single-stop` | 未单独探测 | `ESP_ERR_INVALID_RESPONSE`，日志原码 `0x108` | 未尝试；C1 仅测首笔写入 | 未读 | 1 次热启动 | `serial-C1-run1.log` |
| C1：全断电冷启动 | 主 agent 已启动串口守候，等待用户完成断电重上电 | 未测 | 未测 | 未测 | 未测 | 0 次 | 待记录 |
| C2：仅一次地址探测 | 当前上电状态，PA_EN=0，I²S RX 时钟已启动；首个且唯一 I²C 操作为 `i2c_master_probe(0x41)` | `ESP_ERR_NOT_FOUND`，原码 `0x105` | 未执行寄存器读写 | 未执行 | 未读 | 1 次 | `serial-C2-run1.log` |
| C3：同总线地址对照 | 当前上电状态；仅 probe，三轮变换地址顺序；无寄存器访问/PN532/DMA | 第 0 轮：`0x41/0x18/0x19` 无应答、`0x6B` 应答；第 1、2 轮：`0x19/0x6B` 应答、`0x41/0x18` 无应答 | 不适用 | 不适用 | 不适用 | 3 轮 | `serial-C3-run1.log` |
| C4：上电后定时地址探测 | 约 0.15–30.2 秒，间隔约 2 秒；仅 probe，不访问寄存器或 DMA | `0x41/0x40/0x42/0x43` 各 0/16；`0x6B` 16/16；`0x18` 15/16，约 2 秒起应答；`0x19` 1/16 | 不适用 | 不适用 | 不适用 | 16 轮 | `serial-C4-run1.log` |
| D：关闭 I²S 时钟的定时探测 | 与 C4 同为地址探测；本镜像不启动 I²S 时钟 | `0x40/0x41/0x42/0x43` 各 0/16；`0x6B` 16/16；`0x18` 15/16；`0x19` 4/16 | 不适用 | 不适用 | 不适用 | 16 轮 | `serial-D-run1.log` |
| E：地址速率 ABBA | 不启动 I²S，不发送寄存器数据；同轮交替探测 `0x41` 与 `0x6B`；100→25→25→100 kHz | 四阶段 `0x41` 均 0/16；`0x6B` 均 16/16 | 不适用 | 不适用 | 不适用 | 4 阶段 × 16 次/地址 | `serial-E-reset-run1.log` |
| F：历史 `diagnostics/i2c-isolation` r3 同板重放 | 100→25→25→100 kHz；单独 combined read、分开指针/数据事务与多器件交替访问 | mixed `0x41` 四阶段均 16/16 ACK | 未执行 ES7210 完整初始化；单独 combined read 四阶段均 0/128，split pointer 在 100 kHz 为 2/16、25 kHz 为 16/16 | 未尝试 | mixed `0x0B` 读事务四阶段各 128/128 返回成功；未记录有效寄存器值 | 四阶段，详见下文 | `serial-historical-replay-run1.log` |
| G：同流程加入身份与空地址对照 | F 的四阶段顺序；phase 0/3 在 mixed 末次 `0x0B` 成功后立即读 `0x3D/0x3E`；phase 3 结束后单独 probe | mixed `0x41` 四阶段各 16/16 ACK；phase 3 mixed 后单独 probe `0x41/0x40/0x42/0x43/0x45/0x50/0x55` 各 0/16 | 未执行完整初始化写入 | 未尝试 | phase 0/3 `0x3D=0x72`、`0x3E=0x10` 均 `ESP_OK`；mixed `0x0B` 四阶段各 128/128，值均 `0x00`；split 在 100 kHz 读 `0xFF`、25 kHz 读 `0x00` | 四阶段，详见下文 | `serial-identity-run1.log` |
| H：启动首笔地址探测 | G 的基本隔离顺序；在首笔寄存器事务前增加一次 `0x41` probe | 首笔 `ESP_ERR_NOT_FOUND (0x105)`；phase 0 mixed 后 `0x41` 15/16 成功、另 1 次为其他错误；phase 1–3 mixed 后各 16/16 | 未执行完整初始化写入 | 未尝试 | mixed `0x0B` 四阶段各 128/128 事务成功；本镜像未记录数据值 | 单次启动，四阶段 | `serial-first-probe-run1.log` |
| I：no-PCA-write 单因素对照 | G 的 mixed 顺序，仅不执行 PCA9557 安全锁存写；100→25→25→100 kHz | mixed `0x41` 四阶段各 16/16 ACK；phase 3 mixed 结束后单独 `0x41` probe 1/16 ACK | 未执行 ES7210 完整初始化写入 | 未尝试 | 100 kHz phase 0/3 mixed `0x0B` 各 126/128 成功，其中 125 次 `0xFF`、1 次 `0x00`；`0x3D=0xFF`、`0x3E=0x10`；25 kHz phase 1/2 mixed `0x0B` 各 128/128 且全 `0x00` | 单次启动，四阶段 | `serial-no-pca-run1.log` |
| J：25 kHz 身份窗口 | 在 I 的 phase 1/2 mixed 循环末，先用 ES7210 25 kHz 设备句柄读 `0x3D/0x3E`，再用总线默认 100 kHz 对空地址 `0x55` probe，随后再读相同身份寄存器 | 空地址 `0x55` 两阶段均 `ESP_ERR_NOT_FOUND (0x105)`；之后 mixed `0x41` 各 16/16 ACK | 未执行完整初始化写入 | 未尝试 | phase 1/2 首次身份读均 `0x72/0x10` 且 `ESP_OK`；空地址 probe 后两寄存器均 `ESP_ERR_INVALID_RESPONSE`，值无效；mixed `0x0B` 仍各 128/128 且全 `0x00` | 两个 25 kHz 阶段 | `serial-25k-gate-run1.log` |
| K：25 kHz 极简首笔读 | 仅 ES7210 25 kHz 读，初始指针写和 16 轮 `0x3D/0x3E/0x0B`；轮间约 50 ms；末尾读 QMI `0x00` 后再读两身份寄存器；无 PCA/PN532/I²S/ES 写入 | 未做地址 probe | 首笔 ES `0x0B` 指针事务 `ESP_ERR_INVALID_RESPONSE (0x108)`；未尝试初始化写入 | 未尝试 | round 0 三读均 NACK；round 1–15 每轮首个 `0x3D` NACK，随后 `0x3E=0x10`、`0x0B=0x00` 均成功；末尾 QMI `0x00=0x05` 成功后 ES `0x3D=0x72`、`0x3E=0x10` 均成功 | 一次启动，16 轮 | `serial-first25k-run1.log` |
| L：25 kHz 空地址 probe 后 bus reset | 与 J 相同的 phase 1/2 身份窗口，在默认 100 kHz 的 `0x55` probe 后立即调用 `i2c_master_bus_reset`，再以 25 kHz 读身份 | `0x55` 两阶段均 `ESP_ERR_NOT_FOUND (0x105)`；`bus_reset` 两阶段均 `ESP_OK` | 未执行 ES7210 初始化写入 | 未尝试 | phase 1/2 probe 前 `0x3D=0x72`、`0x3E=0x10`；reset 后两寄存器均 `ESP_ERR_INVALID_RESPONSE`；随后 mixed `0x0B` 各 128/128 且全 `0x00` | 两个 25 kHz 阶段 | `serial-probe-reset-run1.log` |
| M：25 kHz 极简读顺序互换 | 与 K 相同的只读条件和约 50 ms 轮间隔，仅把每轮 `0x3D→0x3E→0x0B` 改为 `0x0B→0x3E→0x3D` | 未做地址 probe | 首笔指针 `0x0B` NACK；未尝试初始化写入 | 未尝试 | round 0 三笔全 NACK；round 1–15 每轮首笔 `0x0B` 均 NACK，其后的 `0x3E=0x10`、`0x3D=0x72` 均成功；末尾 QMI `0x00=0x05` 后 ES 身份两读成功 | 一次启动，16 轮 | `serial-order-swap-run1.log` |
| N：25 kHz 安全写前门禁 | M 的读顺序后，先要求身份与原寄存器连续读稳定，只有通过才尝试受控写入及恢复 | 未做地址 probe | POST_QMI 身份 `0x72/0x10` 成功后，门禁两轮身份读和 `0x12` 读均 `ESP_ERR_INVALID_RESPONSE`；日志明确 `WRITE_TEST skipped`，**没有写入测试寄存器** | 否 | round 1–15 `0x0B` 作为每轮首笔持续 NACK；未取得写后读回 | 一次启动 | `serial-safe-write-run1.log` |
| O：25 kHz 突发只读 | 16 轮预读后，读 QMI `0x00`，连续 8 次读 ES7210 `0x3D/0x3E/0x12`，不写设备配置 | 未做地址 probe | 不适用 | 未尝试 | 8 次中 `0x3D` 一次返回错误值 `0x77`，`0x12` 首读 NACK、次读 `0x00`；其余 `0x3D=0x72`、`0x3E=0x10` 见日志 | 一次启动，8 笔 burst | `serial-burst-read-run1.log` |
| P：10 kHz 突发只读 | 与 O 类似的只读流程，ES7210 设备句柄降为 10 kHz | 未做地址 probe | 不适用 | 未尝试 | burst 中 `0x3D` 三次均 NACK，`0x3E` 三次均 `0x10`；`0x12` 首读 NACK、次读 `0x00` | 一次启动，8 笔 burst | `serial-10k-run1.log` |
| Q：旧版 I²C 驱动 25 kHz 只读 | 使用 `driver/i2c.h` 的 `i2c_param_config`、`i2c_driver_install` 与 `i2c_master_cmd_begin`；读寄存器采用 repeated START，不写芯片配置；两次复位重放 | 未做地址 probe | 首笔 `0x0B` 指针事务 `ESP_FAIL`；未执行配置写入 | 未尝试 | 两次日志一致：burst 第二次 `0x3D=0xFF` 虽返回 `ESP_OK` 但值错误；`0x12` 首读 `ESP_FAIL`、次读 `0x00`。round 1–15 每轮约 50 ms 空闲后首笔 `0x0B` 15/15 失败；后续 `0x3E=0x10` 14/15 成功（round 7 超时），`0x3D=0x72` 15/15 成功 | 两次复位，各 16 轮及 8 笔 burst | `serial-legacy-run1.log`、`serial-legacy-run2.log` |
| R：GPIO 软件 I²C 只读 | GPIO10/11 开漏模拟 SDA/SCL，约 20 μs 半周期；不启动 I²S，不用硬件 I²C 控制器，不写芯片配置 | 未做地址 probe；idle SDA/SCL=`1/1` | 未执行配置写入 | 未尝试 | round 1–15 约 50 ms 空闲后首笔 `0x0B` 均 stage 2（地址写 NACK），后续 `0x3E=0x10`、`0x3D=0x72` 均 15/15；burst n1 stage 5（读地址 NACK），n4/n7 stage 2（写地址 NACK），n6 `0x12=0x00`；其余成功身份值正确 | 一次启动，16 轮及 8 笔 burst | `serial-bitbang-run1.log` |
| S：GPIO 软件 I²C + I²S 静音发送候选 | 在 R 基础上启动 I²S TX 并写入静音缓冲；两次复位；**审计 NO-GO**：`feed_silence` 写失败路径无退避，且无物理 MCLK 波形测量；实验后已恢复 R 镜像 | 未做地址 probe；idle SDA/SCL=`1/1` | 未执行配置写入 | 未尝试 | 两次日志均记录静音缓冲写成功计数 58；round 1–15 首笔 `0x0B` 均 stage 2，后续身份 `0x10/0x72` 均成功；burst n1 与 n6 为 stage 5（读地址 NACK），其他身份读成功，n7 `0x12=0x00`。不能据此证明实际 MCLK 波形或其因果作用 | 两次复位，各 16 轮及 8 笔 burst | `serial-mclk-run1.log`、`serial-mclk-run2.log` |
| T：GPIO 软件 I²C 单次 `0x12` 写回 | 预先要求身份 `0x72/0x10` 与 `0x12=0x00` 两轮读门禁，随后尝试 `0x12=0x20` 并恢复 `0x00` | 未做地址 probe | 门禁通过；首次 toggle 写 stage 2（写地址 NACK），未见 `0x20`；恢复 `0x00` 写及读回成功；`WRITE_TEST pass=0` | 否 | 最终身份仍 `0x72/0x10`；本轮没有证明写入 `0x20` 成功 | 一次启动 | `serial-writeback-run1.log` |
| U：GPIO 软件 I²C 有界重试 `0x12` 写回 | T 的门禁与目标值，允许写及读有限次重试；两次复位 | 未做地址 probe | 两次均：toggle 首笔 stage 2 NACK，第二笔 stage 0 ACK，读回 `0x20`；restore 首笔 stage 3（寄存器地址 NACK），第二笔 stage 0 ACK，读回 `0x00`，最终身份 `0x72/0x10`；`WRITE_TEST pass=1` | 否 | 仅该受控寄存器可逆写回得到两次复现；NACK 仍在，未运行完整初始化 | 两次复位 | `serial-writeback-retry-run1.log`、`serial-writeback-retry-run2.log` |
| V：官方 ES7210 驱动完整控制序列隔离 | `esp_codec_dev 1.6.2` 官方驱动，仅临时补丁使读改写在预读失败时退出；GPIO 软件 I²C 适配层有界重试；仅控制面，不运行 PCM/I²S，PA_EN=0；两次复位 | 预读身份 `0x72/0x10` 两次通过；未单独 probe | 两次 `constructed=1`、`set_fs=0`、`enable=0`；均 59 次成功写，分别 69/70 次尝试，terminal=0；关键寄存器读回与最后写值一致 | 否 | 两次 `0x0B=0x00`、`normal=0`，仍为 power-down；`verified=0`。日志将 `0x43/0x44=0x10` 与错误预期 `0x1A` 比较而标 semantic=0；核官方时序后实际最终值应为 `0x10`，该两项不构成故障证据 | 两次复位 | `serial-codec-retry-run1.log`、`serial-codec-retry-run2.log` |
| W：官方控制序列 + MCU 时钟 + DMA 预读 | 延续 V 的临时补丁与有界重试，启用 I²S RX 时钟，软件读取 MCU 的 GPIO3/BCLK、GPIO12/WS 边沿，并做一次 1024 B DMA primer；仅状态门禁通过才运行后续三窗 PCM 候选 | 未单独 probe | 两次 `constructed=1`、`set_fs=0`、`enable=0`；关键寄存器含修正后的 `0x43/44=0x10` 全部与预期及最后写值相符；59 次成功写，65/72 次尝试，terminal=0 | 是，仅 512 词 primer；三窗 PCM 候选跳过 | 两次 MCU 端 100000 轮采样读得 BCLK 边沿 74236/74235、WS 边沿各 2322；U12 芯片脚未测。primer `ESP_OK`、1024 B，但左右各 256 词全 `-1`、变化 0，`nonzero=512` 只是 `-1 ≠ 0`，不代表有效音频；`0x0B` 在 RX 前后均 `0x00`、normal=0，`verified=0` | 两次复位 | `serial-clock-pcm-run1.log`、`serial-clock-pcm-run2.log` |
| X：同镜像 PCNT 观测 MCU 输出 GPIO | 在 W 控制/DMA 门禁后，用 ESP32-C6 PCNT 约 2 ms 窗口读取 MCU 输出 GPIO12/WS、GPIO3/BCLK、GPIO0/MCLK 的上升沿；两次复位 | 未单独 probe | 关键寄存器读回均符合预期；两次 `0x0B` 前后均 `0x00`、normal=0 | 是，仅 512 词 primer；三窗候选仍跳过 | 两次 PCNT：WS 均 33 边沿/2016 μs≈16.369 kHz；BCLK 均 1025 边沿/2003/2002 μs≈511.732/511.988 kHz；MCLK 均 8201 边沿/2002 μs≈4.096403 MHz。MCU GPIO 输入路径上的计数**不等于** U12.5/9/10 芯片焊盘实测；DMA 左右词仍全 `-1`、无变化 | 两次复位 | `serial-mclk-pcnt-run1.log`、`serial-mclk-pcnt-run2.log` |
| Y：TX+RX 16 kHz 静音单因素 | 在 X 的时钟/控制/primer 诊断上加 I²S TX 静音馈送，DOUT 未接物理引脚；两次复位 | 未单独 probe | 关键寄存器读回一致，两次 `0x0B` 前后均 `0x00`、normal=0 | 是，仅 512 词 primer；三窗候选跳过 | 两次均 TX `done=1`、完整写 71 次、失败写 58 次、完整写字节合计 72704；因此不是持续全成功馈送。MCU PCNT WS≈16.35/16.36 kHz、BCLK≈511.99 kHz、MCLK≈4.094 MHz，与 X 近似；DMA 左右词仍全 `-1`、无变化 | 两次复位 | `serial-tx-silence-run1.log`、`serial-tx-silence-run2.log` |
| Z：TX+RX 24 kHz 采样率对照 | 在 Y 基础上仅把 I²S 时钟配置和 codec `set_fs` 采样率从 16 改为 24 kHz；其他功能代码差异仅为日志与注释；两次复位 | 未单独 probe | 身份 `0x72/0x10`、关键寄存器读回符合预期；两次 `0x0B` 前后均 `0x00`、normal=0；控制面 transient/terminal 均为 0 | 是，仅 512 词 primer；三窗候选跳过 | 两次 TX `done=1`、完整写 106 次、失败写 57 次，不能视为持续全成功。MCU PCNT WS≈23.81 kHz、BCLK≈767.73 kHz、MCLK≈6.14 MHz，随采样率变化；DMA 左右词仍全 `-1`、无变化 | 两次复位 | `serial-tx-24k-run1.log`、`serial-tx-24k-run2.log` |
| AA：24 kHz / 32 bit 时隙对照 | 在 Z 基础上仅将 I²S slot width、WS width 调为 32 bit；采样率和有效数据位仍为 24 kHz、16 bit；两次复位 | 未单独 probe | 身份 `0x72/0x10`、关键寄存器读回符合预期；两次 `0x0B` 前后均 `0x00`、normal=0；控制面 transient=2/3、terminal=0 | 是，仅 512 词 primer；三窗候选跳过 | 两次 TX `done=1`、完整写 107 次、失败写 56 次。MCU PCNT WS≈23.83 kHz、BCLK≈1.534 MHz、MCLK≈6.144 MHz；BCLK 约翻倍而状态/DMA 未改善，左右词仍全 `-1`、无变化 | 两次复位 | `serial-tx-24k-slot32-run1.log`、`serial-tx-24k-slot32-run2.log` |
| AB：24 kHz / RX TDM4 对照 | 在 AA 的 TX 标准 I²S 2×32 基础上，仅将 RX 由标准双槽 32 bit 改为 TDM 四槽 16 bit，帧长均为 64 bit；ES7210 仍选 MIC1+2、`0x12=0x00`；两次复位 | 未单独 probe | 身份 `0x72/0x10`、关键寄存器读回符合预期；两次 `0x0B` 前后均 `0x00`、normal=0；控制面 transient=3/3、terminal=0 | 是，仅 2048 B / 1024 词 TDM primer；三窗 PCM 验收跳过 | TX 两次各完整写 108、失败写 57；MCU PCNT WS≈23.82/23.83 kHz、BCLK≈1.536 MHz、MCLK≈6.144 MHz。TDM primer 1024 词全部 `-1`、变化 0；`nonzero=1024` 仅由负一造成 | 两次复位 | `serial-tx-24k-tdm4-run1.log`、`serial-tx-24k-tdm4-run2.log` |
| AC：16 kHz 启用后持续读状态 90 秒 | 回到 Y 的 16 kHz、标准双槽 16-bit、MIC1+2、TX 静音配置；初始控制/DMA 诊断后不再改 ES7210 配置，每 5 秒双读确认 `0x0B`，至 90 秒；单次复位 | 未单独 probe | 初始官方控制序列成功，`0x0B` 前后 `0x00`；5/20/60/90 秒身份 `0x72/0x10` 和 `0x00/01/06/12=0x41/0x34/0x00/0x00` | 是，仅初始 512 词 primer；三窗 PCM 验收跳过 | 18 次 5 秒间隔状态读均 valid=1、`0x00`；90 秒末 TX task active=1，MCU PCNT WS≈15.984 kHz、BCLK≈512.487 kHz、MCLK≈4.0964 MHz；TX 完整写 5647、失败写 **4519**，不能称连续成功馈送。初始 DMA 仍全 `-1`；未在 90 秒末另采 PCM | 单次启动，读至 90 秒 | `serial-90s-run1.log` |
| AD：16 kHz enable 前预供时 90 秒 | 16 kHz 标准双槽 16-bit、MIC1+2、TX 静音；codec 构造后、`set_fs/enable` 前维持 MCU I²S 时钟约 90.125 秒，不发控制事务；随后调用官方配置/enable，并读状态至 enable 后约 6 秒；单次复位 | 未单独 probe | 预供时前身份 `0x72/0x10`；配置/enable 后关键寄存器读回匹配；`0x0B` 在 RX 前、RX 后与 enable 后 1–6 秒均读得 `0x00`、normal=0，控制面 terminal=0 | 是，仅 1024 B / 512 词 primer；三窗 PCM 验收跳过 | 预供时起止 MCU PCNT WS≈15.9 kHz、BCLK≈512 kHz、MCLK≈4.094/4.096 MHz，TX task active=1；DMA 左右词仍全 `-1`。TX 完整写 6034、失败写 **4828**，不代表连续成功馈送 | 单次启动，90.125 秒预供时 | `serial-preclock90-run1.log` |
| AE：16 kHz 增益 24 dB 单因素 | 在 Y 的 16 kHz、标准双槽、MIC1+2、TX 静音基础上，enable 后显式调用 `codec->set_mic_gain(24.0f)`；与历史同板增益读值对齐，但并非旧版完整 wrapper A/B；两次复位 | 未单独 probe | 两次增益调用返回 0，`0x43/0x44` 均读回 `0x18`；`0x0B` 在 RX 前后均有效读到 `0x00`、normal=0；关键控制寄存器读回匹配 | 是，仅 1024 B / 512 词 primer；三窗 PCM 验收跳过 | 两次 DMA 左右各 256 词固定 `-1`、变化 0；TX 各完整写 71 次、失败写 58 次，不能称连续成功馈送。增益单变量未恢复状态或 PCM | 两次复位 | `serial-gain24-run1.log`、`serial-gain24-run2.log` |
| AF：I²C 只读路径交叉与静态快照 | 同一镜像中先用 GPIO 开漏软件 I²C 25 kHz 读身份、状态及 `0x14/0x15`、`0x47–0x4C`；释放引脚后用硬件 I²C 100 kHz combined、25 kHz split 分别读身份/状态；两次复位 | 未单独 probe | 两次软件 25 kHz 与硬件 25 kHz split 均读 `0x3D/0x3E=0x72/0x10`、`0x0B=0x00`。软件快照两次均为 `0x14/15=0x00`、`0x47–4A=0x08`、`0x4B=0x00`、`0x4C=0xFF`；硬件 100 kHz combined 多次 `ESP_ERR_INVALID_RESPONSE`，第二轮仅首笔 `0x3D=0x72` 成功，其失败返回的占位值不得当寄存器值 | 是，仅 1024 B / 512 词 primer；三窗 PCM 验收跳过 | 两次 DMA 左右词仍固定 `-1`；可靠的两条读路径在本镜像条件下同指向 power-down，静态快照未见已知差异。100 kHz combined 的时序/事务失败需独立分析，不能推为永久不支持 100 kHz | 两次复位 | `serial-crosscheck-run1.log`、`serial-crosscheck-run2.log` |
| AG：硬件 I²C 内部上拉关闭对照 | 相对 AF 源码唯一改动为后置硬件 I²C `enable_internal_pullup=true→false`；软件 I²C、控制序列、100 kHz combined 与 25 kHz split 测试顺序不变；两次复位 | 未单独 probe | 两轮 100 kHz combined 各 6/6 `ESP_ERR_INVALID_RESPONSE`，失败占位值无效；25 kHz split 各 6/6 事务成功，身份 `0x72/0x10`、状态 `0x00` | 是，仅 1024 B / 512 词 primer；三窗 PCM 验收跳过 | 主 codec 前后状态均 `0x00`、DMA 左右词全 `-1`。与 AF 内部上拉开启时 12 笔中仅 1 笔成功比较，只能说关闭内部上拉未恢复本轮 100 kHz 稳定读，25 kHz 状态结论不变；不能用少量样本判断开关优劣或修复因果 | 两次复位 | `serial-no-pullup-run1.log`、`serial-no-pullup-run2.log` |
| AH：input-only 真实 wrapper 路径包 | 走旧音频设备封装的 `open`、`data_fmt`、`data_enable` 与两次增益调用；同时保留当前诊断控制、时钟、DMA 读回；这是多项路径包对照，不是单因素，也未复刻旧整机先播放后录音时序；两次复位 | 未单独 probe | 两次 `WRAPPER_PATH` 各项返回均 0、调用各 1 次，spy_failed=0、terminal=0、fault=0；9 项控制寄存器语义读回均匹配，`0x43/0x44=0x18`；`0x0B` 在 DMA 前后均为 `0x00` | 是，仅 1024 B / 512 词 primer；三窗 PCM 验收跳过 | 两次 DMA 左右各 256 词固定 `-1`。MCU PCNT WS≈15.86 kHz、BCLK≈512 kHz、MCLK≈4.094 MHz；U12 焊盘未测。两次控制统计均为 reads76/read_attempts204/writes65/write_attempts75/transient55/terminal0。wrapper 路径包未恢复状态或 PCM | 两次复位 | `serial-wrapper-run1.log`、`serial-wrapper-run2.log` |
| 冷启动复测 | 每次完全断电及等待时间待记录 | 待测 | 待测 | 待测 | 待测 | 待测 | 待记录 |

A 轮 `PREINIT_ID` 的 `0x3D/0x3E/0x3F` 读取均返回 `ESP_ERR_INVALID_RESPONSE`。B 轮删掉这三次预读，首笔写入仍失败，后续九笔均失败；因此**这三次预读不是当前首笔写失败的必要条件**。两轮初始化失败后，`0x00/01/02/06/08/0B/11/12/40–4C` 的读取也全为同类错误。日志打印的 `0x7C` 是未成功读取时的变量值，**不得解释为芯片寄存器内容**。日志中的 `ret=269` 即十六进制 `0x10D`，本节以驱动原始错误码为准。两轮只分离了“有无三次预读”，**尚未比较显式 IDF I²C 原语或另一种事务格式**。

若无法独立硬复位 ES7210，需注明 A/B 执行顺序和每轮初始器件状态，不能把软复位重复当成独立冷启动。

C1 的一次**热启动**已证明：即使绕开 `esp_codec_dev` 驱动初始化，直接发送单笔 `[0x00, 0xFF]` 的 STOP 结束写事务，此时仍返回 `ESP_ERR_INVALID_RESPONSE`。但该实验不包含完全断电后的器件复位，不能排除前序状态；全断电冷启动仍在等待，不能把热启动结果外推为冷启动结论。C1 与 A/B 使用的 API 路径及错误码记录形式不同，`ret=269` 与 `raw=0x108` 不应机械比较数值来推断新的故障层级。

C2 进一步缩到**仅一次地址探测**，当前上电状态下 `0x41` 返回 `ESP_ERR_NOT_FOUND`。历史最小 I²C 程序曾多轮探测到 `0x41`，两组证据发生条件和时间不同；C2 说明“当前状态地址未应答”，**不能单独证明 U12 损坏、PCB 断线或该地址从未可用**。

C3 已取得同总线对照：`0x6B` 三轮均应答，`0x19` 后两轮应答，而 `0x41`、`0x18` 三轮均无应答。这证明**当前测试状态下 I²C 控制器及整条共享总线并非完全失联**，但不证明应答地址的器件身份，也不定位 `0x41` 无应答是 U12 供电、时钟、复位、地址、总线局部电气条件、器件前序状态还是装配问题。首轮 `0x19` 无应答、后两轮应答提示地址响应随时间或访问顺序变化；单次日志不能拆分这两个因素。

C4 的定时探测使时间边界更清楚：`0x41` 及相邻 `0x40/0x42/0x43` 在 16 轮均无应答；`0x6B` 始终应答；`0x18` 从约 2 秒起持续应答，`0x19` 仅在约 8 秒的一轮应答。`0x18` 有可能是 PCA9557 地址脚 A0 为低时的地址，**不能凭地址将其认定为 ES8311**。主 agent 对实时 EDA 网表的复核显示 `AU_3V3` 由 U6 输出，ES7210 与 ES8311 同电源域，IMU 属另一电源域。用户在 MASTER ON 时测得 C25/C26 非接地端对 GND 约 **3.3 V**：这证明 `AU_3V3` 在两个电容测点有电压，削弱了“U6 完全无输出”的分支；**尚未测 U12 芯片近端各电源脚，不能宣称 ES7210 供电完整或将供电分支完全排除**。

用户随后确认 U12 附近 C83.2 与 C87/C88.2 两处均约 **3.3 V**。这提高了“U12 近旁供电测点有电压”的证据等级，但**仍不能证明所有 U12 供电脚焊接良好、芯片内部供电有效或上电时序正确**。D 镜像关闭 I²S 时钟后重复 16 轮地址探测，`0x41` 及 `0x40/0x42/0x43` 仍全部无应答；`0x6B` 始终应答，`0x18` 15/16、`0x19` 4/16。故**缺少 I²S 时钟不是本次 `0x41` 地址 NACK 的必要原因**；此结论不等于 I²S 时钟对后续正常采样不重要。

E 镜像用 100→25→25→100 kHz 的 ABBA 顺序，只发送地址字节，每个阶段对 `0x41` 和 `0x6B` 各探测 16 次：`0x41` 四阶段均 **0/16**，`0x6B` 四阶段均 **16/16**。因此，**单纯把总线速率从 100 kHz 降至 25 kHz 不能恢复当前 ES7210 地址应答**；该对照也再次证明同一控制器可以访问至少一个同总线设备。此软件测试已经缩到地址阶段；后续进行 R61/R62 上电电压和 R60 完全断电阻值检查，见下段。

用户随后完成上述三点测量：R61.2/AD0 对地 **3.3 V**，R62.2/AD1 对地 **0 V**，完全断电时 R60 两端 **0 Ω**。按当前设计，这说明外部地址绑带和地桥测点符合预期，也连同 C25/C26、C83、C87/C88 的测量削弱了“整片音频电源域无电”及“外部地址绑带取错”的简单解释。它**不能证明 U12 焊脚实际接到这些网络、芯片内部状态或 I²C 时序正常**。用户已主动断电，COM5 目前不出现不构成新故障。下一次上电应以同板、同接线原样复现 2026-11-11/12 的 `diagnostics/i2c-isolation` r3 诊断，比较当时与现在的地址应答；目前不能先判 U12 损坏。

随后同板重放历史 `diagnostics/i2c-isolation` r3：四个 100→25→25→100 kHz 阶段中，多器件交替访问流程的 `0x41` 地址探测各 **16/16 ACK**，`0x0B` 读取事务各 **128/128 返回成功**；单独 combined read 四阶段均 **0/128**，分开指针/数据事务在 100 kHz 为 **2/16**、25 kHz 为 **16/16**。PCA9557 安全写四阶段各 128/128 成功，但其读事务多数失败。**这仅证明本程序特定访问顺序和事务形状下有成功返回；日志没有记录有效的 `0x0B` 数值，未验证 ES7210 真实配置、冷启动稳定性或 PCM 数据。**先前 C2–E 的 `0x41` NACK 与本次 ACK 属不同镜像/事务序列下的真实差异，不能据此判定 U12 损坏或麦克风已恢复；下一门禁应分离触发地址应答变化的具体事务顺序，并验证寄存器值及初始化写入。

G 在同流程中增加身份读取与空地址对照：phase 0/3 的 mixed 流程末次 `0x0B` 成功后，立即读取 `0x3D=0x72`、`0x3E=0x10`，两次均返回 `ESP_OK`；mixed `0x0B` 四阶段各 **128/128**，成功值全部为 `0x00`。这使“真实 U12 在特定事务序列中曾应答”有了地址以外的身份读值支持。与此同时，split `0x0B` 在 100 kHz 虽有成功事务却读到 `0xFF`，25 kHz 读到 `0x00`；phase 3 离开 mixed 后，单独 probe `0x41`、三个邻近地址和 `0x45/0x50/0x55` 各 **0/16**。**身份读值不等于通信稳定；`0x0B` 的两种值不能在尚未核对状态机与访问序列前直接解释为有效配置。**仍未执行 ES7210 完整初始化写入/读回或 PCM 采样。下一步做“移除 PCA9557 安全写”单因素对照，保持其他事务顺序与条件不变，查明 mixed 成功是否依赖该写操作。

[ES7210 数据手册，寄存器 `0x0B`](https://files.waveshare.com/wiki/common/ES7210_DS.pdf)明示 `CHIP STATUS` 复位默认 `0x00`，低两位 `00` 表示 power-down；G 是**未执行 ES7210 初始化**的纯 I²C 诊断，因此 mixed 读到 `0x00` 与默认值相容，**不能独立证明芯片故障**。它与此前“已经尝试完整初始化和采样后仍读 `0x0B=0`”属于不同前置条件，不能混作同一次状态判断。H 在相同启动中先于任何寄存器事务探测 `0x41`，首笔返回 `ESP_ERR_NOT_FOUND (0x105)`；随后 mixed 流程 phase 0 为 **15/16 成功、1 次其他错误**，phase 1–3 各 **16/16 成功**。这进一步证实地址应答依赖本次事务序列或前序状态，但仍未分离具体触发操作；不能把 15/16 写成 16/16。

I 仅移除 G 的 PCA9557 安全锁存写，mixed `0x41` 在四阶段仍各 **16/16 ACK**，因此该写操作**不是获得 mixed 地址 ACK 的必要条件**。但 100 kHz phase 0/3 的 mixed `0x0B` 各只有 **126/128** 次事务成功；在各 126 次成功返回中，**125 次为 `0xFF`、1 次为 `0x00`**，身份寄存器 `0x3D` 也读到错误的 `0xFF`，而 `0x3E=0x10`。25 kHz phase 1/2 的 mixed `0x0B` 各 128/128 成功且全为 `0x00`。phase 3 离开 mixed 后单独 `0x41` probe 为 **1/16 ACK**，其余 15 次无应答。**100 kHz 的地址 ACK 和 I²C API 成功返回不能视为可靠寄存器数据。**该单因素对照只排除 PCA 写是 ACK 必要条件；100 kHz 数据退化与移除写操作同时出现，但跨镜像/上电状态尚未严格控制，不能直接归因 PCA 写或宣布 25 kHz 已可用于完整音频配置。

J 在 25 kHz phase 1/2 的 mixed 窗口加入前后身份读取：前次 `0x3D=0x72`、`0x3E=0x10` 均 `ESP_OK`；随后总线默认 **100 kHz** 的空地址 `0x55` probe 返回 `ESP_ERR_NOT_FOUND (0x105)`；紧接着用 ES7210 **25 kHz** 设备句柄读取两寄存器均为 `ESP_ERR_INVALID_RESPONSE`，值无效。两阶段现象相同；随后 mixed `0x0B` 仍各 128/128 且全 `0x00`，mixed `0x41` 各 16/16 ACK。**这证明身份读取在该操作序列后暂时失败，不证明 NACK 本身是原因。**probe 与设备读取的速率不同，主机频率切换、控制器状态、时序或器件前序状态均未分离；也不能据 mixed 后恢复 ACK 宣称 25 kHz 全程稳定。

K 从启动约 145 ms 起只进行 25 kHz 的 ES7210 读事务，首笔 `0x0B` 指针写即 `ESP_ERR_INVALID_RESPONSE`；round 0 的 `0x3D/0x3E/0x0B` 全部 NACK。round 1–15 每轮相隔约 50 ms：第一个 `0x3D` **15/15 NACK**，但紧随其后的 `0x3E=0x10` 与 `0x0B=0x00` 均成功。末尾先读 QMI 寄存器 `0x00=0x05`，随后 ES7210 `0x3D=0x72`、`0x3E=0x10` 均成功。**部分访问在约 225 ms 即出现，不需要历史隔离程序长时间预热。**持续 15 轮“每轮首笔失败、后续两笔成功”也不能用单纯上电等待不足解释。末尾身份完全读通与 QMI 访问在时间上相邻，但同时经历了 16 轮 ES7210 事务及等待，不能把变化归因于 QMI 的特异唤醒作用。仍未验证连续稳定身份读取、ES7210 初始化寄存器写/读回及 PCM。

L 在 J 的 25 kHz phase 1/2 身份窗口中，默认 100 kHz 空地址 `0x55` probe 返回 `ESP_ERR_NOT_FOUND` 后显式调用 `i2c_master_bus_reset`，两阶段都返回 **`ESP_OK`**。然而紧接着以 25 kHz 再读 `0x3D/0x3E`，两寄存器在两阶段都返回 `ESP_ERR_INVALID_RESPONSE`；之后 mixed `0x0B` 各 **128/128** 成功且全为 `0x00`。**显式 bus reset 没有立即恢复身份读取**，因此不能直接把此前现象归为“仅是 IDF 控制器状态机残留”；它也不能排除更广义的速率切换、时序、器件内部状态或电气条件。后续继续以交换读顺序控制首笔事务效应。

M 与 K 仅交换每轮三寄存器的顺序。round 0 仍三笔全 NACK；round 1–15 每轮约 50 ms 空闲后，首笔换成 `0x0B`，结果 **15/15 NACK**，随后的 `0x3E=0x10`、`0x3D=0x72` 各 **15/15 成功**。这使 K 中“`0x3D` 地址/寄存器特异失败”的解释不成立；**失败跟随每轮空闲后的首笔事务**。末尾 QMI `0x00=0x05` 后 ES 两身份读取成功，与 K 一致，但仍不能归因为 QMI 特异唤醒。此对照尚未区分 ES7210 芯片状态、ESP-IDF 主机行为、总线电气条件或时序，也没有初始化写入或 PCM 结果。

N 尝试进入安全写入验证，但设有先读身份和 `0x12` 原值的门禁。虽然 POST_QMI 曾读得 `0x3D=0x72`、`0x3E=0x10`，紧随的两轮门禁中身份及 `0x12` 均返回 `ESP_ERR_INVALID_RESPONSE`，因此程序记录 **`WRITE_TEST skipped`，没有对 ES7210 执行测试写入**。本轮不能描述为写成功或写失败，只能说写入前的稳定读取门禁未通过。

O 的 25 kHz 只读突发共有 8 笔：其中一次 `0x3D` 虽返回 `ESP_OK` 却得到与前述身份值不符的 `0x77`；`0x12` 首读 NACK，第二读 `0x00`。P 将设备读速率降至 10 kHz，突发中的 `0x3D` 三次全 NACK、`0x3E` 三次均成功返回 `0x10`，`0x12` 仍是首读 NACK、次读 `0x00`。失败事务日志里的旧变量值均无效，不能当作寄存器值。**降至 10 kHz 未让这些连续读取稳定；25 kHz 的 ACK 也不能保证数据正确。**两轮都未进行 ES7210 配置写入或 PCM 采样。

Q 改用 IDF 旧版 `driver/i2c.h` 命令链，在 25 kHz 下只读，包含 repeated START 读寄存器和单独指针事务；两次复位日志给出相同的主要失败形状。burst 第二次 `0x3D` 返回 `ESP_OK`，值却为错误的 `0xFF`；`0x12` 首读 `ESP_FAIL`、次读 `0x00`。约 50 ms 空闲后，round 1–15 的首笔 `0x0B` 均失败；后续 `0x3E=0x10` 各 14/15 成功（两次日志 round 7 均超时），`0x3D=0x72` 各 15/15 成功。**旧驱动也出现首笔失败和成功码配错误数据，因此不能把该现象归因为新版 I²C API 独有；旧驱动并未修复通信，也不能凭此分辨芯片状态、主机时序或电气路径。**本程序未写 ES7210 配置，未初始化采样或运行 PCM。

R 用 GPIO 开漏软件模拟 I²C，避开 ESP32-C6 硬件 I²C 控制器与 IDF I²C 驱动。日志读到 SDA/SCL 空闲均为高；round 1–15 每轮约 50 ms 空闲后的首笔 `0x0B` 均在写地址阶段 NACK，后续 `0x3E=0x10` 和 `0x3D=0x72` 均成功。burst 的 n1 在读地址阶段 NACK，n4、n7 在写地址阶段 NACK；其余成功身份读值正确，n6 的 `0x12=0x00`。**软件 I²C 仍出现器件地址应答失败，硬件 I²C 控制器不是唯一必要条件**；软件实现仍共享 GPIO、板级电气与器件状态，不能因此直接锁定物理故障。

S 是在 R 基础上增加 I²S TX 静音缓冲写入的受限候选。两次复位日志各记录 58 次成功写入，仍每轮空闲后的首笔 `0x0B` 地址写 NACK；burst 的 n1、n6 均在读地址阶段 NACK，未得到稳定通信。日志中的静音写计数只证明软件调用完成，**未测量物理 MCLK 引脚波形**。此外 `feed_silence` 的失败分支缺少退避，代码审计结论为 **NO-GO**；该候选不能作 MCLK 因果判定，测试后已烧回并校验 R 的无 I²S 软件 I²C 只读镜像。R/S 均未写 ES7210 配置或采集 PCM。

T/U 开始检查控制面是否**完全不可写**。T 在身份和 `0x12=0x00` 两轮读门禁通过后，仅执行单次 `0x12` bit5 写 `0x20`；该次在地址写阶段 NACK，读不出 `0x20`，随后写回 `0x00` 并确认原值，`pass=0`。U 保持相同门禁与目标值，改为有界重试；两次复位均在 toggle 首笔地址 NACK、第二笔 ACK 后读得 `0x20`，restore 首笔寄存器地址 NACK、第二笔 ACK 后读回 `0x00`，末尾身份仍为 `0x72/0x10`，各为 `pass=1`。**这两次证明受控条件下 `0x12` 一个寄存器可以可逆写回，排除“控制面完全不可写”这一绝对假说；它们同时证明 NACK 仍存在，不能外推至稳定初始化、多寄存器配置或 PCM 音频。** `pass=1` 仅是单寄存器写回门禁，不是麦克风验收通过。

V 使用 `esp_codec_dev 1.6.2` 官方 ES7210 驱动跑完整控制调用链；唯一临时补丁是在读改写操作的预读失败时立即返回，避免把无效值当作原寄存器值。两次复位均通过身份预读，`es7210_codec_new` 成功，`set_fs(16 kHz/16 bit/双通道)`、`enable(true)` 返回 0。适配层每次记录 59 笔控制写调用成功、69/70 次底层写尝试、41 次瞬态失败、0 次终态失败；`0x01/40/41/42/11/12/43/44/4B` 的最终读值均与最后一次写入相符。这**证明有界重试下多寄存器控制面可完成事务与读回，不再是完全卡在首笔写**。但两次 `0x0B` 均读得 `0x00`、`normal=0`，即在本次已运行初始化与 enable 的前提下，芯片状态仍处于数据手册定义的 power-down，而非正常工作态；没有采集 PCM。日志的 `verified=0` 因状态门禁失败仍成立。

V 的诊断程序把 `0x43/0x44` 的语义预期硬编码为 `0x1A`，导致日志对此两项标为 `semantic=0`；这是**诊断预期错误**。官方驱动 open 路径先设置 30 dB 增益，但随后 enable/start 路径再次调用 `es7210_mic_select`，以 `codec->gain` 当前值更新增益低四位，最终写入并读回 `0x10`。因此这两项实际值与本次官方代码执行顺序相符，不能列为故障证据；修正预期后，`0x0B` 仍是未通过的独立门禁。本隔离程序没有启动 I²S/MCLK 或采集 PCM；本次 power-down 状态不定位器件本体、外部时钟或电气根因。此临时补丁和测试程序都不是量产固件修复，不能据此宣称麦克风已恢复。

W 在修正 `0x43/0x44` 诊断预期后启用 I²S RX。两次复位中，软件从 MCU GPIO3/GPIO12 读取到约 74235 个 BCLK 边沿、2322 个 WS 边沿（各 100000 次轮询）；这只支持 **MCU 端 GPIO 有边沿**，不能替代 U12 的时钟输入脚或 PCB 走线波形测量。一次 DMA primer 均返回 `ESP_OK`、1024 B/512 个 16 bit 词，但左右声道各 256 词全为 `-1`、变化次数 0；日志的 `nonzero=512` 仅说明 `-1` 不等于 0，绝非音频活动。`0x0B` 在 primer 前后均为 `0x00`、normal=0，因此后续三窗 PCM 候选按门禁跳过。**已复现“有 MCU 时钟边沿和 DMA 返回字节，但无有效输入”的现象；尚未证明 U12 实际收到时钟，也没有通过 PCM 验收。**

X 用 PCNT 对同一 MCU 输出 GPIO 再做频率自检，两次复位的 2 ms 计数几乎一致：GPIO12/WS 约 16.37 kHz、GPIO3/BCLK 约 512 kHz、GPIO0/MCLK 约 4.096 MHz。这比 W 的软件轮询更明确地证明**MCU GPIO 输入路径读到对应输出节拍**，但没有跨越 PCB 走线及 U12.5/9/10 芯片焊盘；不能写成“ES7210 已收到时钟”。控制寄存器仍按预期读回，`0x0B` 在 DMA 前后仍为 `0x00`，primer 仍全 `-1`，三窗 PCM 候选仍因状态门禁跳过。故当前缩小到“MCU 端时钟已产生、芯片正常态和数据输出仍未建立”的证据边界，物理时钟到达、芯片状态机与数据线需继续分离验证。

Y 只增加 I²S TX 静音馈送，物理 DOUT 不使用。两次复位各记录 `done=1`、71 次完整写、58 次失败写；**不能将 `done=1` 写成连续无失败发送**。与此同时 MCU PCNT 三钟与 X 接近，`0x0B` 在 RX 前后仍 `0x00`，DMA primer 的左右各 256 词仍全 `-1`，后续三窗 PCM 候选被状态门禁跳过。这个对照表明加入 TX 后仍未产生有效音频，并削弱“仅因缺 TX 才没有 MCU 时钟”的解释；由于 TX 存在 58 次写失败，尚不能排除持续馈送要求，也不能据软件计数判断 U12 引脚实际波形。

Z 将 Y 的 I²S 时钟和 codec 采样率同步改为 24 kHz，源码其余差异为日志与注释。两次复位 MCU PCNT 三钟随之升至 WS≈23.81 kHz、BCLK≈767.73 kHz、MCLK≈6.14 MHz；`0x0B` 在 DMA 前后仍 `0x00`、normal=0，primer 左右各 256 词全 `-1`、无变化，三窗 PCM 候选仍跳过。TX 每轮 106 次完整写、57 次失败写，并非无间断全成功馈送。**在本镜像和这两次复位条件下，16→24 kHz 未改变失败结果**；不能据此断言采样率对所有配置无影响，更不能推定 U12 芯片焊盘实测时钟或物理根因。

AA 保持 24 kHz 与 16 bit 有效样本，只把标准 I²S 时隙/WS 宽度从 16 调为 32 bit。两次复位 MCU PCNT 的 BCLK 约翻倍至 1.534 MHz，WS/MCLK 仍约 23.83 kHz/6.144 MHz；控制面有 2/3 次瞬态失败、0 次终态失败，关键寄存器读回正确。`0x0B` 在 RX 前后仍为 `0x00`、DMA primer 全 `-1` 且无变化，三窗 PCM 门禁仍未通过。TX 各有 107 次完整写、56 次失败写。**在本测试条件下，仅将时隙加宽到 32 bit 未恢复正常态或有效 PCM**；不能推论 U12 输入焊盘上的实际时钟、电气质量或其他时隙格式。

AB 保持 TX 的 24 kHz 标准双槽 32 bit 帧作为时钟主路径，将 RX 改为同样 64 bit 帧长的四槽 16 bit TDM。两次复位均有 MCU GPIO 端 WS/BCLK/MCLK 约 23.8 kHz/1.536 MHz/6.144 MHz，但 `0x0B` 在 RX 前后均 `0x00`；TDM DMA 的 2048 B / 1024 词全部 `-1`，三窗 PCM 门禁未通过。TX 各有 108 次完整写、57 次失败写。**本条件下将 RX 改为四槽 TDM 并未解决故障**；U12 输入脚波形及真正的数据槽仍未物理测得，不能把该阴性结果外推到所有 TDM 配置。

AC 回到 16 kHz 标准双槽 16-bit、MIC1+2 与 TX 静音，在 enable 完成后保持配置不变，仅按 5 秒间隔读取状态至 90 秒。18 次 `0x0B` 双读均有效且为 `0x00`；5/20/60/90 秒身份及主要配置读值不变。末尾 MCU 端 WS/BCLK/MCLK 仍有约 15.984 kHz/512.487 kHz/4.0964 MHz 的 PCNT 计数，TX task active=1；但 TX 完整写 5647 次的同时失败写 4519 次，不能称静音数据持续成功馈送。初始 DMA primer 全 `-1`，90 秒末未再采 PCM。**本镜像中 enable 后单纯持续等待 90 秒不会自发进入 normal**；实验未测试“先建立时钟再 enable”，且历史同板 11/12 日志并不支持此前连续供时约 84 秒的说法；不能排除写入顺序、配置或时钟到达 U12 的问题。

AD 在同一 16 kHz 标准双槽配置中，于 codec 构造后、`set_fs/enable` 前保持 MCU 端时钟和 TX task 活跃约 90.125 秒，期间不发 codec 控制事务；随后执行官方配置/enable。MCU PCNT 在预供时起止读得 WS≈15.9 kHz、BCLK≈512 kHz、MCLK≈4.094/4.096 MHz；关键寄存器读回匹配。但 `0x0B` 在 RX 前后及 enable 后每秒读至 6 秒均为 `0x00`，DMA primer 左右各 256 词全 `-1`，三窗 PCM 门禁未通过。TX 记录 6034 次完整写与 4828 次失败写，软件任务活跃不等于每次写成功。**在本次受控流程中，预供时 90 秒未解除正常态门禁**；PCNT 测的是 MCU GPIO，不能证明 U12 焊盘收到时钟。此实验不同于历史 11/12 的输出启停时序，也不证明其他预时钟/复位顺序必然无效。

AE 在 Y 的 16 kHz TX+RX 静音镜像上，仅增加显式 `codec->set_mic_gain(24.0f)` 与对应诊断读回。两次复位调用均返回 0，`0x43/0x44` 均为 `0x18`，与历史同板增益读值一致；但 RX 前后 `0x0B` 均仍为 `0x00`、normal=0，DMA 左右各 256 词固定 `-1`，三窗 PCM 门禁未通过。TX 各有 71 次完整写、58 次失败写。**单独消除增益值差异未恢复正常态或 PCM**；此实验不是旧版完整 wrapper、初始化顺序或历史整机配置的 A/B。

AF 在两次复位中交叉验证同一镜像的三种读法：GPIO 开漏软件 I²C 25 kHz 与硬件 I²C 25 kHz split 均成功读出 ES7210 身份 `0x72/0x10` 和状态 `0x0B=0x00`；软件读回的 `0x14/0x15=0x00`、`0x47–0x4A=0x08`、`0x4B=0x00`、`0x4C=0xFF` 两次一致，与同板 11/12 日志 `.tmp/lcd-20261112-full-boot.log` 第 209–224 行的对应值一致；该旧日志第 144 行记录与当前样机相同的标识，具体标识不写入公开证据。旧日志当时 `0x0B=0x02`，说明这些静态值相同并不足以解释状态差异。硬件 I²C 100 kHz combined 六笔中首次复位全部返回 `ESP_ERR_INVALID_RESPONSE`，第二次仅首笔身份高字节 `0x3D=0x72` 成功，后五笔失败；失败日志中的 `INVALID_0x7C/0x72` 是占位值，不能用于判断芯片状态。两条本条件下可靠的读路径均读得 power-down，静态快照没有观察到已知差异；**无法据此证明隐藏寄存器全无差异或判断 100 kHz 永久不可用**。DMA primer 仍全 `-1`，麦克风功能未通过。

另已定位与 11/12 同板日志匹配的历史诊断镜像 `.tmp/hi-yaka-lab-lcd-20261112.bin`，SHA-256 见索引。该镜像当前**未重新烧录**，分区、完整源码提交和复测条件尚未核定；它只是后续同板历史行为复测的候选资源，不能列为本轮功能通过证据。

AG 与 AF 的源码对照仅改变后置硬件 I²C 的 `enable_internal_pullup` 为 false。AG 两次复位的 100 kHz combined 各六笔均返回 `ESP_ERR_INVALID_RESPONSE`；25 kHz split 各六笔都成功，读出身份 `0x72/0x10`、状态 `0x00`。主 codec 读到的状态和 DMA 无效采样亦未改变。AF 内部上拉开启时 100 kHz combined 合计 1/12 成功，AG 关闭时 0/12；**这些少量复位与事务只支持“关闭未恢复 100 kHz 稳定读”**，不足以判定关闭更差、开启能修复或内部上拉是根因；25 kHz split 与当前 power-down 结论保持一致。

AH 两次复位走 input-only 旧音频设备 wrapper 路径：`open/data_fmt/data_enable` 均返回 0，调用计数各 1、spy_failed=0；两次增益设置均返回 0。9 项原驱动寄存器语义核对全部一致，`0x43/0x44=0x18`，控制面终态错误为 0；但 `0x0B` 在 DMA 前后仍读到 `0x00`，1024 B primer 左右词仍固定 `-1`。MCU GPIO 端 PCNT WS≈15.86 kHz、BCLK≈512 kHz、MCLK≈4.094 MHz，不能代替 U12 焊盘波形。控制重试仍多（两轮各 transient55、terminal0）。**该 wrapper 路径包也未恢复正常态或有效 PCM**；它同时改变多项调用路径，且未执行旧整机“先播放、后 ACT_B 录音”的真实时序，不能称为旧固件精确复刻。下一步优先在用户在场时复现已定位的旧镜像 ACT_B 路径，或取得 U12 近端硬件量测，避免继续无目标地排列软件参数。

另核对版本假说：历史 Yaka 工程 `F:/Project/GitHub/esp32-project/yaka-firmware` 的 HEAD 为 `d4c5c51`，`dependencies.lock` 锁定 `espressif/esp_codec_dev 1.3.5`；其 `managed_components/.../device/es7210/es7210.c` 与当前 1.6.2 实验保存的 **未打临时补丁原始 `es7210.c`** 均为 SHA-256 `8C741993683579FFBFFD05A227533F4A54D086921DE8ED99FF6C12F6574D5921`。因此不能把本次故障归因于这两个版本间 **ES7210 驱动原始源码算法发生回归**；构建环境、适配层、调用参数和实际硬件仍可能不同。`xiaozhi-esp32-test_0707` 是另一工程，其锁版本 1.3.4，不作为这项历史 Yaka 比对的来源。

## 4. PCM 数据 Gate

仅在控制面完整初始化成功后执行。A/B 日志明确写明 `GATE1 FAIL initialization; DMA intentionally skipped`，C1 也明确不运行 DMA，因此 **PCM Gate 未执行**，不能记作本轮 PCM 失败。记录原始 DMA 数据，不以 WAV 文件存在、API 返回成功或单纯“非零”判断麦克风正常。

| 场景 | I²S 模式/采样率/槽数 | GPIO13 数据边沿 | PCM 峰值/RMS/非零率 | 相对静音基线变化 | 证据文件 |
| --- | --- | --- | --- | --- | --- |
| 静音 | 待记录 | 待测 | 待测 | 基线 | 待记录 |
| MIC1 近场声音 | 待记录 | 待测 | 待测 | 待比较 | 待记录 |
| MIC2 近场声音 | 待记录 | 待测 | 待测 | 待比较 | 待记录 |

若控制面失败，本节标记“未执行：前置 Gate 未通过”，不得写成 PCM 测试失败。

## 5. 原始日志与哈希索引

| 文件/产物 | 用途 | SHA-256 | 保存位置 |
| --- | --- | --- | --- |
| `serial-gate1-run1.log` | 第一版实板启动、首笔写失败与关键寄存器读回失败 | `30BF08709A66C732850ACAC05194919630A4417095D28D7B6B547CF7B2C9735D` | `.tmp/hi-yaka-es7210-gate-20261120/serial-gate1-run1.log` |
| `serial-gate1-B-run1.log` | B 轮移除预读后的实板日志 | `0B3D095E749A2F62ED0B593DF0FA9B817846FF6FD102057543A4770B27584C05` | `.tmp/hi-yaka-es7210-gate-no-preread-20261120/serial-gate1-B-run1.log` |
| `serial-C1-run1.log` | C1 热启动原始单笔写事务 | `951C58A157D277F0A17AE5E85EDC11857DC7A95F660E20C20C6012BD6F4A39A3` | `.tmp/hi-yaka-es7210-raw-first-write-20261120/serial-C1-run1.log` |
| `hi_yaka_es7210_raw_first_write.bin` | C1 热启动镜像 | `5ADC6406B5897794C355E43764DBF013F136E7810202A8317941A184EFAF4192` | `.tmp/hi-yaka-es7210-raw-first-write-20261120/dist/hi_yaka_es7210_raw_first_write.bin` |
| `serial-C2-run1.log` | C2 当前上电状态单次 `0x41` 地址探测 | `4F42F496E5A10D693F30870EE554D3C34EAADABC8575A0CDC48527F69AF70472` | `.tmp/hi-yaka-es7210-probe-only-20261120/serial-C2-run1.log` |
| `hi_yaka_es7210_probe_only.bin` | C2 地址探测镜像 | `12C69A9BA99609B183246112959A4CF75FE2A6DF2FEF3CDDB19E172BD1E93A55` | `.tmp/hi-yaka-es7210-probe-only-20261120/dist/hi_yaka_es7210_probe_only.bin` |
| `serial-C3-run1.log` | C3 三轮同总线 peer probe | `5B3D3B44AC9DE5336C1FAF1F228C4D0650E13721E2EEABEC6C052223F5E12490` | `.tmp/hi-yaka-es7210-peer-probe-20261120/serial-C3-run1.log` |
| `hi_yaka_es7210_peer_probe.bin` | C3 peer probe 镜像 | `4C0390FBD27540F33A4D488FF64897C703E5C4776CE21ACC47F3BE1DF8867292` | `.tmp/hi-yaka-es7210-peer-probe-20261120/dist/hi_yaka_es7210_peer_probe.bin` |
| `serial-C4-run1.log` | C4 上电后 16 轮定时地址探测 | `658ED855674036D674675759B69D32CF1AA79AA4A7869E968B94E9F3E4A5656C` | `.tmp/hi-yaka-es7210-timed-probe-20261120/serial-C4-run1.log` |
| `hi_yaka_es7210_timed_probe.bin` | C4 定时探测镜像 | `E5BD2EE9BDA6BCA44428F79259D43EF3FC727583806DBAF8F0CD5D8742208EA5` | `.tmp/hi-yaka-es7210-timed-probe-20261120/dist/hi_yaka_es7210_timed_probe.bin` |
| `serial-D-run1.log` | D 无 I²S 时钟下 16 轮地址探测 | `B6AEC3646B0604A2CB7FC85942F111A40FF4F252DDAD13704A08F9CFBEB65BC8` | `.tmp/hi-yaka-es7210-timed-probe-no-i2s-20261120/serial-D-run1.log` |
| `hi_yaka_es7210_timed_probe_no_i2s.bin` | D 无 I²S 时钟探测镜像 | `AC06300B68797EEA487967DB1D42EDF340BACABEC1E8A026319938FC21CF2775` | `.tmp/hi-yaka-es7210-timed-probe-no-i2s-20261120/dist/hi_yaka_es7210_timed_probe_no_i2s.bin` |
| `serial-E-reset-run1.log` | E 地址速率 ABBA 原始串口日志 | `74D228312087F852F9E33C274D855FC856854481B76A50C8A9F0C2A0CF5C2C32` | `.tmp/hi-yaka-es7210-address-rate-abba-20261120/serial-E-reset-run1.log` |
| `hi_yaka_es7210_address_rate_abba.bin` | E 地址速率 ABBA 镜像 | `A6E9E5607DB15385107DB0D447C47C4B8747DBB16094E34C00291BABD9864890` | `.tmp/hi-yaka-es7210-address-rate-abba-20261120/dist/hi_yaka_es7210_address_rate_abba.bin` |
| `serial-historical-replay-run1.log` | F 历史 I²C 隔离程序同板重放串口日志 | `286D6C5ADB2135AAF6387F8520F89197DEE4B4D02687E13E1ACC3DABDE7EFD3C` | `.tmp/hi-yaka-i2c-isolation-replay-20261120/serial-historical-replay-run1.log` |
| `hi_yaka_i2c_historical_replay.bin` | F 历史 I²C 隔离程序重放镜像 | `B7F0F8EB0F439678AB3918A6E19C3E52B63ABCCDF97265FCECB06771833BB1A2` | `.tmp/hi-yaka-i2c-isolation-replay-20261120/dist/hi_yaka_i2c_historical_replay.bin` |
| `serial-identity-run1.log` | G ES7210 身份读值、`0x0B` 数据和空地址对照原始日志 | `517A796474BCA0F6B3369D7ECE29C0DA5DD0CE5EBAA49C0FDB0E44832C787523` | `F:/Project/GitHub/workshop/codex-doing/.tmp/hi-yaka-i2c-identity-controls-20261120/serial-identity-run1.log` |
| `hi_yaka_i2c_identity_controls.bin` | G 身份与空地址对照镜像 | `AD11107E0E772DC39CB9C93B5CE6C27A666BA31DB76B64A9BAEF9A337F5C6031` | `F:/Project/GitHub/workshop/codex-doing/.tmp/hi-yaka-i2c-identity-controls-20261120/dist/hi_yaka_i2c_identity_controls.bin` |
| `serial-first-probe-run1.log` | H 首笔地址 NACK 与 mixed 后地址 ACK 对照原始日志 | `7F20D14BB2D1EBB6B8749597F8CD33D1FE15B432DC48F9B0C8F1D5F466013555` | `.tmp/hi-yaka-i2c-isolation-first-probe-20261120/serial-first-probe-run1.log` |
| `hi_yaka_i2c_first_probe.bin` | H 启动首笔地址探测镜像 | `428A43D3C7B3B9A57B0519F151323F0A379C9D40FC2B4D94FF5095C4190CE630` | `.tmp/hi-yaka-i2c-isolation-first-probe-20261120/dist/hi_yaka_i2c_first_probe.bin` |
| `serial-no-pca-run1.log` | I no-PCA-write 单因素对照原始日志 | `E4C7F6C38F680C039F761FDF1503AFDFFDE7E544C7460B9317D3A1FAAC009DCB` | `F:/Project/GitHub/workshop/codex-doing/.tmp/hi-yaka-i2c-no-pca-write-20261120/serial-no-pca-run1.log` |
| `hi_yaka_i2c_no_pca_write.bin` | I no-PCA-write 镜像 | `3EB3EEB46257E6E784B9B6194822BA0258CCF9D9202400287D5DAC6EDFA8A75F` | `F:/Project/GitHub/workshop/codex-doing/.tmp/hi-yaka-i2c-no-pca-write-20261120/dist/hi_yaka_i2c_no_pca_write.bin` |
| `serial-25k-gate-run1.log` | J 25 kHz 身份窗口前后读取原始日志 | `BA35C0F0A31E1A040118C9A92D2CC12915A4BC3F0CE998E2988B6716830B46FB` | `F:/Project/GitHub/workshop/codex-doing/.tmp/hi-yaka-i2c-25k-identity-gate-20261120/serial-25k-gate-run1.log` |
| `hi_yaka_i2c_25k_identity_gate.bin` | J 25 kHz 身份窗口镜像 | `5367E20B8C6D94F3C9D48CD14D94D4609013CC9ADEE3139A8B0F0149E6DE3529` | `F:/Project/GitHub/workshop/codex-doing/.tmp/hi-yaka-i2c-25k-identity-gate-20261120/dist/hi_yaka_i2c_25k_identity_gate.bin` |
| `serial-first25k-run1.log` | K 25 kHz 极简首笔读原始日志 | `3AD325211337CA7553C6E14E6C386524723EE09A58EC0836C3DBF91A10FDFB18` | `F:/Project/GitHub/workshop/codex-doing/.tmp/hi-yaka-i2c-25k-first-transaction-20261120/serial-first25k-run1.log` |
| `hi_yaka_i2c_25k_first_transaction.bin` | K 25 kHz 极简首笔读镜像 | `7FDD1DF7F869AD1A7BF71F236F5654B5C7386B2CCDA018BD3D24B9580AA3AE08` | `F:/Project/GitHub/workshop/codex-doing/.tmp/hi-yaka-i2c-25k-first-transaction-20261120/dist/hi_yaka_i2c_25k_first_transaction.bin` |
| `serial-probe-reset-run1.log` | L 25 kHz 空地址 probe 后 bus reset 原始日志 | `CDF6FC1CED5DE7762A4B24F86D96505996CD3C5A68DA2124B9BE9D6DE822A689` | `F:/Project/GitHub/workshop/codex-doing/.tmp/hi-yaka-i2c-25k-probe-reset-20261120/serial-probe-reset-run1.log` |
| `hi_yaka_i2c_25k_probe_reset.bin` | L bus reset 对照镜像 | `13FD7B7584A269AA3316B2EB245E88B0F8BAF6342320701A2287314C12A1370D` | `F:/Project/GitHub/workshop/codex-doing/.tmp/hi-yaka-i2c-25k-probe-reset-20261120/dist/hi_yaka_i2c_25k_probe_reset.bin` |
| `serial-order-swap-run1.log` | M 25 kHz 极简读顺序互换原始日志 | `67CD94A27E4D02CB7E0E0AB2687ACF9CEA3D52DAF01C11B32FB0BE343BF54202` | `F:/Project/GitHub/workshop/codex-doing/.tmp/hi-yaka-i2c-25k-order-swap-20261120/serial-order-swap-run1.log` |
| `hi_yaka_i2c_25k_order_swap.bin` | M 顺序互换镜像 | `6DB99D808D68B207C9B0B763B83E368908DA83E3ABBA8E505BEEC0B15DE9B1B0` | `F:/Project/GitHub/workshop/codex-doing/.tmp/hi-yaka-i2c-25k-order-swap-20261120/dist/hi_yaka_i2c_25k_order_swap.bin` |
| `serial-safe-write-run1.log` | N 安全写前门禁日志；写入被跳过 | `1A27196DFFD8A45BABFAC6C6CC735B374529BA5EC1DF90B700C1371FE34640F7` | `F:/Project/GitHub/workshop/codex-doing/.tmp/hi-yaka-i2c-25k-safe-write-20261120/serial-safe-write-run1.log` |
| `hi_yaka_i2c_25k_safe_write.bin` | N 门禁镜像 | `4A175F703BD75E9B61AA9C287A80DDDCDACF9D45E5F30B1CF261AB885227F045` | `F:/Project/GitHub/workshop/codex-doing/.tmp/hi-yaka-i2c-25k-safe-write-20261120/dist/hi_yaka_i2c_25k_safe_write.bin` |
| `serial-burst-read-run1.log` | O 25 kHz 突发只读日志 | `4BBCBC95BA3AC9014FF1A3F563DDC02EBC0A2986C73DFF5E979589575B5AF0D0` | `F:/Project/GitHub/workshop/codex-doing/.tmp/hi-yaka-i2c-25k-burst-read-20261120/serial-burst-read-run1.log` |
| `hi_yaka_i2c_25k_burst_read.bin` | O 25 kHz 只读镜像 | `287DE39289693E52CA3CB2328AFF38135127A8080077C168981CB2F8428449C1` | `F:/Project/GitHub/workshop/codex-doing/.tmp/hi-yaka-i2c-25k-burst-read-20261120/dist/hi_yaka_i2c_25k_burst_read.bin` |
| `serial-10k-run1.log` | P 10 kHz 突发只读日志 | `CE8F65C0152638282D723EDC89F2C9DDB71DD89050A4F2B3A401FC92B5EB8AA8` | `F:/Project/GitHub/workshop/codex-doing/.tmp/hi-yaka-i2c-10k-burst-read-20261120/serial-10k-run1.log` |
| `hi_yaka_i2c_10k_burst_read.bin` | P 10 kHz 只读镜像 | `CF0C3385FDD4669580459FA48739DF56254195D0C85A4BB053F36B537679EE87` | `F:/Project/GitHub/workshop/codex-doing/.tmp/hi-yaka-i2c-10k-burst-read-20261120/dist/hi_yaka_i2c_10k_burst_read.bin` |
| `serial-legacy-run1.log` | Q 旧版 I²C 驱动 25 kHz 只读，首次复位日志 | `EA058CE0A79D406162C6AD71B83659DD37DD14F7A931C89A6F22F75B0CD48232` | `F:/Project/GitHub/workshop/codex-doing/.tmp/hi-yaka-i2c-25k-legacy-read-20261120/serial-legacy-run1.log` |
| `serial-legacy-run2.log` | Q 同镜像第二次复位日志 | `0E10CBF5AF87E9D65D05D571A0450463B240565CF1EFF5B862CD9DC907658FC9` | `F:/Project/GitHub/workshop/codex-doing/.tmp/hi-yaka-i2c-25k-legacy-read-20261120/serial-legacy-run2.log` |
| `hi_yaka_i2c_25k_legacy_read.bin` | Q 旧版 I²C 驱动只读镜像 | `91A610A735CC75DDE231A6ED133026D6B5BACBCAB03ED79068187920254E35F7` | `F:/Project/GitHub/workshop/codex-doing/.tmp/hi-yaka-i2c-25k-legacy-read-20261120/dist/hi_yaka_i2c_25k_legacy_read.bin` |
| `serial-bitbang-run1.log` | R GPIO 软件 I²C 只读原始日志 | `1ED871769B8D66950A7DD0573CA547B9F1273FAFE2CC6BCE9A25FAC9DF72086D` | `F:/Project/GitHub/workshop/codex-doing/.tmp/hi-yaka-i2c-bitbang-read-20261120/serial-bitbang-run1.log` |
| `hi_yaka_i2c_bitbang_read.bin` | R GPIO 软件 I²C 只读镜像；S 后已恢复 | `E9B5DAFA4F38FB2874D5D57434A6A9EE908FB4FA162B0E8D3632BC527FAB5BB1` | `F:/Project/GitHub/workshop/codex-doing/.tmp/hi-yaka-i2c-bitbang-read-20261120/dist/hi_yaka_i2c_bitbang_read.bin` |
| `serial-mclk-run1.log` | S 受限 MCLK 候选首次复位日志 | `5D8ED20CC446ACF235650214C01C125097B596658EB152BA6FB337FF8F26A2E2` | `F:/Project/GitHub/workshop/codex-doing/.tmp/hi-yaka-i2c-bitbang-mclk-20261120/serial-mclk-run1.log` |
| `serial-mclk-run2.log` | S 同镜像第二次复位日志 | `2B44A235E9D5FD7739265ACE88E09D4CD9F56BDC1F49B2CC05CD6E7F56033D51` | `F:/Project/GitHub/workshop/codex-doing/.tmp/hi-yaka-i2c-bitbang-mclk-20261120/serial-mclk-run2.log` |
| `hi_yaka_i2c_bitbang_mclk.bin` | S 受限 MCLK 候选镜像；审计 NO-GO，已从板上恢复 R | `E71CBA9D73A289C81544800D800170DE4BCD760EEC70EA33805EC4E04DBDFAE6` | `F:/Project/GitHub/workshop/codex-doing/.tmp/hi-yaka-i2c-bitbang-mclk-20261120/dist/hi_yaka_i2c_bitbang_mclk.bin` |
| `serial-writeback-run1.log` | T 单次 `0x12` 写回原始日志；`pass=0` | `EC039ED8824B7253234E25E3C6E39E6FFA0BE4E4FCF96EA05BE271CA74323714` | `F:/Project/GitHub/workshop/codex-doing/.tmp/hi-yaka-i2c-bitbang-writeback-20261120/serial-writeback-run1.log` |
| `hi_yaka_i2c_bitbang_writeback.bin` | T 单次写回镜像 | `B11D2458FBD3F30161B4CCE434015BB324E8FF5BD0605C1DC85722C6A30141AB` | `F:/Project/GitHub/workshop/codex-doing/.tmp/hi-yaka-i2c-bitbang-writeback-20261120/dist/hi_yaka_i2c_bitbang_writeback.bin` |
| `serial-writeback-retry-run1.log` | U 有界重试首次复位日志；`pass=1` | `C45C3201B24646C39715F361AA0F8BA5A03FDC11C08076B800DEC48902356F4E` | `F:/Project/GitHub/workshop/codex-doing/.tmp/hi-yaka-i2c-bitbang-writeback-retry-20261120/serial-writeback-retry-run1.log` |
| `serial-writeback-retry-run2.log` | U 同镜像第二次复位日志；`pass=1` | `243EBAE9553699A5AFBC89C4FFD391719FA3623802966771758C4B8DE00D09A3` | `F:/Project/GitHub/workshop/codex-doing/.tmp/hi-yaka-i2c-bitbang-writeback-retry-20261120/serial-writeback-retry-run2.log` |
| `hi_yaka_i2c_bitbang_writeback_retry.bin` | U 有界重试写回镜像 | `832139D392EDB5A8C076204413359A3D9C843C2F24A9C7E2FC5E35A3E28DD921` | `F:/Project/GitHub/workshop/codex-doing/.tmp/hi-yaka-i2c-bitbang-writeback-retry-20261120/dist/hi_yaka_i2c_bitbang_writeback_retry.bin` |
| `serial-codec-retry-run1.log` | V 官方驱动完整控制面首次复位日志 | `227ABB252047AA00472DABF60AE014C42E528AB006752AD856F47CF6BB35B4DD` | `F:/Project/GitHub/workshop/codex-doing/.tmp/hi-yaka-es7210-codec-retry-20261120/serial-codec-retry-run1.log` |
| `serial-codec-retry-run2.log` | V 同镜像第二次复位日志 | `DB9611DF1A856B376B96AE23E1D3DCAD2576C424D40A16465EC3EFCEB023BB91` | `F:/Project/GitHub/workshop/codex-doing/.tmp/hi-yaka-es7210-codec-retry-20261120/serial-codec-retry-run2.log` |
| `hi_yaka_es7210_codec_retry.bin` | V 官方驱动加临时读改写补丁的控制面隔离镜像 | `3B62977D26204EB90589AE44DB2025FD587884882D1D1D142F4815B5276DBF62` | `F:/Project/GitHub/workshop/codex-doing/.tmp/hi-yaka-es7210-codec-retry-20261120/dist/hi_yaka_es7210_codec_retry.bin` |
| `es7210-check-read-before-rmw.patch` | V 官方驱动临时补丁，读改写预读失败即返回 | `B0234395714F7671E0B658F6223B37D6833D63019D655B0AC495DABC6DD2A876` | `F:/Project/GitHub/workshop/codex-doing/.tmp/hi-yaka-es7210-codec-retry-20261120/patches/es7210-check-read-before-rmw.patch` |
| `serial-clock-pcm-run1.log` | W MCU 时钟/DMA 预读首次复位日志 | `AD2CF33A0C47B78AD6057D4581D11C979BB7B56A8EE4E9D3A972205DB633430E` | `F:/Project/GitHub/workshop/codex-doing/.tmp/hi-yaka-es7210-codec-clock-pcm-20261120/serial-clock-pcm-run1.log` |
| `serial-clock-pcm-run2.log` | W 同镜像第二次复位日志 | `A0992111E6285D8CF79A042D4DBA8B7CECBC07359D770A77307D1B96D88A5079` | `F:/Project/GitHub/workshop/codex-doing/.tmp/hi-yaka-es7210-codec-clock-pcm-20261120/serial-clock-pcm-run2.log` |
| `hi_yaka_es7210_codec_clock_pcm.bin` | W 时钟与 DMA 预读诊断镜像 | `6EA5BD6CDBC7BC9D264E5E828BE3F74ABA94E2FC16EF4661F52E08239DD38070` | `F:/Project/GitHub/workshop/codex-doing/.tmp/hi-yaka-es7210-codec-clock-pcm-20261120/dist/hi_yaka_es7210_codec_clock_pcm.bin` |
| `serial-mclk-pcnt-run1.log` | X PCNT 同镜像首次复位日志 | `8EB82F5D9A2C726D573B92A2732DFC36563C4D4286B607A3A116D522A1213EEE` | `F:/Project/GitHub/workshop/codex-doing/.tmp/hi-yaka-es7210-mclk-pcnt-20261120/serial-mclk-pcnt-run1.log` |
| `serial-mclk-pcnt-run2.log` | X 同镜像第二次复位日志 | `A184EA90968B2C9E6B826ABD279B0CF5A5E28D211A47C5DF46BC1F20E69EE3FE` | `F:/Project/GitHub/workshop/codex-doing/.tmp/hi-yaka-es7210-mclk-pcnt-20261120/serial-mclk-pcnt-run2.log` |
| `hi_yaka_es7210_mclk_pcnt.bin` | X MCU GPIO0/3/12 PCNT 自检镜像 | `C401A1E6029D31AC8FDA6EA88F68E8D3AABB6B8E76FA46E48891C258DE05DC04` | `F:/Project/GitHub/workshop/codex-doing/.tmp/hi-yaka-es7210-mclk-pcnt-20261120/dist/hi_yaka_es7210_mclk_pcnt.bin` |
| `serial-tx-silence-run1.log` | Y TX+RX 静音首次复位日志 | `B39C57E77BB13FDC32A4DB6D6B075D6184DD402C21AA6D38F7A164E125A79B28` | `F:/Project/GitHub/workshop/codex-doing/.tmp/hi-yaka-es7210-tx-silence-ab-20261120/serial-tx-silence-run1.log` |
| `serial-tx-silence-run2.log` | Y 同镜像第二次复位日志 | `70A199267329206A208B2EFFE9BF02905A563E8E710B119BD656084BC2CD2E3E` | `F:/Project/GitHub/workshop/codex-doing/.tmp/hi-yaka-es7210-tx-silence-ab-20261120/serial-tx-silence-run2.log` |
| `hi_yaka_es7210_tx_silence_ab.bin` | Y TX+RX 16 kHz 静音镜像 | `70C4249B7AA2F9D7F367861E41A7D694D8E086B23248B7D9A19542E02B8EBC5A` | `F:/Project/GitHub/workshop/codex-doing/.tmp/hi-yaka-es7210-tx-silence-ab-20261120/dist/hi_yaka_es7210_tx_silence_ab.bin` |
| `serial-tx-24k-run1.log` | Z TX+RX 24 kHz 首次复位日志 | `9B25E4651FD5C04B05ADE1E678E20632E66B8A95F05DBD4D313EE37321C96C87` | `F:/Project/GitHub/workshop/codex-doing/.tmp/hi-yaka-es7210-tx-24k-ab-20261120/serial-tx-24k-run1.log` |
| `serial-tx-24k-run2.log` | Z 同镜像第二次复位日志 | `A9895E6FCACBB5FA44B3FB4B49FDCCB1ED8B52D83350F72F2EE2AD2102243D2E` | `F:/Project/GitHub/workshop/codex-doing/.tmp/hi-yaka-es7210-tx-24k-ab-20261120/serial-tx-24k-run2.log` |
| `hi_yaka_es7210_tx_24k_ab.bin` | Z TX+RX 24 kHz 静音镜像 | `2196B60AFE56D10AB26743B2010724E2586BB68320D08C4D7AA0E4556DBD566C` | `F:/Project/GitHub/workshop/codex-doing/.tmp/hi-yaka-es7210-tx-24k-ab-20261120/dist/hi_yaka_es7210_tx_24k_ab.bin` |
| `serial-tx-24k-slot32-run1.log` | AA 24 kHz / 32 bit 时隙首次复位日志 | `49F4A422472FDC0630A08E319E7124B6BCC6CB98E1938544FF2AA8C2CD21B481` | `F:/Project/GitHub/workshop/codex-doing/.tmp/hi-yaka-es7210-tx-24k-slot32-ab-20261120/serial-tx-24k-slot32-run1.log` |
| `serial-tx-24k-slot32-run2.log` | AA 同镜像第二次复位日志 | `F8927BDDABC0A3D8B18B2E77558775959D2C8FE1E37C2F137CFCBB59E545CC90` | `F:/Project/GitHub/workshop/codex-doing/.tmp/hi-yaka-es7210-tx-24k-slot32-ab-20261120/serial-tx-24k-slot32-run2.log` |
| `hi_yaka_es7210_tx_24k_slot32_ab.bin` | AA 24 kHz / 32 bit 时隙镜像 | `4E040607505B4717AC2CA52F4F2E01E17608F658BEA59366793743D9D140BE9F` | `F:/Project/GitHub/workshop/codex-doing/.tmp/hi-yaka-es7210-tx-24k-slot32-ab-20261120/dist/hi_yaka_es7210_tx_24k_slot32_ab.bin` |
| `serial-regstate.log` | 历史 `…:d6:b4` 样机 normal、PCM 全 0；非本轮同板 | `5C5A83A78C95EBD799A9BB186D8BCDE4462FE18241E472AE80A022E938DEB046` | `F:/Project/GitHub/workshop/codex-doing/projects/yaka/hi-yaka-firmware/.tmp/lab-i2s-edges/serial-regstate.log` |
| `lcd-20261112-full-boot.log` | 历史 `…:d6:c0` 同板 normal、PCM 全 0、GPIO13 零边沿 | `EF4D091FA4178B0C06DA84B177786BFCC2E4762C9201F6A0743E38C6558E8F8A` | `F:/Project/GitHub/workshop/codex-doing/projects/yaka/hi-yaka-firmware/.tmp/lcd-20261112-full-boot.log` |
| `audio-ab-5fa5b35-flash-20261112.log` | 2026-11-12 esptool BASE MAC 佐证 `…:d6:c0` | `4E5762D5B830025C914547F38E733C0B8EC63A418FD88A948EA7CAFE30B7C144` | `F:/Project/GitHub/workshop/codex-doing/projects/yaka/hi-yaka-firmware/.tmp/audio-ab-5fa5b35-flash-20261112.log` |
| `current-board-read-mac.log` | 本轮 esptool BASE MAC 佐证 `…:d6:c0` | `187874862F3F0FFF7C70214C9F62FEF9E1A59810C41E7F4574636BEBB4EAD581` | `F:/Project/GitHub/workshop/codex-doing/.tmp/hi-yaka-es7210-tx-24k-tdm4-ab-20261120/current-board-read-mac.log` |
| `serial-tx-24k-tdm4-run1.log` | AB TX STD2×32 / RX TDM4×16 首次复位日志 | `2A1E2C94C13B2D6EA6D2BEDB8C3B861030B8055CA90D3EC62AB45A280CD6C191` | `F:/Project/GitHub/workshop/codex-doing/.tmp/hi-yaka-es7210-tx-24k-tdm4-ab-20261120/serial-tx-24k-tdm4-run1.log` |
| `serial-tx-24k-tdm4-run2.log` | AB 同镜像第二次复位日志 | `B80532FB76FBCD4EEF3456C88F17E4FBB5893BB8B3F40D77EBD9C269DE3CE4DB` | `F:/Project/GitHub/workshop/codex-doing/.tmp/hi-yaka-es7210-tx-24k-tdm4-ab-20261120/serial-tx-24k-tdm4-run2.log` |
| `hi_yaka_es7210_tx_24k_tdm4_ab.bin` | AB 24 kHz / RX TDM4 诊断镜像 | `27A5806EAD89A58043F2E3D35685DEB5A97B537EF3969E1CD7AC849F09260CAB` | `F:/Project/GitHub/workshop/codex-doing/.tmp/hi-yaka-es7210-tx-24k-tdm4-ab-20261120/dist/hi_yaka_es7210_tx_24k_tdm4_ab.bin` |
| `serial-90s-run1.log` | AC 16 kHz enable 后 90 秒状态轮询日志 | `919D1F27746A74A2861EC50536032B5BA89731A25D893B429E03A05D3A0A3D69` | `F:/Project/GitHub/workshop/codex-doing/.tmp/hi-yaka-es7210-16k-90s-state-20261120/serial-90s-run1.log` |
| `hi_yaka_es7210_16k_90s_state.bin` | AC 16 kHz 90 秒状态诊断镜像 | `79339877C6984C5F8736F65B643D4E1B1CDEDBB84F9D9C5D53216E6A8664679B` | `F:/Project/GitHub/workshop/codex-doing/.tmp/hi-yaka-es7210-16k-90s-state-20261120/dist/hi_yaka_es7210_16k_90s_state.bin` |
| `serial-preclock90-run1.log` | AD 16 kHz enable 前预供时 90 秒实板日志 | `06F419A0DD93731BBB0BA39B797AC06F5D85F7042A9A94802F626E5A26BD881F` | `F:/Project/GitHub/workshop/codex-doing/.tmp/hi-yaka-es7210-16k-preclock90-20261120/serial-preclock90-run1.log` |
| `hi_yaka_es7210_16k_preclock90.bin` | AD 16 kHz enable 前预供时诊断镜像 | `0F62A8F39DB256A1FB0415252CED158B3508BA37BD2B62D7A6225E1F75D4459A` | `F:/Project/GitHub/workshop/codex-doing/.tmp/hi-yaka-es7210-16k-preclock90-20261120/dist/hi_yaka_es7210_16k_preclock90.bin` |
| `serial-gain24-run1.log` | AE 16 kHz 增益 24 dB 首次复位日志 | `26D6E5E0B2A005CD714691ED5771D5495349E388EC2F465C8323513DC96EAF8A` | `F:/Project/GitHub/workshop/codex-doing/.tmp/hi-yaka-es7210-16k-gain24-ab-20261120/serial-gain24-run1.log` |
| `serial-gain24-run2.log` | AE 同镜像第二次复位日志 | `8BBA5C37FDBED222E5C39F8141B87D4FE22E6A9E64DD00993CD2B6DCE8F08E6C` | `F:/Project/GitHub/workshop/codex-doing/.tmp/hi-yaka-es7210-16k-gain24-ab-20261120/serial-gain24-run2.log` |
| `hi_yaka_es7210_16k_gain24_ab.bin` | AE 16 kHz 增益 24 dB 对照镜像 | `2A508F14C9D314BA118AB8C6F505EAAFA7D0A7A191C61A206865ED008E041727` | `F:/Project/GitHub/workshop/codex-doing/.tmp/hi-yaka-es7210-16k-gain24-ab-20261120/dist/hi_yaka_es7210_16k_gain24_ab.bin` |
| `serial-crosscheck-run1.log` | AF I²C 只读路径交叉首次复位日志 | `0DBB7293EEDD990F31754B9B818F8E4E8FD4F09DBBB300FE2356196E23619B18` | `F:/Project/GitHub/workshop/codex-doing/.tmp/hi-yaka-es7210-i2c-read-crosscheck-20261120/serial-crosscheck-run1.log` |
| `serial-crosscheck-run2.log` | AF 同镜像第二次复位日志 | `DE06991CE23879F3ED2EFD8F241655AA58BCBAAE15A56F275834F4302B00ED5E` | `F:/Project/GitHub/workshop/codex-doing/.tmp/hi-yaka-es7210-i2c-read-crosscheck-20261120/serial-crosscheck-run2.log` |
| `hi_yaka_es7210_i2c_read_crosscheck.bin` | AF I²C 只读路径交叉诊断镜像 | `1260DA196FEAC8171BD39417002202F351E4FCC338A3E59F0CC63DB78AD31369` | `F:/Project/GitHub/workshop/codex-doing/.tmp/hi-yaka-es7210-i2c-read-crosscheck-20261120/dist/hi_yaka_es7210_i2c_read_crosscheck.bin` |
| `lcd-20261112-full-boot.log` | 同板 11/12 历史身份、状态与寄存器快照；仅作历史对照 | `EF4D091FA4178B0C06DA84B177786BFCC2E4762C9201F6A0743E38C6558E8F8A` | `F:/Project/GitHub/workshop/codex-doing/projects/yaka/hi-yaka-firmware/.tmp/lcd-20261112-full-boot.log` |
| `hi-yaka-lab-lcd-20261112.bin` | 已定位未重刷的 11/12 历史镜像候选；不算新实板结果 | `916F75CD8907DC9E0123E34B0F0AABDD04D7F28F5481491819C0237CAE7C1F4B` | `F:/Project/GitHub/workshop/codex-doing/projects/yaka/hi-yaka-firmware/.tmp/hi-yaka-lab-lcd-20261112.bin` |
| `serial-no-pullup-run1.log` | AG 内部上拉关闭首次复位日志 | `482FB8A455F6137FA588E652381684542B62B0B676E066327B16377260E9DBEE` | `F:/Project/GitHub/workshop/codex-doing/.tmp/hi-yaka-es7210-i2c-no-internal-pullup-ab-20261120/serial-no-pullup-run1.log` |
| `serial-no-pullup-run2.log` | AG 同镜像第二次复位日志 | `4688AA9A67A4BDDC455C6307E1758EE4F05DB2BA4917B375B0875883F65A9B55` | `F:/Project/GitHub/workshop/codex-doing/.tmp/hi-yaka-es7210-i2c-no-internal-pullup-ab-20261120/serial-no-pullup-run2.log` |
| `hi_yaka_es7210_i2c_no_internal_pullup.bin` | AG 内部上拉关闭对照镜像 | `F13A151BFFE2DD5CFD4DC42D51D673C302275075B7AF537CEB6E85ED9876152E` | `F:/Project/GitHub/workshop/codex-doing/.tmp/hi-yaka-es7210-i2c-no-internal-pullup-ab-20261120/dist/hi_yaka_es7210_i2c_no_internal_pullup.bin` |
| `serial-wrapper-run1.log` | AH input-only wrapper 路径首次复位日志 | `2F9F375AC7711AC91110CF57338E3A2D2E93C804AE28D99957557CDADCDE57ED` | `F:/Project/GitHub/workshop/codex-doing/.tmp/hi-yaka-es7210-wrapper-input-only-20261120/serial-wrapper-run1.log` |
| `serial-wrapper-run2.log` | AH 同镜像第二次复位日志 | `14008939FC2B305162EA32BA93DDB7BBE232417EE9A4C4568762B9F9ACA80EE2` | `F:/Project/GitHub/workshop/codex-doing/.tmp/hi-yaka-es7210-wrapper-input-only-20261120/serial-wrapper-run2.log` |
| `hi_yaka_es7210_wrapper_input_only.bin` | AH input-only wrapper 路径诊断镜像 | `1F7624062F7106A7DF31330EE4212625A7CB009E0B9934D293D8026E00220BC2` | `F:/Project/GitHub/workshop/codex-doing/.tmp/hi-yaka-es7210-wrapper-input-only-20261120/dist/hi_yaka_es7210_wrapper_input_only.bin` |
| `dependencies.lock` | 历史 Yaka HEAD `d4c5c51`，锁定 `esp_codec_dev 1.3.5` | `39E4151E96DC3010E20ADB325D5E84690C95675DD4D5B643DCFFD03278ACF863` | `F:/Project/GitHub/esp32-project/yaka-firmware/dependencies.lock` |
| 历史 `es7210.c` | 历史 Yaka 1.3.5 组件中的原始驱动源码 | `8C741993683579FFBFFD05A227533F4A54D086921DE8ED99FF6C12F6574D5921` | `F:/Project/GitHub/esp32-project/yaka-firmware/managed_components/espressif__esp_codec_dev/device/es7210/es7210.c` |
| 当前 `vendor-original/es7210.c` | 1.6.2 实验中的未打补丁原始驱动源码 | `8C741993683579FFBFFD05A227533F4A54D086921DE8ED99FF6C12F6574D5921` | `F:/Project/GitHub/workshop/codex-doing/.tmp/hi-yaka-es7210-codec-retry-20261120/vendor-original/es7210.c` |
| B 轮诊断镜像 | 仅移除初始化前三次预读 | `3512F9EC564FF6BE03D40D3E1DE16AA9F983A93E3BA68BB43695E89AAEF32EEB` | `.tmp/hi-yaka-es7210-gate-no-preread-20261120/`，文件名待核对 |
| `ota0-before.bin` | 烧录前应用分区备份，供恢复 | `2B2BF4939DB4BA60A72465792C55F1B37BAB1D51ABBC65ABEBFFB8F2CE505719` | `.tmp/hi-yaka-es7210-gate-20261120/ota0-before.bin` |
| 待记录 | 原始 PCM / 统计 | 待记录 | 待记录 |
| 待记录 | 固件构建输出 | 待记录 | 待记录 |

日志摘录仅保留判定所需的原行，完整原始日志以文件和哈希追溯。任何串口掉线、重启或烧录失败须另行记录，不能从缺失的时间段推断器件行为。

## 6. 结论与下一门禁

- **已确认（高）：**A/B 两轮 ES7210 首笔寄存器写均失败，移除三次预读未改变结果；C1 热启动原始写失败；C2–E 的 `0x41` 在各自测试条件下无应答；F/G 同板重放中 mixed `0x41` 四阶段各 16/16 ACK，G 的 phase 0/3 身份寄存器读取均返回 `0x72/0x10`，但 G 的 phase 3 独立 `0x41` probe 为 0/16，单独 combined read 仍失败。H 同启动首笔 `0x41` NACK，mixed 后 phase 0 为 15/16、后 3 阶段各 16/16 应答。I 移除 PCA 写后 mixed `0x41` 仍四阶段各 16/16，但 100 kHz mixed `0x0B` 多数成功返回为 `0xFF`，身份 `0x3D` 也读为 `0xFF`。J 的 25 kHz 身份值在 100 kHz 空地址 probe 前为 `0x72/0x10`、之后两次读取均失败；因速率切换与操作顺序混杂，不能归因空地址 NACK。K 的 25 kHz 极简读从 round 1 起部分寄存器可读，末尾完整身份可读，但首笔和每轮首个 `0x3D` 均失败。L 中 bus reset 返回 ESP_OK 后身份读仍失败，随后 mixed 读恢复。M 与 K 交换三寄存器顺序后失败跟随约 50 ms 空闲后的首笔 `0x0B`，随后 `0x3E/0x3D` 真值成功；不是 `0x3D` 特异。N 的写入前门禁未通过，测试写入未发生；O 的 25 kHz burst 出现 `0x3D=0x77` 错误值，P 的 10 kHz burst 中 `0x3D` 三读均失败。Q 旧版 I²C 驱动的两次复位均复现首笔失败与 `ESP_OK` 配错误的 `0x3D=0xFF`，且 `0x3E` 各有一次超时。R 避开硬件 I²C 控制器后仍有空闲后首笔地址写 NACK；S 候选增加 I²S 静音发送后也未消除 NACK，且代码审计 NO-GO、未测物理 MCLK 波形，S 后曾恢复 R。T 单次写回首次地址 NACK、`pass=0`；U 两次复位经有限次重试均将 `0x12` 从 `0x00` 写至 `0x20` 并读回，再恢复 `0x00` 并读回，`pass=1`，但两步首次写仍 NACK。V 两次复位中官方驱动控制调用均返回成功，多寄存器写入与读回一致；然而在已执行 enable 后 `0x0B` 仍为 power-down 的 `0x00`，没有 PCM。`0x43/0x44=0x10` 与驱动最终写入一致，日志硬编码 `0x1A` 是诊断预期错误。G 未初始化时 `0x0B=0x00` 与手册默认值相容。W 两次启用 I²S RX，MCU GPIO3/12 软件读得边沿，但 DMA primer 的左右 256 词各全 `-1`，`0x0B` 前后均 `0x00`；后续三窗 PCM 候选因状态门禁失败跳过。X 两次同镜像 PCNT 进一步读得 MCU GPIO12/WS≈16.37 kHz、GPIO3/BCLK≈512 kHz、GPIO0/MCLK≈4.096 MHz，状态与 DMA 结果不变；未测 U12 芯片焊盘。Y 加 TX 静音后两次各 71 次完整写、58 次失败写，PCNT 三钟近似，`0x0B` 和 DMA 全 `-1` 未改善。Z 仅将采样率从 16 改至 24 kHz，两次 MCU 三钟升至约 23.81 kHz/767.73 kHz/6.14 MHz，控制面无终态失败，但 `0x0B=0x00`、DMA 全 `-1` 未改善；TX 每次仍有 57 次失败写。AA 仅把 24 kHz 时隙从 16 加宽至 32 bit，BCLK 约翻倍，但两次 `0x0B=0x00`、DMA 全 `-1`，TX 各有 56 次失败写。AB 将 RX 改为四槽 TDM 后两次 `0x0B=0x00`，2048 B DMA 全 `-1`，TX 各有 57 次失败写。AC 在 enable 后等至 90 秒，18 次状态仍 `0x00`；TX 有 5647 次完整写与 4519 次失败写。AD 在 enable 前预供时约 90.125 秒后，状态至 enable 后 6 秒仍 `0x00`、DMA 全 `-1`；TX 有 6034 次完整写与 4828 次失败写。历史 Yaka 锁定的 1.3.5 与当前 1.6.2 未打补丁原始 `es7210.c` 哈希相同。
- **样机身份与历史状态（高）：**早期 `…:d6:b4` 与本轮 `…:d6:c0` 是不同物理板，不能做同板 A/B；但 11/12 的 `…:d6:c0` 与本轮 esptool 记录为同板，曾在 ACT_B 录音时读到 `0x0B=0x02`，与此同时原始 RX 全 0、GPIO13 零边沿。故“本板永久无法进入 normal”的推断不成立；normal 也不是 PCM 正常的证据。11/12 与本轮部分寄存器值/固件访问序列不同，具体差异根因未定。完整 MAC 只存原始日志，公开文档使用遮蔽标识。
- **推断及置信度：**地址应答和读事务成功受镜像或访问顺序影响（高）；单纯器件永久损坏或永久断线无法解释同板 F 重放的应答（中），但尚不能定位确切触发事务。三次预读造成首笔写失败被 A/B 削弱（高）；Q 的旧驱动同样出现首笔失败和错误读值，削弱“新版 I²C API 独有故障”的解释（高）；R 的软件 I²C 同样出现地址 NACK，说明硬件 I²C 控制器并非该现象的唯一必要条件（高）。S 只支持“候选软件发送静音时故障仍在”的受限观察，不支持物理 MCLK 已输出或 MCLK 因果结论。仍不区分芯片、主机时序与电气路径。测得的供电、绑带和地桥测点符合设计（高），但不能证明 U12 焊脚、内部状态或 I²C 时序正常。D 排除缺少 I²S 时钟是 C4 地址 NACK 的必要条件；E 排除单纯降至 25 kHz 即可恢复其地址应答（均高）。当前控制面已证明单寄存器可逆写，以及官方序列在有界重试下可完成多寄存器写入/读回；但 `0x0B` 未进入 normal，仍有瞬态失败；W 的单次 DMA primer 无有效波形，完整三窗 PCM Gate 未执行，麦克风仍为 FAIL / NO-GO。MCU GPIO 边沿不等于 U12 输入脚已收到时钟。
- **仍未知：**U12 芯片脚时钟/供电、U12.11→R65→GPIO13 分段波形等未取得的新证据。
- **下一门禁：**围绕“空闲后首笔事务”做受控对照，分离等待时间、事务格式、主机配置与总线电气条件；另在相同上电条件下分离 probe 的默认速率切换、空地址 NACK 和前序状态。10 kHz/25 kHz 先前只读结果不足以证明稳定写入，bus reset 也不足以立即恢复读值。如今 `0x12` 可逆写回及官方多寄存器控制调用/读回已通过受限测试；下一门禁是澄清 `0x0B` 长期 power-down 的条件，隔离 MCLK、芯片状态机及供电/电气变量，记录物理时钟或有约束的软件时钟证据。先区分 MCU GPIO 输出边沿与 U12 时钟脚实际输入；只有状态正常且控制面稳定后才运行三窗 PCM Gate，不能把 DMA 返回 1024 B 或全 `-1` 的 `nonzero=512` 当作麦克风验收。

此附件完成后，只将业务结论、阻断状态和下一优先动作压缩回写到 [YAKA-0006 飞书摘要页](https://scnlb1lk96sb.feishu.cn/docx/Q1u5df5QWomTj0x0xkfcSoz4nvh)；详细命令、寄存器和长日志留在本附件。

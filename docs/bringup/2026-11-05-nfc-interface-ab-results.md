# HiYaKa NFC 主机接口 A/B 实板记录

> 日期：2026-11-05（Asia/Shanghai）
> 对象：先前开发样机，L7 已拆，MASTER ON，COM5/CH340K；样机编号与 PCB 实装修订号未独立记录
> 范围：仅 PN532 宿主接口；未开启 RF 场、未读卡；问题仍为 `HIYAKA-NFC-INC-001 / YAKA-0005`
> 构建：`aliyun_raynode` / `espressif/idf:v6.1`；Windows 只烧录和采串口

## 1. 对照基线

| 项目 | 已验收旧版硬件工程 | 当前 HiYaKa 工程 | 结论 |
|---|---|---|---|
| PN532 与晶振 | U10，LCSC `C880904`，PN5321A3HN/C106,51；X1 `C524673`，27.12 MHz/10 pF | U13、X1 的器件号和参数相同 | 不能从料号推断本次两块实物的焊接或芯片真伪。 |
| 主机模式绑带 | I0/SET0 经 R62 10 kΩ 下拉，I1/SET1 经 R63 10 kΩ 下拉，`I1:I0=00` | I0 经 R73 10 kΩ 下拉，I1 经 R74 10 kΩ 上拉到 `SYS_3V3`，`I1:I0=10` | 与 NXP 模式表对应：旧版 HSU、当前版 I²C。网络事实由两工程活体 EDA 回读。 |
| 宿主连线 | U10.27/28 到 `IO17_HOST_TXD` / `IO18_HOST_RXD` | U13.27/28 到 GPIO11/SCL、GPIO10/SDA | 改用 I²C 是本次显著设计差异。 |
| 核心供电与复位 | VBAT=`SYS_3V3`；PVDD、DVDD=`VREF`；复位 10 kΩ 上拉和 100 kΩ 下拉 | 同一拓扑，位号分别为 R71/R75 | PVDD 接内部 VREF 不符合 NXP 对宿主供电的推荐描述，但并非新版独有变化；不得直接宣称为已定位根因。 |
| 历史固件证据 | `ChatChatTech/yaka-firmware@540c9ce` 的板级实现未发现 PN532 驱动；`main/dev/duck` 源码检索也未找到 | 现固件有 PN532 I²C 驱动及上电、延时探测 | 操作者报告旧版 NFC 已验收，实际验收程序/日志尚未找到；不能把历史仓库代码说成当年的 NFC 验收实现。 |

NXP 官方文档规定接口选择 `00=HSU`、`10=I²C`；I²C 写/读地址分别为 8 位 `0x48/0x49`，因此当前固件使用 7 位 `0x24` 正确。NXP 同时描述 PVDD 应接宿主供电、DVDD 由片内 LDO 产生；当前和旧版工程都没有按该典型连接方式实现。

## 2. 实板操作与结果

先前已存的无屏幕实验固件重启后，四个其它 I²C 器件 `0x18/0x19/0x41/0x6B` 应答、PN532 `0x24` NACK；三次版本请求和约 5.6 秒的延时探测失败。独立诊断镜像 SHA-256 为 `422adb7b857eadec871172bee563192528502f3b02446942e761e32199d0938d`，编译为 ESP32-C6/16 MiB/DIO；COM5 以 115200 baud 烧到现有 OTA0 应用区，esptool 写入哈希校验通过。

诊断镜像启动后，三轮 I²C 探测均只有 `0x24` 返回 `ESP_ERR_NOT_FOUND`，其它四个地址每轮均为 `ESP_OK`。向 `0x24` 直接发送 PN532 `GetFirmwareVersion` 原始帧，返回 `ESP_ERR_INVALID_RESPONSE`。随后释放 I²C 并把同两脚切为 PN532 HSU 对应的 UART TX/RX；三轮发送 `0x55` 唤醒字节及版本命令后，接收字节数均为 0。此时 GPIO23/PA_EN 保持低电平。未做芯片脚、晶振、模式绑带的当次波形或电压测量。

实验后烧回原 `hi-yaka-lab-headless` 应用，镜像 SHA-256 为 `8a140d79fdff64eedcb0f0434746c4ab9000881f9c0fb6d0e6036fe2fd1ffba7`，写入哈希校验通过。重启日志中的项目名 `xiaozhi`、编译时间 `Nov 4 2026 15:49:45`、ELF SHA 前缀 `850aaa7ab` 与实验前一致；PA 安全互锁有效，NFC NACK 继续复现。

| 原始日志（本地 `.tmp/`，不入 Git） | SHA-256 | 用途 |
|---|---|---|
| `nfc-old-board-reboot-2026-11-05.log` | `f6edda8748c7e372263078ab8b4de6d031952a42a6be5f3dd728ab3391112cdd` | 实验前正常固件重启 |
| `nfc-interface-ab-old-board-2026-11-05.log` | `7f08e93e483b44e55df101ff7018393f4292df72e0ab4972c681ad3ccb344fe1` | 独立 I²C/HSU A/B |
| `nfc-old-board-restored-2026-11-05.log` | `533f7a3323b4cecf36f6f49d9f248379e8c9409f11d960f3d872c4b1713eb0b6` | 原镜像恢复与故障复现 |

## 3. 判别与下一步

- **已确认，置信度高：**`0x24` 的地址级 NACK 不依赖现固件 PN532 库；直接原始命令也失败，且其它共享 I²C 器件正常。它不是“没找到 NFC 标签”。
- **推断，置信度中：**单纯误入 HSU 模式的可能性下降，因为按旧版引脚方向配置 UART 并发出唤醒/版本帧后未收到数据；但无响应也可能是供电、复位、晶振、焊接或线到芯片脚的问题，不能据此证明 I²C 模式已被实际采样。
- **未知：**U13 芯片脚处的 VBAT/PVDD/DVDD、RSTPD_N、I0/I1、27.12 MHz、SDA/SCL 波形；旧版 NFC 具体验收程序；当前两块样机的 U13 实装质量。软件实验不能把这些未知项变成已确认事实。

下一次具备维修台夹具时，先同一上电窗口记录 U13.40/VBAT、.23/PVDD、.39/DVDD、.38/RSTPD_N、.16/I0、.17/I1 的实际电压，再看 X1 是否起振，最后看 U13.27/.28 是否收到地址 `0x48`（8 位写）及 ACK 时隙。若电源/绑带/时钟和芯片脚总线均成立而仍无 ACK，优先用 X-ray 与定点换 U13 做同板 A/B；若这些前置条件不成立，按第一个失效节点返修。下一版 ECO 应将 PVDD 改为宿主供电并验证，不应把现板的 VREF 与 `SYS_3V3` 直接短接。

## 4. 来源

- 当前及旧版活体 EasyEDA Pro V4.1.71 工程的 PN532 页、器件属性和网络；同一 easyeda-agent v1.10.0 服务端口 `60832` 通过窗口 ID/工程 UUID 分别读取。
- [NXP PN532/C1 数据手册](https://www.nxp.com/docs/en/nxp/data-sheets/PN532_C1.pdf)：Table 72 的宿主模式、§8.4 的 PVDD/DVDD 供电；[PN532 用户手册](https://www.nxp.com/docs/en/user-guide/141520.pdf)：I²C 地址、HSU 唤醒及 `GetFirmwareVersion` 帧。
- [NFC 专项问题](HIYAKA_NFC_NO_ACK_INCIDENT.md)与本次三份原始串口日志。

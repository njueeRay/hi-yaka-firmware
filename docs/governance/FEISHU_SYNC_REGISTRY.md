# HiYaKa 飞书文档同步登记

> 最后更新：2026-11-05
> 飞书目录：[HiYaKa](https://scnlb1lk96sb.feishu.cn/drive/folder/Qz1ifJDCvlYKz1dCBO2cFl9Bn3c)
> 原则：Git Markdown 保存可审计的工程事实源；飞书保存协作副本、跨文档索引和附件入口。

当前飞书目录按用途分为 `01 产品与使用`、`02 硬件与固件研发`、`03 协作、决策与调研`、`04 测试与发布`、`90 制造与发布归档` 和 `99 历史资料（只读）`。旧 `02 工程基线与接口契约` 已废止并移入历史区，当前接口事实以硬件主文档和硬件连接与固件接口表为准。

## 1. 主文档映射

| 本地事实源/治理对象 | 飞书文档 | Token | Revision | 当前状态 |
|---|---|---|---:|---|
| `docs/governance/PROJECT_CHARTER.md` 与项目索引 | [00 HiYaKa 项目总览与文档导航](https://scnlb1lk96sb.feishu.cn/docx/Kl74deqLtoteWyxeshlci43qn0g) | `Kl74deqLtoteWyxeshlci43qn0g` | 44 | 顶层唯一入口；当前阶段、U11/BOOST `NO-GO`、L7 隔离边界、M2–M6 L0 与受控恢复顺序已回读 |
| `.agents/skills/yaka-project/references/yaka-3.0-prd.md` | [01 产品需求与交互状态](https://scnlb1lk96sb.feishu.cn/docx/T3isd0m8jo2bVEx37noc9Oq3noh) | `T3isd0m8jo2bVEx37noc9Oq3noh` | 53 | 完整产品 PRD v0.4；现行产品/系列语义统一为 HiYaKa；OTA 已对齐 MVP Must 与验收矩阵；位于 `01 产品与使用` |
| `docs/product/PRODUCT_DATA_AND_USER_MANUAL.md` | [02 产品规格与使用手册](https://scnlb1lk96sb.feishu.cn/docx/XohYdP65uo5zfTxkMEScyzXinag) | `XohYdP65uo5zfTxkMEScyzXinag` | 5 | 现行产品语义统一为 HiYaKa；位于 `01 产品与使用` |
| `docs/governance/FIRMWARE_PRD.md` | [01 固件 PRD 与里程碑](https://scnlb1lk96sb.feishu.cn/docx/DwmvdwnfboJKX3xpbyIc1b0bntc) | `DwmvdwnfboJKX3xpbyIc1b0bntc` | 195 | 麦克风 A/B 实板结果、固定码值、ES7210 状态冲突和未通过结论已回读；架构 v1.6 附件保留 |
| `docs/hardware/HARDWARE_DESIGN_AND_DEVELOPMENT.md` | [02 硬件设计与研发主文档](https://scnlb1lk96sb.feishu.cn/docx/I2SIdpqZnouTxkxYEuLcnyJxndc) | `I2SIdpqZnouTxkxYEuLcnyJxndc` | 141 | 3.6 已标明麦克风旧长版为历史附件，当前 LCD/NFC/麦克风三份六节专项分析及动态状态以独立台账记录为准 |
| `docs/hardware/HARDWARE_MODULE_INTERFACES.md` | [03 硬件连接与固件接口表](https://scnlb1lk96sb.feishu.cn/docx/UdVhdzlP6oAxXZxzyMIc25SxnMk) | `UdVhdzlP6oAxXZxzyMIc25SxnMk` | 46 | ACT_B/PWR_B 实板映射、IMU 基础数据、GPIO13 无翻转、无屏幕 NFC 对照及 R63/R64/R65 阻值已回读；PA 继续硬禁用 |
| `docs/hardware/HIYAKA_U11_THERMAL_INCIDENT_AND_RECOVERY.md` | [04 U11 异常发热失效复盘与现板恢复方案](https://scnlb1lk96sb.feishu.cn/docx/XoLCdhpQqoQjjDxHEhyciFfxnJe) | `XoLCdhpQqoQjjDxHEhyciFfxnJe` | 8 | `HIYAKA-HW-INC-001` 根因保留；新增 L7 拆除后的现场观察、开发隔离条件与发布决定 |
| `docs/governance/DEVELOPMENT_COLLABORATION_AND_DOCUMENTATION_STANDARD.md` | [01 开发协作与文档维护规范](https://scnlb1lk96sb.feishu.cn/docx/K33VdjETzooOrTxIQD9cOYXln5c) | `K33VdjETzooOrTxIQD9cOYXln5c` | 15 | 已回读执行环境边界：Windows 只做烧录/串口/实板调试，构建与测试统一在 `aliyun_raynode` 或 GitHub Actions/Linux |
| 仓库、上游与技术决策 | [02 决策与版本记录](https://scnlb1lk96sb.feishu.cn/docx/NWmcdvVpgov54exQOkIcvvOunmd) | `NWmcdvVpgov54exQOkIcvvOunmd` | 14 | 保存正式 ADR 与版本影响 |
| `.agents/skills/yaka-project/references/yaka-3.0-owner-decisions-and-todos.md` | [03 当前角色、Owner 决策与待办](https://scnlb1lk96sb.feishu.cn/docx/Y5SqdV9zloa3eLxQZALcTDQ4nOe) | `Y5SqdV9zloa3eLxQZALcTDQ4nOe` | 17 | 已覆盖旧静态放行与“等待样机”状态；同步 L7 隔离、PN532/QMI8658A、WAV、M1 恢复边界及下一版 ECO 待办 |
| 外部资料治理与 `docs/research/VELXIO_SIMULATION_EVALUATION.md` | [04 外部参考与技术调研](https://scnlb1lk96sb.feishu.cn/docx/Cn2bdVrSboCRKLxuDErcWWSUnpc) | `Cn2bdVrSboCRKLxuDErcWWSUnpc` | 11 | 现行适用版本统一为 HiYaKa；位于 `03 协作、决策与调研` |
| `docs/governance/EVIDENCE_POLICY.md`、`docs/governance/ISSUE_ANALYSIS_TEMPLATE.md` 与 `docs/bringup/` | [01 测试与发布](https://scnlb1lk96sb.feishu.cn/docx/RXKYdmVHaobeHNx39uNcl8BAnbf) | `RXKYdmVHaobeHNx39uNcl8BAnbf` | 51 | 2026-11-05 模板已加入“记录方式”并附 Markdown；首页当前结论和 YAKA-0004/5/6 入口已回读，动态状态由多维表格维护 |
| 三项独立专项问题与动态状态 | [HiYaKa 反馈与处理台账](https://scnlb1lk96sb.feishu.cn/base/PE5BbvVtOaHxI4soxwKc9w7Cn7b) | Base `PE5BbvVtOaHxI4soxwKc9w7Cn7b` / 表 `tblLefBEZhKEUhcC` | 43 | YAKA-0004 LCD=`reczz28VrGurKogr`、YAKA-0005 NFC=`reczz28VrGurLWAH`、YAKA-0006 麦克风=`reczz28VrGurNE5G`；逐条回读为“处理中”且各有独立分析附件 |
| 历史仓库与旧资料归档规则 | [00 历史资料索引](https://scnlb1lk96sb.feishu.cn/docx/KlRxdMk4ho6A5Txs3dqc3KG2nLd) | `KlRxdMk4ho6A5Txs3dqc3KG2nLd` | 10 | 位于 `99 历史资料（只读）` |
| 已废止的旧接口摘要 | [90 [已废止] 工程基线与接口契约](https://scnlb1lk96sb.feishu.cn/docx/NBzHd2mvgoHQ06xLNXOc0A0tnoe) | `NBzHd2mvgoHQ06xLNXOc0A0tnoe` | 24 | 仅供历史追溯，不得作为当前接口输入 |

2026-11-01 完成目录分层、文档重命名、旧接口摘要废止及制造状态纠偏；当前映射以本登记文件所在 Git 提交为准。

2026-11-02 使用 `lark-cli` 完成产品 PRD 与固件 PRD 的章节级同步和回读：产品 PRD revision 24，固件 PRD revision 29。同步过程未全文覆盖云文档，保留既有图片、评论和历史块；架构材料采用新版本文件名追加，未静默替换旧附件。

2026-11-02 完成 HiYaKa 命名合同同步：可读/驼峰形式统一为 `HiYaKa`，全小写短横线形式为 `hi-yaka`，全大写下划线形式为 `HI_YAKA`；板子归属统一写 ChatChat。固件 PRD 更新到 revision 47，硬件主文档更新到 revision 58，协作规范更新到 revision 10；架构图 v1.2 的四页正文图片及 Markdown、draw.io、PDF 附件均已回读。对应私有仓库提交：`25cd393`。

2026-11-02 完成无代际后缀命名复核与架构图语义治理：HiYaKa 自有代码、构建标识、路径、现行文档及架构图不再使用代际后缀；四页架构图均重复完整的七色语义和实线/虚线语义。固件 PRD 更新到 revision 64，硬件主文档更新到 revision 70，协作规范更新到 revision 12；架构图 v1.3 的四页正文图片及 Markdown、draw.io、PDF 附件均已回读；四份硬件审计材料的命名对齐版作为新附件追加，旧附件保留历史追溯。

2026-11-02 完成 PH2 阶段闭环同步：私有仓库 PR #1 已合入 `main`；ESP-IDF 6.1 主目标、host contract、board contract 和 18 个代表性上游板型构建全部通过。飞书固件 PRD 的 PH2 六项 checklist、阶段证据、PH3 状态、当前结论与架构评审状态已局部更新并回读到 revision 71；未覆盖既有图片、附件、评论或历史块。PH2 仅关闭 L0 软件阶段，不据此关闭 M1 或创建产品 Release。

2026-11-02 完成 PH3 阶段闭环同步：私有仓库 PR #3 已合入 `main`；可配置输入、对话恢复、可靠录音、宿主故障注入、辅助能力降级和页面流程均通过 host contract、ESP-IDF 6.1 主目标、board contract 与 18 个代表性上游板型构建。飞书固件 PRD 的 PH3 六项 checklist、阶段证据、PH4 状态、当前结论与架构评审状态已局部更新并回读到 revision 76。宿主文件系统证据不替代 TF/FAT 掉电和实板验证，不创建产品 Release。

2026-11-02 完成 PH4 阶段闭环同步：私有仓库 PR #5 已合入 `main`；ESP-IDF 6.1 主目标、host contract、board contract 和 19 个代表性板型构建全部通过。飞书固件 PRD 的 PH4 六项 checklist、资源实测、PH5 状态和当前结论已局部更新；架构 Markdown、draw.io 编辑源和四页 PDF 更新为 v1.4，旧 v1.3 展示附件从正文移除但 token 继续保留用于历史追溯。命名修订后的 Markdown 以当前 SHA-256 重新上传，旧 v1.4 Markdown 展示块删除但 token 继续保留。关键章节和附件均已回读到 revision 99。PH4 仅关闭 L0 软件合同，生产签名、真实掉电、首启确认和实板回滚仍未验证，不创建产品 Release。

2026-11-02 完成 PH5 阶段闭环同步：私有仓库 PR #7 已合入 `main` 提交 `c6de179`；18 项 HiYaKa 定向测试、ESP-IDF 6.1 构建、资源门禁和候选产物上传全部通过，合并镜像 SHA-256 为 `029a24d9cc436899f5eeabcfd41ae97d97a50f762e45cfebcd87181a93471f57`。飞书固件 PRD 的 PH5 五项 checklist、当前结论、架构说明/图片和附件已回读到 revision 125；项目总览的当前阶段和更新时间已回读到 revision 35；硬件主文档周日志回读到 revision 81；测试发布页新增首板验证章节和四份附件并回读到 revision 18。PH5 只关闭 L0 测试设计与工具准备，全部真实硬件状态仍为未知，M1 不关闭，也不创建产品 Release。

2026-11-02 完成现行语义复核：skill、产品 PRD、产品手册、硬件主文档、接口表、Owner 决策、项目总览、测试发布页和外部调研页统一使用 `HiYaKa`；文件系统中的历史附件名、旧仓库名、第三方上游板型和外部许可证版本继续按原名保留。飞书逐页精确替换并回读，未全文覆盖或改写历史附件。

2026-11-02 完成 M2–M5 实板前适配同步：私有仓库 PR #11 合入 `main` 提交 `5838a1e`；21 项定向测试、ESP-IDF 6.1 安全/实验构建、20 个构建变体、资源门禁、provenance 与候选产物上传全部通过。飞书固件 PRD 的 M2–M5 状态、L0 说明、当前进度和下一步已回读到 revision 150；接口表的 LCD/PARLIO 与 TF/SPI2 资源合同回读到 revision 22；硬件主文档单一周日志回读到 revision 83。v1.5 Markdown、draw.io、四页 PDF 和四页正文图已追加并回读。所有新增结论均为 L0，M1–M5 不关闭，不创建产品 Release。

2026-11-02 完成顶层入口与执行台账对齐：项目总览把当前阶段更新为“M1 安全基线与 M2–M5 实板前 L0 适配就绪”，回读到 revision 37；Owner 决策与待办把固件角色重点和 `YAKA-ACT-002`–`YAKA-ACT-005` 更新为“L0 已完成、等待实板验证”，回读到 revision 15。本地项目 skill 的固件状态和 Owner 待办引用同步更新。

2026-11-02 完成公开快照复核：`ChatChatTech/yaka` PR #2 合入 `main` 提交 `0489e83`；公开 main CI #41394982112 的主机契约、21 项定向测试、全部实板前适配编译、ESP-IDF 6.1 正式构建、资源门禁和产物上传全部通过。应用镜像为 1,930,544 / 3,866,624 bytes（49.93%），DIRAM 为 126,429 / 452,112 bytes（27.96%），合并镜像 SHA-256 为 `53d46ad164755da084d1d27aaf482a068c750afd131a73903b522b9cbde86573`。硬件主文档单一周日志回读到 revision 84。上述证据仍为 L0，不表示实板通过。

2026-11-04 建立 `HIYAKA-HW-INC-001`：两块样机在 U11/BOOST 支路复现快速异常发热；确认 GPIO23/PA_EN 复位弱上拉与错误 L7 精确料号形成 P0 失效链。新增飞书原生专项复盘 revision 2；硬件主文档 revision 88、固件 PRD revision 153、接口表 revision 25、测试发布页 revision 19 已完成增量同步、旧状态定点修正和回读。当前未返修样机、直接烧录、扬声器播放和下一版制造均为 `NO-GO`；拆除 U11 或完成等效物理隔离后，才能从限流 M1 重新建立 L1 证据。专项 Markdown SHA-256：`040d147e9fadb9d2a60b7747e8150331f0990a54fbe42e0a784f8a376790efe1`。

2026-11-04 完成首板风险遏制与全外设 L0 增量同步：当前开发样机拆除 L7 后不再快速升温，`HIYAKA-HW-INC-001` 根因和原始 `FAIL` 证据不覆盖，非功放 M1 调整为受控 `CONDITIONAL GO`。PN532/QMI8658A 已从地址探测升级为完整 L0 业务驱动，实验室构建新增麦克风和解码输出 WAV；PR #15 的板级/主机合同、产品核心及 20 个代表性板型矩阵全部通过。固件 PRD revision 157、硬件主文档 revision 90、接口表 revision 31、专项复盘 revision 8、项目总览 revision 44、Owner 决策台账 revision 17 均已更新并回读；操作者离场期间 MASTER OFF，功放播放和下一版制造继续 `NO-GO`。

同日追加架构 v1.6：将 M2–M6 L0、PN532/QMI8658A、数字音频 WAV、L7/PA 禁用边界和 `aliyun_raynode` 命名写回唯一 draw.io 源，并重新检查四页统一颜色/线型图例。旧 v1.5 附件不覆盖；v1.6 的 Markdown、draw.io、PDF 和四页正文图追加到固件 PRD，回读到 revision 171。

同日冻结执行环境边界：Windows 本地机只承担 USB/串口、烧录、实板调试和日志采集；固件构建、host tests、合同检查、资源门禁和候选产物统一在 `aliyun_raynode` 或 GitHub Actions/Linux 完成。项目 skill 已加入同一硬规则并通过结构校验；飞书协作规范回读到 revision 15。

同日首板专项诊断同步：LCD 寄存器回读已发生，但背光 IO0 开漏且无上拉，R85 两端约 0 V，屏幕全黑；PN532 `0x24` 持续 NACK，但 C108/VREF 与 `RES&OFF` 均约 3 V，先前“芯片未供电”判断已撤销；麦克风 TDM 与标准双麦 I²S 的原始 DMA 均全零。固件 PRD revision 187、接口表 revision 36、硬件主文档 revision 99 已局部编辑并回读；硬件主文档文末上传专项 Markdown，3.1 建立索引，同一自然周只保留一个周日志。

同日追加双声道实板对照：提交 `a8edac9` 的实验室固件经 `aliyun_raynode` ESP-IDF 6.1 构建，GitHub Actions 板级契约、产品核心和板型矩阵均成功；COM5 首次高速写入中断，降为 115200 baud 后写入与哈希校验成功。冷启动日志确认 I²S RX 双声道，原始 DMA 和 TF WAV 均全零；全地址扫描仅见另外四颗 I²C 器件。飞书固件 PRD revision 188、硬件主文档 revision 103 已局部同步并回读；最新版专项诊断附件已追加并重新索引。

2026-11-05 首板诊断增量：活体网表和实板 ADC 事件纠正 `PWR_B` 与 `ACT_B`；后者红灯、WAV 启停复验通过。MCU 侧三路 I²S 时钟活动、GPIO13/SDOUT1 无翻转；ES7210 状态为正常、双麦选通且未静音，音频数据仍全零。无屏幕构建的 240 秒对照中 PN532 仍 NACK，未见 PCA9557 间歇重试；有屏幕构建仍见重试，因条件不同不直接归因。操作者断电实测 R63/R64/R65 约 0/0/51 Ω。固件 PRD revision 194、硬件主文档 revision 122、原生接口表 revision 46 已定点同步并回读；最新接口和专项 Markdown 附件已追加并在 3.1/3.2 索引。

同日追加旧版对照：只读核验已验收 YaKa v2.0 的 EasyEDA Codec/MCU 页与历史固件 `540c9ce`，ES7210、两颗 ZTS6216、MICBIAS/供电结构及 S3 引脚配置相互吻合；与 HiYaKa 的主要差异为 S3→C6 引脚复用、采样率/时隙及驱动版本。当前零音频根因仍未确定。实验室固件提交 `6f4e26c` 增加 GPIO12/13 只读路由记录，尚待实板回读。专项 Markdown 最新版已追加至硬件主文档末尾，3.1 索引及同周 checklist 更新并回读至 revision 126；旧附件保留追溯。

同日增加麦克风软件 A/B：独立 `hi-yaka-lab-audio-ab` 实验变体在 `aliyun_raynode` 的 ESP-IDF 6.1 容器完整构建，Linux 137 项 host tests 通过；实验依次检查 GPIO1 内部 I²S 回环、ES7210 单独 RX，最后恢复原双工基线。当前 MASTER OFF，未烧录、未取得新采样，不改变麦克风 FAIL。实验方案 Markdown 已追加到硬件主文档文末，3.6 索引和同周 checklist 更新并回读至 revision 130。

同日完成麦克风 A/B 实板回读：内部回环证明 MCU I²S 接收/DMA 可读已知数据；ES7210 单 RX 与独立原双工在 GPIO13 只见固定全 0/全 -1，弱上下拉决定码值，25 秒后仍无有效音频。0x0B 状态在不同实验中读到 0 与 2，不能判定 U12 本体损坏。结果附件追加至硬件主文档末尾并在 3.6 索引；同周日志、checklist 及固件 PRD 均局部同步并回读至主文档 revision 136、固件 PRD revision 195。实验后已恢复原无屏幕实验镜像，麦克风、LCD、NFC 仍未通过。

同日完成 `HIYAKA-AUDIO-INC-001` 麦克风专项问题文档：按现象、实板排查、候选根因、条件化解决路径和关闭条件整理，明确目前没有已证实的硬件根因或已验证修复。Markdown 作为新附件加入硬件主文档 3.6 索引，附件 token 与索引均回读至 revision 139。源码提交 `caffda8` 对应 GitHub Actions 板级契约、产品核心及板型矩阵三个工作流全部成功。

同日将 LCD、NFC、麦克风拆为三份六节专项问题：LCD 背光控制缺陷已定位但未返修，NFC NACK 和麦克风无数据的根因尚未证实。`ISSUE_ANALYSIS_TEMPLATE.md` 首次作为附件加入“01 测试与发布”并在“记录方式”建立入口；该页首页从旧的“全部未实板验证”更新为当前受控样机状态，回读 revision 51。三项分别创建台账 YAKA-0004/0005/0006，上传各自 Markdown 到记录附件并逐条回读至 Base rev 43；硬件主文档将旧麦克风长版标记为历史并链接当前台账，回读 revision 141。原始反馈字段不覆盖，之后在处理意见、附件与状态维护增量。

双仓库发布策略及公开仓库日志对应本地提交：`b18d267`；公开仓库首次提交：`ChatChatTech/yaka@4ea1c48`（2026-10-29）；当前公开固件快照：`ChatChatTech/yaka@0489e83`（2026-11-02）。

## 2. 附件登记

| 附件 | 目标文档 | SHA-256 | 飞书文件 Token | 状态 |
|---|---|---|---|---|
| `docs/bringup/2026-11-05-audio-ab-plan.md`（麦克风软件 A/B 实验） | 02 硬件主文档 | `856ab25e1028ce515374ed03fb4f140debc77c7d24c502ed4fa02062e7debb21` | `CTokbQLRcouK9GxYkPlcLn1fnAb` | 2026-11-05 已上传并在 3.6 索引，附件块 `doxcnREJrpycsIPYMnO6T3HYod3` |
| `docs/bringup/2026-11-05-audio-ab-results.md`（麦克风 A/B 实板结果） | 02 硬件主文档 | `afeabbacef9926b56f6e39901910c5eb0edcc1aed538cdee3a5acac0` | `GyHQbwIqmoARfixoR5HcaOfDn37` | 2026-11-05 已上传并在 3.6 索引，附件块 `doxcnlSk75lySnYkCag0D1h16gf`；原始日志仅本地保留 |
| `docs/bringup/HIYAKA_MICROPHONE_DATA_PATH_INCIDENT.md`（HIYAKA-AUDIO-INC-001） | 02 硬件主文档 | `80f1443e8143bd4be7095f6433df7e5faf646f4003e7be570dfb89a237aa3f49` | `T5pgb21Ppoeaf2xL5ncc0gh6nve` | 2026-11-05 已上传并在 3.6 索引，附件块 `doxcnPDOLwg6s1i8lcpC1IIFIvf`；未覆盖 A/B 原始结果附件 |
| `docs/governance/ISSUE_ANALYSIS_TEMPLATE.md`（现行六节模板） | 01 测试与发布 | `4214e7c971541c622540f3c05d61121f6a3917f21cb51bc67bfc01a2e61df954` | `GkTjbjjTboa1xgx1VQCcW7xPnaf` | 文末附件块 `doxcnWKWhFTNbX78j19igkgyguf`，记录方式已建立入口 |
| `docs/bringup/HIYAKA_LCD_BACKLIGHT_INCIDENT.md`（HIYAKA-LCD-INC-001） | Base YAKA-0004 附件 | `a0da0cbf1d724960b32b5e596cd1125fa12b81603ef32ea1681401eaaf948b09` | `K9OEblqhloZjeOxXLRhceqkKnCc` | `reczz28VrGurKogr`，当前版 |
| `docs/bringup/HIYAKA_NFC_NO_ACK_INCIDENT.md`（HIYAKA-NFC-INC-001） | Base YAKA-0005 附件 | `975ec93c1a07f029e612b9eac808f09a2b8c507c001fde2edd885c8bfd7f75bc` | `JcVybQrKIofaeaxQXDtcgA0Hnee` | `reczz28VrGurLWAH`，当前版 |
| `docs/bringup/HIYAKA_MICROPHONE_DATA_PATH_INCIDENT.md`（六节模板修订版） | Base YAKA-0006 附件 | `93d9d6f55671ff18527d6590ae4afe0a5435a38dd2dacf91fae77567d1666029` | `BBJIb9DZ9oWe1vxkIFAcBbhZndh` | `reczz28VrGurNE5G`，当前版；02 主文档旧长版保留历史 |
| `docs/bringup/2026-11-04-lcd-nfc-mic-investigation.md`（旧板 S3 工程/固件交叉对照现行版） | 02 硬件主文档 | `a6aa0b599e402534a94ae9664b50cf3302e903cbaf1d3446664a3e7c4ccc5ee4` | `MRgOb1H04ombgyxNk5DcscLcnmd` | 2026-11-05 已上传并在 3.1 索引，附件块 `doxcnaM1YVkVe57RtsK2UlVKiuh` |
| `docs/bringup/2026-11-04-lcd-nfc-mic-investigation.md`（R63/R64/R65 实测历史版） | 02 硬件主文档 | `63d899e3fd8540a65a33a156891171177696b6406c86b77bba928f8b2fe408e7` | `BFtYbHkufoa34gxmOFZcRGvTnyg` | 2026-11-05 已上传，旧附件保留追溯 |
| `docs/bringup/2026-11-04-lcd-nfc-mic-investigation.md`（引脚活动、芯片状态、无屏幕对照，R65 实测前） | 02 硬件主文档 | `e012635d49485d7197aa7468e006ddb6d273d1b336ea2dd96684a30389bd0829` | `UmXBbuci6oIkcHxKQeocKEwinKf` | 当日中间版，保留追溯 |
| `docs/bringup/2026-11-04-lcd-nfc-mic-investigation.md`（含双声道复测，历史版） | 02 硬件主文档 | `56b1f312dcf53d48735054aa616f5058c7b2ed1d3477ff5f360f279a5c6d1d89` | `U55Vb5WpPox8e3xdz16cOb59nEe` | 2026-11-04 上传并在 3.1 曾索引，保留追溯 |
| `docs/bringup/2026-11-04-lcd-nfc-mic-investigation.md`（早期首板诊断） | 02 硬件主文档 | `3ac0df0247699d7354cb2b24126b3379ef2adc367b1ff707f0f42ca571b4cd0c` | `Nxstb4VVkoQie3xb8pSccutPn08` | 历史附件，保留追溯 |
| `docs/yaka/firmware-architecture.md`（历史 v1.0） | 01 固件 PRD 与里程碑 | `7d8f7ae0b8880c075e05da9f75afced80deccec991d9a8abca8d1f9a9cc72598` | `QSQRbd2TWo07WPxn4pncxqE0n3f` | 历史附件，保留追溯 |
| `docs/yaka/firmware-architecture.md`（历史 v1.1） | 01 固件 PRD 与里程碑 | `c58d8b1a469a3eb2762a11f0005f3a7f2dada1a8395ef34031d99ba758222ead` | `KiMMbiCSaoMr7HxyYYbcEFx7nv0` | 历史附件，保留追溯 |
| `docs/yaka/diagrams/yaka-firmware-architecture.drawio`（历史 v1.1 编辑源） | 01 固件 PRD 与里程碑 | `5b8e078c62cd60f4d876b36394b5a2e47bd013be86612f7fa2d4ad780f70c2f5` | `JmDSb87W0osq7fxP4d2cTnDqnjg` | 历史附件，保留追溯 |
| `docs/yaka/diagrams/yaka-firmware-architecture.drawio.pdf`（历史 v1.1 四页） | 01 固件 PRD 与里程碑 | `6e8a382820079c0525d4175fb4c7d941a53661eccbb317fdd2363272bf0594ec` | `M35PbyWWloF9Jbxja5Vcv1ZlnJe` | 历史附件，保留追溯 |
| `docs/yaka/diagrams/yaka-firmware-architecture.drawio.png`（历史 v1.1 第 1 页） | 01 固件 PRD 与里程碑 | `ede5e24d0156b6406090c76fe2abc5d839f6f58f0bd2d32d64b3304d69d0891f` | `MCkbbCJevoa8pQx4nLEcp9TxnCe` | 历史正文资源，保留追溯 |
| `docs/yaka/diagrams/yaka-firmware-architecture-page-2.drawio.png`（历史 v1.1） | 01 固件 PRD 与里程碑 | `a373edc30857a1847dfd2b4f0b414a94be7bf3c41a620e6dec853fec689e9052` | `VYmLbDms8ohJNMxGC3ZcjBdZnpg` | 历史正文资源，保留追溯 |
| `docs/yaka/diagrams/yaka-firmware-architecture-page-3.drawio.png`（历史 v1.1） | 01 固件 PRD 与里程碑 | `85ae3fae57387cfbc2e978d1a40d1358728d2442c8073e3bf9c1f3d602884836` | `VQJbb5zFJoorsHxllyqc05ofnuh` | 历史正文资源，保留追溯 |
| `docs/yaka/diagrams/yaka-firmware-architecture-page-4.drawio.png`（历史 v1.1） | 01 固件 PRD 与里程碑 | `d0fab37b8f152be60e3444e7b0088fb3d9e6aa05ec125a6755d6a8dfea899091` | `HDKQbQJizoNRFlxGT9BcZUK5nbd` | 历史正文资源，保留追溯 |
| `docs/yaka/firmware-architecture.md`（v1.2） | 01 固件 PRD 与里程碑 | `7ab743bede2d38eafecec2317f7d4fbcd0d35954f605f9bc68b08a6d68650c40` | `W03vbO4wdofVDux6hwzc3WxfnOf` | 2026-11-02 已上传并回读 |
| `docs/yaka/diagrams/yaka-firmware-architecture.drawio`（v1.2 编辑源） | 01 固件 PRD 与里程碑 | `0aafcea734e464bcd81e701f5a0204fe8c91a2f9e7a6e1d93150751add6ffecb` | `I2Okb2LUNoXJ7dxVkNVcGdfxn9c` | 2026-11-02 已上传并回读 |
| `docs/yaka/diagrams/yaka-firmware-architecture-v1.2.pdf`（四页） | 01 固件 PRD 与里程碑 | `1be7d91ea5cf1a5733be4f93de208899af8dbdec535a57ad38eeba8b155681a7` | `CcxYb4v8Yot3zcxqcaPcERTGnSc` | 2026-11-02 已上传并回读 |
| `docs/yaka/diagrams/yaka-firmware-architecture.drawio.png`（v1.2 第 1 页） | 01 固件 PRD 与里程碑 | `acc98a0da6303226eb71aa63666f69cd09bf28b7fd9a7fb8f8fb182456b60627` | `D5OdbpnqLohKhFxGkCrcYFCYn1b` | 已插入正文并回读 |
| `docs/yaka/diagrams/yaka-firmware-architecture-page-2.drawio.png`（v1.2） | 01 固件 PRD 与里程碑 | `5d6398cbb3786ba67a897952e05d09640115dbaec763b34a2629827da2b0ef8c` | `O2dxbiu5mo96YyxeM0acQDngnpt` | 已插入正文并回读 |
| `docs/yaka/diagrams/yaka-firmware-architecture-page-3.drawio.png`（v1.2） | 01 固件 PRD 与里程碑 | `948165d3cb38886646b0e98b3ab11035db72c4d75875b962201a4a204270f436` | `DpSEbl3nHop5p8xR0EUczCWsn0d` | 已插入正文并回读 |
| `docs/yaka/diagrams/yaka-firmware-architecture-page-4.drawio.png`（v1.2） | 01 固件 PRD 与里程碑 | `e977a50a7c874227cbc76ad3863a8bd505f2b828fe585283043647e975dc5356` | `XM5Ybo5oboluavx5qw0cjduSn6b` | 已插入正文并回读 |
| `docs/yaka/firmware-architecture.md`（v1.3） | 01 固件 PRD 与里程碑 | `4fcc7715aca818cd73835b91342b16bde622f7b68f8c60e3bf70ddec08b768a7` | `Zvo8bXDI8oakj3xrJ6mcrAvOnub` | 历史附件；已从正文展示移除，token 保留追溯 |
| `docs/yaka/diagrams/yaka-firmware-architecture.drawio`（v1.3 编辑源） | 01 固件 PRD 与里程碑 | `eb5d131fc59e79e3cfb8c5f7491b8fc5ab70e44f2848fb20656733dd0c34b587` | `GupTbN80TopBhfx0ERKcaSCqnfc` | 历史附件；已从正文展示移除，token 保留追溯 |
| `docs/yaka/diagrams/yaka-firmware-architecture-v1.3.pdf`（四页） | 01 固件 PRD 与里程碑 | `75d67567c0eadec86424f24744ef0421415837bfd0730407ac12583e31e9095c` | `J8nebPQlooI5H9xjvr5cO4lUnKg` | 历史附件；已从正文展示移除，token 保留追溯 |
| `docs/yaka/diagrams/yaka-firmware-architecture.drawio.png`（v1.3 第 1 页） | 01 固件 PRD 与里程碑 | `d096e606406c8ecae39e42756f198d9d95256e912361dda84257b9669f60d914` | `Q3HEb6GCBowRE9xJI8xchoeTnuE` | 历史正文资源；已移除展示，token 保留追溯 |
| `docs/yaka/diagrams/yaka-firmware-architecture-page-2.drawio.png`（v1.3） | 01 固件 PRD 与里程碑 | `28d227aa6413f172b4348ea30cd1b064073929f745ec6bc83396d15426dcab63` | `KRxhbAtl4ogDjMxOu5ZcmncFnIc` | 历史正文资源；已移除展示，token 保留追溯 |
| `docs/yaka/diagrams/yaka-firmware-architecture-page-3.drawio.png`（v1.3） | 01 固件 PRD 与里程碑 | `28f3a2f68e5ab573a67c6fe2e9d4859443b28b5fe2afa8cf165051ba89f21741` | `PgcubFOkUoUwx3xPGbEcej9rnJf` | 历史正文资源；已移除展示，token 保留追溯 |
| `docs/yaka/diagrams/yaka-firmware-architecture-page-4.drawio.png`（v1.3） | 01 固件 PRD 与里程碑 | `9bd4a1c04abb223c83ab0f80060535b2174faf95cb5e2c7f3f3d966c125e7d58` | `Y7whbvECPoN6oux1NDxc7kfwncg` | 历史正文资源；已移除展示，token 保留追溯 |
| `docs/yaka/firmware-architecture.md`（v1.4，上传后命名修订前） | 01 固件 PRD 与里程碑 | `1ec727b3f86f59e47313b279b62808f764c6976663ee125aeb2ab7868efcaa8e` | `YmBYbZicGoCmsNxRcRZceEEknFc` | 历史附件；已从正文展示移除，token 保留追溯 |
| `docs/yaka/firmware-architecture.md`（v1.4，PH4 命名修订版） | 01 固件 PRD 与里程碑 | `85d71639e3b77652b40626c5258a704f91240c88b60cadab78b4d778023856b9` | `KDU9b5jZmorxQCxwpR4cRDajnMc` | 历史附件；已移除展示，token 保留追溯 |
| `docs/yaka/firmware-architecture.md`（v1.4，PH5 状态版） | 01 固件 PRD 与里程碑 | `de99195f25ac17ffec937366a0c7fbcad26f95dd467f1eb30f5b8fbb99b02207` | `M7CLbITYNoCm4axmJsic5cg6n7f` | 历史附件，保留追溯 |
| `docs/yaka/diagrams/yaka-firmware-architecture.drawio`（v1.4 编辑源） | 01 固件 PRD 与里程碑 | `b82f3dd643f84aabf483fbd36f65bee8586c3a9077ac68ff40be485c8ce62e27` | `OAtjbtKkPokUYCxJLRyc05ccndc` | 历史附件，保留追溯 |
| `docs/yaka/diagrams/yaka-firmware-architecture-v1.4.pdf`（四页） | 01 固件 PRD 与里程碑 | `228587df581ca8aa1356e58d47453b04cbc9fffd93538556f71094c383746b30` | `OBVmbC2SGof2cox1M53cLlbUnnc` | 历史附件，保留追溯 |
| `docs/yaka/diagrams/yaka-firmware-architecture.drawio.png`（v1.4 第 1 页） | 01 固件 PRD 与里程碑 | `1778f9a49a5a7dea7ac6a532ba083b19dffb01a299bb64bb70fd83d165076ce8` | `ZyBebfPfLoaGIbxrBNCcj8VlnIf` | 历史正文资源，保留追溯 |
| `docs/yaka/diagrams/yaka-firmware-architecture-page-2.drawio.png`（v1.4） | 01 固件 PRD 与里程碑 | `cc1fffa9af437a923eb61dccad59fe25100f3a3ea2a3a898977a204590d2d06d` | `G9bPbTZtJon5ZmxOomEcLo4Xn7e` | 历史正文资源，保留追溯 |
| `docs/yaka/diagrams/yaka-firmware-architecture-page-3.drawio.png`（v1.4） | 01 固件 PRD 与里程碑 | `b08f4e09489236db3db6b0a4eca1067f09ff47bad1cc206a08402204120ae583` | `L0iTbjLThoDHnPxyUTmcSN6ynvg` | 历史正文资源，保留追溯 |
| `docs/yaka/diagrams/yaka-firmware-architecture-page-4.drawio.png`（v1.4） | 01 固件 PRD 与里程碑 | `542196211e63e3603472d54b114d53f9f8c852fc7fade183676f7297e9ce328d` | `MARZbPQeaoYWupxGCAKcxs6OnQf` | 历史正文资源，保留追溯 |
| `docs/yaka/firmware-architecture.md`（v1.5） | 01 固件 PRD 与里程碑 | `c552cfdd51cb7d5b59b0f527bf4118a6d174cef30c686ad7a9e93111f675d713` | `FjosbZu52ozQZ6xX8q0cKehEnrb` | 2026-11-02 已上传并回读，当前版本 |
| `docs/yaka/diagrams/yaka-firmware-architecture.drawio`（v1.5 编辑源） | 01 固件 PRD 与里程碑 | `b3ec340282585f1e4b1954f3e52a4988dcd2cbecfb4072f0994917430af117ac` | `EHzEbMtdHohJjExwlBWczvlannh` | 2026-11-02 已上传并回读，当前编辑源 |
| `docs/yaka/diagrams/yaka-firmware-architecture-v1.5.pdf`（四页） | 01 固件 PRD 与里程碑 | `fd7cacb751e55a1df63ce609ed530630a4da68cf54b535527e81a757649935e0` | `KbGjbiMxKow65ExKH0IcZ6bHnof` | 2026-11-02 已上传并回读，当前评审版 |
| `docs/yaka/diagrams/yaka-firmware-architecture.drawio.png`（v1.5 第 1 页） | 01 固件 PRD 与里程碑 | `64350aaffbc144a26cca156d6584f065d063f69915fcc83913777b5f0319a39b` | `LVLDbfy8eontrmxaLDVcgdBunsj` | 当前正文图片，caption 已回读 |
| `docs/yaka/diagrams/yaka-firmware-architecture-page-2.drawio.png`（v1.5） | 01 固件 PRD 与里程碑 | `76048a919cf033844c7561ad6c233eab568232834ce9669389c73446ecad1810` | `Ys2dbYe7XoACaKx3WEMcB6fNnhf` | 当前正文图片，caption 已回读 |
| `docs/yaka/diagrams/yaka-firmware-architecture-page-3.drawio.png`（v1.5） | 01 固件 PRD 与里程碑 | `c2a9bad2720212d538873272fe5d5cc04b36f3b26de2825b89014ea648e76d49` | `GqLhbz5Rsos2uVx5jmec2dJmnEb` | 当前正文图片，caption 已回读 |
| `docs/yaka/diagrams/yaka-firmware-architecture-page-4.drawio.png`（v1.5） | 01 固件 PRD 与里程碑 | `664bfb8ae62b5b0698f140501de80d194aad7897d6267bd2c0ada59ed93fdc04` | `JxX4bAZJWoJgMSxl1seckSiCnE4` | 当前正文图片，caption 已回读 |
| `docs/yaka/firmware-architecture.md`（v1.6） | 01 固件 PRD 与里程碑 | `0c891ca3ec1a7d2cbf1727be2c5f40172ecfbec1dc22aa95b39664a20add386c` | `CdW8bHMwCopmbjxFLcichavcnGf` | 2026-11-04 已追加并回读，当前版本 |
| `docs/yaka/diagrams/yaka-firmware-architecture.drawio`（v1.6 编辑源） | 01 固件 PRD 与里程碑 | `db673d702e3f9b7241eb12838bb7bccba23db9fa14ff579d53327b7f97a4047b` | `JOBUbJXcMoCPgsxVfr3cHZqynAf` | 2026-11-04 已追加并回读，当前编辑源 |
| `docs/yaka/diagrams/yaka-firmware-architecture-v1.6.pdf`（四页） | 01 固件 PRD 与里程碑 | `85ae335cc56ec9d9902d0f9bd74bb289a1dd6a49faf92b0bbcd34b01b60472d9` | `K2tubv3AFoCkgvxShnec5bgonDi` | 2026-11-04 已追加并回读，当前评审版 |
| `docs/yaka/diagrams/yaka-firmware-architecture.drawio.png`（v1.6 第 1 页） | 01 固件 PRD 与里程碑 | `ca12508b73fa5b8bffe7430e5b663f34cc1a100a6578ef6ca472611035192ca4` | `T1eAbkdbpoqOVXxlA36cOd4Knsb` | 当前正文图片，caption 已回读 |
| `docs/yaka/diagrams/yaka-firmware-architecture-page-2.drawio.png`（v1.6） | 01 固件 PRD 与里程碑 | `76048a919cf033844c7561ad6c233eab568232834ce9669389c73446ecad1810` | `ZUaeby5I4okizYxSwLJcXCVvnwd` | 当前正文图片，caption 已回读 |
| `docs/yaka/diagrams/yaka-firmware-architecture-page-3.drawio.png`（v1.6） | 01 固件 PRD 与里程碑 | `90affa36d5e4a4566e7b049b636c5e736c9886a2bc7c799078ad12b84a22fff0` | `MTz6baqq8orjhzxYnJDcxOrjnfe` | 当前正文图片，caption 已回读 |
| `docs/yaka/diagrams/yaka-firmware-architecture-page-4.drawio.png`（v1.6） | 01 固件 PRD 与里程碑 | `ac6b281aef287f318b3cff3d418f323122b8bf6ff451ac22ed3224dafc7ccb2d` | `MdnVbdL5ZopnbPxySOPcBGiVnrh` | 当前正文图片，caption 已回读 |
| `docs/bringup/README.md` | 01 测试、问题与发布记录 | `4545be34fcc32da155f05b1f85cb388b9da822d91ac75c2e9225e2a6b0133a66` | `LpOibBygzo7pQixiIZkcPjTTn3d` | PH5 首板验证入口，已上传并回读 |
| `docs/bringup/FIRST_BOARD_VALIDATION.md` | 01 测试、问题与发布记录 | `88f61a7c019d03d78bf7868b74e665d09228913ef2d9a7d63954d794a691ef22` | `Dxz0b1rfkoBlKyxdvuAcGaLYnke` | PH5 上电与基础验证手册，已上传并回读 |
| `docs/bringup/MODULE_TEST_MATRIX.md` | 01 测试、问题与发布记录 | `052ab98e1c26555b2eb3701070e78ce9d1db89efc12fb16519b3a291112c3cdd` | `LqO7bSkR5oh2ITxEsw9cYjCEnsh` | PH5 模块测试矩阵，已上传并回读 |
| `docs/bringup/EVIDENCE_LEVEL_MATRIX.md` | 01 测试、问题与发布记录 | `e767921715b34e05e4f3be30df457e5d18aacb7e58d44c0475fcf84ce01bcfe4` | `YNlUbgAX3o1Yj9xjoRmcpcwknff` | PH5 M1–M7/L0–L3 映射，已上传并回读 |
| `docs/hardware/attachments/yaka-3.0-final-schematic-pcb-reaudit-2026-10-21.md`（历史命名前） | 02 硬件主文档 | `d2da46dcd980060614016da17d24c9b153577c8c454f2737622a448693bbd1cb` | `UVG7bcGxEoQl2Lx5rvfcgr8unwh` | 历史附件，保留追溯 |
| `docs/hardware/attachments/yaka-3.0-schematic-audit-and-eco-spec.md`（历史命名前） | 02 硬件主文档 | `b6fe1b2f634010054158d67e5f3538f11909c551cf893fda26aad7ff219abcc9` | `KGdgbjotXowmxrxelnoc4Rf7nfb` | 历史附件，保留追溯 |
| `docs/hardware/attachments/yaka-3.0-pcb-comprehensive-audit-and-eco-spec.md`（历史命名前） | 02 硬件主文档 | `fe0129892773decae14c210b9e4db0402e42ffb7e46f25444fabb7fbd2378455` | `H5TPb0JwLo9OwxxXPMMcyBnQnLd` | 历史附件，保留追溯 |
| `docs/hardware/attachments/yaka-3.0-prefreeze-release-manufacturing-and-sourcing-checklist.md`（历史命名前） | 02 硬件主文档 | `21827082c722903b4b8ac7c6289d42e3f838cc548a14e96d565cc6aa1ca03c90` | `DnyPbFCe9o5hQ7xX1CzcVrBjn5e` | 历史附件，保留追溯 |
| `docs/hardware/attachments/yaka-3.0-final-schematic-pcb-reaudit-2026-10-21.md`（HiYaKa 命名对齐版） | 02 硬件主文档 | `989833d9d258c159bcfffc29cf5a0adf6dff4f011991d0aca2db161ca41affe6` | `D9S2bw745ojtpdxizoucqz6Lnig` | 2026-11-02 已上传并回读，当前版本 |
| `docs/hardware/attachments/yaka-3.0-schematic-audit-and-eco-spec.md`（HiYaKa 命名对齐版） | 02 硬件主文档 | `33271da0cde503cc164089cb64a49ccb7bb8a4cbbd742f2f0265efd9b590fddb` | `Fjhfb2BWYo2ghOxejaecf4B7nbg` | 2026-11-02 已上传并回读，当前版本 |
| `docs/hardware/attachments/yaka-3.0-pcb-comprehensive-audit-and-eco-spec.md`（HiYaKa 命名对齐版） | 02 硬件主文档 | `61a3aa814fd473b3254afcbe2daeaf7c11f8a2cfe730692e814472b84395cdfc` | `QT8Vbd585oCSSJxsBK1cvUjkn7c` | 2026-11-02 已上传并回读，当前版本 |
| `docs/hardware/attachments/yaka-3.0-prefreeze-release-manufacturing-and-sourcing-checklist.md`（HiYaKa 命名对齐版） | 02 硬件主文档 | `a64c8ee05cac9f4f414f42f41c8ed362fe688e904babe2f9d06ac64aea95e318` | `K7tXbxTHBoiuSFx6Gijck9k5njf` | 2026-11-02 已上传并回读，当前版本 |
| `docs/hardware/attachments/yaka-3.0-procurement-and-consignment-list.csv` | 02 硬件主文档 | `10a8a83fbe50177ba737962a3b2a028e0196384ff88f6b26f9ace4e475a4fc46` | `XeMxbTcEZoqMUuxkN8jcMxHnngz` | 已上传 |
| `docs/hardware/HARDWARE_MODULE_INTERFACES.md`（2026-11-05 现行附件） | 02 硬件主文档 | `be4d3b9185f75a1948abbc95b2d0bd324215daea890c3b83c0b7f4db80fd97ab` | `TPFub69t6owUqdxNRUmch6WEnEa` | 已上传并在 3.2 索引 03 原生文档和本附件 |
| `docs/hardware/HARDWARE_MODULE_INTERFACES.md`（当日中间版） | 02 硬件主文档 | `435f6fe763633265419066d43c7c87c77dd4d7d66680cfd80d38f5f455a74713` | `QYFDbZqqLoCanwxOggIcr1ornld` | 中间版，保留追溯 |
| `docs/hardware/HARDWARE_MODULE_INTERFACES.md`（当日 ACT_B/IMU 增量） | 02 硬件主文档 | `a6be2a8cb217fa2fa96a11508c9c12a15d48aeee9c1f679cfc4ea9c3a7b0ea54` | `Ra8qb5Z01o7oDGx9FTuc8A23n17` | 中间版，保留追溯 |
| `docs/hardware/HARDWARE_MODULE_INTERFACES.md`（历史版） | 02 硬件主文档 | `7054463d187430309db1d0a751759d2e0193af359f53a625600fa16aa07913ea` | `DRy0bQ6dIobLjOx5WO6cVcpsnoc` | 历史附件，保留追溯 |
| `docs/governance/DEVELOPMENT_COLLABORATION_AND_DOCUMENTATION_STANDARD.md` | 01 开发协作与文档维护规范 | `2b6970f4a8aa3102a443f5c0fc9b5e0bff8c04b197084d3cfd2f646961ac2d70` | `IaDebP4yBoqK0ux6Mtkc4oqhnUc` | 已上传 |
| `docs/research/VELXIO_SIMULATION_EVALUATION.md` | 04 外部参考与技术调研 | `79614967175b25c04e50eb8ac4707cd07a8cddd52c69d9b52ab81c23b672a986` | `ZsLYbpRNlomJiZx9XVAcJjCUnHe` | 已上传并补充原生摘要 |

## 3. 同步与冲突规则

1. 修改前读取本地和飞书的当前版本；禁止依据截图猜测现状。
2. 本地表格中的飞书 revision 是最近一次已回读版本，不是并发锁；写入前仍须重新读取 revision。
3. 飞书已有图片、评论和历史不做全文覆盖；对治理文档采用带日期的增量章节，对三份主文档采用受控重导入或章节级更新。
4. 附件不得静默替换。内容变更后以新文件名或新版本上传，并更新完整 SHA-256、文件 Token 和适用硬件版本。
5. 本地与飞书冲突时暂停自动同步，先判断哪一端持有更高等级证据；不得用“更新时间较新”替代事实判定。
6. 每次同步后至少回读标题目录、关键结论和 revision；只有 API 成功但未回读，不计为同步完成。

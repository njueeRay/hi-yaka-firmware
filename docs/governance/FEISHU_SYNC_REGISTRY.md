# HiYaKa 飞书文档同步登记

> 最后更新：2026-11-25
> 飞书目录：[HiYaKa](https://scnlb1lk96sb.feishu.cn/drive/folder/Qz1ifJDCvlYKz1dCBO2cFl9Bn3c)
> 原则：Git Markdown 保存可审计的工程事实源；飞书保存协作副本、跨文档索引和附件入口。

> **2026-11-25 最新同步：**YAKA-0004 本地专项新增“硬件／软件 I²C 隔离诊断已准备”L0 记录，本地 SHA-256 `B1B16414034B5529888B38164336E365573D966D2E2628757C9ECC957E8D564E`；飞书原生正文 [YAKA-0004](https://scnlb1lk96sb.feishu.cn/docx/SLgSdvt0PoIvcqxJ2uAcCjNQnDe) revision 145 已回读同段。隔离提交 `dcbe017` 的 IDF 6.1 镜像已构建、默认 HOLD、未烧录；COM5 当前枚举但板卡与 PN532 模式脚未复核。Base 维持此前 revision 100 的“处理中”，M2 未验收；本次未上传新附件，也未改动公开仓库。

> **2026-11-25 此前增补：**YAKA-0004 已加入同板 ES7210 软件 I²C 历史旁证，明确它降低“纯 IDF 硬件控制器问题”作为通用解释的优先级，但不证明与 PCA9557 同源；c0 的 COM5 当时未枚举，未做新烧录。飞书正文 revision 144 已回读两份原始数字采样的时序复算，Base YAKA-0004 最新进展与“处理中”状态回读至 revision 100；本地专项 SHA-256 见下方映射，M2 仍未验收。

> **GitHub 可见性勘误与撤回（2026-11-25）：**个人 `njueeRay/hi-yaka-firmware` 实际为 PUBLIC。内部 YAKA-0004 增补及服务器运维记录曾误经 [PR #18](https://github.com/njueeRay/hi-yaka-firmware/pull/18) 合入公开 feature 分支，已由 [PR #19](https://github.com/njueeRay/hi-yaka-firmware/pull/19) 撤回；当前 feature 分支文件树与 PR #18 前一致。公开提交及 PR 历史仍可访问，不能声称已彻底移除。飞书 revision 140 与本地专项继续维护，暂不再次推往 PUBLIC 仓库；仓库可见性待 Owner 确认。

> **2026-11-24 当前同步结果（优先于下方历史条目）：**`YAKA-0004` 屏幕与 `YAKA-0005` PN532 云端标题和正文编号已补齐，分别为 revision 62、22。`YAKA-0009` 的引脚/PCB、同板 A/B、双麦近场及日志哈希已完整并入 `YAKA-0006` 第 8 节；0006 是唯一现行麦克风问题正文（revision 115），0009 旧页（revision 24）已加归档说明并移入 `99 历史资料`，两个 Base 编号仍为“已完成”且保留旧链接。`YAKA-0008` 现为下一版小型化方案研究，移入 `03 协作、决策与调研`（revision 6）；`YAKA-0010` 为下一版电源与电量待办，移入 `02 硬件与固件研发`（revision 18），本地已补齐云端原有电量计章节，并把新增精确 SOC 标为待产品裁决。测试与发布索引 revision 91，Base revision 97，均已回读。正式产品录音、共享 I²C、LCD 稳定性、目标 NFC I²C、扬声器及整机发布仍未验收。面向外部阅读的 [硬件功能测试情况](../bringup/HIYAKA_HARDWARE_FUNCTION_AUDIT_2026-11-24.md)目前先提供本地 Markdown 草稿，待审阅后制作 PDF，不视为已同步飞书。

> **2026-11-23 历史实板与中间导入态记录：**YAKA-0009 的 c4 同板 GPIO0/1 交换后数字采集恢复，双麦近场有响应；两次后续无屏诊断 WAV 正常关闭，但 c4 较早两次 TF 超时的插卡状态未独立核实，不能定因。另一块 c0 样机本轮最初未插 TF，插卡后旧无屏 3/3、同修复版无屏 6/6 挂载；带屏原始 PARLIO 因 LCD/PCA 在 TF 前中止，容错 PARLIO 虽到 READY 仍只有背光，同板旧 GPIO 3/3、新全功能 GPIO 2/2 有画面，10 MHz PARLIO 2/2 与 1 MHz PARLIO 1/1 仅背光；25 kHz I²C 仍有 PCA/LCD 错误。实验后 c0 已恢复先前无屏镜像并校验、TF 挂载、进入 READY。YAKA-0006 仍需 I²C 与长稳闭环。该中间态 live 原理图/PCB 音频网名已对齐，但当时 native DRC 32/80/1 未通过；**当前完整导入后的 PCB 原生 DRC 已重跑为 0 项，这组旧错误不再代表现版**。飞书 YAKA-0009 revision 21、固件 PRD revision 225、YAKA-0006 revision 103、YAKA-0004 revision 51、硬件主文档 revision 146、接口页 revision 55，Base revision 91。下方早期状态说明保留同步史。

> **公开提交边界：**个人仓库 2026-11-23 核对为 PUBLIC；只将经公开扫描的 `FIRMWARE_PRD.md` 通过 [PR #17](https://github.com/njueeRay/hi-yaka-firmware/pull/17) 合入 `feat/m1-unified-bringup`（提交 `6b8e1b8`，合并 `a14da519`）。该 PR 仅一个文件；自动检查为空，原因是该目标分支和纯文档路径未命中现有工作流触发条件。内部 YAKA 专项、同步台账、原始日志及制造资料没有进入此次公开提交。

当前飞书目录按用途分为 `01 产品与使用`、`02 硬件与固件研发`、`03 协作、决策与调研`、`04 测试与发布`、`90 制造与发布归档` 和 `99 历史资料（只读）`。旧 `02 工程基线与接口契约` 已废止并移入历史区，当前接口事实以硬件主文档和硬件连接与固件接口表为准。

## 1. 主文档映射

| 本地事实源/治理对象 | 飞书文档 | Token | Revision | 当前状态 |
|---|---|---|---:|---|
| `docs/governance/PROJECT_CHARTER.md` 与项目索引 | [00 HiYaKa 项目总览与文档导航](https://scnlb1lk96sb.feishu.cn/docx/Kl74deqLtoteWyxeshlci43qn0g) | `Kl74deqLtoteWyxeshlci43qn0g` | 44 | 顶层唯一入口；当前阶段、U11/BOOST `NO-GO`、L7 隔离边界、M2–M6 L0 与受控恢复顺序已回读 |
| `projects/yaka/docs/yaka-3.0-prd.md` | [01 产品需求与交互状态](https://scnlb1lk96sb.feishu.cn/docx/T3isd0m8jo2bVEx37noc9Oq3noh) | `T3isd0m8jo2bVEx37noc9Oq3noh` | 53 | 完整产品 PRD v0.4；现行产品/系列语义统一为 HiYaKa；OTA 已对齐 MVP Must 与验收矩阵；位于 `01 产品与使用` |
| `docs/product/PRODUCT_DATA_AND_USER_MANUAL.md` | [02 产品规格与使用手册](https://scnlb1lk96sb.feishu.cn/docx/XohYdP65uo5zfTxkMEScyzXinag) | `XohYdP65uo5zfTxkMEScyzXinag` | 5 | 现行产品语义统一为 HiYaKa；位于 `01 产品与使用` |
| `docs/governance/FIRMWARE_PRD.md` | [01 固件 PRD 与里程碑](https://scnlb1lk96sb.feishu.cn/docx/DwmvdwnfboJKX3xpbyIc1b0bntc) | `DwmvdwnfboJKX3xpbyIc1b0bntc` | 253 | M2 改为业务摘要：c0 的 GPIO 诊断画面已目视可见；PCA 控制与刷新仍有错误，产品界面及稳定性未验收，技术证据留在缺陷 YAKA-0004。云端 M2 区块回读；本地 SHA-256 `68896C56D2178498EE6EC739D94289F5A7EF7BAE4640AEE74AFE708541B0EDDF`，架构附件保留 |
| `docs/hardware/HARDWARE_DESIGN_AND_DEVELOPMENT.md` | [02 硬件设计与研发主文档](https://scnlb1lk96sb.feishu.cn/docx/I2SIdpqZnouTxkxYEuLcnyJxndc) | `I2SIdpqZnouTxkxYEuLcnyJxndc` | 146 | 3.1 增加两份核查文档入口并纠正 LCD/NFC/双麦/DRC 当前摘要；数据手册附件与历史块保留，已回读 |
| `docs/governance/HARDWARE_FIRMWARE_RELEASE_GATES.md` | [跨层设计、测试与发布核查原则](https://scnlb1lk96sb.feishu.cn/docx/Yt7hdcxNlow4FVxCgdYcot7tnUg) | `Yt7hdcxNlow4FVxCgdYcot7tnUg` | 9 | 位于 `02 硬件与固件研发`；2026-11-23 已在同一token原地同步当前286/858/DRC0、NS4150B、新LCD物理连接、历史反例边界及NFC首发只读门禁并回读。飞书保留精简原则页，本地详版SHA-256 `BDD2F2CFCF7EF1ADF7C24C1E85FBE9D8BE9DB6B492BFEF4F24D6D6F8D9C1DDA3` |
| `docs/hardware/HIYAKA_HIGH_RISK_INTERFACE_STATIC_REVIEW.md` | [高风险接口静态可行性复核](https://scnlb1lk96sb.feishu.cn/docx/IvfLdQUjsoWlfyxED6oc6FzFneb) | `IvfLdQUjsoWlfyxED6oc6FzFneb` | 4 | 位于 `02 硬件与固件研发`；本地 SHA-256 `82CA84949D608C05303FF470CA707C0ACEB799E3408297A14DC54709B717DD28`；新增 LCD/PCA 与 ACT_B 误归因边界；原始 DRC 清单仅本地；已回读 |
| `docs/hardware/HARDWARE_MODULE_INTERFACES.md` | [03 硬件连接与固件接口表](https://scnlb1lk96sb.feishu.cn/docx/UdVhdzlP6oAxXZxzyMIc25SxnMk) | `UdVhdzlP6oAxXZxzyMIc25SxnMk` | 55 | 增量提示 GPIO0/1 物理映射交叉、纠正接口矩阵四格及音频 ASCII 示意，指向 YAKA-0009；未实测修正，已回读 |
| `docs/hardware/HIYAKA_U11_THERMAL_INCIDENT_AND_RECOVERY.md` | [YAKA-0001｜U11/BOOST 异常发热与恢复专项复盘](https://scnlb1lk96sb.feishu.cn/docx/XoLCdhpQqoQjjDxHEhyciFfxnJe) | `XoLCdhpQqoQjjDxHEhyciFfxnJe` | 40 | 2026-11-20 已锁定 `ES8311 + NS4150B` 下一版路线，明确“决策已锁定、原理图/PCB/实板未闭环”；补全 V2.1 差分输入、CTRL 下拉、Bypass/去耦、BTL、PA 前 AEC 参考及固件 PA 使能条件；已回读 |
| `docs/research/HIYAKA_MINIATURIZATION_SPECIAL_STUDY.md` | [YAKA-0008｜下一版小型化方案研究（待决策）](https://scnlb1lk96sb.feishu.cn/docx/BIOGdKeRco1t8IxvKhwcVv7Cnmf) | `BIOGdKeRco1t8IxvKhwcVv7Cnmf` | 6 | 位于 `03 协作、决策与调研`；候选方案未批准；Base `reczz28WMS5BRuu3` |
| `docs/hardware/HIYAKA_NEXT_REVISION_POWER_AND_BATTERY_TASKS.md` | [YAKA-0010｜下一版电源与电量设计待办](https://scnlb1lk96sb.feishu.cn/docx/U9HudXTSsoNRTyxITRucI0LrnSg) | `U9HudXTSsoNRTyxITRucI0LrnSg` | 18 | 位于 `02 硬件与固件研发`；开关额定风险须处理，新增电量计待产品裁决；Base `reczz28cyNCxqKpT` |
| `docs/bringup/HIYAKA_AUDIO_GPIO01_PIN_MAPPING_INCIDENT.md` | [YAKA-0009｜音频 GPIO0/1 引脚映射错配（已并入 YAKA-0006）](https://scnlb1lk96sb.feishu.cn/docx/EMWxdp3fzoGZUAxpvCHcKlMpnTe) | `EMWxdp3fzoGZUAxpvCHcKlMpnTe` | 24 | 仅为旧链接重定向；原正文移入 `99 历史资料`，完整现行证据见 YAKA-0006 第 8 节；Base `reczz28cyHMsoPVa` 保留编号与原始反馈 |
| `docs/governance/DEVELOPMENT_COLLABORATION_AND_DOCUMENTATION_STANDARD.md` | [01 开发协作与文档维护规范](https://scnlb1lk96sb.feishu.cn/docx/K33VdjETzooOrTxIQD9cOYXln5c) | `K33VdjETzooOrTxIQD9cOYXln5c` | 17 | 已回读 2026-11-20 当前硬件资料读取边界：本轮只读已保存资料及实板日志，暂不读实时 EDA；恢复实时核验需 Owner 新指示。附件版本 `7710271471838350641` 的 source_file SHA 与本地一致 |
| 仓库、上游与技术决策 | [02 决策与版本记录](https://scnlb1lk96sb.feishu.cn/docx/NWmcdvVpgov54exQOkIcvvOunmd) | `NWmcdvVpgov54exQOkIcvvOunmd` | 14 | 保存正式 ADR 与版本影响 |
| `projects/yaka/docs/yaka-3.0-owner-decisions-and-todos.md` | [03 当前角色、Owner 决策与待办](https://scnlb1lk96sb.feishu.cn/docx/Y5SqdV9zloa3eLxQZALcTDQ4nOe) | `Y5SqdV9zloa3eLxQZALcTDQ4nOe` | 17 | 已覆盖旧静态放行与“等待样机”状态；同步 L7 隔离、PN532/QMI8658A、WAV、M1 恢复边界及下一版 ECO 待办 |
| 外部资料治理与 `docs/research/` | [04 外部参考与技术调研](https://scnlb1lk96sb.feishu.cn/docx/Cn2bdVrSboCRKLxuDErcWWSUnpc) | `Cn2bdVrSboCRKLxuDErcWWSUnpc` | 17 | 已登记 FoloToy 竞品与 HiYaKa 小型化专项；位于 `03 协作、决策与调研` |
| `docs/governance/TEST_AND_RELEASE_INDEX_PROTOCOL.md` | [测试与发布｜主索引与发布协议](https://scnlb1lk96sb.feishu.cn/docx/RXKYdmVHaobeHNx39uNcl8BAnbf) | `RXKYdmVHaobeHNx39uNcl8BAnbf` | 91 | 现行入口仅指 YAKA-0006；0009 归档追溯；0008/0010 分工与目录已回读 |
| `artifacts/yaka/full_hardware_evidence_audit_20261120/HIYAKA_硬件功能全量证据审计.md` | [硬件功能现状全量汇总审计](https://scnlb1lk96sb.feishu.cn/docx/IqzadBuxIomUbLx0AfGcBj4Tnvb) | `IqzadBuxIomUbLx0AfGcBj4Tnvb` | 27 | 2026-11-20 已更新麦克风与功放两行：麦克风为 `FAIL，最终根因未定`，不再将 I²C 物理层写成已证实；功放为 `R1.6 FAIL，下一版 NS4150B 决策已锁定、未验证`；已回读 |
| 问题/发布记录与动态状态 | [HiYaKa 反馈与处理台账](https://scnlb1lk96sb.feishu.cn/base/PE5BbvVtOaHxI4soxwKc9w7Cn7b) | Base `PE5BbvVtOaHxI4soxwKc9w7Cn7b` / 表 `tblLefBEZhKEUhcC` | 100 | YAKA-0004 最新进展已更新至地址阶段 NACK 与当前 M2 未验收，状态仍“处理中”，原始反馈字段保留；其余记录以实时台账为准 |
| 历史仓库与旧资料归档规则 | [00 历史资料索引](https://scnlb1lk96sb.feishu.cn/docx/KlRxdMk4ho6A5Txs3dqc3KG2nLd) | `KlRxdMk4ho6A5Txs3dqc3KG2nLd` | 10 | 位于 `99 历史资料（只读）` |
| 已废止的旧接口摘要 | [90 [已废止] 工程基线与接口契约](https://scnlb1lk96sb.feishu.cn/docx/NBzHd2mvgoHQ06xLNXOc0A0tnoe) | `NBzHd2mvgoHQ06xLNXOc0A0tnoe` | 24 | 仅供历史追溯，不得作为当前接口输入 |

2026-11-01 完成目录分层、文档重命名、旧接口摘要废止及制造状态纠偏；当前映射以本登记文件所在 Git 提交为准。

2026-11-23 音频 GPIO0/1 接口缺陷独立登记：Base 自动分配 `YAKA-0009`（`reczz28cyHMsoPVa`），新专项 `EMWxdp3fzoGZUAxpvCHcKlMpnTe` revision 5；历史下一版硬件小项文档曾自行占用 `YAKA-0009` 但未入台账，补登后自动分配 `YAKA-0010`（`reczz28cyNCxqKpT`），保留原链接并改云端/本地标题。YAKA-0006 revision 103、接口页 revision 55、测试发布索引 revision 79 均增量回读。Base revision 91；麦克风仍 FAIL，GPIO 交换修正尚未实测，用户当前暂停固件测试。此同步为局部修订，不表示本地全文与飞书逐字一致。

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

2026-11-05 换入样机从空 Flash 完整烧录后，再次复现 LCD 无背光、PN532 NACK 和麦克风无有效数据；TF 插卡复位后挂载成功，ACT_B 红灯录音与 PWR_A 短振复验。单次复测事实写入 `2026-11-05-replacement-board-retest.md` 并附至“01 测试与发布”，该页 section 5 增加摘要并回读 revision 54；三条原缺陷记录各追加进展和日期版 Markdown 附件，Base rev 47 回读仍为“处理中”。

同日原开发样机完成独立 PN532 I²C/HSU A/B：`0x24` 三轮 NACK，切至 HSU 后三轮无返回；实验镜像已撤回并恢复原固件。`YAKA-0005` 增量记录和两份 Markdown 附件已回读至 Base rev 49，附件 token 分别为 `WMD4bYIcZoDSIPxY7qAc1FD9n0b`（日期版专项问题）与 `Lw61bWU3YoxVeMxBoxFcgewjnWd`（接口 A/B 记录）。状态维持“处理中”，NFC 未验收。

2026-11-06 按 Owner 确认精简 HiYaKa 反馈与处理台账：LCD、NFC、麦克风的初版、日期中间版和 A/B 汇总版共 7 个过时附件已从记录移除；重新上传三份不带日期的当前专项分析，NFC 另保留独立接口 A/B 实板记录。三条“处理意见/最新进展”压缩为当前结论、业务影响和唯一下一步，原始反馈未覆盖，状态均保持“处理中”。附件与字段逐条回读至 Base rev 56。

同日按 Owner 确认重构专项问题模板及 LCD/NFC/麦克风三份当前报告：主结构统一为“结论与业务影响、现象、因果链与证据分层、当前 PCBA/下一版方案、关闭标准、证据来源”。四份原生 Markdown 均在原 token 上生成新版本并回读；LCD 当前 PCBA 的 `R85.1 → 2.2 kΩ → C118.2` 人工补救被明确标为无需重新制板但需要飞线焊接的待验证措施，不作为下一版正式设计。LCD 台账摘要同步至 Base rev 57，状态仍为“处理中”。

同日修复实验室构建的 PWR_A 黄灯反馈：提交 `68e8765` 在录音灯优先模式下短暂显示黄灯约 250 ms，随后恢复红灯或熄灯。换入样机上，操作者确认录音中和录音停止后的短振、黄灯及灯态恢复；串口抓到对应 PWR_A 事件。三项 GitHub Actions 均成功。修订后的复测 Markdown 作为新附件追加，测试发布页摘要和附件已回读至 revision 57；LCD、NFC、麦克风缺陷仍为“处理中”，未因该交互修复而关闭。

同日按 Owner 批准治理 `04 测试与发布`：原页原地重塑为“测试与发布｜主索引与发布协议”并回读 revision 61；U11 专项保留 token 移入该目录并按 YAKA-0001 重命名；新建 YAKA-0002/0004/0005/0006 四篇原生文档，均从本地现行 Markdown 导入并回读 revision 2。Base 不改字段和状态，仅在五条记录补充当前摘要与专项链接，回读 rev 58。文件夹最终只有主索引、Base 和五篇现行记录共 7 个对象；未补造 YAKA-0003，未删除历史附件或制造归档。

同日补充两项下一版治理入口：YAKA-0007 登记 D13–D15 普通过孔落在小型 GND 焊盘内导致回流吸锡、焊点少锡的制造可靠性问题，原生文档加入产线截图，Base 记录附两张原始证据图；YAKA-0008 将既有小型化调研从外部参考附件提升为“测试与发布”原生子文档和优化项。Base 回读 rev 60，主索引回读 revision 63；YAKA-0003 继续留空、不复用。

2026-11-07 补齐外购器件原始资料归档：硬件主文档 3.1 新增“外购器件数据手册附件”，从制造归档中的既有文件 Token 复制挂载 `lcd-h0160s002-v1-spec.pdf`、`lcd-h0160s002-mechanical-drawing.jpg` 与 `battery-303260-650mah-spec.doc`，并逐项登记 SHA-256；回读到 revision 145。新生成的文档附件 Token 分别为 `Ydenb7ijHoTDFDxp6dscNRFdnjd`、`BeN6bKEmMoIhmsxkgQycrMn1nad`、`Dcr8bnlbCorisyxPToCc1XgGnfg`。扬声器仍只有历史商品截图，未伪装为正式数据手册。

2026-11-12 更新 YAKA-0001 功放专项：新增 0.25–1 W 的相对声压差与听感量化，明确 `0.81 W → 1 W` 仅约 `+0.92 dB`；下一版默认切换为 `ES8311 + NS4150B` 电池直供，HT8691R 仅作为未来高响度和低电量音量一致性路线。同步补充两条路线各自的执行边界与关闭标准，并回读飞书 revision 37；未覆盖历史失效证据。

双仓库发布策略及公开仓库日志对应本地提交：`b18d267`；公开仓库首次提交：`ChatChatTech/yaka@4ea1c48`（2026-10-29）；当前公开固件快照：`ChatChatTech/yaka@0489e83`（2026-11-02）。

## 2. 附件登记

| 附件 | 目标文档 | SHA-256 | 飞书文件 Token | 状态 |
|---|---|---|---|---|
| `docs/governance/TEST_AND_RELEASE_INDEX_PROTOCOL.md` | 测试与发布｜主索引与发布协议 | `b13974a7f49dd76e0ea69976643a2a58c8bd27cfb296b0271a37daf5d134d9fb` | `RXKYdmVHaobeHNx39uNcl8BAnbf` | 新增 YAKA-0007/0008 入口并回读 revision 63 |
| `docs/bringup/HIYAKA_R1_6_FIRST_BOARD_VALIDATION_AND_RELEASE.md` | YAKA-0002 原生文档 | `f21d9920eeb512b7de39fa2542b3aed7f289aef6b6d44dfdf52ab90817c3abca` | `XT2wdW6HboKbAux61PUcvhGJnqh` | 2026-11-06 导入并回读 revision 2 |
| `docs/bringup/HIYAKA_LCD_BACKLIGHT_INCIDENT.md` | YAKA-0004 原生文档 | `923B99880CBC2DE53CDD6289CA474EE14095B23488330DA035194B1E18DF8A30` | `SLgSdvt0PoIvcqxJ2uAcCjNQnDe` | 2026-11-23 同板旧 GPIO 3/3、新 GPIO 2/2 可见，同源码 10 MHz PARLIO 2/2 与 1 MHz 1/1 仅背光；API 成功不等于画面。PCA/LCD 错误仍在；c0 插卡后无屏 9/9 挂载，c4 TF 插卡条件未知。PCNT 诊断主控侧 27/46 时钟上升沿（预期 8），0x2B 因 PCA 失败计数无效，FPC 端未测；官方 C6/IDF6.1 公共 PARLIO 门控不支持；转测 GPIO 提速与资源/稳定性，LL/改板为后备，M2 未验收。GPIO 提速日志约 2.0 秒/色、旧版约 4.25 秒/色；boot1/boot3 用户目视五色和横屏文字正常，boot2仅日志；三轮设备计时逐项相同不算方差；boot4 主机旁路计时与约2秒/色吻合，PCA/LCD 错误仍在。新增 c0 GPIO fast 基线0–20s/20–40s及晚期60s PCA事务首失/耗尽/刷新三指标，内部上拉ON/OFF同窗对照未消除 INVALID_RESPONSE 等事务失败，外部R11/R12状态未由此判定；ON版单次目视完整、温度正常，11/24 安全定时诊断显示写后立即读顺序混杂、反序A/B已完成：六组长空档均首笔失败、第二笔成功，根因未定；上一轮已恢复GPIO fast基线并目视完整、温度正常，本轮反序测试后再次烧回并校验、目视完整且温度正常，但PCA重试499/耗尽24/刷新20，bus_reset安全写ABBA四组各64次全失败、前置复位未改善；PCA/QMI同总线首访对照将失败收缩至PCA访问路径，非整总线普遍首笔失败；PCA/QMI诊断后GPIO fast已烧回校验、串口五色ESP_OK/READY，用户目视完整、温度正常；同窗PCA首失516/耗尽19/绘制失败17，PCA首访九相矩阵异常、QMI首访正常、0x18不能判A0；首访矩阵后GPIO fast已恢复校验并目视完整、温度正常，但PCA首失420/耗尽21/绘制17，全功能QMI预访ABBA A首写32/32失败、B0/32但后段绘制仍失败；实验后原GPIO fast已恢复校验，串口五色/READY但本轮无目视/温度；PCA首失421/耗尽21/绘制19，LVGL非首错必要条件，被动轨迹114条首失30/耗尽16，命令类C3→03三试失败，QMI预访不能外推；诊断后GPIO fast基线已恢复校验，五色ESP_OK/READY，PCA首失402/耗尽18/绘制16；延后重试警告两次轨迹耗尽0/0、全局0/0与1/1，首访异常未消；实验后原GPIO快显已恢复校验、五色/READY，PCA耗尽20/绘制18；静默计数候选两次约120秒各耗尽1/绘制1，均早于WiFi初始化；原GPIO基线已恢复校验、五色/READY，但同窗耗尽20/绘制18，本轮无目视/温度；空闲首访分级对照：纯 250 微秒等待不能解除首败；探测 `0x19`/`0x55` 及不查 ACK 的原始地址事务均使所测后续首写成功，仍未分离总线动作与内部准备。增强取证两次同板 EN 复位的 UUID、UART SHA、nonce、CRC 互异且独立复算通过，排除简单复制；非全板冷启或 iid，第一轮报告窗口仍有 1 次终态/绘制失败。取证后原 GPIO fast 基线恢复校验，五色/READY，同窗 PCA 三试失败 17 次、绘制失败 15 次；用户离线无新目视或温度确认，M2 仍开放。失败处理设计识别 shadow 失效、主/CS释放双错误与马达ON返回失败后的OFF定时风险，均属待实板验证的潜在路径；安全0x40会使LCD复位，后续须重初始化。阶段 A 隔离 HEAD 487ec30，IDF 6.1 full GPIO Lab/headless 均构建通过；host 以实际 PCA/LCD IO 候选源码在 GPIO/PARLIO 两模式注入 PASS，覆盖主/CS释放双错、flush回调、再入、quiesce 边界及正常马达输出位保持。真实 worker/LVGL 锁/物理脚/面板重初始化未验，Audit 离线继续 GO、实板 NO-GO；主仓和样机基线未改，M2开放。 阶段 B2b 隔离 HEAD 481fc34 的 Linux host PARLIO/GPIO 故障注入均 PASS；完整镜像离线审计已通过，实板验证仍 NO-GO，PCA 首写根因未定，M2 开放。 PCNT 隔离旁听镜像 e6f5ad4 已构建并通过离线审计，尚未烧板、无计数或 ACK 结论；开关旁听 A/B 待实测，M2 开放。 返回码勘误：普通 PCA 写入/寄存器读的 INVALID_RESPONSE 仅证明事务未完成，不能判物理 NACK；probe 原始 NOT_FOUND/TIMEOUT 与既有计数保留，PCA 根因未定。 PN532 原厂资料确认 I0/I1 须确定绑带，c0 R73/R74 已拆且悬空，后续共享 I²C 归因受混杂；11/04 原绑带时 PCA 已失败，悬空非全部历史故障必要条件。待用户醒后断电固定模式再实测，M2 开放。 代码核查：PCA 重试的 pdMS_TO_TICKS(1) 在本轮100Hz构建折为0 tick，不保证1ms退避；仅可能影响连续重试，不能解释空闲首访，未做修复A/B，M2开放。 PCNT 旁听已修订至 5a2367d5，208656B镜像离线审计GO；旧e6f5ad4镜像停用。聚合计数仅辅助SCL阶段、不能判第9位ACK；未烧板，M2开放。 25kHz日志44.28–44.40s含IDF硬件超时/清总线与复位失败、PCA错误及LCD绘制失败；仅限晚期窗口，不能解释全部INVALID_RESPONSE或由事后SDA/SCL高电平推断当时健康。M2开放。 M2 屏幕验收草案已入专项：区分 c0 旧几何定位与目标 172×320 L2，建议同板同镜像五次冷启、30 分钟刷新及一次受控恢复，日志与目视双轨；次数时长非既定 PRD 门槛，尚未执行，M2 开放。顶部下一步已改为固定 PN532 绑带后以 GPIO 路径收敛 PCA/LCD 问题并完成验收；仅在实测性能不足时考虑 PARLIO 或下版硬件，未要求立即返修。RAW ISR 原版/补丁版隔离镜像离线审计GO，仅观测软件窗口ISR状态，当时均未烧板、无RAW实测，实板NO-GO、M2开放。官方IDF提交09f0c5c作为低置信待证线索：跨核复现条件与C6不同；纯stock/官方10行补丁A/B当时拟做、未烧板，M2开放。stock/fixed官方10行纯对照镜像IDF6.1离线GO，应用/配置/分区一致，均无RAW/PCNT/probe；RAW为独立工具；当时未烧板、M2开放。纯A/B日志门禁工作树HEAD0467df5，新增summarize.py及17项合成日志测试PASS；仅验证单次UART日志结构与计数，镜像未重构建/当时未烧板，M2开放。c0本轮rawclear纯A/B均0/32首写、重试后读回；RAW ISR与LCD_CS no-op/翻转对照收窄必要条件；1/20ms与增强遥测均门禁INVALID不得判改善；两次GPIO基线恢复分开记录，最终回滚后用户目视色块与横向文字完整、温度正常；错误仍在，M2开放。11/25 c0同板固定身份 GPIO A/B：0x55 预访消除本窗长空闲首访失败，但刷新耗尽仍在；非idle首败约8.76%/9.52%且负载不等，A 目视完整但初始填充报错、B无目视。原GPIO fast回滚写入校验并启动READY，用户确认温度正常无持续振动，本次回滚无实屏目视；M2开放。11/25第一性原理交叉审计补U3地址/上拉/RC预算/锁存读回边界、下一U3端逐字节波形与屏幕版本隔离；根因未定、M2开放。11/25 c0片上 PARLIO RX 诊断 PREP83 第二轮读提前失败，两次目标A/B均未执行；boot2最终安全双读回未证；原GPIO fast回滚已校验/READY但仍有重试与绘制失败、无本次目视。M2开放。11/25 PREP83受控复测同板两次各8试次有效，目标写首试全失败、重试锁存读回C3；片上捕获未覆盖异常事务，ACK/根因未知。原GPIO fast已校验回滚并进READY，仍有PCA耗尽/绘制失败，无本次目视；M2开放。11/25 目标写片上帧在B组见0x32地址NACK/STOP，第二帧三ACK/STOP；A/B首次API失败、第二次ESP_OK，但本轮延迟读回无效，实体根因未定；原GPIO fast已115200 no-stub烧回并Hash verified、45秒日志READY，仍494条retry2/3、22耗尽、18 draw错误，且无本轮实屏或温度确认；M2开放。正文回读 revision 144，旧附件保留；2026-11-25 微雪四模式外部对照新增 3.7 节并回读（正文 revision 136）：三模式实屏已确认，PARLIO 仅设备端 15/15、实屏待确认；本地专项 docs/bringup/2026-11-25-waveshare-c6-lcd-transport-comparison.md SHA-256 367BE1C12EA5DD65E40E1499481C59562606084ADB37C0E3DB2DD2B157BA9B1F，尚未上传附件；M2 仍开放 |
| `docs/bringup/HIYAKA_NFC_NO_ACK_INCIDENT.md` | YAKA-0005 原生文档 | `4083f2ceffdd47591c72545f436014885e58408404fb245ef2caff847f7dcdcd` | `XLQjdFjhhoxx3oxvGM3cQK9CnQb` | 2026-11-06 导入并回读 revision 2；Base 仍保留原附件 token |
| `docs/bringup/HIYAKA_MICROPHONE_DATA_PATH_INCIDENT.md` | [YAKA-0006 原生文档](https://scnlb1lk96sb.feishu.cn/docx/Q1u5df5QWomTj0x0xkfcSoz4nvh) | `2FD21656980872F1FF7FDE8687DE7C83C061A0E19AA18F96F657C6B5646BBC8C` | `Q1u5df5QWomTj0x0xkfcSoz4nvh` | 2026-11-23 增量写入 YAKA-0009 静态接口错配、旧交叉审计表述纠正与暂停测试边界；正文 revision 103 已回读，原附件未覆盖，本地全文与云端仍为局部同步 |
| `docs/bringup/2026-11-20-microphone-v2-v3-cross-audit.md` | YAKA-0006 原生文档附件 | `843638F21AA3B75AB759076C8AA958C0B70827E851FE92137D753D270AFAAF40` | `J3i8btiEKocuCoxOcLuchqBhnQg` | 2026-11-20 原 token 覆盖上传版本 `7710279909515463689`；附件块 `doxcncnnxwlOGq5xlmGJcOfchZg` 保留，仅移除末尾多余空行；source_file 回读 SHA 与本地相同 |
| `docs/bringup/2026-11-20-es7210-control-ab-evidence.md` | YAKA-0006 原生文档附件 | `D69E40739EE614353C98F9D7E7B934463ABE701FA4F186246F58F9648CEAC76B` | `KkOrbhnFQoYu8px5WwZcIy53n3c` | 2026-11-20 原 token 覆盖上传版本 `7710279873708690463`；附件块 `doxcnPVCtYQurFUJiQvg4WZc07s` 保留。含 A/B、C1–C4、D/E、F–AH 实验；仅将引用行尾空格改为 `<br>`，source_file 回读 SHA 与本地相同 |
| `docs/bringup/HIYAKA_TF_ESD_VIA_IN_PAD_SOLDER_WICKING.md` | YAKA-0007 原生文档 | `7395d2065561f36d82c952f9523a6cffc514ce7f253e74ba82138db8fb79d084` | `Lj5wdN0GJo2OThxKEDNczPcMn9b` | 2026-11-06 导入；正文插入两张产线证据图；Base 记录 `reczz28WMS5BSdkc` |
| `docs/research/HIYAKA_MINIATURIZATION_SPECIAL_STUDY.md` | YAKA-0008 原生文档 | `ebc451e4294f779eb86d0e3db6739b7effc1fa311a10b98499386b4302e2437b` | `BIOGdKeRco1t8IxvKhwcVv7Cnmf` | 2026-11-06 导入为“测试与发布”子文档；Base 记录 `reczz28WMS5BRuu3` |
| `docs/bringup/2026-11-05-audio-ab-plan.md`（麦克风软件 A/B 实验） | 02 硬件主文档 | `856ab25e1028ce515374ed03fb4f140debc77c7d24c502ed4fa02062e7debb21` | `CTokbQLRcouK9GxYkPlcLn1fnAb` | 2026-11-05 已上传并在 3.6 索引，附件块 `doxcnREJrpycsIPYMnO6T3HYod3` |
| `docs/bringup/2026-11-05-audio-ab-results.md`（麦克风 A/B 实板结果） | 02 硬件主文档 | `afeabbacef9926b56f6e39901910c5eb0edcc1aed538cdee3a5acac0` | `GyHQbwIqmoARfixoR5HcaOfDn37` | 2026-11-05 已上传并在 3.6 索引，附件块 `doxcnlSk75lySnYkCag0D1h16gf`；原始日志仅本地保留 |
| `docs/bringup/HIYAKA_MICROPHONE_DATA_PATH_INCIDENT.md`（HIYAKA-AUDIO-INC-001） | 02 硬件主文档 | `80f1443e8143bd4be7095f6433df7e5faf646f4003e7be570dfb89a237aa3f49` | `T5pgb21Ppoeaf2xL5ncc0gh6nve` | 2026-11-05 已上传并在 3.6 索引，附件块 `doxcnPDOLwg6s1i8lcpC1IIFIvf`；未覆盖 A/B 原始结果附件 |
| `docs/governance/ISSUE_ANALYSIS_TEMPLATE.md`（现行六节模板） | 01 测试与发布 | `e7527b1c42a471782616934c4e4b69ee8e15fe884ed5edfdd0af3730a23bdcb8` | `GkTjbjjTboa1xgx1VQCcW7xPnaf` | 2026-11-06 原 token 新版本 `7704729289190558737` 已回读；文末附件块 `doxcnWKWhFTNbX78j19igkgyguf` |
| `docs/bringup/2026-11-05-replacement-board-retest.md`（换入样机复测初版） | 01 测试与发布 | `a5454a5e686d1670086892d638d6eb4b14de50a093f55091320eeaf8bf8946e2` | `GWXRb5ajvobnNOxprcSck8UUnbe` | 历史附件保留，附件块 `doxcnl4PqcLCYqXbBZqBKLjq4hc` |
| `docs/bringup/2026-11-05-replacement-board-retest.md`（换入样机复测修订版，飞书文件名带 `-r2`） | 01 测试与发布 | `2838bc2ae77c2bc883698813a155b290c6104d02d1ef109c9d4869019db29b42` | `Stkvb6ewroFo8sxfvIxcmM7gnqe` | 2026-11-05 已上传并回读，附件块 `doxcndlL4WQutvqgCodGt3IZnKg`，section 5 摘要 revision 57 |
| `docs/bringup/HIYAKA_LCD_BACKLIGHT_INCIDENT.md`（HIYAKA-LCD-INC-001 当前版） | Base YAKA-0004 附件 | `7a6317ec56700dc3e81281066ca435ecfdf06431e247fdab95ce60612a475871` | `NxdMbx5Baozbb0xrWzucGItPnee` | 2026-11-06 原 token 新版本 `7704729293531646978` 已回读；人工补救摘要同步至 `reczz28VrGurKogr`，Base rev 57 |
| `docs/bringup/HIYAKA_LCD_BACKLIGHT_INCIDENT.md`（2026-11-12 快照） | Base YAKA-0004 附件 | `67cf7687e82179b664f82f2fa4f40ee8291a886c89acb2d42720ca986c5a5ee9` | `UCs2bgSmZoYVPCx4KI8cDAlGn8S` | 附件名 `HIYAKA_LCD_BACKLIGHT_INCIDENT-2026-11-12.md`；Base rev 68 已回读，历史附件保留 |
| `docs/bringup/HIYAKA_LCD_BACKLIGHT_INCIDENT.md`（2026-11-12 r2 快照） | Base YAKA-0004 附件 | `a4f568f69bb8277a4ec6bfc17d2bbc761c501687226797d4fcd9a0952d77c98f` | `UvAibeoyioVQAHxr9OLcKJiOnYf` | 附件名 `HIYAKA_LCD_BACKLIGHT_INCIDENT-2026-11-12-r2.md`；Base rev 71 已回读 |
| `docs/bringup/HIYAKA_LCD_BACKLIGHT_INCIDENT.md`（2026-11-12 r3 快照） | Base YAKA-0004 附件 | `aca293edbaa1235be789abf1e0f46ef76156735d29c08ae7a9f3cd159d7eb230` | `YiBNb82phoYyJ6xTtmMc1Gyfnme` | 附件名 `HIYAKA_LCD_BACKLIGHT_INCIDENT-2026-11-12-r3.md`；Base rev 72 已回读 |
| `docs/bringup/HIYAKA_LCD_BACKLIGHT_INCIDENT.md`（2026-11-12 r4 快照） | Base YAKA-0004 附件 | `3525d0fed515eaee88f7ca50beb8381fd97680acc6521beb064bf9f3443c064a` | `UaXQbHWe5owjZ6xrdNYcZiSlnnc` | 附件名 `HIYAKA_LCD_BACKLIGHT_INCIDENT-2026-11-12-r4.md`；Base rev 74 已回读，台账摘要含 343 次重试和 12 次刷新失败 |
| `docs/bringup/HIYAKA_LCD_BACKLIGHT_INCIDENT.md`（独立 I²C 对照版） | Base YAKA-0004 附件 | `EDD351FFC1CBF5E9D2BD02DB9844E5B53F432976A24F7FCF688750F0D2A4A53A` | `QLIqb8ixkoR4eNxJI3ncuwvRn3b` | 附件名 `HIYAKA_LCD_BACKLIGHT_INCIDENT-2026-11-12-i2c-ab.md`；Base rev 83 已追加 |
| `docs/bringup/2026-11-12-pca-audio-shared-i2c-ab-evidence.md`（r3） | Base YAKA-0004 附件 | `8C4E54D4DDE0E49767863148C2C468FBEF87C127F2E65503F77159DADAF19529` | `Q9TVbrDxXoWjUOxTQcmcR52qnBg` | 独立 I²C 对照原始计数与结论；Base rev 83 已追加，原始串口日志仅本地 |
| 用户提供的 LCD 实屏照片（2026-11-12） | Base YAKA-0004 附件 | `e7ed34583d6cfeac401f748336f0b2b342bcde3c33558927bd24a8b4d75d26ff` | `Cu6zbG8JZopcJhxr87Fcunq8nAg` | 初版竖屏与边带照片；Base rev 74 已回读，保留为实物证据 |
| `docs/bringup/HIYAKA_NFC_NO_ACK_INCIDENT.md`（HIYAKA-NFC-INC-001 当前版） | Base YAKA-0005 附件 | `4083f2ceffdd47591c72545f436014885e58408404fb245ef2caff847f7dcdcd` | `PpFDb1hAwooxO0xwrV3cz1zNn5d` | 2026-11-06 原 token 新版本 `7704729297650453521` 已回读；状态仍为“处理中” |
| `docs/bringup/2026-11-05-nfc-interface-ab-results.md`（NFC 独立接口 A/B 记录） | Base YAKA-0005 附件 | `0cd4f4ba265ad2dd18b594e3ad78c0346d16f0eb531b7460a0e95be8957e0454` | `Lw61bWU3YoxVeMxBoxFcgewjnWd` | `reczz28VrGurLWAH`，保留为当前专项以外的独立实板证据 |
| `docs/bringup/2026-11-12-nfc-hsu-rework-validation.md`（commit `1af7e0d` 快照） | Base YAKA-0005 附件 | `a0ff8c2c7c15d5a0cdfd3ea53a1dd68f8040fb6b6da4e2eddf00e60a0818f778` | `ZhBZbi9fYomMrwxaSJBch0XYnwe` | 2026-11-12 追加版本化附件，Base rev 66 已回读；悬空绑带与下一版建议已同步，状态“处理中” |
| `docs/bringup/HIYAKA_MICROPHONE_DATA_PATH_INCIDENT.md`（HIYAKA-AUDIO-INC-001 当前版） | Base YAKA-0006 附件 | `bfdfc201741d33b8bd00183014684314c1ef765ed5a622ba60bca71aa7e9042d` | `VQjgbsUdNoz6pbxrn8ncOe0BnNc` | 2026-11-06 原 token 新版本 `7704729302100610030` 已回读；状态仍为“处理中” |
| `docs/bringup/HIYAKA_MICROPHONE_DATA_PATH_INCIDENT.md`（独立 I²C 对照版） | Base YAKA-0006 附件 | `28541AB3D5775A25761263AAA56AB1C4D9547C7084BB928CA2FF4B240857E843` | `K8FUbFVkJouNNLxRoOmch8uOn9e` | 附件名 `HIYAKA_MICROPHONE_DATA_PATH_INCIDENT-2026-11-12-i2c-ab.md`；Base rev 84 已追加 |
| `docs/bringup/2026-11-12-pca-audio-shared-i2c-ab-evidence.md`（r3） | Base YAKA-0006 附件 | `8C4E54D4DDE0E49767863148C2C468FBEF87C127F2E65503F77159DADAF19529` | `LHN6bBMvdoQNxHxGPCMcMrHwn8d` | 独立 I²C 对照原始计数与结论；Base rev 84 已追加，摘要 Base rev 85 回读 |
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
| `docs/hardware/attachments/yaka-3.0-schematic-audit-and-eco-spec.md`（HiYaKa 命名对齐版） | 02 硬件主文档 | `9637e640d50c9967e2c6949cab715cbf211b5823c3578668644d9c5dc2439997` | `Fjhfb2BWYo2ghOxejaecf4B7nbg` | 2026-11-21 原位覆盖并回读一致；文件版本 `7710331749300964547`；主文档 revision 145（覆盖前后未变） |
| `docs/hardware/attachments/yaka-3.0-pcb-comprehensive-audit-and-eco-spec.md`（HiYaKa 命名对齐版） | 02 硬件主文档 | `61a3aa814fd473b3254afcbe2daeaf7c11f8a2cfe730692e814472b84395cdfc` | `QT8Vbd585oCSSJxsBK1cvUjkn7c` | 2026-11-02 已上传并回读，当前版本 |
| `docs/hardware/attachments/yaka-3.0-prefreeze-release-manufacturing-and-sourcing-checklist.md`（HiYaKa 命名对齐版） | 02 硬件主文档 | `a64c8ee05cac9f4f414f42f41c8ed362fe688e904babe2f9d06ac64aea95e318` | `K7tXbxTHBoiuSFx6Gijck9k5njf` | 2026-11-02 已上传并回读，当前版本 |
| `docs/hardware/attachments/yaka-3.0-procurement-and-consignment-list.csv` | 02 硬件主文档 | `10a8a83fbe50177ba737962a3b2a028e0196384ff88f6b26f9ace4e475a4fc46` | `XeMxbTcEZoqMUuxkN8jcMxHnngz` | 已上传 |
| `docs/hardware/HARDWARE_MODULE_INTERFACES.md`（2026-11-05 现行附件） | 02 硬件主文档 | `be4d3b9185f75a1948abbc95b2d0bd324215daea890c3b83c0b7f4db80fd97ab` | `TPFub69t6owUqdxNRUmch6WEnEa` | 已上传并在 3.2 索引 03 原生文档和本附件 |
| `docs/hardware/HARDWARE_MODULE_INTERFACES.md`（当日中间版） | 02 硬件主文档 | `435f6fe763633265419066d43c7c87c77dd4d7d66680cfd80d38f5f455a74713` | `QYFDbZqqLoCanwxOggIcr1ornld` | 中间版，保留追溯 |
| `docs/hardware/HARDWARE_MODULE_INTERFACES.md`（当日 ACT_B/IMU 增量） | 02 硬件主文档 | `a6be2a8cb217fa2fa96a11508c9c12a15d48aeee9c1f679cfc4ea9c3a7b0ea54` | `Ra8qb5Z01o7oDGx9FTuc8A23n17` | 中间版，保留追溯 |
| `docs/hardware/HARDWARE_MODULE_INTERFACES.md`（历史版） | 02 硬件主文档 | `7054463d187430309db1d0a751759d2e0193af359f53a625600fa16aa07913ea` | `DRy0bQ6dIobLjOx5WO6cVcpsnoc` | 历史附件，保留追溯 |
| `docs/governance/DEVELOPMENT_COLLABORATION_AND_DOCUMENTATION_STANDARD.md` | 01 开发协作与文档维护规范 | `E72FE0843BEDFFD06E8950A8CC548BC359394D0C61C5AAF0A1A8440D7A53B711` | `IaDebP4yBoqK0ux6Mtkc4oqhnUc` | 2026-11-20 同 token 上传版本 `7710271471838350641`；source_file 回读 SHA 与本地相同 |
| `docs/research/VELXIO_SIMULATION_EVALUATION.md` | 04 外部参考与技术调研 | `79614967175b25c04e50eb8ac4707cd07a8cddd52c69d9b52ab81c23b672a986` | `ZsLYbpRNlomJiZx9XVAcJjCUnHe` | 已上传并补充原生摘要 |
| `docs/research/HIYAKA_MINIATURIZATION_SPECIAL_STUDY.md` | 04 外部参考与技术调研（历史附件） | `ebc451e4294f779eb86d0e3db6739b7effc1fa311a10b98499386b4302e2437b` | `Xa2pbrG2VoqalZxRqrzcJWw2nBb` | 2026-11-05 已上传；2026-11-06 已提升为 YAKA-0008 原生子文档，旧附件仅保留来源追溯 |

## 3. 同步与冲突规则

1. 修改前读取本地和飞书的当前版本；禁止依据截图猜测现状。
2. 本地表格中的飞书 revision 是最近一次已回读版本，不是并发锁；写入前仍须重新读取 revision。
3. 飞书已有图片、评论和历史不做全文覆盖；对治理文档采用带日期的增量章节，对三份主文档采用受控重导入或章节级更新。
4. 附件不得静默替换。内容变更后以新文件名或新版本上传，并更新完整 SHA-256、文件 Token 和适用硬件版本。
5. 本地与飞书冲突时暂停自动同步，先判断哪一端持有更高等级证据；不得用“更新时间较新”替代事实判定。
6. 每次同步后至少回读标题目录、关键结论和 revision；只有 API 成功但未回读，不计为同步完成。

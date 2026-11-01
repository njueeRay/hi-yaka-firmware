# HiYaKa 飞书文档同步登记

> 最后更新：2026-11-02
> 飞书目录：[HiYaKa](https://scnlb1lk96sb.feishu.cn/drive/folder/Qz1ifJDCvlYKz1dCBO2cFl9Bn3c)
> 原则：Git Markdown 保存可审计的工程事实源；飞书保存协作副本、跨文档索引和附件入口。

当前飞书目录按用途分为 `01 产品与使用`、`02 硬件与固件研发`、`03 协作、决策与调研`、`04 测试与发布`、`90 制造与发布归档` 和 `99 历史资料（只读）`。旧 `02 工程基线与接口契约` 已废止并移入历史区，当前接口事实以硬件主文档和硬件连接与固件接口表为准。

## 1. 主文档映射

| 本地事实源/治理对象 | 飞书文档 | Token | Revision | 当前状态 |
|---|---|---|---:|---|
| `docs/governance/PROJECT_CHARTER.md` 与项目索引 | [00 HiYaKa 项目总览与文档导航](https://scnlb1lk96sb.feishu.cn/docx/Kl74deqLtoteWyxeshlci43qn0g) | `Kl74deqLtoteWyxeshlci43qn0g` | 35 | 顶层唯一入口；当前阶段已同步为 PH0–PH5 L0 前置基线完成，等待 M1 首板 Bring-up |
| `.agents/skills/yaka-project/references/yaka-3.0-prd.md` | [01 产品需求与交互状态](https://scnlb1lk96sb.feishu.cn/docx/T3isd0m8jo2bVEx37noc9Oq3noh) | `T3isd0m8jo2bVEx37noc9Oq3noh` | 53 | 完整产品 PRD v0.4；现行产品/系列语义统一为 HiYaKa；OTA 已对齐 MVP Must 与验收矩阵；位于 `01 产品与使用` |
| `docs/product/PRODUCT_DATA_AND_USER_MANUAL.md` | [02 产品规格与使用手册](https://scnlb1lk96sb.feishu.cn/docx/XohYdP65uo5zfTxkMEScyzXinag) | `XohYdP65uo5zfTxkMEScyzXinag` | 5 | 现行产品语义统一为 HiYaKa；位于 `01 产品与使用` |
| `docs/governance/FIRMWARE_PRD.md` | [01 固件 PRD 与里程碑](https://scnlb1lk96sb.feishu.cn/docx/DwmvdwnfboJKX3xpbyIc1b0bntc) | `DwmvdwnfboJKX3xpbyIc1b0bntc` | 125 | PH2–PH5 已按 L0 证据关闭；PR #7、18 项定向测试、ESP-IDF 6.1 正式 `main` 构建、产物 SHA-256 和证据边界已回读；架构 Markdown 与四页图注已同步到当前口径；位于 `02 硬件与固件研发` |
| `docs/hardware/HARDWARE_DESIGN_AND_DEVELOPMENT.md` | [02 硬件设计与研发主文档](https://scnlb1lk96sb.feishu.cn/docx/I2SIdpqZnouTxkxYEuLcnyJxndc) | `I2SIdpqZnouTxkxYEuLcnyJxndc` | 81 | HiYaKa 现行语义、PH2–PH5 周日志和 G1/L1 下一阶段已回读；位于 `02 硬件与固件研发` |
| `docs/hardware/HARDWARE_MODULE_INTERFACES.md` | [03 硬件连接与固件接口表](https://scnlb1lk96sb.feishu.cn/docx/UdVhdzlP6oAxXZxzyMIc25SxnMk) | `UdVhdzlP6oAxXZxzyMIc25SxnMk` | 17 | 当前接口权威页；现行硬件语义统一为 HiYaKa；Markdown 附件保留在硬件主文档 |
| `docs/governance/DEVELOPMENT_COLLABORATION_AND_DOCUMENTATION_STANDARD.md` | [01 开发协作与文档维护规范](https://scnlb1lk96sb.feishu.cn/docx/K33VdjETzooOrTxIQD9cOYXln5c) | `K33VdjETzooOrTxIQD9cOYXln5c` | 12 | HiYaKa 板级事实、无代际后缀命名与板级契约用语已回读；位于 `03 协作、决策与调研` |
| 仓库、上游与技术决策 | [02 决策与版本记录](https://scnlb1lk96sb.feishu.cn/docx/NWmcdvVpgov54exQOkIcvvOunmd) | `NWmcdvVpgov54exQOkIcvvOunmd` | 14 | 保存正式 ADR 与版本影响 |
| `.agents/skills/yaka-project/references/yaka-3.0-owner-decisions-and-todos.md` | [03 当前角色、Owner 决策与待办](https://scnlb1lk96sb.feishu.cn/docx/Y5SqdV9zloa3eLxQZALcTDQ4nOe) | `Y5SqdV9zloa3eLxQZALcTDQ4nOe` | 9 | 当前角色、待裁决事项和团队输入；现行产品/系列语义统一为 HiYaKa |
| 外部资料治理与 `docs/research/VELXIO_SIMULATION_EVALUATION.md` | [04 外部参考与技术调研](https://scnlb1lk96sb.feishu.cn/docx/Cn2bdVrSboCRKLxuDErcWWSUnpc) | `Cn2bdVrSboCRKLxuDErcWWSUnpc` | 11 | 现行适用版本统一为 HiYaKa；位于 `03 协作、决策与调研` |
| `docs/governance/EVIDENCE_POLICY.md` 与 `docs/bringup/` | [01 测试、问题与发布记录](https://scnlb1lk96sb.feishu.cn/docx/RXKYdmVHaobeHNx39uNcl8BAnbf) | `RXKYdmVHaobeHNx39uNcl8BAnbf` | 18 | PH5 首板验证章节、四份执行附件、L0/L1–L3 边界和正式 `main` 证据已回读；位于 `04 测试与发布` |
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

双仓库发布策略及公开仓库日志对应本地提交：`b18d267`；公开仓库首次提交：`ChatChatTech/yaka@4ea1c48`（2026-10-29）。

## 2. 附件登记

| 附件 | 目标文档 | SHA-256 | 飞书文件 Token | 状态 |
|---|---|---|---|---|
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
| `docs/yaka/firmware-architecture.md`（v1.4，PH5 状态版） | 01 固件 PRD 与里程碑 | `de99195f25ac17ffec937366a0c7fbcad26f95dd467f1eb30f5b8fbb99b02207` | `M7CLbITYNoCm4axmJsic5cg6n7f` | 2026-11-02 已上传、定位并回读，当前版本 |
| `docs/yaka/diagrams/yaka-firmware-architecture.drawio`（v1.4 编辑源） | 01 固件 PRD 与里程碑 | `b82f3dd643f84aabf483fbd36f65bee8586c3a9077ac68ff40be485c8ce62e27` | `OAtjbtKkPokUYCxJLRyc05ccndc` | 2026-11-02 已上传并回读，当前编辑源 |
| `docs/yaka/diagrams/yaka-firmware-architecture-v1.4.pdf`（四页） | 01 固件 PRD 与里程碑 | `228587df581ca8aa1356e58d47453b04cbc9fffd93538556f71094c383746b30` | `OBVmbC2SGof2cox1M53cLlbUnnc` | 2026-11-02 已上传并回读，当前评审版 |
| `docs/yaka/diagrams/yaka-firmware-architecture.drawio.png`（v1.4 第 1 页） | 01 固件 PRD 与里程碑 | `1778f9a49a5a7dea7ac6a532ba083b19dffb01a299bb64bb70fd83d165076ce8` | `ZyBebfPfLoaGIbxrBNCcj8VlnIf` | 当前正文图片，caption 已回读 |
| `docs/yaka/diagrams/yaka-firmware-architecture-page-2.drawio.png`（v1.4） | 01 固件 PRD 与里程碑 | `cc1fffa9af437a923eb61dccad59fe25100f3a3ea2a3a898977a204590d2d06d` | `G9bPbTZtJon5ZmxOomEcLo4Xn7e` | 当前正文图片，caption 已回读 |
| `docs/yaka/diagrams/yaka-firmware-architecture-page-3.drawio.png`（v1.4） | 01 固件 PRD 与里程碑 | `b08f4e09489236db3db6b0a4eca1067f09ff47bad1cc206a08402204120ae583` | `L0iTbjLThoDHnPxyUTmcSN6ynvg` | 当前正文图片，caption 已回读 |
| `docs/yaka/diagrams/yaka-firmware-architecture-page-4.drawio.png`（v1.4） | 01 固件 PRD 与里程碑 | `542196211e63e3603472d54b114d53f9f8c852fc7fade183676f7297e9ce328d` | `MARZbPQeaoYWupxGCAKcxs6OnQf` | 当前正文图片，caption 已回读 |
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
| `docs/hardware/HARDWARE_MODULE_INTERFACES.md` | 02 硬件主文档 | `7054463d187430309db1d0a751759d2e0193af359f53a625600fa16aa07913ea` | `DRy0bQ6dIobLjOx5WO6cVcpsnoc` | 已上传并在 3.2 索引 03 原生文档 |
| `docs/governance/DEVELOPMENT_COLLABORATION_AND_DOCUMENTATION_STANDARD.md` | 01 开发协作与文档维护规范 | `2b6970f4a8aa3102a443f5c0fc9b5e0bff8c04b197084d3cfd2f646961ac2d70` | `IaDebP4yBoqK0ux6Mtkc4oqhnUc` | 已上传 |
| `docs/research/VELXIO_SIMULATION_EVALUATION.md` | 04 外部参考与技术调研 | `79614967175b25c04e50eb8ac4707cd07a8cddd52c69d9b52ab81c23b672a986` | `ZsLYbpRNlomJiZx9XVAcJjCUnHe` | 已上传并补充原生摘要 |

## 3. 同步与冲突规则

1. 修改前读取本地和飞书的当前版本；禁止依据截图猜测现状。
2. 本地表格中的飞书 revision 是最近一次已回读版本，不是并发锁；写入前仍须重新读取 revision。
3. 飞书已有图片、评论和历史不做全文覆盖；对治理文档采用带日期的增量章节，对三份主文档采用受控重导入或章节级更新。
4. 附件不得静默替换。内容变更后以新文件名或新版本上传，并更新完整 SHA-256、文件 Token 和适用硬件版本。
5. 本地与飞书冲突时暂停自动同步，先判断哪一端持有更高等级证据；不得用“更新时间较新”替代事实判定。
6. 每次同步后至少回读标题目录、关键结论和 revision；只有 API 成功但未回读，不计为同步完成。

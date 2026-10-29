# YaKa 飞书文档同步登记

> 最后更新：2026-10-29  
> 飞书目录：[YaKa 3.x](https://scnlb1lk96sb.feishu.cn/drive/folder/Qz1ifJDCvlYKz1dCBO2cFl9Bn3c)  
> 原则：Git Markdown 保存可审计的工程事实源；飞书保存协作副本、跨文档索引和附件入口。

## 1. 主文档映射

| 本地事实源/治理对象 | 飞书文档 | Token | Revision | 2026-10-29 状态 |
|---|---|---|---:|---|
| `docs/governance/PROJECT_CHARTER.md` 与项目索引 | [00 项目首页与版本索引](https://scnlb1lk96sb.feishu.cn/docx/Kl74deqLtoteWyxeshlci43qn0g) | `Kl74deqLtoteWyxeshlci43qn0g` | 26 | 已追加双仓库发布策略并回读 |
| 产品需求摘要与状态机 | [01 YaKa 3.x 产品需求与状态机](https://scnlb1lk96sb.feishu.cn/docx/T3isd0m8jo2bVEx37noc9Oq3noh) | `T3isd0m8jo2bVEx37noc9Oq3noh` | 4 | 已追加产品边界并回读 |
| 硬件/固件接口摘要 | [02 工程基线与接口契约](https://scnlb1lk96sb.feishu.cn/docx/NBzHd2mvgoHQ06xLNXOc0A0tnoe) | `NBzHd2mvgoHQ06xLNXOc0A0tnoe` | 18 | 已按 R1.6 附件复核并回读 I²C 契约 |
| 仓库、上游与技术决策 | [03 版本、决策与变更日志](https://scnlb1lk96sb.feishu.cn/docx/NWmcdvVpgov54exQOkIcvvOunmd) | `NWmcdvVpgov54exQOkIcvvOunmd` | 11 | 已记录私有开发/公开快照决策并回读 |
| 外部资料治理 | [04 外部参考与调研库](https://scnlb1lk96sb.feishu.cn/docx/Cn2bdVrSboCRKLxuDErcWWSUnpc) | `Cn2bdVrSboCRKLxuDErcWWSUnpc` | 4 | 已追加来源优先级并回读 |
| `docs/governance/EVIDENCE_POLICY.md` | [05 验证、缺陷与发布](https://scnlb1lk96sb.feishu.cn/docx/RXKYdmVHaobeHNx39uNcl8BAnbf) | `RXKYdmVHaobeHNx39uNcl8BAnbf` | 4 | 已追加 L0–L3 证据规则并回读 |
| `docs/governance/FIRMWARE_PRD.md` | [06 YaKa 3.0 固件 PRD](https://scnlb1lk96sb.feishu.cn/docx/DwmvdwnfboJKX3xpbyIc1b0bntc) | `DwmvdwnfboJKX3xpbyIc1b0bntc` | 4 | 新建、附件上传、目录回读完成 |
| `docs/product/PRODUCT_DATA_AND_USER_MANUAL.md` | [07 YaKa 3.0 产品数据与使用手册](https://scnlb1lk96sb.feishu.cn/docx/XohYdP65uo5zfTxkMEScyzXinag) | `XohYdP65uo5zfTxkMEScyzXinag` | 2 | 新建并完成目录回读 |
| `docs/hardware/HARDWARE_DESIGN_AND_DEVELOPMENT.md` | [08 YaKa 3.0 硬件设计与研发主文档](https://scnlb1lk96sb.feishu.cn/docx/I2SIdpqZnouTxkxYEuLcnyJxndc) | `I2SIdpqZnouTxkxYEuLcnyJxndc` | 25 | 项目操作台结构及公开仓库日志已回读，五个权威附件保留 |
| 历史仓库与旧资料归档规则 | [99 历史归档索引](https://scnlb1lk96sb.feishu.cn/docx/KlRxdMk4ho6A5Txs3dqc3KG2nLd) | `KlRxdMk4ho6A5Txs3dqc3KG2nLd` | 6 | 已记录历史/私有/公开三仓库边界并回读 |

最近一次主文档结构同步对应本地提交：`a0a4f3b`（2026-10-29）。

## 2. 附件登记

| 附件 | 目标文档 | SHA-256 | 飞书文件 Token | 状态 |
|---|---|---|---|---|
| `docs/yaka/firmware-architecture.md` | 06 固件 PRD | `7d8f7ae0b8880c075e05da9f75afced80deccec991d9a8abca8d1f9a9cc72598` | `QSQRbd2TWo07WPxn4pncxqE0n3f` | 已上传 |
| `docs/hardware/attachments/yaka-3.0-final-schematic-pcb-reaudit-2026-10-21.md` | 08 硬件主文档 | `d2da46dcd980060614016da17d24c9b153577c8c454f2737622a448693bbd1cb` | `UVG7bcGxEoQl2Lx5rvfcgr8unwh` | 已上传 |
| `docs/hardware/attachments/yaka-3.0-schematic-audit-and-eco-spec.md` | 08 硬件主文档 | `b6fe1b2f634010054158d67e5f3538f11909c551cf893fda26aad7ff219abcc9` | `KGdgbjotXowmxrxelnoc4Rf7nfb` | 已上传 |
| `docs/hardware/attachments/yaka-3.0-pcb-comprehensive-audit-and-eco-spec.md` | 08 硬件主文档 | `fe0129892773decae14c210b9e4db0402e42ffb7e46f25444fabb7fbd2378455` | `H5TPb0JwLo9OwxxXPMMcyBnQnLd` | 已上传 |
| `docs/hardware/attachments/yaka-3.0-prefreeze-release-manufacturing-and-sourcing-checklist.md` | 08 硬件主文档 | `21827082c722903b4b8ac7c6289d42e3f838cc548a14e96d565cc6aa1ca03c90` | `DnyPbFCe9o5hQ7xX1CzcVrBjn5e` | 已上传 |
| `docs/hardware/attachments/yaka-3.0-procurement-and-consignment-list.csv` | 08 硬件主文档 | `10a8a83fbe50177ba737962a3b2a028e0196384ff88f6b26f9ace4e475a4fc46` | `XeMxbTcEZoqMUuxkN8jcMxHnngz` | 已上传 |

## 3. 同步与冲突规则

1. 修改前读取本地和飞书的当前版本；禁止依据截图猜测现状。
2. 本地表格中的飞书 revision 是最近一次已回读版本，不是并发锁；写入前仍须重新读取 revision。
3. 飞书已有图片、评论和历史不做全文覆盖；对治理文档采用带日期的增量章节，对三份主文档采用受控重导入或章节级更新。
4. 附件不得静默替换。内容变更后以新文件名或新版本上传，并更新完整 SHA-256、文件 Token 和适用硬件版本。
5. 本地与飞书冲突时暂停自动同步，先判断哪一端持有更高等级证据；不得用“更新时间较新”替代事实判定。
6. 每次同步后至少回读标题目录、关键结论和 revision；只有 API 成功但未回读，不计为同步完成。

# HiYaKa 固件文档索引

> 本地 Markdown 是可审查的工程事实源；飞书是协作副本和附件入口。双端 token、revision、附件哈希和冲突规则见[飞书同步登记](governance/FEISHU_SYNC_REGISTRY.md)。

## 三份核心开发文档

建议按以下顺序阅读。三份文档分别回答“做什么”“怎样分阶段交付”“接到什么硬件上”，不能互相替代。

| 顺序 | 本地/GitHub 文档 | 面向读者 | 飞书协作副本 |
|---:|---|---|---|
| 1 | [HiYaKa 产品需求与交互状态](../.agents/skills/yaka-project/references/yaka-3.0-prd.md) | 产品、交互、研发、测试；理解用户、场景、范围和验收 | [01 产品需求与交互状态](https://scnlb1lk96sb.feishu.cn/docx/T3isd0m8jo2bVEx37noc9Oq3noh) |
| 2 | [固件 PRD 与里程碑](governance/FIRMWARE_PRD.md) | 固件、测试、项目管理；查看硬件前置阶段、M1–M7 和完成定义 | [01 固件 PRD 与里程碑](https://scnlb1lk96sb.feishu.cn/docx/DwmvdwnfboJKX3xpbyIc1b0bntc) |
| 3 | [硬件连接与固件接口表](hardware/HARDWARE_MODULE_INTERFACES.md) | 固件、硬件、验证；查询 GPIO、总线、地址、极性和初始化顺序 | [03 硬件连接与固件接口表](https://scnlb1lk96sb.feishu.cn/docx/UdVhdzlP6oAxXZxzyMIc25SxnMk) |

代码开发前请再阅读[固件总体架构与开发基线](yaka/firmware-architecture.md)及其[四页可编辑架构图](yaka/diagrams/yaka-firmware-architecture.drawio)。架构图明确区分当前实现、上游复用、待批准规划和实板验证门禁；不能把图中的橙色规划节点当成已经存在的代码。

同步规则：Markdown 是可审计事实源；飞书用于协作阅读。每关闭一个硬件前置阶段或产品固件里程碑，先提交证据和本地 Markdown，再推送 GitHub，随后同步飞书并回写 revision。实板到达前完成的阶段只允许标记为 L0，不得把编译、模拟或 host test 写成硬件通过。

## 文档目录

| 类别 | 文档 | 作用 |
|---|---|---|
| 产品 | [HiYaKa 产品 PRD](../.agents/skills/yaka-project/references/yaka-3.0-prd.md) | 用户、场景、产品范围、交互、需求 ID 和产品验收 |
| 决策 | [当前角色、Owner 决策与待办](../.agents/skills/yaka-project/references/yaka-3.0-owner-decisions-and-todos.md) | 已冻结/暂行决定、待 Owner 裁决和团队输入 |
| 治理 | [项目任务书](governance/PROJECT_CHARTER.md) | 北极星、范围、权限和完成定义 |
| 治理 | [固件 PRD](governance/FIRMWARE_PRD.md) | 固件需求、里程碑与验收真值 |
| 治理 | [开发协作与文档规范](governance/DEVELOPMENT_COLLABORATION_AND_DOCUMENTATION_STANDARD.md) | 双仓库、上游同步、PR、周日志、双端同步与发布规则 |
| 治理 | [证据政策](governance/EVIDENCE_POLICY.md) | 证据等级和发布结论规则 |
| 治理 | [飞书同步登记](governance/FEISHU_SYNC_REGISTRY.md) | 本地与飞书映射 |
| 架构 | [固件总体架构](yaka/firmware-architecture.md) | 代码现状、目标分层、任务/数据流、小智复用边界和文件落点 |
| 架构 | [四页固件架构图](yaka/diagrams/yaka-firmware-architecture.drawio) | 总体分层、任务并发、关键数据流、构建/上游/CI/文档治理；附 PNG 与 PDF 导出 |
| 架构 | [OTA、资源预算与构建追溯合同](yaka/update-assurance.md) | 双 OTA 分区、升级包验证、首启确认、资源门禁和候选产物追溯 |
| 项目主文档 | [硬件设计与研发主文档](hardware/HARDWARE_DESIGN_AND_DEVELOPMENT.md) | 阶段规划、技术链路、材料索引、工作日志与附件入口 |
| 硬件接口 | [硬件连接与固件接口表](hardware/HARDWARE_MODULE_INTERFACES.md) | GPIO、总线、模块连接、初始化顺序和首板验证矩阵 |
| 产品 | [产品规格与使用手册](product/PRODUCT_DATA_AND_USER_MANUAL.md) | 当前可用能力、操作和限制 |
| 阶段 | [M1 基线](yaka/m1-baseline.md) | 当前最小板级启动范围 |
| 技术调研 | [Velxio 前置仿真评估](research/VELXIO_SIMULATION_EVALUATION.md) | C6 仿真覆盖、限制、接入方式和限时 spike 判据 |

## 飞书协作入口

- [HiYaKa 文档目录](https://scnlb1lk96sb.feishu.cn/drive/folder/Qz1ifJDCvlYKz1dCBO2cFl9Bn3c)
- [01 产品需求与交互状态](https://scnlb1lk96sb.feishu.cn/docx/T3isd0m8jo2bVEx37noc9Oq3noh)
- [01 固件 PRD 与里程碑](https://scnlb1lk96sb.feishu.cn/docx/DwmvdwnfboJKX3xpbyIc1b0bntc)
- [02 产品规格与使用手册](https://scnlb1lk96sb.feishu.cn/docx/XohYdP65uo5zfTxkMEScyzXinag)
- [02 硬件设计与研发主文档](https://scnlb1lk96sb.feishu.cn/docx/I2SIdpqZnouTxkxYEuLcnyJxndc)
- [03 硬件连接与固件接口表](https://scnlb1lk96sb.feishu.cn/docx/UdVhdzlP6oAxXZxzyMIc25SxnMk)
- [01 开发协作与文档维护规范](https://scnlb1lk96sb.feishu.cn/docx/K33VdjETzooOrTxIQD9cOYXln5c)
- [03 当前角色、Owner 决策与待办](https://scnlb1lk96sb.feishu.cn/docx/Y5SqdV9zloa3eLxQZALcTDQ4nOe)


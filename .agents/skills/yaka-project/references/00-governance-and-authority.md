# 00 治理与权威性

## 证据优先级

1. 当前 EasyEDA Pro 活体工程、原理图/PCB 绑定、BOM、DRC 和结构化端点数据；
2. `R1.6` 封存前原理图/PCB/PRD 全链路复审及冻结制造包；
3. 经批准的 YaKa 3.0 产品 PRD；
4. 精确料号的厂商资料和官方 ESP-IDF/小智上游源码；
5. 本仓库治理文档、固件 PRD、架构和测试记录；
6. 历史 `yaka-firmware`、旧飞书材料和社区资料，仅作线索。

冲突时保留冲突，不用低优先级材料覆盖高优先级事实。硬件发生变更后，必须同步检查固件硬件契约、产品能力声明、制造资料和 bring-up 测试。

## 本地文档职责

| 文档 | 负责内容 |
|---|---|
| `.agents/skills/yaka-project/references/yaka-3.0-prd.md` | 产品级 PRD：用户、场景、产品范围、交互、需求 ID 和产品验收；同步到飞书“01 产品需求与交互状态” |
| `.agents/skills/yaka-project/references/yaka-3.0-owner-decisions-and-todos.md` | 当前角色、已冻结/暂行决定、待 Owner 裁决和团队输入；同步到飞书“当前角色、Owner 决策与待办” |
| `docs/governance/PROJECT_CHARTER.md` | 北极星、范围、决策权限和完成定义 |
| `docs/governance/FIRMWARE_PRD.md` | 固件需求、里程碑、兼容合同和验收 |
| `docs/yaka/firmware-architecture.md` | 软件分层、数据流和上游复用边界 |
| `docs/hardware/HARDWARE_DESIGN_AND_DEVELOPMENT.md` | 项目操作台：阶段规划、跨域链路、材料索引与连续研发日志；详细硬件事实留在专项附件 |
| `docs/product/PRODUCT_DATA_AND_USER_MANUAL.md` | 对用户和测试人员可见的产品数据与操作 |
| `docs/governance/EVIDENCE_POLICY.md` | 证据等级、记录字段和结论规则 |
| `docs/governance/FEISHU_SYNC_REGISTRY.md` | 本地与飞书文档、revision 和附件映射 |

产品 PRD 和 Owner 决策台账作为项目 skill 的版本化 reference 保存，是跨硬件、固件和应用的项目资料；固件仓库内的 `FIRMWARE_PRD.md` 只负责把已批准产品要求翻译成固件合同，不得取代产品 PRD。`projects/yaka/docs/` 下同名文件是本次迁移来源，不再作为 skill 的唯一依赖。

## 变更合同

每次跨层修改先写清：基线、目标、修改范围、继承项、未知项、是否授权外部写入。完成后更新受影响文档、测试证据和同步登记。产品 `TBD` 不由驱动实现擅自冻结。

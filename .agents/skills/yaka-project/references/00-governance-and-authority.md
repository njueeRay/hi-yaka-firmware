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
| `docs/governance/PROJECT_CHARTER.md` | 北极星、范围、决策权限和完成定义 |
| `docs/governance/FIRMWARE_PRD.md` | 固件需求、里程碑、兼容合同和验收 |
| `docs/yaka/firmware-architecture.md` | 软件分层、数据流和上游复用边界 |
| `docs/hardware/HARDWARE_DESIGN_AND_DEVELOPMENT.md` | 硬件主文档、接口契约、制造与研发日志入口 |
| `docs/product/PRODUCT_DATA_AND_USER_MANUAL.md` | 对用户和测试人员可见的产品数据与操作 |
| `docs/governance/EVIDENCE_POLICY.md` | 证据等级、记录字段和结论规则 |
| `docs/governance/FEISHU_SYNC_REGISTRY.md` | 本地与飞书文档、revision 和附件映射 |

## 变更合同

每次跨层修改先写清：基线、目标、修改范围、继承项、未知项、是否授权外部写入。完成后更新受影响文档、测试证据和同步登记。产品 `TBD` 不由驱动实现擅自冻结。

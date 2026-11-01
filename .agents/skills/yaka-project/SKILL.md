---
name: yaka-project
description: 维护 YaKa 3.0 的产品、硬件、固件、验证证据及飞书同步文档。处理 YaKa 需求、原理图/PCB/SMT、板级接口、小智上游同步、固件实现、bring-up、发布或项目文档治理时使用。
---

# YaKa 项目治理

先读 [00-governance-and-authority.md](references/00-governance-and-authority.md)，确定事实来源、文档职责和本次变更合同。不要把历史 YaKa 仓库、旧飞书材料或一次编译结果当成当前硬件/产品真值。

根据任务只加载相关 reference：

- 产品范围、交互、状态机或需求 ID：读 [10-product-and-prd.md](references/10-product-and-prd.md)，再按其中入口读取完整产品 PRD。
- 当前角色、Owner 决策、待确认事项或“谁来做什么”：读 [15-owner-decisions-and-todos.md](references/15-owner-decisions-and-todos.md)。
- 原理图、PCB、器件、引脚、总线、SMT、采购或首板：读 [20-hardware-and-manufacturing.md](references/20-hardware-and-manufacturing.md)。
- 固件架构、小智复用、ESP-IDF、分支或上游同步：读 [30-firmware-and-upstream.md](references/30-firmware-and-upstream.md)。
- 验证、日志、缺陷、资源预算、发布结论：读 [40-validation-and-evidence.md](references/40-validation-and-evidence.md)。
- 修改本地/飞书文档、附件或同步登记：读 [50-document-sync.md](references/50-document-sync.md)。

## 命名合同

1. 板子的对外名称和语义名称统一为 `HiYaKa`，归属组织为 `ChatChat`；需要同时表达归属时写“ChatChat HiYaKa”，不要写“HiYaKa V3”“HiYaka”“Hi YaKa V3”或把 `V3` 当成板名后缀。
2. `YaKa 3.0` 是当前产品版本语义，不等于板子名“V3”。产品、架构、测试、发布和用户文案都遵守此区分。
3. 新命名按表示法固定：可读/驼峰形式用 `HiYaKa`；全小写短横线形式用 `hi-yaka`；全大写下划线形式用 `HI_YAKA`。不得自行派生 `HiYaka`、`Hi-Yaka`、`HIYAKA` 等变体。
4. 现有 `chatchat/hi-yaka-v3`、`hi-yaka-v3`、`HiYakaV3Board`、`BOARD_TYPE_HI_YAKA_V3` 和 `hi_yaka_v3_board.cc` 只属于当前内部实现标识。只有在精确引用构建参数、目录、文件、类或宏时才以代码格式原样出现，并标明“旧内部兼容标识”；不得把它们传播成新的产品名、板名、标题或用户可见字符串。
5. 重命名既有实现标识属于单独的代码迁移，必须同时评估构建选择链、CI、产物命名、上游同步和兼容性；文档口径治理不得顺手改代码标识。

## 不可破坏的边界

1. 活体 EDA 和封存复审决定硬件事实；批准版 PRD 决定产品行为；代码和旧文档不能反向覆盖二者。
2. `build success` 只属于软件 L0 证据，不证明外设、声学、功耗、掉电恢复或整机通过。
3. ESP32-C6 无 PSRAM；设计必须使用有界队列、流式处理和可测内存预算。
4. YaKa 产品差异通过 board、适配层和领域服务隔离；不要把产品逻辑散落到小智上游核心。
5. 文档更新同时维护状态、日期、证据、未决项和同步登记；不得用“已支持”描述尚无实板证据的能力。
6. 任何涉及硬件接口、制造或发布的变更，都沿着“需求→功能→器件/引脚→PCB→固件→制造→验证”追踪影响。

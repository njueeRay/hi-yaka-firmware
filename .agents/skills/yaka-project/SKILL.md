---
name: yaka-project
description: 维护 HiYaKa 的产品、硬件、固件、验证证据及飞书同步文档。处理 HiYaKa 需求、原理图/PCB/SMT、板级接口、小智上游同步、固件实现、bring-up、发布或项目文档治理时使用。
---

# HiYaKa 项目治理

先读 [00-governance-and-authority.md](references/00-governance-and-authority.md)，确定事实来源、文档职责和本次变更合同。不要把历史 YaKa 仓库、旧飞书材料或一次编译结果当成当前硬件/产品真值。

根据任务只加载相关 reference：

- 产品范围、交互、状态机或需求 ID：读 [10-product-and-prd.md](references/10-product-and-prd.md)，再按其中入口读取完整产品 PRD。
- 当前角色、Owner 决策、待确认事项或“谁来做什么”：读 [15-owner-decisions-and-todos.md](references/15-owner-decisions-and-todos.md)。
- 原理图、PCB、器件、引脚、总线、SMT、采购或首板：读 [20-hardware-and-manufacturing.md](references/20-hardware-and-manufacturing.md)。
- 固件架构、小智复用、ESP-IDF、分支或上游同步：读 [30-firmware-and-upstream.md](references/30-firmware-and-upstream.md)。
- 验证、日志、缺陷、资源预算、发布结论：读 [40-validation-and-evidence.md](references/40-validation-and-evidence.md)。
- 修改本地/飞书文档、附件或同步登记：读 [50-document-sync.md](references/50-document-sync.md)。

## 命名合同

1. 板子的对外名称和语义名称统一为 `HiYaKa`，归属组织为 `ChatChat`；需要同时表达归属时写“ChatChat HiYaKa”。不得添加代际或版本后缀，也不得派生其他大小写或分隔形式。
2. `HiYaKa` 同时是当前系列和当前板级语义；除不可变的历史文件名、第三方上游标识与外部版本引用外，产品、架构、测试、发布和用户文案不得再出现代际后缀。
3. 新命名按表示法固定：可读/驼峰形式用 `HiYaKa`；全小写短横线形式用 `hi-yaka`；全大写下划线形式用 `HI_YAKA`。除此之外不新增其他变体。
4. 代码与构建链统一使用：目录和 build ID 为 `chatchat/hi-yaka` / `hi-yaka`，类名为 `HiYaKaBoard`，宏为 `BOARD_TYPE_HI_YAKA`，文件名为 `hi_yaka_board.cc`。日志、测试、CI、图表和文档必须与这组标识一致。
5. 审计命名时同时检查内容和路径；不得仅修改标题而保留带代际后缀的目录、文件、类、宏、构建产物或日志字段。
6. Windows 本地机只用于 USB/串口、烧录、实板调试和日志采集；不得在 Windows 上编译固件或运行全仓测试。ESP-IDF 构建、host tests、合同检查和候选产物统一在 `aliyun_raynode` 或 GitHub Actions 的 Linux 环境执行并回读。

## 不可破坏的边界

1. 活体 EDA 和封存复审决定硬件事实；批准版 PRD 决定产品行为；代码和旧文档不能反向覆盖二者。
2. `build success` 只属于软件 L0 证据，不证明外设、声学、功耗、掉电恢复或整机通过。
3. ESP32-C6 无 PSRAM；设计必须使用有界队列、流式处理和可测内存预算。
4. HiYaKa 产品差异通过 board、适配层和领域服务隔离；不要把产品逻辑散落到小智上游核心。
5. 文档更新同时维护状态、日期、证据、未决项和同步登记；不得用“已支持”描述尚无实板证据的能力。
6. 任何涉及硬件接口、制造或发布的变更，都沿着“需求→功能→器件/引脚→PCB→固件→制造→验证”追踪影响。

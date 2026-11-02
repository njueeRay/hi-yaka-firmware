# HiYaKa 固件许可证范围

本仓库不是单一许可证工程。禁止商业使用只适用于 ChatChatTech 拥有或有权许可的 HiYaKa 增量成果，不覆盖小智或其他第三方已经通过 MIT 等许可证授予的权利。

## 1. ChatChatTech 非商业许可范围

下列路径中的 ChatChatTech 原创内容适用 [PolyForm Noncommercial License 1.0.0](LICENSES/PolyForm-Noncommercial-1.0.0.md)：

- `main/boards/chatchat/hi-yaka/`
- `main/yaka/`
- `config/hi-yaka-assurance.json`
- `partitions/hi-yaka-16m.csv`
- `scripts/hi_yaka_assurance.py`
- `scripts/hi_yaka_evidence.py`
- `scripts/tests/test_hi_yaka_*.py`
- `tests/yaka_host/`
- `docs/bringup/`
- `docs/governance/FIRMWARE_PRD.md`
- `docs/governance/EVIDENCE_POLICY.md`
- `docs/hardware/HARDWARE_MODULE_INTERFACES.md`
- `docs/yaka/`

此外，ChatChatTech 对 `main/application.cc`、`main/CMakeLists.txt`、`main/Kconfig.projbuild`、`main/ota.cc`、`main/ota.h`、构建工作流及其他混合文件所作的原创修改，也适用 PolyForm Noncommercial 1.0.0。原文件中来自小智或其他第三方的部分仍服从原许可证。

## 2. 上游与第三方范围

小智上游代码继续适用 [小智 MIT 许可证](LICENSES/MIT-xiaozhi-esp32.txt)。其他组件以各自目录中的许可证、版权声明和依赖元数据为准。本仓库的许可证变更不会撤销、缩小或替换任何第三方已经授予的权利。

## 3. 允许与禁止

- 允许：符合 PolyForm 条款的个人学习、研究、实验、非商业修改和非商业分发；
- 不授予：企业内部商业使用、收费交付、销售设备或固件、商业服务集成、以商业目的分发衍生版本；
- 商业使用：必须事先取得 ChatChatTech 的独立书面商业许可；
- 商标：任何许可证均不授予 `ChatChat`、`ChatChatTech` 或 `HiYaKa` 名称、标识和品牌的使用权。

`PolyForm Noncommercial` 属于 source-available（源代码可见）许可证，不是 OSI 定义的开源许可证。若本说明与完整许可证文本冲突，以完整许可证文本为准。

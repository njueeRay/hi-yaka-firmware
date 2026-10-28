# 上游同步约定

本仓库保留 `78/xiaozhi-esp32` 的完整 Git 历史。

```text
origin   https://github.com/njueeRay/hi-yaka-firmware.git
upstream https://github.com/78/xiaozhi-esp32.git
```

当前产品基线固定为 `v2.5.2` / `49b4380b3faa0ceadd28a3e78aaf0f311c46c57e`。不要直接跟随上游 `main`。

同步流程：

1. `git fetch upstream --tags`；
2. 从最新产品 `main` 创建 `sync/xiaozhi-vX.Y.Z`；
3. 合并目标上游 release tag，保留 merge commit；
4. 只处理上游差异和冲突，不混入新功能；
5. 运行 host tests、YaKa 板型构建和至少一个上游 ESP32-C6 代表板型构建；
6. 通过独立 PR 合入 `main`；
7. 在实板回归完成前，不发布 YaKa OTA。

YaKa 对上游核心的修改应尽可能收敛到少量显式接缝。板级引脚和硬件时序只存在于 `main/boards/chatchat/hi-yaka-v3`，不得修改其他板型来迁就 YaKa。

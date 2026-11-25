# 2026-11-25 云端构建目录与主仓迁移记录

本文只记录 `aliyun_raynode` 的构建存储维护；不作为 HiYaKa LCD 缺陷或固件验收证据。

## 首次复核状态（旧实验工作树迁移之前）

- `/data` 挂载自 `/dev/vdb1`，文件系统为 `ext4`，UUID 为 `02035c3b-ea36-4b9a-ad73-77ea885f21a8`；`/etc/fstab` 中为 `UUID=02035c3b-ea36-4b9a-ad73-77ea885f21a8 /data ext4 defaults,nofail 0 2`。
- 2026-11-25 复核时，`/data` 容量 40 GiB，已用约 989 MiB、可用约 38 GiB；根分区 `/` 显示 69 GiB 总量、约 1.3 GiB 可用。这里的数值是复核时快照，不代表此前空间变化的精确差值。
- 两份隔离构建目录现位于 `/data/hi-yaka-build/idle-previsit-fixed-A-20261125` 与 `/data/hi-yaka-build/idle-previsit-fixed-B-20261125`，`du -sh` 各约 495 MiB。主 Agent 记录为从根分区迁移；本次复核确认目的地存在与大小，未独立核迁移前目录大小或文件逐项哈希。

## 复核命令

在本机使用 SSH 对服务器执行只读命令：`findmnt -no SOURCE,FSTYPE,UUID,TARGET /data`、`df -h / /data`、`grep -F ' /data ' /etc/fstab`、`du -sh /data/hi-yaka-build/*`。上述输出与首次复核状态一致。以后清理构建目录前，应先核工作树引用和必要产物归档。

## 迁移前的路径约束

在首次复核及旧实验工作树迁移阶段，主仓仍保留在 `/srv/projects/hi-yaka-firmware`，**当时尚未移动**。当时提出的候选做法是把项目实体迁至 `/data/projects/hi-yaka-firmware`，从旧路径建立软链接以兼容脚本；后续实际执行情况见文末。

## 第一阶段：先迁旧实验工作树（2026-11-25）

新盘已按 UUID 写入 `/etc/fstab` 并挂载到 `/data`。复查发现正式项目路径为 `/srv/projects/hi-yaka-firmware`，该仓有未提交修改，因此第一阶段先不移动主仓，也不移动 `/srv/projects/esp-idf`。主仓随后迁移，见下节；原有 `/srv/projects/hi-yaka-firmware` 访问路径与 `/srv/projects` 目录结构均保留。

为给根分区留出构建余量，仅迁移一份未运行的旧实验工作树：实体从 `/home/ray/hi-yaka-mclk-dout-swap-20261123` 复制并校验至 `/data/projects/hi-yaka/hi-yaka-mclk-dout-swap-20261123`，原路径改成指向新实体的软链接。复制后执行 `rsync -aHnci --delete`，无差异再切换路径；切换后确认链接解析目标与目录可访问。没有迁移其他项目、ESP-IDF、NRF 镜像或正在运行的容器。曾临时创建的 `/home/ray/projects` 入口因不符合服务器现有 `/srv/projects` 规范，已撤销。

该阶段处理后 `df -h` 显示：根分区约 2.4 GiB 可用（97% 已用），`/data` 约 36 GiB 可用（8% 已用）。这是两次独立命令的显示值，不用于精确计算迁移释放字节数。后续又迁移了主仓，见下节。

## 主仓迁移完成后的状态（2026-11-25）

主 Agent 记录：先对 `/srv/projects/hi-yaka-firmware` 执行 `rsync -aH` 到 `/data/projects/hi-yaka-firmware`，再以 `rsync -aHnci --delete` 空差异作复制核验；切换前后 Git HEAD 均为 `1358b05`，`git status --porcelain` 内容的 SHA 相同，原备份路径已删除。当前 `/srv/projects/hi-yaka-firmware` 是指向 `/data/projects/hi-yaka-firmware` 的软链接，旧路径仍可访问；`git worktree list` 显示数据盘实体路径。**本次复核独立确认了软链接目标、HEAD、worktree 列表和磁盘快照；rsync 的原始输出、porcelain SHA 与备份删除只依据主 Agent 当时记录，未在此重新执行迁移。**

当前 `df -h / /data` 显示根盘 `69G/62G/3.6G/95%`，数据盘 `40G/4.0G/35G/11%`。`/srv/projects/esp-idf` 仍为原目录；主 Agent 记录 Docker 与 NRF 未动。首次约 1.3 GiB、中间约 2.4 GiB、当前约 3.6 GiB 是三个时间点的可用空间快照，不应相减当作精确迁移字节数。后续新构建继续使用 `/data/hi-yaka-build`；引用主仓的脚本可继续使用 `/srv/projects/hi-yaka-firmware`，但需要实体路径判断的工具应识别当前软链接。

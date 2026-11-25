# HiYaKa NFC 主机接口 A/B 诊断程序

这个独立 ESP-IDF 程序只用于已拆除 L7 的受控开发样机。启动时立即将 GPIO23/PA_EN 设为低电平，不启用屏幕、音频、TF 或 NFC 射频场。它先在 GPIO10/11 上检查五个预期 I²C 地址，并向 PN532 的 7 位地址 `0x24` 发送原始 `GetFirmwareVersion` 帧；释放 I²C 后，将 GPIO11 接为 UART TX、GPIO10 接为 UART RX，以 115200 baud 发送 PN532 HSU 唤醒字节和同一命令，记录收到的原始字节。

构建环境为 Linux 上的 ESP-IDF v6.1；Windows 仅负责串口、烧录和日志采集。诊断程序并非正常产品固件，不能作为 NFC 功能验收依据。实验前确认目标板 L7 已拆、温度正常，并保存当前应用镜像身份；实验后烧回原应用、校验写入哈希和启动日志。

2026-11-05 首次实板结果见 [NFC 接口 A/B 记录](../../docs/bringup/2026-11-05-nfc-interface-ab-results.md)。本程序不自动判断 PN532 根因；HSU 收不到数据不能证明芯片一定运行在 I²C 模式。

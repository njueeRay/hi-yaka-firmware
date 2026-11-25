# PN532 HSU 临时改线诊断

仅用于已将 `SET1:SET0` 实物改为 `00`、L7 已拆的 HiYaKa 样机。UART1 TX=GPIO11、RX=GPIO10；不启用屏幕、音频功放或其他外设。**没有 I²C 预扫描。**启动后把 PN532/C106 LowVbat 唤醒前缀和第一条 `SAMConfiguration`（normal mode）作为连续字节流发送，再读取版本并进行 12 次 Type A 寻卡。串口是主控 USB/CH340K 的 115200 baud；PN532 HSU 也使用 115200 baud，但为独立 UART。

此程序不构成正式固件。每次烧录前记录镜像 SHA 和原应用镜像，实验后恢复。`rx_bytes` 和原始十六进制帧用于审计；版本应答不等于实际读卡通过。原始方案参见 `diagnostics/nfc-interface-ab/`。HSU 模式下 PN532 的 TX 与板上共享 I²C SDA 重用同一条线；**不能把带屏产品固件与当前 `00` 绑带混用**。若下一版使用 I²C，C106 物理绑带应为 `I0=SET0=1、I1=SET1=0`，还需独立完成 I²C 实板验证。

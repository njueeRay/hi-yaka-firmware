# 20 硬件与制造

## 当前硬件基线

- ESP32-C6、16 MiB PY25Q128HA Flash、无 PSRAM；
- ST7789 200×320 非触控 LCD；
- ES7210 双麦采集及播放参考、ES8311 + HT8691R 播放；
- TF/SDSPI、PN532 I²C、QMI8658A、PCA9557；
- GPIO10/11 共享 I²C，首板 100 kHz；地址 `0x18/0x19/0x24/0x41/0x6A`；
- MASTER_SW 硬切整机电源，掉电恢复属于正常产品场景；
- MCU 只能从 BAT ADC 估算电压，不能宣称可观测完整充电终态。

## 当前 P0 与开发隔离边界（2026-11-04）

- 两块样机已在 U11/HT8691R BOOST 支路复现快速异常发热；根因证据保持 `FAIL`，不得以拆件后的短时不发热覆盖；
- GPIO23/`PA_EN` 在 ESP32-C6 复位期间默认弱上拉，可在应用固件运行前开启 HT8691R BOOST；
- L7 实装 `CMI160808J220KT / C361669` 为 mA 级 0603 叠层信号电感，不满足 HT8691R 的 DCR 与峰值电流要求；
- 当前开发样机已拆除 L7，随后观察窗口内不再快速升温；这只隔离了主要异常能量通路，不等于 U11/BOOST 根因关闭或恢复扬声器能力；
- 本地权威复盘入口：`docs/hardware/HIYAKA_U11_THERMAL_INCIDENT_AND_RECOVERY.md`；
- 当前样机可在操作者在场、温度持续监测、扬声器断开和 PA 软件硬禁用时受控继续非功放 G1/M1；操作者离场时保持 MASTER OFF；扬声器播放和下一版制造仍为 `NO-GO`。

## 权威附件

优先读 `docs/hardware/attachments/README.md` 中标记为 authority 的文件。原理图、PCB、SMT/采购材料分别解决不同问题，不能用某一份报告替代全部门禁。

## 固件接口变更

任何 pin、总线、地址、极性、电源时序、器件后缀、DNP 或装配变更都必须同步：board config、驱动初始化、健康检查、制造附件、bring-up 步骤和实板证据。

PCA9557 必须先写输出锁存 `0x40`，再写方向 `0x38`，最后读回。LCD、PA、RGB、马达上电保持安全关闭。

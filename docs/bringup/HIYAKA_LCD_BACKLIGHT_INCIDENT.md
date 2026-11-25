# YAKA-0004｜HiYaKa 显示屏无背光与显示稳定性

> 问题编号：`YAKA-0004`（历史内部别名：`HIYAKA-LCD-INC-001`）<br>
> 级别：`P1`<br>
> 首次确认：2026-11-04<br>
> 最新复核：2026-11-25（同板固定身份 GPIO A/B 与 PCA9557 第一性原理交叉审计）<br>
> 影响范围：HiYaKa R1.6 原开发样机及换入样机<br>
> 当前状态：一块样机飞线后背光已亮，低速 GPIO 横屏候选版曾显示颜色与文字，方向正常且原雪花带消失；同一镜像再次烧录后首次只有背光，复位后才重新出现颜色或文字。I²C 控制通道仍大量重试，整项显示验收仍为 `FAIL`。飞线具体焊点和阻值尚未记录，不能据此认定全部样机已修复

> **2026-11-23 当前源勘误：**本专项主体记录R1.6旧板“LCD_BL无上拉”的事故与飞线补救；当前YaKa v3源已有上拉。审查期间曾误用旧`H0160S002 V1`资料，该资料不适用于现行屏。当前LCD1 MPN=`ZJY-LBS147TC-IG01`、supplierId为空，`C9900058759`仅为商城候选；[本地原厂PDF](../../../../../artifacts/lcd-1p47-source/ZJY147TC-IG01.pdf)证实ST7789V3、172×320、pin9=SCL、pin10=SDA。PCB为GPIO6→pad9、GPIO7→pad10，物理连接正确；当前板级配置仍MOSI=GPIO6、SCLK=GPIO7、200×320，实际已烧录镜像未知。该差异是固件移交提醒、非PCB连接P0；PCB无需重走线。旧`VIEW_GAP_Y=20`不可继承，RAM居中34仅为候选，须以实板边框/四色条冻结。导出BOM前另须绑定并核对精确C号。

## 1. 结论与业务影响

- **旧板结论：**R1.6 屏幕无背光的直接根因已经确认：PCA9557 的 IO0 只能下拉或释放，但旧板 `LCD_BL` 没有上拉，导致 Q10 始终无法得到有效高电平。当前源已补上拉。
- **影响：**画面链已证明可工作，横屏方向和边带问题已在当次实屏消除；通信稳定性仍不满足显示、菜单及整机交互验收。
- **阶段决策：**显示功能与整机发布 `NO-GO`；其它模块 `CONDITIONAL GO`。
- **下一步：**待恢复实板测试、且能安全断电操作时，先固定并记录 PN532 的模式绑带；以已显示画面的 GPIO 路径定位和修复 PCA9557 重试及 LCD 刷新失败，再完成同板冷启动、持续刷新和异常恢复验收。只有 GPIO 路径稳定后实测界面速度仍不达标，才评估 PARLIO 或下一版硬件总线；当前不要求立即返修 PN532。

“原板完全无背光”的直接根因置信度：**高**。飞线后该板背光已亮；低速 GPIO 模式下画面链和 FPC 数据路径已取得实屏证据，置信度**高**。PARLIO 模式为何未显示、PCA9557 地址与 I²C 为什么大量重试仍**未知**，不能把横屏当次成功提升为整项显示验收通过。

### M2 屏幕实板验收计划（草案，尚未执行）

以下次数与时长是**测试方案建议，不是既定 PRD 门槛**；M2 仍未勾选。c0 旧 GPIO 画面用于故障定位，旧日志的 `200×320` 传输不能替代目标屏 `ZJY-LBS147TC-IG01 / ST7789V3 / 172×320` 的 L2 验收。

1. **锁定样机与版本：**在任何测试前记录 run ID、板卡 MAC/修订、实装屏型号与方向、飞线/返修及 PN532 模式绑带、固件 SHA 和 ESP-IDF 版本。每轮保持同板同镜像；冷启动须记录完全断电、重新上电与观察时间。
2. **检查用户所见：**依次核启动、首页、状态、错误画面；检查色阶、四边边框、横向方向、低电亮度和持续刷新，确认无裁切、雪花或明显撕裂，并核首帧出现后才开背光。屏幕故障时还要看到明确降级，核心功能不被卡死。
3. **建议复测与判据：**同板同 SHA 至少 5 次独立冷启动（现行烧录模板最低 3 次）、至少 30 分钟持续刷新、一次经安全审查的受控软件异常与恢复。建议正常流程和恢复流程的 PCA 终态失败及 LCD 绘制失败均为 0；目视画面正确是必要条件。另报告刷新耗时 p95/p99、最差连续 10 秒的首试失败情况和最长连续失败串；即使重试掩盖终态错误，只要首试失败持续显著出现，也不得签署“I²C 稳定”。首试失败率的正式阈值须在界面刷新目标确定后另行冻结，不在本草案擅定。
4. **双轨留证：**每轮保存带时间戳的原始串口日志及 SHA-256，并拍摄能对应同一时间轴的屏幕照片或视频；记录用户目视、错误计数、异常恢复前后状态及未通过项。API 返回成功不能替代画面证据。用户休息期间不烧录、不复位、不做目视测试。

## 2. 现象

两块样机接入 LCD FPC 后，屏幕均与关机时一样黑，无微光。固件能够访问 PCA9557 地址 `0x19`，背光寄存器写入后也可读回，但首块样机 R85 两端均约 0 V，Q10 栅极未被拉高，R84 两端压差接近零，说明背光支路没有形成正常电流。

换入样机同样无背光，并出现 LCD 传输超时和后续 LVGL 看门狗；这些属于仍需区分的软件运行或显示数据链现象，不能解释上电即无背光。

2026-11-12，用户报告一块样机按临时补救方案飞线后屏幕背光亮起，但初版未见文字或屏幕纯色；最初误把板上 RGB 灯当作屏幕五色，随后已纠正。ESP-IDF 6.1 回查还发现旧五色实验位于 `DISPON` 指令之前，故旧五色不可见不具备隔离数据链的判别力。该样机的飞线位置、阻值和背光电流尚无实测记录。

同日补做提前发送 `DISPON` 的 PARLIO 实板实验：C118 实测 3.3 V，背光亮、温度正常；PCA9557 输入端口读回在空闲态为 RST=1、CS=1、DC=0、BL=1，五次纯色 API 均成功，但用户目视确认 LCD 仍无画面。低速 GPIO 版完整烧录后，用户提供的照片**确实显示颜色和配网文字**，因此 FPC/面板数据链至少在该模式下可工作。照片最初显示画面竖置、文字裁切和长边雪花条；改为 320×200 横屏并试用 20 像素 RAM 偏移后，用户确认文字方向正常、雪花带消失。GPIO 版的 PCA9557 输出寄存器写入频繁失败；最新 48 秒启动日志中有 **343 条重试警告**，多次重试耗尽，末尾输入端口读回也失败；LVGL 接管后另有 **12 次**像素发送失败。C19（U3 旁供电）实测 3.3 V；EDA 核对 R8=10 kΩ 连接 U3 地址脚 A0，用户实测 R8.1 固定 3.3 V、R8.2 在 0 与 3.3 V 间变化。若测量稳定且落点正确，这是一项地址绑带异常，与 `0x19` 偶发无应答相容，但具体硬件根因仍未隔离。**画面方向和边带当次通过，显示稳定性仍 FAIL。**详见[飞线后 LCD 与模块复测](2026-11-12-lcd-flywire-and-module-retest.md)。

同一块板完成麦克风 I²C 对照后，已将相同横屏镜像烧回并通过设备端哈希校验。首次复位，用户目视**仅背光、无任何颜色或文字**；串口虽记录五次纯色 API `ESP_OK`，仍有 305 条 PCA9557 写重试、13 次耗尽、12 次像素发送失败。保持同镜像同接线再复位一次，用户观察到颜色或文字，温度正常；随后确认仍显示、文字沿长边横向排列、无雪花带。第二次日志仍有 318 条重试、19 次耗尽、15 次像素发送失败。画面具有明显间歇性，不能将一次成功提升为稳定验收。无屏幕、且跳过启动 I²C 扫描的音频对照仍发生 PCA9557 读写失败，说明屏幕流量与扫描均不是失联的必要条件；PN532 SET0/SET1 当前悬空，R8/A0 也尚未定因。详见[共享 I²C 同板对照证据](2026-11-12-pca-audio-shared-i2c-ab-evidence.md)。

随后独立最小程序只操作共享 I²C，`0x19` 地址探测全部应答，PCA9557 输出寄存器安全值写入也 128/128 成功，但连续单独寄存器读多数返回 `ESP_ERR_INVALID_RESPONSE`；结果依赖事务类型与前序操作。它削弱了“PCA9557 地址始终丢失或芯片彻底损坏”的解释。恢复同一横屏镜像后，串口再次记录 298 条写重试、13 次耗尽和 12 次像素发送失败；用户却确认**横向画面正常、温度正常，R8 靠 U3/A0 一端稳定 3.3 V**。当前状态下，A0 漂移的优先级因此降低，但总线波形及其它硬件条件尚未排除。画面内容与日志错误需要分开验收。

**返回码口径勘误：**本专项中的普通 `i2c_master_transmit`/寄存器读返回 `ESP_ERR_INVALID_RESPONSE`，只证明该事务没有以 `I2C_STATUS_DONE` 结束，不能单凭返回码断言器件在地址或数据阶段真实 NACK。[ESP-IDF v6.1 同步事务源码](https://github.com/espressif/esp-idf/blob/v6.1/components/esp_driver_i2c/i2c_master.c#L681-L685)对非 DONE 状态统一返回该码；中断路径还可能涉及 NACK、超时或仲裁丢失。已单列的 `i2c_master_probe` 的 `ESP_ERR_NOT_FOUND`/`ESP_ERR_TIMEOUT` 仍按原始探测结果记录。下文所有普通写入失败次数均指**事务未完成**，物理 ACK/NACK 与具体失败阶段待波形或更细的底层状态验证；旧日志中的原文不据此改写，也不能从本勘误推定 PCA 芯片或 PCB 根因。

2026-11-23 带屏 Lab 新日志进一步收窄因果：开机约 181 ms 已出现 PCA9557 输出寄存器 `0x01` 写入失败，LCD 传输到约 281 ms 才初始化，因此 **LCD 不是最早这笔写入失败的必要条件**。但 LCD 的 CS/DC 控制每次变化会通过 `SetLcdControl → WriteOutputLocked` 写同一寄存器；活跃显示阶段的日志有每秒约 80 条重试警告，说明显示路径可能显著放大既有总线/事务问题，**不能把高频写入本身定为物理根因**。日志：工作区 `.tmp/hi-yaka-c4-main-lab-runtime-boot1-20261123.log`，SHA-256 `438CD62A5B969CC57080F308291DB92E2E7CC560585F45042918B8C577BC246D`；代码路径 `main/boards/chatchat/hi-yaka/hi_yaka_expander_lcd_io.cc`、`hi_yaka_pca9557.cc`。下一判别应保持同板、同镜像其余条件不变，比较有/无 LCD 控制写的事务总数、首失时点与恢复情况；PCA9557/I²C 错误首发根因仍未知。

**同一 c4 样机后续带屏/无屏对照：**带屏 Lab 的约 32.3 秒日志中，PCA9557 `0x01` 写重试 1,644 条、耗尽 20 次，TF 已挂载。无屏自动诊断的两次启动日志分别约 31.3 秒和 21.3 秒，均仅 6 条写重试、1 次耗尽；两次 TF 都报 `ESP_ERR_TIMEOUT`，`AUTOTEST_GATE tf=0 allowed=0`。两类镜像的运行路径与日志窗口不同，这**不是严格控制 LCD 单变量的故障率实验**；但结合 LCD 的 CS/DC 每次改变均会写 `0x01`，可判显示控制负载显著增加写入及重试次数。最初约 181 ms 的写失败在 LCD 初始化前已发生，首发根因仍未知。TF 在 c4 带屏镜像曾挂载、无屏两次超时，提示间歇挂载风险；**两次超时当时的插卡状态未独立核实**，不能凭这三次记录确定 TF 器件故障或归因显示配置。原始日志：工作区 `.tmp/hi-yaka-c4-main-lab-runtime-boot1-20261123.log`（SHA-256 见上）、`.tmp/hi-yaka-c4-headless-autorecord-boot1-20261123.log`（`4C6A5F52E9F00451D3CE16E164C8298C3FB0229E7B5F49207D7566852ED9DC12`）、`.tmp/hi-yaka-c4-headless-autorecord-boot2-20261123.log`（`EB9D28F40C209DCF734013796E9DA72D776E7588AD40E8E70F25837E78750B03`）。

**2026-11-23 c0 样机的新对照，不能与上段 c4 混算：**本板 Wi-Fi STA MAC 后缀 `…d6:c0`。用户确认本轮最初没有插 TF 卡，因此该时点的挂载超时只作无卡基线；插卡后，旧无屏镜像三次暖复位均挂载 TF（3/3），同一修复版无屏镜像六次复位也均挂载 TF（6/6）。这 9 次仅证明 **c0 在已插卡、这些镜像与启动窗口内可挂载**，不证明 TF 对坏卡、掉电或长期运行可靠；上段 c4 的插卡状态尚未独立核对，其超时风险仍待查。

同一 c0 插卡后，原始带屏 PARLIO 镜像在 PCA9557 写失败、LCD 刷新失败之后于约 11.24 秒在 `lcd_display.cc:117` 的 `ESP_ERROR_CHECK(ESP_ERR_INVALID_RESPONSE)` 中止，**发生于 TF 初始化前，故本次 TF 未测**。用户目视该 PARLIO 显示只有背光。容错带屏 PARLIO 镜像已挂载 TF 并进入 `BRINGUP READY`，但本次容错分支并未实际触发；运行期间仍有 PCA9557 写失败与 LCD 刷新失败，用户仍只看到背光。慢速 GPIO 全功能镜像的有效第二次启动同样挂载 TF、进入 `BRINGUP READY`，仍记录 PCA9557 写失败和刷新失败；**屏幕视觉结果尚待用户反馈**，不可将串口 `ESP_OK` 或进入 READY 写成画面通过。三类镜像之间不仅 LCD 传输模式不同，容错路径和运行时间也不同，因此只提供阶段判别，不构成严格单变量故障率实验。

**25 kHz 共享 I²C 对照仍为阴性：**同一 c0 板的慢速 GPIO 全功能镜像将共享 I²C 总线设为 25 kHz，TF 于约 33.12 秒挂载、约 39.94 秒进入 `BRINGUP READY`；至该日志结束，PCA9557 `0x01` 写失败记录 461 条、重试耗尽 50 次，LCD 刷新仍报错。44.28–44.40 秒的原始日志还直接出现 IDF `clear bus failed`、`I2C hardware timeout detected`、`reset hardware failed`，伴随 PCA 的 `ESP_ERR_INVALID_STATE`/`ESP_ERR_INVALID_RESPONSE` 和 LCD 绘制失败；**这只证明该晚期窗口包含硬件超时及恢复失败，不解释此前首笔或全部 `INVALID_RESPONSE`**。随后读到 SDA/SCL 为高电平，也不能倒推故障瞬间总线健康。**降速没有消除当前失败**；该窗口与前述镜像的运行时长和实际事务量不同，不能直接比较绝对失败条数来判断故障率高低。用户对这版画面的视觉反馈仍待确认。源码提交 `1944c10`，应用镜像 SHA-256 `E46972BE5C7AF6806622FCA572F0061ED20ABB3686FE5CA9C6C425FA87717E1B`；日志 `.tmp/hi-yaka-c0-lcd-gpio-i2c25-boot1-20261123.log:727-739`，SHA-256 `2EAE4E26116702756CD6E0A84F49BBB1D27AB3BBA031E1528DF0CCA54C264BB8`。

实验结束后已将 c0 恢复为先前同修复版无屏镜像（SHA-256 `9702027529E8A1BCE6F63DA4F6BE8C6821B169F907A65F5CA74E8D0C2F70C622`）并完成烧录校验；恢复启动日志于约 690 ms 挂载 TF、7.49 秒进入 `BRINGUP READY`。这只证明恢复后的基础启动，不补足带屏画面验收。日志 `.tmp/hi-yaka-c0-restored-headless-20261123.log`，SHA-256 `34AC0D3745EE2A7C5BB735703A8FDC59232D7B09C80F408D886CE0DFF927CCE5`。

**同一 c0 样机的显示传输对照（用户目视）：**历史低速 GPIO 横屏镜像连续三次启动，用户均看见画面（3/3）；该旧应用镜像 SHA-256 为 `4D436FF4867AD641ECE6ACCAFD642F719561378E76FFE1C9EC5CC6823FFF287A`。新全功能慢速 GPIO 镜像两次启动也均看见画面（2/2），应用镜像 SHA-256 `ACE56A4D9B035568A53F87379A72B22FBD1E7859E7D80C0108BFD3546A51250C`，源码 `33e3469` 加 profile `367fe12`。**同一源码 `33e3469` 的 10 MHz PARLIO 镜像**两次启动，用户均确认 LCD 只有背光、没有画面（0/2）；应用镜像 SHA-256 `9B3CE08D9B0F02E8900C5EDF3D94D3AFA10C37EAE6029A4DB47312B09B79D609`。PARLIO 日志中的绘制 API 成功不能替代显示面板的目视结果。这支持“当前 10 MHz PARLIO 路径有独立显示缺口”，但尚不能区分 PARLIO 时钟/时序、数据路由、面板时序裕量或 PCA9557 控制失败的具体贡献；两组仅各两次，不宣称长期可靠率。此前 25 kHz I²C 对照只说明降速未消除 PCA 错误，也未使 10 MHz PARLIO 画面通过。1 MHz PARLIO 后续实测见下段。

**1 MHz PARLIO 后续单因素：**上述候选镜像 SHA-256 `9C82F3BBE66BEC6755FA45DB8F8B074B421B309DBB483C95C775CBE97C7EC10C` 已烧录并校验；用户目视仍只有背光，温度正常。TF 于约 16.04 秒挂载、约 22.87 秒进入 READY，PCA9557 写和 LCD 刷新仍报错。将 PARLIO 时钟由 10 MHz 降至 1 MHz **未使本次画面恢复**，不能由此单独确定控制、数据或电气根因。日志 `.tmp/hi-yaka-c0-parlio-1mhz-boot1-20261123.log`，SHA-256 `C3E3D515F5B73467332870DC687424AE5FDC57535996D34E0247C8E9C813B203`。

**PARLIO 时钟窗口直接计数：**c0 同板 PCNT 诊断镜像 SHA-256 `A72BB28ECD8EE35818A77E274111EE25D81F581BF2CCD4CC97104A158092248B` 完整烧录校验。发送 `CASET` 命令 `0x2A` 时，在 CS 已拉低、正式传输前的 20 μs 空窗，主控 GPIO7/SCLK 已记录 27 个上升沿；正式命令窗口记录 46 个上升沿，而单字节命令预期为 8 个。MOSI 高位计数 3，与 `0x2A` 的高位数量一致。故本次在**主控 pad 测量点**高置信确认有额外时钟，CS 与时钟/数据边界不符合预期；这为 PARLIO 仅背光提供直接的驱动时序排查方向。用户仍只见背光、温度正常。`RASET` `0x2B` 因 PCA9557 I²C 错误导致传输失败，计数为 `-1`，**没有有效计数**。FPC/屏幕端波形未实测，具体错误时序来源、修正效果及长期可靠性均未验证。日志 `.tmp/hi-yaka-c0-parlio-pcnt-boot1-20261123.log:70`，SHA-256 `06EA2C76E3D4F2A7BCA14E2E041E86F2D59E9D083CBAAB0F785F065482751281`。

**ESP-IDF 6.1 官方能力边界与下一步：**[C6 `soc_caps.h`](https://github.com/espressif/esp-idf/blob/v6.1/components/soc/esp32c6/include/soc/soc_caps.h)未定义 `SOC_PARLIO_TX_CLK_SUPPORT_GATING`；[PARLIO TX 公共 API](https://github.com/espressif/esp-idf/blob/v6.1/components/esp_driver_parlio/src/parlio_tx.c#L2265-L2276)在此条件下对 `clk_gate_en=1` 返回 `ESP_ERR_NOT_SUPPORTED`。底层寄存器存在相关位，不代表该芯片在当前 SDK 受公共 API 支持。因此停止继续排列公共 API PARLIO 门控/频率参数，优先在已能显示的 GPIO 路径测可达到的刷新速度、CPU/内存占用、TF 并行运行与连续显示稳定性；只有 GPIO 路径达不到产品需求时，才评估经独立审查的 LL 方案或下一版硬件。**这是后续实验路线，尚未证明 GPIO 路径满足产品指标，M2 仍未验收。**

**GPIO 提速候选的首轮性能记录：**同一 c0 板烧录并校验应用镜像 SHA-256 `09E9B9A9C60B508C6E9CF17B3AB4E88007E730E6CC43E39BA40D892AD6C77BD1`；五色传输每色均为 `ESP_OK`、200 行、128000 字节，首次启动各色 `draw_us` 依次为 2027596、2026210、2027810、2010214、2035829 μs。与旧 GPIO 候选约 4.25 秒/色的日志计时相比，候选传输约快 2.1 倍；第二次复位同样记录五色计时，TF 挂载且到 READY。**用户已确认 boot1 实屏完整显示五色和横向文字，无明显雪花或缺字，温度正常**；boot2 仅有串口日志、无独立目视；boot3 用户再次确认五色和横向文字完整、无明显雪花或缺字、温度正常，日志 TF 挂载并到 READY。因此目视证据是 boot1 与 boot3 两次，不能把 boot2 的 API 成功算作第三次视觉通过。三个日志中的五个 `draw_us` 数值逐项完全相同；不同采集时间、ROM 启动字节与后段 ADC 读数说明不是文件复制，但早段数值相同的原因未定，不能据此算三次独立性能方差，另见主机旁路计时；PCA9557 重试及 LCD 刷新失败仍在，也未测试完整 UI 帧率、CPU/内存成本和持续稳定性。仅将此列为性能候选，不判产品时序合格或 M2 通过。日志 `.tmp/hi-yaka-c0-gpio-fast-boot{1,2,3}-20261123.log`，SHA-256 分别为 `9339F11EAD124C9DA1464A06372130EE3A5883D59711EBB6A750F92D7C1D85C9`、`4DC8B95F3771FF75833DB077BF4462E2B7474DDAA72D93E36110ABEE001C11A1`、`27CFF0A9F572A6D71C6276C942BB9BD78DF7DFDA4370D0C9669DF9D0981D4F3F`。

**主机旁路计时复核：**同镜像第四次启动使用电脑接收串口行的到达时间独立计时。五色完成行于主机计时 2703、6218、9765、13281、16812 ms 到达；相邻间隔 3515、3547、3516、3531 ms。扣除源码中每色后固定 1500 ms 等待，得到约 2015、2047、2016、2031 ms，与设备报告的约 2.01–2.04 秒/色吻合，支持**本轮全屏传输约 2 秒**，排除将快速日志重放误读成新性能结果。boot4 只有串口数据，未取得用户独立目视。此前 boot1–3 设备计时逐项相同仍不用于计算三次独立性能方差。日志 `.tmp/hi-yaka-c0-gpio-fast-host-timed-boot4-20261123.log`，SHA-256 `D9221C6E92C7FF5924C6B1330130E74C73EDF542184AC6E154DDBAB69FC638A3`。PCA9557/LCD 错误与 M2 验收边界不变。

**下一轮单变量排查基线与判据：**固定 c0 样机、GPIO fast 诊断镜像 SHA-256 `09E9B9A9C60B508C6E9CF17B3AB4E88007E730E6CC43E39BA40D892AD6C77BD1` 及相同显示负载，目标是区分 PCA9557 `0x01` 写入首试事务失败（返回 `ESP_ERR_INVALID_RESPONSE`）、重试耗尽和 LCD/LVGL 刷新失败。boot1 按设备 uptime 分段：0–20 秒 `retry 2/3` 首试失败记录 197、耗尽 1、刷新失败 0；20–40 秒分别为 250、20、17。另一段同镜像晚期旁路日志覆盖 uptime 约 639.88–699.83 秒，60 秒中相应为 601、0、0。晚期和启动阶段负载不同，不能拿绝对次数直接算改善率；首试失败仍未消失。对照时先核同板、同镜像、同启动阶段、显示事务量和用户可见画面，再比较首失时点、每类失败数、TF/READY、温度与持续刷新。**随后已完成内部上拉关/开的单变量 A/B，结果见下段。**这些记录和判据用于收缩问题，M2 仍须画面、控制链和稳定性分别验收。原始日志仅本地 `.tmp/hi-yaka-c0-gpio-fast-boot1-20261123.log`（SHA-256 `9339F11EAD124C9DA1464A06372130EE3A5883D59711EBB6A750F92D7C1D85C9`）及 `.tmp/hi-yaka-c0-fast-gpio-live-60s-20261123.log`（SHA-256 `A37149CBC7ADBA68935D5868E4C7FFC8CF3EA84F3BD29C72DAB5168D58C4C330`）。

**内部上拉单变量 A/B 结果：**同一 c0 板，保持 GPIO fast 显示路径和五色测试，仅将 ESP32-C6 I²C 主机内部上拉从关改为开。上拉关闭基线镜像 SHA-256 `09E9B9A9C60B508C6E9CF17B3AB4E88007E730E6CC43E39BA40D892AD6C77BD1`；上拉开启镜像 SHA-256 `E821A689EA56FC3543D3499663EA26674C21AEDEAB15052BB5E5CCCF13E7A5A2`，已烧录哈希校验。按同一设备 uptime 窗口计数：0–20 秒首试失败/重试耗尽，关为 `197/1`、开为 `209/1`，两版刷新失败均为 0；20–40 秒首试失败/重试耗尽/刷新失败，关为 `250/20/17`、开为 `244/22/19`。两版五色各传 128000 字节，单色日志耗时都约 2.0 秒。**开启内部上拉没有消除返回 `ESP_ERR_INVALID_RESPONSE` 的事务失败、重试耗尽或刷新失败**，本轮不支持将它视为修复；这也不能证明外部 R11/R12 缺失、异常或完好，硬件上拉状态须另核。ON 版日志 `.tmp/hi-yaka-c0-gpio-pullup-boot1-20261123.log`，SHA-256 `0DBC8D9ECFBDD6EAF8A42EEA47179B901E49A275A451541E8ED36402996F238C`。用户已对 ON 版作一次目视确认：画面完整显示、温度正常；这只覆盖该次可见性，控制链错误与持续稳定性未关闭，M2 仍未验收。

**2026-11-24 PCA9557 安全定时诊断：**目标是只操作安全的输出锁存 `0x01=0x40` 与方向 `0x03=0x38`，避免误开启功放，再观察相同设备在不同事务间隔下的写入。首版每组只尝试一笔，五次 `SAFE_INIT` 的首笔均返回 `ESP_ERR_INVALID_RESPONSE`，其节奏与后续试验不同，故停止该版、不能作间隔结论。修正版镜像 SHA-256 `1F773409D4C09D223456FD0F41DF07E166A3F744D688867B2669F95FC863171F`：初次 `0x01` 写失败，约 390 μs 后第二笔成功；`0x01/0x03` 两个安全值均写入并读回验证。随后按 12 组、每组 128 次记录写后立即读：正向早段 0/1/5 ms 的写失败分别为 `1/128`、`18/128`、`64/128`；正反序的 20/100/200 ms 共六组均 `128/128` 写失败；反向后段 5/1/0 ms 写失败分别为 `2/128`、`36/128`、`52/128`，其中后段 5 ms 读失败 `43/128`。**不能简单归因于“间隔越长越差”**：每笔写后立即读混杂事务顺序和设备状态，约 146 μs 的短时 `ESP_OK` 读与下一笔写失败高度相关，方向性和电气/主机根因仍未定；随后已做读写顺序互换 A/B，结果见下段。全部定时事务在日志中标记完成后，逐条打印 1536 笔结果时出现 task WDT；采集阶段未记录重启，WDT 不是前述失败计数的解释。修正版日志 `.tmp/hi-yaka-c0-pca-safe-timing-retry-boot1-20261124.log`，SHA-256 `DA504059E7AF189402657842D054BF18C26B03D36D3E57A52201B137240619E5`；首版日志 `.tmp/hi-yaka-c0-pca-safe-timing-boot1-20261124.log`，SHA-256 `6EC8CEB3459927EF4F8DCB6E070BE44ED6EF61D317E4D17FF2A4CC6970000E3F`。M2 仍开放。

**读写反序 A/B：**隔离提交 `cc2d0f41` 只把安全定时矩阵每轮从“写 `0x01`→读 `0x01`”改为“读 `0x01`→写 `0x01`”；镜像 SHA-256 `A8547C7A314C9079B4558C1E00B521C63371A03D63D6AB160DFD78D2EB213AF9`。正反序的 20/100/200 ms 六组，每组 128 次：**首读均 128/128 失败，而后写均 0/128 失败**。与前轮相同长空档下首写 128/128 失败、后读多成功的结果交叉，强烈支持“所测 PCA9557 访问序列中，长空档后的首笔 PCA 事务失败”，而不是单纯“写比读更易失败”；置信度高。后续 QMI8658A 同总线对照表明这不是共享 I²C 总线上普遍的首事务失败。该现象仍不能区分 PCA 本地访问路径、主控控制器/ESP-IDF 状态、总线电气及器件行为，不能直接判 PCA9557 芯片损坏或归为官方 erratum。短间隔组存在混合结果，不据此宣称精确阈值。报告阶段每 16 行让出 CPU，本轮未见 task WDT；最后附加的安全读 `read01` 仍返回 `ESP_ERR_INVALID_RESPONSE`，所以不能宣称实验结束时两个寄存器均读回通过。原始日志 `.tmp/hi-yaka-c0-pca-safe-timing-read-first-boot1-20261124.log`，SHA-256 `050FC6C9AAAD8972EBBB927DA6BF6D439300E19CD8B81E9C6B67D24D8B4D4480`。M2 仍开放。

前轮诊断后已将 GPIO fast 基线镜像 SHA-256 `09E9B9A9C60B508C6E9CF17B3AB4E88007E730E6CC43E39BA40D892AD6C77BD1` 以 no-stub、115200 烧回 c0 的 OTA0，esptool 哈希校验通过；恢复日志约 21.63 秒 TF 挂载、约 28.52 秒进入 READY。用户目视确认画面完整、温度正常。此为读写反序前的恢复记录。日志 `.tmp/hi-yaka-c0-gpio-fast-restored-boot1-20261124.log`，SHA-256 `D876C9EDAE29C46CCDE162CD9E3F70F61FBFF018E3E494FCE0E610A7CCE510F5`。

**读写反序后的基线恢复：**同一 c0 板将 GPIO fast 镜像 SHA-256 `09E9B9A9C60B508C6E9CF17B3AB4E88007E730E6CC43E39BA40D892AD6C77BD1` 再次以 no-stub、115200 写入 OTA0（`0x20000`），烧录输出 `Hash of data verified`。恢复日志记录五色各 200 行、128000 字节，约 2.0 秒/色，并进入 `BRINGUP READY`；用户再次目视确认完整显示、温度正常。同一日志仍有 PCA 重试 499 次、耗尽 24 次、LCD 绘制失败 20 次，因此这只证明实验后已恢复可见基线，**不表示 PCA 控制链或 M2 已通过**。日志 `.tmp/hi-yaka-c0-gpio-fast-restored-after-read-first-boot1-20261124.log`，SHA-256 `4BD7ADD0826147B15CD39A4DDF250A8BF036C1AF87FCB74F4CC3A801411A4EEB`。

**前置总线复位的安全写 A/B：**隔离提交 `1f2b9bbc`，镜像 SHA-256 `F949843FAB33E94325A5E82246B6FE952CF7E6CD774C6288009DFBC77ACD7E30`。同一 c0 板仅对安全锁存值做写入，以 20 ms 为目标间隔，按 A（不复位）→B（写前 `bus_reset`）→B→A 各做 64 次；四组写入均 **64/64 返回 `ESP_ERR_INVALID_RESPONSE`**，两组 B 的 `bus_reset` 均返回成功（`reset_fail=0`），复位本身约 87–101 μs。第一组最短实际空档约 11.7 ms，其余约 19.8 ms，故不把每笔都写成精确 20 ms。实验末尾安全值 `0x01=0x40`、`0x03=0x38` 均写入并读回，`PA_EN=0`。结论限于：**此受控流程的写前 `bus_reset` 没有改善失败**；它不能单独判定 ESP-IDF 控制器状态、总线电气或 PCA9557 器件根因。当前仍是诊断镜像，GPIO fast 恢复及目视待另记；M2 继续开放。日志 `.tmp/hi-yaka-c0-pca-reset-abba-boot1-20261124.log`，SHA-256 `EE63220AD7EDCE934AA33FD8CD5ADE88C235EEAABD204B575DFEF9FD9771A3F0`。

**PCA9557 与 QMI8658A 同总线首访顺序对照：**同一 c0 板的独立安全诊断镜像 SHA-256 `F807E3B6C212D80282737BFB6215F2AF1D6FE9D64F049F577B921E0FC5AA5BE1`，以 20 ms 为目标空档，按 PCA 首访→QMI 首访→QMI 首访→PCA 首访的 ABBA 顺序各做 64 次。PCA 首访两组均 **64/64 返回 `ESP_ERR_INVALID_RESPONSE`**，随后 QMI 的 WHOAMI 均成功读为 `0x05`；QMI 首访两组均 **0/64 失败**，随后 PCA 安全写均成功。失败的 PCA 首笔约 146 μs，QMI 读约 450 μs，作为第二笔的 PCA 写约 330 μs。首组最短实际空档约 11.7 ms，其余约 19.2–19.4 ms，不能写成每笔都精确 20 ms。实验末尾安全锁存 `0x01=0x40` 与方向 `0x03=0x38` 双写并读回验证，`PA_EN=0`。这组证据**否定“共享 I²C 总线长空档后任何首事务都会失败”的泛化**，把问题收缩到 PCA9557 的访问或本地事务特性；仍不能判定芯片损坏、PCB 局部电气或主控驱动的具体根因。下一步应保持同板、相同间隔和安全值，分别核 PCA 访问的时序/地址及 SDA/SCL 实际波形，避免继续用仅返回码推断真实 NACK。该诊断结束时板上仍为实验镜像；后续已烧回 GPIO fast，记录见下段。M2 仍开放。日志 `.tmp/hi-yaka-c0-pca-qmi-abba-boot1-20261124.log`，SHA-256 `4B2567102D7341500765BA2C6A14DA89FCD3C5664FF984BD4C2EB6E661CEBCD7`。

**PCA/QMI 对照后的 GPIO 基线恢复：**同一 c0 板已将 GPIO fast 基线镜像 SHA-256 `09E9B9A9C60B508C6E9CF17B3AB4E88007E730E6CC43E39BA40D892AD6C77BD1` 以 no-stub、115200 写回 OTA0（2,678,112 字节），烧录输出 `Hash of data verified`。恢复后约 45 秒串口日志记录五色传输均 `ESP_OK`，进入 `BRINGUP READY`，但同一日志有 PCA9557 `0x01` 首试失败 516 次、重试耗尽 19 次、LCD 绘制失败 17 次，错误返回 `ESP_ERR_INVALID_RESPONSE`。**用户已确认本次实屏完整显示、温度正常；这证明可见基线已恢复，控制链错误仍在。**恢复日志 `.tmp/hi-yaka-c0-restored-fast-gpio-20261124.log`，SHA-256 `18EF53D84F89F4413076F0213315D957FE48DF310A66C42C5E2B0693791AA519`。M2 仍开放。

**PCA9557 首访矩阵：**c0 同板、独立安全诊断镜像 SHA-256 `257970CF65689F8E79328277C6486812B73A1ECC8F42154791BC97881F846E1E`，以 20 ms 为目标空档完成九相、每相 32 轮。下表“失败”只指该相首访事务；每轮随后读取 PCA9557 安全锁存 `0x01=0x40` 和方向 `0x03=0x38`，均各 32/32 得到正确值。

| 首访事务 | 相数 | 首访结果 | 随后验证 |
|---|---:|---|---|
| `probe(0x19)` | 2 | 各 32/32 `ESP_ERR_NOT_FOUND` | PCA 安全读回均正确 |
| `probe(0x6B)` | 2 | 各 0/32 失败 | QMI8658A WHOAMI 均为 `0x05`，PCA 安全读回正确 |
| `probe(0x18)` | 1 | 32/32 `ESP_ERR_NOT_FOUND` | PCA 安全读回正确；`0x18` 也属于 ES8311，不能据此判 PCA9557 A0 电平或实际地址 |
| PCA 直接接收 | 2 | 各 32/32 返回 `ESP_ERR_INVALID_RESPONSE` | PCA 安全读回正确 |
| PCA 合并事务读 `0x03` | 2 | 各 32/32 返回 `ESP_ERR_INVALID_RESPONSE` | PCA 安全读回正确 |

**结论边界：**PCA 首访在本矩阵中持续异常，而同一总线 QMI 首访可成功；随后的 PCA 真实寄存器值又能读回。这让“PCA9557 空闲后首访或其地址/事务路径异常”具有**高置信度**，并进一步排除“共享总线任意器件首访都会失败”的说法。`probe(0x19)` 的 `NOT_FOUND` 和普通访问的 `INVALID_RESPONSE` 是不同 API 返回码，不能合并解释为已实测物理 NACK；`0x18` 亦不能用来定 A0。具体根因仍未知，不能宣称 PCA9557 芯片损坏。下一步保持同板、安全值与时间窗口，核 `0x19` 地址来源和 PCA 首访时序；如有逻辑分析仪，再对照 SDA/SCL 的真实应答。末尾安全寄存器读回 `verified=1`、`PA_EN=0`。本矩阵结束时板上为诊断镜像；随后已恢复 GPIO fast 并目视确认，记录见下段。M2 开放。日志 `.tmp/hi-yaka-c0-pca-first-access-boot1-20261124.log`，SHA-256 `299F00B6C983AC518CAD4E1771829C204EE2ED0449761061BF43AA34F28E74C9`。

**首访矩阵后的 GPIO 基线恢复：**同一 c0 板将 GPIO fast 基线镜像 SHA-256 `09E9B9A9C60B508C6E9CF17B3AB4E88007E730E6CC43E39BA40D892AD6C77BD1` 再次写回 OTA0（2,678,112 字节），烧录日志明确 `Hash of data verified`。约 35 秒启动日志中，五色各完成 200 行、128000 字节传输并返回 `ESP_OK`，随后进入 `BRINGUP READY`；**用户已再次确认实屏完整显示、温度正常**。同一日志仍有 PCA 首试失败 420 次、重试耗尽 21 次、ST7789 绘制失败 17 次；可见性恢复不等于 PCA 控制链修复。烧录日志 `.tmp/hi-yaka-c0-post-first-access-restore-20261124-flash.log`，SHA-256 `A0B34619965BA5A277613BF049A02CB8CAB9CA86E63654E2D14B5F1D8F71CE51`；启动日志 `.tmp/hi-yaka-c0-post-first-access-restore-boot1-20261124.log`，SHA-256 `78FAEE82BB25258317B94D35BFC4394B9F483231110679D277C246D4D532570B`。M2 仍未验收。

**全功能路径的 QMI 预访 A/B：**c0 同板 GPIO fast 全功能镜像 SHA-256 `0A0FB166BB9C9D7772C8C818F9171659E9C6B7E49B6799635A6434C303F5C914` 连续做两次独立软件复位。每次在同形 PCA9557 控制写前按 A（不预访 QMI）8 次→B（先读 QMI8658A 身份）8 次→B 8 次→A 8 次；两轮汇总 A 的 PCA 首写 **32/32 返回 `ESP_ERR_INVALID_RESPONSE`**，B 为 **0/32 首写失败**，B 的 QMI 身份读 **32/32 为 `0x05`**。A 的 PCA 首次失败约 143 μs，但均经第二次尝试成功；B 的 QMI 预读约 446–485 μs，随后的 PCA 首写约 327 μs 且一次成功。每次诊断收齐四相后停止介入正常控制链。**这证明在所测时间窗和全功能路径中，先进行一次有效 QMI 总线事务与 PCA 首写成功强相关；还不能判定 QMI 特异作用、单纯事务预热、额外延时或底层状态哪一项才是因果条件。**停止介入后两轮仍出现 PCA 写错误，各有 17 次 ST7789 绘制失败；不能把该预访标为完整修复或 M2 通过。这两次记录时板上仍为实验镜像；后续已恢复原 GPIO fast 基线，证据见下段。boot1/boot2 日志 `.tmp/hi-yaka-c0-qmi-previsit-abba-boot{1,2}-20261124.log`，SHA-256 分别为 `BF4AC9631D5919A853A7F44C7D17EE3B20BE327C3B90DC557B2FF2B739BFFB71`、`74A86DC299FE150A0AB8F6C128B55C868962B8831E7F6A4A76E1BBBF5F322BB2`。

**QMI 预访 A/B 后的基线恢复与时序边界：**同一 c0 板已将原 GPIO fast 镜像 SHA-256 `09E9B9A9C60B508C6E9CF17B3AB4E88007E730E6CC43E39BA40D892AD6C77BD1` 写回 OTA0（2,678,112 字节），烧录日志显示 `Hash of data verified`。约 35 秒启动日志五色测试 5/5 返回 `ESP_OK`，进入 `BRINGUP READY`，且无 `QMI_PREVISIT_PHASE`，证明该次运行未继续启用预访诊断。日志仍有 PCA 首试失败 421 次、重试耗尽 21 次、ST7789 `draw_bitmap` 失败 19 次。**本次用户已下线，没有新的实屏目视或温度反馈；先前基线曾可见且温度正常，不等于本次已复核。**闪存日志 `.tmp/hi-yaka-c0-post-qmi-abba-restore-20261124-flash.log`，SHA-256 `67350384F8BDEF4ACE64EC2C3D18FBF0820A6B7D621A4A8F2712D57A37141504`；启动日志 `.tmp/hi-yaka-c0-post-qmi-abba-restore-boot1-20261124.log`，SHA-256 `6C22DAF566F5ECCA504C9834FF8E6AD984C60559F9D6CD892082F790F74991C4`。

同次启动 PCA `0x01` 首次失败/耗尽在约 0.19 秒已出现，早于约 20.84 秒启动的 LVGL 任务，因此 **LVGL 不是 PCA 首次失败的必要条件**。约 20.88 秒出现连续重试耗尽并伴随绘制失败，但这只构成时间关联；ES8311 等更晚启动，也不是首错的前置条件。源码 `hi_yaka_pca9557.cc` 的重试间隔使用 `vTaskDelay(pdMS_TO_TICKS(1))`；若运行配置为 100 Hz，则 1 ms 会换算为 0 tick，可能只让出执行机会。LVGL 负载使重试更难成功是**待验证假说**，不能从日志直接判为根因。下一步应在同板同镜像下独立记录 LVGL 前后 PCA 访问间隔、首试失败、耗尽与实际事务量，必要时再测总线波形。M2 仍开放。

**LVGL 阶段的被动 PCA 轨迹：**c0 同板镜像 SHA-256 `C282D8D664C056FE42BECBCA3349897F48414D1B1DB200D1B9065B5EA99E83EA` 在设备运行时间 20.5–23 秒收集 114 条轨迹，丢失 0 条；其中首试失败 30、三次重试后仍失败 16。典型 `seq=63–72` 均来自 `taskLVGL`，PCA 输出锁存从 `C3` 写为 `03`（释放 CS、命令 D/C 为低）时，三次尝试均返回 `ESP_ERR_INVALID_RESPONSE`；相邻尝试间隔多为约 6.7–8.9 ms 和约 7.7 ms，失败调用本身多为约 150–250 μs，远长于此前短间隔诊断中出现成功的时间尺度。`seq=63` 首次调用较长，故不能把所有失败调用一律记为 150–250 μs。**QMI 预访 ABBA 只覆盖了同形像素块结束、CS 释放的 `next=C3` 写入；没有验证这里的 `C3→03` 命令类终态失败会被预访修复。**本轨迹因此收紧了“QMI 预访有效”的适用范围，不构成完整 LCD 修复。

轨迹采集本身的同步输出以及 `vTaskDelay(0)` 都可能拉长后续尝试间隔；当前无法分辨是负载、调度、PCA 访问时序还是电气因素导致失败。下一步需在相同命令路径上做单因素延时/预访对照，并核日志采集对时序的扰动；不能据这份被动轨迹判定根因。原始日志 `.tmp/hi-yaka-c0-pca-passive-trace-boot1-20261124.log`，SHA-256 `BFC2B9A98D597908A141691816138209160C8FE854C4F4FCF8D664F75DAD8FA8`。诊断后原基线已恢复，见下段；M2 仍开放。

**被动轨迹实验后的基线恢复：**c0 已重新烧入原 GPIO fast 镜像（SHA-256 `09E9B9A9C60B508C6E9CF17B3AB4E88007E730E6CC43E39BA40D892AD6C77BD1`），烧录回报 `Hash of data verified`。重启日志记录红、绿、蓝、白、黑五色传输均 `ESP_OK`，各 200 行、128000 字节，并进入 `BRINGUP READY`；未见被动轨迹诊断输出。同一启动窗口仍有 PCA 首次事务失败 402 次、写重试耗尽 18 次、ST7789 绘制错误 16 次。用户本轮离线，**没有新的实屏目视或温度确认**，不能将此前基线的目视结论搬用到这次启动。烧录日志 `.tmp/hi-yaka-c0-post-passive-restore-20261124-flash.log`，SHA-256 `CC51A2A614EA281CE65913927758505623C2FB585D2DBCAFE39223558FA51F8F`；启动日志 `.tmp/hi-yaka-c0-post-passive-restore-boot1-20261124.log`，SHA-256 `252ABD18FFE782405FB4A5F2BD427BB15DB5BEAF5802D1A061011D520E600D1B`。M2 仍开放。

**失败警告延后输出的单因素对照：**在同一 c0 样机上，诊断镜像 SHA-256 `E2734B670135F1A0D6B7751B5F0127129790E29B8A429864137B52F0716FD5E2` 仅将每次失败时的 `ESP_LOGW` 从重试间移到逻辑写操作结束后；三次 I²C 尝试和 `vTaskDelay(0)` 保持不变。20.5–23 秒的原被动轨迹窗口记录 114 条、首试失败 30、三次耗尽 16；延后警告版两次独立启动分别记录 160 条（各丢弃 5 条）、首试失败 18/23、耗尽均为 0。典型重试间隔由原约 6–8 毫秒缩至约 6–7 微秒。两次启动五色均 5/5 `ESP_OK`、各 200 行和 128000 字节；约 36 秒日志窗内 boot1 的全局写耗尽/绘制失败为 0/0，boot2 为 1/1（发生于 26.9 秒，窗口外）。这为**同步警告输出放大三次耗尽**提供强证据；首访事务异常仍在，且后段仍出现失败，不能判 PCA 根因已解决或 M2 通过。两版轨迹记录数不同，不能把首败计数当作严格错误率。原始日志仅本地：`.tmp/hi-yaka-c0-pca-deferred-log-boot1-20261124.log`，SHA-256 `8054BBF0B9348A304C250364F7A753E9BC491031EAAADCA14DD6B0DCF11F9C5F`；`.tmp/hi-yaka-c0-pca-deferred-log-boot2-20261124.log`，SHA-256 `364A4D04B2AB3B1757E10F770F2836C73AAC91E4B3F4E9D0605FF172090F13F2`。实验后原 GPIO fast 基线已恢复，见下段。

**延后警告实验后的基线恢复：**c0 已烧回原 GPIO fast 镜像（SHA-256 `09E9B9A9C60B508C6E9CF17B3AB4E88007E730E6CC43E39BA40D892AD6C77BD1`，2,678,112 字节），烧录记录 `Hash of data verified`。约 34 秒启动日志记录五色各 200 行、128000 字节且均 `ESP_OK`，进入 `BRINGUP READY`，未见 `PCA_PASSIVE_SUMMARY`，表明本轮日志来自原基线而非诊断镜像；同窗仍有 PCA 写耗尽 20 次、ST7789 绘制错误 18 次。用户本轮离线，**没有新的画面目视或温度确认**；M2 仍开放。烧录日志 `.tmp/hi-yaka-c0-post-deferred-log-restore-20261124-flash.log`，SHA-256 `5F091B6EB95AC482726D887D1E68064E612BDF091A0774A15123C14A211AFFA7`；启动日志 `.tmp/hi-yaka-c0-post-deferred-log-restore-boot1-20261124.log`，SHA-256 `8B4017C7A7021378FF7BED6847655A2C0F4F4D1EBC3A5408267A1C113D916431`。

**静默 PCA 计数候选的两次长窗：**同一 c0 样机烧入诊断镜像 SHA-256 `A5A1D4B0BC315D3AD7293E9D98326FC09C5F9A8622EE66FB628F46422461645A`，两次各观察约 125 秒。约 120 秒累计：boot1 共 8202 次写，首试失败 1248 次、第二次恢复 1247 次、第三次恢复 0 次、耗尽 1 次；boot2 分别为 8211、1237、1236、0、1 次。两次均只有 1 次 ST7789 绘制失败，分别发生于约 23.79/23.73 秒，均早于 `WifiManager: Initializing` 约 10 毫秒，WiFi driver task 还要更晚；因此不能以 WiFi 初始化触发解释这两次失败。23.5 秒前的摘要均无耗尽，30 秒摘要均已有 1 次。`C3→03` 命令类转换耗尽均为 0，终态失败发生在 LCD 的其他 `reg 0x01` 转换。两次五色传输均 `ESP_OK`。与此前原基线约 34 秒窗口的耗尽 20 次、绘制失败 18 次相比，**终态失败显著减少，但未归零**；不同镜像、日志窗口和计数方式不支持精确错误率比较。这两次终态失败发生在 WiFi 初始化之前，WiFi 初始化不是其必要前置事件；其他上游状态仍待核。用户休息，本轮没有新的实屏目视或温度确认；M2 仍开放。日志仅本地：`.tmp/hi-yaka-c0-pca-summary-boot1-20261124.log`，SHA-256 `ED7964A6ED0A70FE4F70F6EB54ADE156A4B40DFF15623EB3C6FE7165AB4C28B9`；`.tmp/hi-yaka-c0-pca-summary-boot2-20261124.log`，SHA-256 `F136A8582CC030F6A8F091B6C7A93E89C8FEF3A9464A4A30AFB239F0BBFBE72F`。该诊断后原快 GPIO 基线已恢复，见下段。

**静默摘要实验后的基线恢复：**c0 已重新烧回原 GPIO fast 镜像（SHA-256 `09E9B9A9C60B508C6E9CF17B3AB4E88007E730E6CC43E39BA40D892AD6C77BD1`，2,678,112 字节），烧录 `Hash of data verified`。约 34 秒启动日志记录五色各 200 行、128000 字节且均 `ESP_OK`，进入 `BRINGUP READY`，未见 `PCA_WRITE_SUMMARY`，确认不再运行摘要诊断镜像；同窗 PCA 写耗尽 20 次、ST7789 绘制错误 18 次。用户本轮离线，**无新的屏幕目视或温度确认**。烧录日志 `.tmp/hi-yaka-c0-post-summary-restore-20261124-flash.log`，SHA-256 `71CB2900454437268244FDA1D620FD0564BA9FFE7EF0BB0F7B2D6CD4C7B52B1E`；启动日志 `.tmp/hi-yaka-c0-post-summary-restore-boot1-20261124.log`，SHA-256 `A76B2EB19940B268682A002736D0592BC2C61584598DD2CDF835E93B63FD3873`。M2 仍开放。

**23.55–24.2 秒 quiet trace 对照：**同一 c0 样机，镜像 SHA-256 `E360E933865CA64E701D8C06DCCA30BC2AD68EE206AD5E4A82CA9B7EC58BDDE1`，三次独立启动的窗口均采到 17 笔、丢弃 0 笔。30 秒摘要的总调用数均为 2692，首试失败分别 330、332、329 次，第二次均恢复，PCA 终态失败均为 0；没有复现无轨迹静默摘要版约 23.7 秒的 1 次终态失败。轨迹中约 23.768 秒首次 `C3→03` 写首笔失败，约 996 微秒后重试成功；后续一些首笔耗时 1–4 毫秒，也有较短重试。`prev` 字段仅记录前一个 PCA 操作，不能用于推断其他总线设备。轻量插桩可能改变访问时序，不能据这三轮零终态判修复。boot2 在 30.34 秒还有一条 `draw_bitmap(229): io tx color failed`，与 30 秒后集中打印时间邻近，但该轮 PCA 终态计数仍为 0；不能把它算作约 23.7 秒的同类 PCA 错误，也不能证明确由打印引起。原始日志仅本地：`.tmp/hi-yaka-c0-pca-quiet-trace-boot1-20261124.log`，SHA-256 `644142DFB6C12C6C2B4984D60ACE1754593545EB511814C73F62AFFB2624A663`；boot2 同目录日志 SHA-256 `77588EAB9B67AD59560ECCF9C568B391FD1C0C4E4E18446B9E64222D37FB4860`；boot3 同目录日志 SHA-256 `4103546FD2C8F7B61BC74CA56FC3663B137EAB695C63189B1E8A2B0D73963367`。实验后原 GPIO fast 基线已恢复，见下段；M2 仍开放。

**quiet trace 实验后的基线恢复：**c0 已重新烧回原 GPIO fast 镜像（SHA-256 `09E9B9A9C60B508C6E9CF17B3AB4E88007E730E6CC43E39BA40D892AD6C77BD1`，2,678,112 字节），烧录 `Hash of data verified`。约 34 秒启动日志五色各 200 行、128000 字节且均 `ESP_OK`，进入 `BRINGUP READY`，未见 quiet trace 输出；同窗 PCA 写耗尽 17 次、ST7789 绘制错误 15 次。用户本轮离线，**无新的实屏目视或温度确认**；M2 仍开放。烧录日志 `.tmp/hi-yaka-c0-post-quiet-trace-restore-20261124-flash.log`，SHA-256 `8B7D2D34A4704725FA06E6361022C48EF20E5A7E303F42F598ED892124A4AF03`；启动日志 `.tmp/hi-yaka-c0-post-quiet-trace-restore-boot1-20261124.log`，SHA-256 `D58BB5686CF9585AA0D9FAF13B13D465F75E4013EF6CFC7FA287390EF5422D28`。

**仅 LCD 路径的第三次重试前 2 tick 回退：**同一 c0 样机，镜像 SHA-256 `B60E64F7037783FF79F26E844967BEF5A464819DE3E49011E74FCC5EFE873349`，两次各观察约 125 秒。仅当 LCD 写在第二次尝试后仍失败，第三次尝试前等待 2 tick；motor、bit、safe init 路径未加等待。约 120 秒累计，boot1 调用 8215 次、首试失败 1221 次、第二次失败及回退各 2 次、第二次恢复 1219 次、第三次恢复 0、耗尽 2 次；boot2 依次为 8205、1244、2、2、1242、0、2 次。两轮各有 2 次 LCD 绘制失败，分别约在 23.80/23.86 秒与 23.82/23.88 秒；两轮五色均 `ESP_OK`，进入 READY。此前未插桩静默摘要版两轮各耗尽 1、绘制失败 1；当前两轮均未恢复且失败次数更高，但样本不足以断言回退普遍使情况恶化。**2 tick 回退不能作为修复。**用户本轮离线，未取得新的实屏目视或温度确认；M2 仍开放。日志仅本地：`.tmp/hi-yaka-c0-pca-third-backoff-boot1-20261124.log`，SHA-256 `9946531DD2ED99395E223A8A5112AC1A218EEC9E75467FA7E75D9FA513D9647F`；boot2 同目录日志 SHA-256 `67E06476647E6D041436000BBBE2B1E2CEBD07C1AC4133957A5F045C024385CF`。实验后原 GPIO fast 基线已恢复，见下段。

**2 tick 回退实验后的基线恢复：**c0 已重新烧回原 GPIO fast 镜像（SHA-256 `09E9B9A9C60B508C6E9CF17B3AB4E88007E730E6CC43E39BA40D892AD6C77BD1`，2,678,112 字节），烧录 `Hash of data verified`。约 34 秒启动日志五色各 200 行、128000 字节且均 `ESP_OK`，进入 `BRINGUP READY`，无回退版计数摘要；同窗 PCA 写耗尽 19 次、ST7789 绘制错误 17 次。用户本轮离线，**无新的实屏目视或温度确认**；M2 仍开放。烧录日志 `.tmp/hi-yaka-c0-post-third-backoff-restore-20261124-flash.log`，SHA-256 `B32C3E2F630B17E30654FC43C2E4E567676938013A9AC846C2E31B9CB8B848F3`；启动日志 `.tmp/hi-yaka-c0-post-third-backoff-restore-boot1-20261124.log`，SHA-256 `845065F4B48B3C667191142EFB60E03F612624D14F45E1F2E88FDFB8D42444AF`。

**两次失败后的第三笔前探测：未触发。**同一 c0 样机，镜像 SHA-256 `847D4F8C4BCD1CB5839D17C64016285795D9AB70452D1C17223EF07400017ECA`，两次各观察约 125 秒。该候选只在前两次 PCA 写均失败后，第三次尝试前探测 `0x19`。约 120 秒累计，boot1 写 8208 次、首试失败 1218 次、第二次失败 0、探测调用 0、第二次恢复 1218 次、耗尽 0；boot2 依次为 8200、1194、0、0、1194、0。两轮均无绘制失败，五色均 `ESP_OK` 并进入 READY。**两次都没有进入探测分支**，因此零终态不能归因于 `0x19` 探测；不同镜像代码或时序变化亦可能影响结果。本轮不能验证该候选是否有效，不能宣称修复。用户离线，未取得新的实屏目视或温度确认；M2 仍开放。原始日志仅本地：`.tmp/hi-yaka-c0-pca-probe-before-third-boot1-20261124.log`，SHA-256 `A1C9F3D2E141CA7C4DEB48BC1459CF13D235F2740050EA64950EBAB849C9A3EA`；boot2 同目录日志 SHA-256 `A2B2C71BCB2E2D2215492FCDAD8D1B73F0B525E35D538DE12EE1433D042EC8ED`。实验后原 GPIO fast 基线已恢复，见下段。

**0x19 探测候选实验后的基线恢复：**c0 已重新烧回原 GPIO fast 镜像（SHA-256 `09E9B9A9C60B508C6E9CF17B3AB4E88007E730E6CC43E39BA40D892AD6C77BD1`，2,678,112 字节），烧录 `Hash of data verified`。约 34 秒启动日志五色各 200 行、128000 字节且均 `ESP_OK`，进入 `BRINGUP READY`，未见探测候选的计数摘要；同窗 PCA 写耗尽 21 次、ST7789 绘制错误 18 次。用户本轮离线，**无新的实屏目视或温度确认**；M2 仍开放。烧录日志 `.tmp/hi-yaka-c0-post-probe-third-restore-20261124-flash.log`，SHA-256 `DF42FB0F8AFC9961BEB0E662AB7F5C205218084332137DEF79B090457300C620`；启动日志 `.tmp/hi-yaka-c0-post-probe-third-restore-boot1-20261124.log`，SHA-256 `CE2217286C2906956AD04326DD09295A883500C0DDE62C3672D6BBA9860A942C`。

**同地址空闲首访 ABBA：**同一 c0 样机，镜像 SHA-256 `AE75160C9722DBAA14EFC733DEA92B841F6BDEC6580BF28340CF485DF615EE90`，两次独立启动各按 A8→B8→B8→A8 执行 32 次，均在运行 20 秒前完成，PCA 空闲约 196 毫秒。A 直接写：每轮 16/16 首笔失败、第二笔恢复；B 先向同地址 `0x19` 发起一次探测再写：每轮 0/16 首笔失败，最终写入全部成功。**探测本身两轮各 0/16 `ESP_OK`**，各 7 次 `ESP_ERR_NOT_FOUND`、9 次 `ESP_ERR_TIMEOUT`，耗时约 135–261 微秒；其失败不等于 PCA 健康。结果支持“一次额外地址事务后，所测首写失败消失”，但不能区分总线活动、经过时间或控制器状态变化；下一步须做等时纯延迟对照，不能直接采用探测作为修复。相同镜像两轮约 23.58 秒仍各出现一次 PCA 写终态失败与一次绘制失败，故不能外推 LCD 稳定。用户离线，无本轮实屏目视或温度确认；M2 仍开放。日志仅本地：`.tmp/hi-yaka-c0-pca-idle-probe-abba-boot1-20261124.log`，SHA-256 `85CCFEE519EEC0B87115636B7D79FE7C9CA80B70E6CEBAFCD9178938A02BD7E9`；boot2 同目录日志 SHA-256 `A75BEB9DCD83171445BD71DFB824781F0596B3A1741B94595CE731EE6C723F07`。实验后原 GPIO fast 基线已恢复，见下段。

**同地址空闲 ABBA 后的基线恢复：**c0 已重新烧回原 GPIO fast 镜像（SHA-256 `09E9B9A9C60B508C6E9CF17B3AB4E88007E730E6CC43E39BA40D892AD6C77BD1`，2,678,112 字节），烧录 `Hash of data verified`。约 34 秒启动日志五色各 200 行、128000 字节且均 `ESP_OK`，进入 `BRINGUP READY`，未见 `PCA_IDLE_PROBE_DONE`；同窗 PCA 写耗尽 20 次、ST7789 绘制错误 18 次。用户本轮离线，**无新的实屏目视或温度确认**；M2 仍开放。烧录日志 `.tmp/hi-yaka-c0-post-idle-probe-restore-20261124-flash.log`，SHA-256 `D26BC4D2F23755B8AA25AB62BC541F2E3458DF807E22CCC0D4541F056EEA138D`；启动日志 `.tmp/hi-yaka-c0-post-idle-probe-restore-boot1-20261124.log`，SHA-256 `CB3AD3DA6471A2C4408B5870CAA3C1D6EC6DD47D05D5862BC864099262982885`。

**250 微秒纯延迟对照（2026-11-24）：**仍用 c0 同一板，隔离提交 `8950506`，诊断镜像 SHA-256 `8672C47EB64488E8F4972D14F1B0921ACE4AA8A354FE380C4AF17FF3FA7F0D66`；OTA0 烧录记录 `Hash of data verified`。两次独立启动均执行 A8→B8→B8→A8，A 直接写，B 在写前仅等待约 250 微秒，不产生前置 I²C 事务；B 实测等待 251–252 微秒。两次的四相均各 8/8 首写返回失败，合计 A、B 各 16/16 首写失败，随后重试成功。启动 1 后段有 1 次 LCD 绘制终态失败，启动 2 未见；这不是屏幕稳定验收。与上轮“先 probe `0x19`”的 B 相 0/16 首写失败对照，**同量级纯等待没有复制探测后的改善**。额外地址事务可能影响总线或器件状态，但两者未直接测量，根因仍未确定，探测也未成为产品修复。用户离线，无本轮实屏或温度确认。日志仅本地：`.tmp/hi-yaka-c0-pca-idle-delay-abba-boot1-20261124.log`（SHA-256 `27DABE399D68F015BDF824F5D8B4E46607764C3966814FCC1F8E1033FA16EAFD`）、`.tmp/hi-yaka-c0-pca-idle-delay-abba-boot2-20261124.log`（SHA-256 `C2517441942D60CC76C48C7483D34FA5B0E9BFE66B9D55692AFCAB1EE8A9FCA1`）；烧录日志 `.tmp/hi-yaka-c0-pca-idle-delay-abba-20261124-flash.log`（SHA-256 `69B08BBEC36A1FC69CE59C0DF78A68DCCB7A436C9D81B21957650FCB8395F8CA`）；诊断后原 GPIO fast 基线已恢复，见下段；M2 仍开放。

**纯延迟对照后的基线恢复：**同一 c0 样机已重新烧回原 GPIO fast 镜像（SHA-256 `09E9B9A9C60B508C6E9CF17B3AB4E88007E730E6CC43E39BA40D892AD6C77BD1`，2,678,112 字节），烧录记录 `Hash of data verified`。约 35 秒启动日志中红、绿、蓝、白、黑五次整屏传输均返回 `ESP_OK`（各 200 行、128000 字节），另有背光设置 `ESP_OK`，并进入 `BRINGUP READY`；未见纯延迟诊断标记。同窗仍有 PCA 写入第三次重试失败 19 次、ST7789 `draw_bitmap` 失败 17 次。用户本轮离线，**无新的实屏目视或温度确认**；M2 未验收。烧录日志 `.tmp/hi-yaka-c0-post-idle-delay-restore-20261124-flash.log`，SHA-256 `9F402DC5022349B3BCB6159A1E2A87A7D6C44C240AC4193A86249BC06433EA24`；启动日志 `.tmp/hi-yaka-c0-post-idle-delay-restore-boot1-20261124.log`，SHA-256 `ECEA2FD9C774F75B49894E7AD9A9EC9885A4A5564E9F102DB5FC35BE8B3A222E`。

**探测地址 A/B（2026-11-24）：**同一 c0 样机，隔离提交 `a33e3cc`，镜像 SHA-256 `3ADF1F1BF859F9FA89E3C2E599E313E73612F74F0DEF2D61AC8C5AFFFB6D3F48`，OTA0 烧录 `Hash of data verified`。两次独立启动均做 A8→B8→B8→A8：A 在 PCA 写前探测 `0x19`，B 改为探测空地址 `0x55`，其余同形。两次各 A 16/16、B 16/16 的后续 PCA 首写成功。**两种探测自身均无一次 `ESP_OK`**：每次启动 `0x19` 为 16/16 `ESP_ERR_NOT_FOUND`；`0x55` 为 15/16 `ESP_ERR_NOT_FOUND`、1/16 `ESP_ERR_TIMEOUT`。不能把“探测完成”写成“地址收到 ACK”。两次约 40 秒启动日志仍分别有 PCA 写终态失败 2/1 次、LCD 绘制失败 2/1 次，前述首写恢复不等于运行稳定。结合“250 微秒纯延迟 B 仍 16/16 首写失败”，PCA 自身地址 `0x19` 不是这项局部改善的必要条件，更像探测共有的总线事务或控制器行为；未直接测量电气波形，**物理根因仍未知**，不将空地址探测当作产品修复。用户离线，本轮无实屏或温度确认；M2 仍未验收。原始日志仅本地：`.tmp/hi-yaka-c0-pca-probe-address-abba-boot1-20261124.log`（SHA-256 `79B5793E2E46CF3709791AB30D39356CF629C724A31FC5E54F3E7C0779E6D71A`）、`boot2`（SHA-256 `BE7D38B57F3C83CCB4F82B9ACABBF1D8135AD3FBC9FD9E4E9742D434CE0CE69D`）；烧录日志 `.tmp/hi-yaka-c0-pca-probe-address-abba-20261124-flash.log`（SHA-256 `607516AC17B92672598931A117296E5DE98D86F13026B747052094A8F37F79FC`）。诊断后的原 GPIO fast 基线已恢复，见下段；M2 仍开放。

**地址探测 A/B 后的基线恢复：**同一 c0 样机已烧回原 GPIO fast 镜像（SHA-256 `09E9B9A9C60B508C6E9CF17B3AB4E88007E730E6CC43E39BA40D892AD6C77BD1`，2,678,112 字节），烧录日志显示 `Hash of data verified`。约 35 秒启动日志中红、绿、蓝、白、黑五次整屏传输均 `ESP_OK`（各 200 行、128000 字节），进入 `BRINGUP READY`，未出现地址探测诊断标记；同窗仍有 PCA 第三次写入重试失败 21 次、ST7789 `draw_bitmap` 失败 17 次。用户已休息，**本轮无新的实屏目视或温度确认**，不能把先前目视观察移作本次确认；M2 仍未验收。烧录日志 `.tmp/hi-yaka-c0-post-probe-address-restore-20261124-flash.log`，SHA-256 `100EF5EBA6799516B623D9FD971319DF3A8CDA28CCE63EBF9625B4B76BF87187`；启动日志 `.tmp/hi-yaka-c0-post-probe-address-restore-boot1-20261124.log`，SHA-256 `5718258367779A9826D544ED1A19A486242348C3B99C1251BED52BBE5535E540`。

**仅地址事务 A/B（2026-11-24）：**同一 c0 样机，隔离提交 `f27773d`，镜像 SHA-256 `D481015474A39CACF9D45DABC50F73670ECC5D05B948D40B0391476A11C3EF11`，OTA0 烧录 `Hash of data verified`。两次均通过重新打开 COM5、清缓冲并拉 EN 形成独立主控复位，未做全板断电；均做 A8→B8→B8→A8：A 先 `probe(0x55)`，B 用原始命令发送 START、`0xAA` 地址字节（`ack_check=false`）、STOP；然后写同一笔 PCA 安全锁存。两轮 A、B 各 16/16 的后续 PCA 首写成功；A 前序每次返回 `ESP_ERR_NOT_FOUND`，B 前序每次返回 `ESP_OK`。**B 的 `ESP_OK` 只表示这段“不检查 ACK”的命令完成，不证明任何器件应答。**两轮约 40 秒窗口 PCA 写重试耗尽与 LCD 绘制失败均为 0，但此前其他同形实验仍出现后段失败，且没有实屏和长稳验收，不能把该诊断镜像当成 M2 修复。结合前轮纯等待阴性、两种地址探测阳性，`probe` 专有路径不是局部首写恢复的必要条件；某种地址事务或两条路径共有的控制器准备动作足以形成该现象，**物理 START/STOP 电平作用与内部准备动作尚未分离**。用户离线，无本轮实屏或温度确认；M2 仍开放。原始日志仅本地：`.tmp/hi-yaka-c0-pca-address-only-abba-boot1-20261124.log`（SHA-256 `A477379826AF65E39A6F1569D4F6837AF7CF8B727C094785D23B286289C2B7BB`）、`.tmp/hi-yaka-c0-pca-address-only-abba-boot2-20261124.log`（SHA-256 `E223B653DA89B07AF397A419E285914DF4505D3FDF8E93CF5183D9FDCE8A9AF1`）；烧录日志 `.tmp/hi-yaka-c0-pca-address-only-abba-20261124-flash.log`（SHA-256 `4DC819570D9CF98A38F58D57854B7E0F2F59748099CDEC8BCE587FF4D645148F`。诊断后的原 GPIO fast 基线已恢复，见下段。

**仅地址事务 A/B 后的基线恢复：**同一 c0 样机已烧回原 GPIO fast 镜像（SHA-256 `09E9B9A9C60B508C6E9CF17B3AB4E88007E730E6CC43E39BA40D892AD6C77BD1`，2,678,112 字节），烧录日志显示 `Hash of data verified`。约 35 秒启动日志中红、绿、蓝、白、黑五次整屏传输均 `ESP_OK`（各 200 行、128000 字节），进入 `BRINGUP READY`，未出现地址事务诊断标记；同窗仍有 PCA 第三次写入重试失败 20 次、ST7789 `draw_bitmap` 失败 18 次。用户已休息，**本轮无新的实屏目视或温度确认**；M2 未验收。烧录日志 `.tmp/hi-yaka-c0-post-address-only-restore-20261124-flash.log`，SHA-256 `3312BBD1BDBB5F6D6893313F53A4C17E8F892CB24CD6D1B1F8F10A4A17C475B0`；启动日志 `.tmp/hi-yaka-c0-post-address-only-restore-boot1-20261124.log`，SHA-256 `C4BF8FDC0B0D54CC54486F1906CE8419A4FE3A3204540EF5950A3E6F67073D1F`。

**地址事务取证增强复核（2026-11-24）：**同一 c0 样机，隔离提交 `8238cfb`，镜像 SHA-256 `91B8045EAD12400D09DB37A4822700D497D3665CDDFE9032494BF4A66F2CB8D6`，OTA0 烧录 `Hash of data verified`。两次均通过 EN 独立复位主控、重新打开 COM5 采集，**并非全板断电冷启动，也不是统计独立同分布试验**。会话 UUID 分别为 `a9a96146578a47b5a89c2bf19bdd4a30`、`aea8e5c79a1a46e398bb3b1dfbde0198`；启动 nonce 为 `1d99e0f9650ffd3d`、`c9288acedd08ac8b`；32 条试验记录的 CRC32 为 `a2869b7b`、`dd3d402d`；原始 UART SHA-256 为 `271fdc7964f4f1823ca094423df7a7b6fcd5c5cca2cfc142c11308a0605946c3`、`e7a64ef1e8f98cbb5aa61af1dec518a5a0c3667eb98ae2f54127dc6569df3510`。审计已从原始字节独立重算哈希和 CRC，ROM 启动位也分别为 `7f`、`7e`，样机 MAC 同为尾号 `d6:c0`，支持“两次同板不同来源的 EN 复位记录”，排除简单文件复制。两轮 A（`probe(0x55)`）、B（原始 START/`0xAA`/STOP，`ack_check=false`）后续 PCA 首写均各 16/16 成功；前序返回码仍不能当地址 ACK。首轮约 30.17 秒、逐条报告窗口内出现一次 `C3→03` 写耗尽和一次 LCD 绘制失败，第二轮约 40 秒内未见；日志输出负载可能影响时序，但尚无单因素证据。**本取证镜像的 A/B 都有前序事务，没有同镜像“无前序”组**；前序作用的因果支持来自此前 250 微秒纯等待阴性对照，当前只能确认来源与结果复现，不能推断物理电气根因或产品稳定性。用户离线，无本轮实屏或温度确认，M2 仍开放。取证日志仅本地：`.tmp/hi-yaka-c0-pca-address-only-evidence-boot1-20261124.log`（SHA-256 `0EE6AE563E4BDC99AF067314105DF97456B3F9CCD40290A5924599B59CB48266`）、`boot2`（SHA-256 `E5469292FF3F273F26AFAF61238B318804795A94C1C4A3DC50ADB1AA01B96AD1`）；烧录日志 `.tmp/hi-yaka-c0-pca-address-only-evidence-20261124-flash.log`（SHA-256 `6390140397A576EBF1AF325ABDC9C2D7E99561E91D3C0A407BA026694920A986`）。取证诊断后的原 GPIO fast 基线已恢复，见下段。

**取证增强复核后的基线恢复：**同一 c0 样机已烧回原 GPIO fast 镜像（SHA-256 `09E9B9A9C60B508C6E9CF17B3AB4E88007E730E6CC43E39BA40D892AD6C77BD1`，2,678,112 字节），烧录日志显示 `Hash of data verified`。约 35 秒启动日志中红、绿、蓝、白、黑五次整屏传输均 `ESP_OK`（各 200 行、128000 字节），进入 `BRINGUP READY`，未出现地址事务诊断标记；同窗仍有 PCA 第三次写入重试失败 17 次、ST7789 `draw_bitmap` 失败 15 次。用户已休息，**本轮无新的实屏目视或温度确认**；M2 未验收。烧录日志 `.tmp/hi-yaka-c0-post-evidence-restore-20261124-flash.log`，SHA-256 `13AC5805BF07389FD9635D83DD660112D832FF18FB25AC5FC6A28810BE70ECBA`；启动日志 `.tmp/hi-yaka-c0-post-evidence-restore-boot1-20261124.log`，SHA-256 `F4202B1FA5213C476CEE83F2FB324E738D058FB34B22E3DDA5C622123118CAF5`。

**失败处理设计与隔离验证（2026-11-24，潜在失效路径，非实板故障）：**当前 PCA9557 的 `0x01` 输出写入三次仍失败时，软件保留旧 shadow，但返回错误不能证明器件未锁存新值，因而 shadow 不再有已验证含义；随后“目标值等于 shadow 就返回成功”的优化可能掩盖控制失败。LCD `FinishTransaction` 同时遇到主传输与 CS 释放失败时，只把主错误作为返回值，释放状态也需单独记录。另一个跨模块风险是 `PulseMotor` 仅在 MOTOR ON 写入返回成功后才启动 OFF 定时器；若写入实际锁存为 ON、返回却失败，当前流程可能没有按时关闭的保证。这些是**潜在失效路径**，没有通过实板故障注入证明已发生。

隔离设计审查稿提出：终态写失败后使 PCA shadow 失效、禁止以旧值推断硬件；分别保存主错误与 CS 释放错误；马达脉冲关闭由独立有界安全路径负责，不能只依赖 ON 写入成功。安全常量 `0x01=0x40` 会使 LCD 复位为低、关闭背光与马达，因此即便安全锁存与方向 `0x03=0x38` 读回成功，LCD 也需完整面板重新初始化与重绘；仅凭寄存器读回不能宣称实际管脚已测到安全电平。设计稿 `.tmp/HIYAKA_LCD_FAILURE_SEMANTICS_DESIGN_20261124.md` 已完成设计审查；阶段 A 的隔离结果见下段。主仓与实板仍使用原 GPIO fast 基线，PCA 首访根因仍未知，M2 未验收。

**阶段 A 隔离结果与门禁：**独立工作树 HEAD `487ec30`（此前检查点 `969f5ef`），ESP-IDF 6.1 云端 full GPIO Lab 与 headless 两配置均构建通过。在 Linux host 以实际 `hi_yaka_pca9557.cc` 和 `hi_yaka_expander_lcd_io.cc` 分别运行 GPIO/PARLIO 两模式故障注入，结果均 PASS：覆盖 CASET/RASET/TX 主错误与仅 CS 释放错误、两错并发时的记录、每次 flush 回调恰一次及回调再入，并确认 `quiesce()` 本身不是回调完成屏障；正常 MOTOR ON/OFF 从 `RST/BL` 已高的 `0x43` 状态出发，保留其它输出位。**尚未覆盖**真实安全 worker、真实 LVGL 锁与并发、物理管脚，以及阶段 B 的面板重新初始化。Audit 仍为“继续离线注入 GO、实板烧录 NO-GO”；构建与 host PASS 不代表整机安全或显示稳定。主仓产品源码和 c0 样机仍是原 GPIO fast 基线，PCA 首访根因未定，M2 未验收。证据仅本地 `.tmp/hi-yaka-lcd-fault-stage-a-20261124/dist/`，其中 `host-fault-results-linux.txt` SHA-256 `5EF947301AF671DE802431AF12012B950B1BB70BD54A4A6F8F333602275A4006`。

**阶段 B2b 隔离候选（尚未实板）：**独立工作树提交 `481fc34` 已加入按故障代次串行执行的 PCA 安全写入、双寄存器读回及旧代次撤销，并为同一 LCD panel 对象安排复位、重新初始化和重绘；背光、马达及对象生命周期设有恢复门禁。Linux host 使用候选源码的故障注入在 PARLIO 与 GPIO 两种模式均 PASS。随后 Audit 对该提交的 ESP-IDF 6.1 full GPIO 与 headless 镜像、源码哈希、OTA0 容量及配置完成离线审查，结论为 GO；这仅表示离线门禁通过，**不等于实物管脚安全或屏幕恢复已验证**。候选尚未烧录到 c0 样机，也未合入主仓；真实 LVGL/worker 并发、物理电平及恢复后的持续显示尚未实测，实板仍为 NO-GO。PCA 空闲后首写异常的根因仍未归属，M2 继续未验收。host 证据仅本地 `.tmp/hi-yaka-lcd-fault-stage-b-20261124/dist/host-b2b-final-linux.log`，SHA-256 `47152526630252F0E6986CD4BDEA56F46C5BEB0F7EADAB53EFC90A8CE45135B0`。

**下一步旁听工具（仅离线就绪）：**PCNT 隔离镜像已修订至提交 `5a2367d5a46409ba4ebcd1ce368eb42bb4271b11`，云端 ESP-IDF 6.1 构建与 Audit 离线终审均 GO；新应用镜像 208656 字节，SHA-256 `2B9A327AAD6992F6D958FED46483CE483B090AE64DEDFFAD954BC7E9E31D40F0`。旧提交 `e6f5ad4` 的镜像不再用于后续试验。PCNT 聚合计数仅可辅助判断 SCL 阶段，**不能判定第 9 位的物理 ACK**；新镜像尚未烧板，没有实测计数或 ACK 结论。待用户可配合时，受控比较旁听开启与关闭的扰动；离线 GO 不等于实板 GO，也不代表 PCA 首访根因已找到，M2 仍未验收。

**RAW ISR 隔离镜像来源与门禁：**独立工作树 `.tmp/hi-yaka-i2c-raw-isr-20261124` 当前 HEAD 为 `1c7a167`，诊断代码提交为 `48d72b9`，后续仅有文档更新；原版镜像 `diagnostics/i2c-isolation/dist/raw-isr-stock.bin` SHA-256 `CC036731FCB981918DFFBE3BFA3F61260E597AB2EA871A18BB7BE662B19A1F50`，补丁版 `raw-isr-patched.bin` SHA-256 `AB57EAEBEC719B96C280A90605E5FDAB92FEDBF0514BF2539F50113BE3AF0642`，两版配置与分区一致，组件覆盖已核，云端 IDF 6.1 构建及 Audit 离线审计 GO。三层镜像随后已用于同板实测，结果见下；均不使用 PCNT，只记录主控软件事务窗口内的 ISR 状态，**不能判定 PCA 物理 ACK 或 PN532 硬件归因**。本段只记离线产物与门禁，M2 仍未验收。

**官方驱动补丁线索（待证假说，置信度低）：**Espressif 在 [`release/v6.1` 的提交 `09f0c5c`](https://github.com/espressif/esp-idf/commit/09f0c5c) 中，于事务和 probe 启用中断前清除残留 raw 中断并复位 `contains_read`；[v6.1](https://github.com/espressif/esp-idf/blob/v6.1/components/esp_driver_i2c/i2c_master.c) 与 [v6.1.1](https://github.com/espressif/esp-idf/blob/v6.1.1/components/esp_driver_i2c/i2c_master.c) 标签的相应位置尚无这组清理。官方报告的复现条件是等待任务与 I²C ISR 位于不同核，而 ESP32-C6 的应用高性能核为单核；其主要现象还包括假成功，**不能直接套作本板 PCA 事务失败的根因**。纯对照隔离工作树 `.tmp/hi-yaka-i2c-rawclear-20261124` 镜像应用源码提交 `09ee18f` 已用云端 IDF 6.1 构建并获 Audit 离线 GO：stock 镜像 196272 字节，SHA-256 `A63DD4E012261A0C86A75DA60087F6A9AA841DC3028EE694C19E1E7AADD8FD93`；加入官方 10 行的 fixed 镜像 196288 字节，SHA-256 `7B6436DF0CDA86D19156FEF57B29840CDD52806FA1A2B5DEF856EF863DB46B43`。两版应用、配置与分区相同，唯一驱动差异为该补丁；**纯 A/B 均不含 RAW、PCNT 或 probe 插桩**，上段 RAW 镜像仅供独立观察。隔离工作树现 HEAD `0467df5`，后续只新增 `tools/summarize.py` 与测试；17 项云端 Python 合成日志测试 PASS、Audit 离线 GO。该门禁仅核单次 UART 日志的事件顺序、安全状态、32 轮试验、读回与计数；跨主控复位的日志须按启动边界拆分，不能据此证明物理 ACK 或镜像已烧入。原镜像未重构建，SHA 不变。后续同板 A/B 结果见下；当前 c0 已恢复原 GPIO fast 基线，M2 开放。

**2026-11-24 同板实板对照：**纯 stock/fixed 官方 rawclear A/B 各经两次主控复位，四份日志每轮均为 PCA 安全目标写入首笔 API 成功 `0/32`、第二笔后目标读回 `32/32`、末尾安全态通过且 `PA=0`。官方 10 行补丁在**此板、此事务与此上电条件**下未改善首写；`ESP_ERR_INVALID_RESPONSE` 本身仍不能判定线上的物理 NACK。证据仅本地 `.tmp/hi-yaka-c0-rawclear-stock-a{1,2}-20261124.log` 与 `.tmp/hi-yaka-c0-rawclear-fixed-b{1,2}-20261124.log`；镜像 SHA 见上段。这些是主控复位后的重复观测，不是独立全板冷启动。

**主控 ISR 观察与控制脚对照：**另一次 RAW ISR 诊断中，stock 首笔 `0/32` 成功；patched 未 arm、arm 各 `0/16` 成功，后续均可重试。arm 时主控内部 ISR `status=0x400`（NACK 事件）及 `0x80`（完成事件）均有记录，但原 `trace_integrity=0` 是把 `raw=0x8212` 中非使能位也当作异常的过严门禁，故该轮只作受限软件观察，**不是物理 ACK/波形证据**。再做安全 ABBA：A 向 `0x01` 写当前值 `0x83`、B 写 `0xC3`（仅翻转 `LCD_CS` bit6），两组各 `0/16` 首笔成功，第二笔恢复，修订后的 trace 门禁为 1。A 读回 `0x83` 只说明锁存一致，不能证明 no-op 写已执行；B 在重试后读回 `0xC3` 才确认锁存改变。故 CS 位翻转**不是首笔失败的必要条件**，仍未定位根因。日志 `.tmp/hi-yaka-c0-raw-isr-{stock,patched}-20261124.log`、`.tmp/hi-yaka-c0-i2c-noop-toggle-20261124.log`（后者 SHA-256 `3ED4E5737B92E030F6883B9C9AEA7555AF8E750FC5F5E252F59C692E6EAFD224`）。

**空闲时长实验未通过门禁：**1 ms/20 ms 两次对照分别仅完成 `31/32` 和 `8/32`，均在中途安全核验失败后提前终止，`conclusion=invalid`。首轮 A 1 ms 已执行的 15 笔有 12 笔首 API 成功、B 20 ms 的 16 笔为 0；第二轮仅有 A 的 8 笔，其中 6 笔 API 返回成功，但 2 笔没有预期的 `0xC3` 读回。`SAFE_FINAL=1` 只说明最终再次操作可恢复；`positive_control=0` 是提前停止而未运行 QMI，并非 QMI 失败。增强遥测镜像更在 `SAFE_INIT` 外层第 2 次后首个 PREP 即失败，试验 `0/32`、`SAFE_FINAL=0`；`read01/read03` 首次失败后读到 `0x38`，不能断言真实输出锁存值。**这三轮不能用于判断缩短空闲时间有效。**日志 `.tmp/hi-yaka-c0-i2c-idle-ab-20261124.log`、`...-idle-ab-repeat-20261124.log`、`...-idle-telemetry-20261124.log`（遥测 SHA-256 `A30AA8584F6DD61B0FB1655194A1AD6262723B239717FACAC86866F615D20394`）。

**恢复与验收状态：**首次诊断后烧回原 GPIO fast 镜像，约 50 秒日志有 551 次 PCA 重试、17 次耗尽、15 次绘制失败；用户当次确认画面完整、温度正常。最后增强遥测结束后又烧回同一基线镜像 SHA-256 `09E9B9A9C60B508C6E9CF17B3AB4E88007E730E6CC43E39BA40D892AD6C77BD1`（2,678,112 字节，烧录哈希校验通过），约 48 秒日志五色 API 均 `ESP_OK`、进入 READY，但仍有 546 次重试、24 次耗尽和 20 次绘制失败；**用户随后目视确认本次回滚后的 LCD 色块和横向文字完整、温度正常；这只闭环恢复观察，未消除运行期错误**。日志 `.tmp/hi-yaka-c0-rawclear-post-restore-gpio-20261124.log` 与 `.tmp/hi-yaka-c0-post-idle-telemetry-restore-gpio-20261124.log`（后者 SHA-256 `9845E93DC761451774A14B58F0197616ECDF7998BDF9ABB87C01803E83FA8D22`）。PCA 首访根因未定，M2 仍未验收。

**2026-11-25 固定身份 GPIO A/B（同一 c0 板，M2 未验收）：**同源提交 `b85534e` 的 A（无空闲预访，应用 SHA-256 `5A6AF7DC5AB0C3715940114115AE1EDD256E042A4071659C98B33BC82703BF80`）与 B（在长空闲后的 PCA 写前访问 `0x55`，应用 SHA-256 `C7794C56348967FCD4A02916BC6F780896FABFD15FDE908800EA7AF06A8A1431`）各运行约 120 秒。A 有 1975 次实际写、226 次首试失败，长空闲首访 `58/58` 失败、1 次三试耗尽；初始化填充第 167 行写失败后软件禁用显示。用户当次仍观察到画面完整、温度正常，但不能据此忽略软件禁用记录。B 有 8286 次实际写、734 次首试失败，长空闲首访 `0/574` 失败，却仍有 23 次耗尽与 19 次 LCD 参数绘制失败；用户对 B 的实屏目视尚待反馈。排除 idle 类写后，A 的首试失败为 `168/1917`（8.76%），B 为 `734/7712`（9.52%）；两版负载与持续方式不同，不能据此判断两比率等效或有显著差异。B 的 23 次耗尽和 19 次参数绘制失败发生在 idle 首访 `0/574` 失败的条件下，说明还存在长空闲首访以外的持续访问问题，具体根因未分离。两版五色诊断 API 均完成，120 秒末的 PCA 读回均返回 `ESP_ERR_INVALID_RESPONSE`，不能把伴随的寄存器占位值当作真实脚位。**结论：**`0x55` 预访在本次窗口消除了所测长空闲首访失败，却没有修复整体刷新，故停止把它当作修复路线；根因仍未定，M2 未通过。A/B 后首次回滚在 COM5 进度 12.1% 中断（exit 2）；重试用 115200 no-stub 将原 GPIO fast 应用（SHA-256 `09E9B9A9C60B508C6E9CF17B3AB4E88007E730E6CC43E39BA40D892AD6C77BD1`）完整写入 OTA0 `0x20000`，共 `2,678,112` 字节，esptool 报 `Hash of data verified`。45 秒启动日志 `.tmp/hi-yaka-c0-idle-previsit-post-rollback-boot-20261125.log`（SHA-256 `421551CE10D32922DDF53421CE86D31931DE7054131CEC722B72C8BD9253A6D4`）记录五色 API 均 `ESP_OK`、28.53 秒进入 READY。用户在回滚过程中确认温度正常、无持续振动；B 版和回滚版均无本次实屏目视确认，不写为画面通过。原始日志 `.tmp/hi-yaka-c0-idle-previsit-fixed-A-135s-20261125.log`（SHA-256 `5474354C2198D79D62E6FB7CD26607A48CC2FF0B917574656CC9231FE6081590`）与 `.tmp/hi-yaka-c0-idle-previsit-fixed-B-135s-20261125.log`（SHA-256 `980DB5FDF5BEFFCBAC54CDA752A817B15C73A10C1FEBC8E4CA8518F0227C85D8`）。

**GPIO 整屏诊断速度边界：**此前 GPIO fast 基线在 `.tmp/hi-yaka-c0-gpio-fast-boot1-20261123.log` 的五次 `128000` 字节整屏传输，设备报告绘制用时平均 `2.025532` 秒，即约 `63.19 kB/s`、`0.4937` 次整屏/秒；这是去掉色块间固定等待后的单次诊断传输速率，**不是应用界面帧率或目标 `172×320` 屏的 M2 性能验收**。

### 第一性原理交叉审计与下一判别证据（2026-11-25）

- **设计事实，高置信：**[冻结原理图网表](../../../../../outputs/YaKa_3.0_SMT_RELEASE_20261021/00_project_archive/YaKa-v3.0-schematic-netlist-20261022.enet)记录 U3 为 `PCA9557PW,118`：U3.3/A0 经 R8 `10 kΩ` 上拉至 `SYS_3V3`，U3.4/A1、U3.5/A2 接地，设计地址为 `0x19`；U3.15/`RESET#` 与主控 U1.4/`CHIP_PU` 同接 `RESET`，U3.16/VDD 接 `SYS_3V3`。[NXP 数据手册](https://www.nxp.com/docs/en/data-sheet/PCA9557.pdf) §7.1 明确地址脚**无内部上拉**，必须确定为高或低。若 A0 在实体上变低，PCA 地址会变为 `0x18`，与本固件的 ES8311 地址相同，存在地址碰撞风险；目前 A0 曾在运行时测得稳定 3.3 V，碰撞**未获实测证实**。
- **总线与脚位边界，高置信设计／低置信实体：**同一冻结网表记录 R11/R12 各 `4.7 kΩ` 上拉 SDA/SCL 至 `SYS_3V3`。[NXP 时序表](https://www.nxp.com/docs/en/data-sheet/PCA9557.pdf)给标准模式 100 kHz 的上升时间上限 `1 µs`。按常用 RC 近似 `t_r≈0.8473RC`，单根总线电容须约 `≤1 µs/(0.8473×4.7 kΩ)=251 pF` 才满足此项；这是**设计预算估算，不是本板电容或边沿测量**。U3 的 IO0 为开漏；U3.13/IO6 接 LCD_CS，诊断写值 `0x83→0xC3` 只翻转 bit6/CS，不翻转 RST。[NXP 寄存器说明](https://www.nxp.com/docs/en/data-sheet/PCA9557.pdf) §7.3 还明确 `0x01` 读回是输出锁存值，不等于真实脚电平；必要时读 `0x00` 并用实体测量交叉核对。
- **已排除的必要条件与下一判别：**无 LCD 的最小诊断仍有 PCA 首笔事务失败，LCD 活动不是该现象的必要条件；同板 `0x55` 预访仅消除所测长空闲首访失败，不能消除持续刷新耗尽。[ESP-IDF v6.1 同步 I²C 实现](https://github.com/espressif/esp-idf/blob/v6.1/components/esp_driver_i2c/i2c_master.c#L3436-L3445)在事务状态非 DONE 时统一返回 `ESP_ERR_INVALID_RESPONSE`，单凭该返回码不能定位到地址、寄存器、数据哪一字节的物理 ACK，也不能独断 U3 损坏。下一项最小必要证据是在**U3 端**同步观测 SDA/SCL 模拟波形、上升沿与逐字节第 9 位 ACK，并同时核 U3.3/A0、U3.15/RESET#、U3.16/VDD；万用表可核静态电平，动态边沿/ACK 需示波器或带模拟通道的分析仪。测量前先确定 PN532 模式绑带，避免悬空接口成为混杂。根因仍未知，M2 开放。
- **屏幕版本隔离：**c0 本轮已验证画面的旧屏路径为 `H0160S002 V1 / 200×320`，其当次固件为 `GPIO6=MOSI、GPIO7=SCLK`，当前主线 `hi_yaka_expander_lcd_io.cc` 的 PARLIO 候选 `data_width=4`；另一会话的新屏 `ZJY-LBS147TC-IG01 / 172×320` 的接口为 `GPIO6=SCLK、GPIO7=MOSI`，其旧候选代码 `data_width=1`。两套屏幕脚位、几何和镜像必须按板卡/实装件隔离，不能把新屏结论套到 c0。PARLIO 硬件上有传输可能性，不等于本轮 PARLIO 实现已通过实屏验证；当前产品路线仍以已显示的 GPIO 路径解决 PCA 稳定性，仅在实测性能不达标时再评估 PARLIO。

**片上 PARLIO RX 隔离诊断（2026-11-25，门禁未通过）：**隔离源码提交 `58c30ca`，应用 SHA-256 `03D39D35A40FBCA0E9C2C141518C1B2718FC12D75B99ACCFA2F5E3038D2F2723`，在同一 c0 板（MAC 后缀 `d6:c0`）的 OTA0 `0x20000` 分区受控烧录校验；分区容量 `0x3F0000`，串口 COM5、115200、no-stub。两次主控复位均在 `PREP83` 的第二轮读取输出寄存器 `0x01` 时约 `161 µs` 返回 `ESP_ERR_INVALID_RESPONSE`，于是提前停在 phase0/index0。100 kHz 的 combined 读至少包含地址写、寄存器、地址读与数据约 36 个 SCL 周期，单按频率计算已约 360 µs；同镜像正常约 451 µs，异常约 149–184 µs 与完整事务不相容。局部接收缓冲预置为零，因此错误返回同时带出的 `0x38/0x40` 不能简单归为未初始化栈值；这些时长和数值仍**不能单独定位物理 NACK 或 IDF 缺陷**，外部相似 issue 也不能替代本板波形。boot1 末尾安全双读回 `verified=1`；boot2 的最终双轮安全读回 `verified=0`，不能把中途一次安全读回成功当作最终确认。两次 `route_same=1`、`pa=0`，QMI 正控与 `0x55` 空地址负控各通过自身门禁；目标 A/B 的样本数均为零，**没有 PARLIO RX 的有效目标数据，也没有物理 ACK 结论**。原始日志 `.tmp/hi-yaka-c0-parlio-rx-boot1-20261125.log`（SHA-256 `6F23220BFD9B555ED1C1A035921EB876E589FB6CDC11AEFB045634AB5C2B1FAC`）与 `...-boot2-20261125.log`（SHA-256 `239C0D23FFCF1D4671493A34B5151B3B6A45CCDCACD33A5F5F8040AF1F0B8D73`）。随后原 GPIO fast 镜像 SHA-256 `09E9B9A9C60B508C6E9CF17B3AB4E88007E730E6CC43E39BA40D892AD6C77BD1` 已重新写入并完成 esptool 哈希校验；45 秒启动日志 `.tmp/hi-yaka-c0-parlio-rx-post-rollback-20261125.log`（SHA-256 `BFF6A6F67F46D927A5B4BD5C119717B121A3714C7F1CFAF5043CBE117B8B6202`）记录五色发送均 `ESP_OK`、28.46 秒 READY，但仍有 499 条 `retry 2/3`、21 次耗尽、18 次 `draw_bitmap` 失败。本次回滚**没有新的实屏目视确认**。烧录日志 `.tmp/hi-yaka-c0-parlio-rx-rollback-20261125.log`（SHA-256 `F71F84D7BBAD735C8A45C3138ABCE480B9A039CF05BD4F9F350927B8D3A79B0A`）。诊断不能判实板 GO，PCA 首访根因未定，M2 继续开放。

**PREP83 受控复测（2026-11-25，有效但未定位根因）：**在同一 c0 板、COM5 上，以隔离源码提交 `64fa417`、应用 SHA-256 `0C408D41AE9D94AD20D66FE37AD50E24DA2F109C21AEE0EF71662A79BC096EBD` 完整写入 OTA0 并校验；两次独立主控启动均执行 `B2/A2/A2/B2` 共 8 试次，`valid=1`。每次 `PREP83` 第二轮读回 8/8 正常，B 组 4/4 捕获到自身完整波形；目标写 8/8 的首次 API 返回 `ESP_ERR_INVALID_RESPONSE`，第二次写成功并读回锁存 `0xC3`。每次 `CONTROL_GATE` 安全读取曾有一笔约 `160 µs` 的 `INVALID_RESPONSE`，随后双轮安全读回通过；这说明准备门禁可重复通过，**不表示短读故障消失**。片上捕获未覆盖目标异常写或该异常读，不能据 B 组正常捕获推断失败事务的逐字节 ACK 或具体根因。两轮结果的 QMI 正控、`0x55` 空地址负控、路由一致性与 PA 关断门禁均通过；与前段首次诊断“零目标样本”区分。原始日志 `.tmp/hi-yaka-c0-prep83-boot1-20261125.log`（SHA-256 `DC45175C4221435649866AE9E4CCE0860224092A3118C5C6AF6310CE3463FFF9`）和 `...-boot2-20261125.log`（SHA-256 `3C075E9B67B489CE03C5AB06FE2AA9FA3757F9A87DACA335A2DD686DC6DD7228`）。实验后已用 115200 no-stub 将原 GPIO fast 应用 SHA-256 `09E9B9A9C60B508C6E9CF17B3AB4E88007E730E6CC43E39BA40D892AD6C77BD1` 写回 `0x20000` 并取得设备哈希校验；45 秒启动日志 `.tmp/hi-yaka-c0-prep83-rollback-boot-20261125.log`（SHA-256 `EEC15FAD503058F3FD84CDBA31012C77DCBD3D0583F11C256F1FABACCF4DDA6C`）记录五色 API 均 `ESP_OK`、28.56 秒 READY，但 `retry 2/3` 仍有 496 条、耗尽 21 次、`draw_bitmap io tx param failed` 17 次，且**没有本次实屏目视确认**。烧录记录 `.tmp/hi-yaka-c0-prep83-rollback-flash-20261125.log`（SHA-256 `D509CB3614F573C3C8C199430A59411EB2C0F3434BFB87F50BD25F6AC8744F9C`）。本轮只证明受控试次可执行并重复见到首次失败，未修复显示链，M2 仍未验收。

**目标写片上数字帧（2026-11-25，地址阶段可定位，实体原因未定）：**同一 c0 板、COM5 以隔离提交 `87d0bdf` 的应用 SHA-256 `EB3046D31EEB2EE66A7E7C6FC8871F083611675D81A56C0BA48BDE7442E8B653` 运行两次独立主控启动；每次 `B2/A2/A2/B2` 共 8 试次且 `valid=1`。A、B 各 4 次目标首写 API 均返回 `ESP_ERR_INVALID_RESPONSE`，第二写均 `ESP_OK`；B 的 4/4 次片上数字捕获显示，首帧只有写地址字节 `0x32`、地址 ACK 位为 NACK、STOP，第二帧为 `0x32/0x01/0xC3` 三字节均 ACK、STOP；两次启动合计 8 组完整失败／重试配对帧。目标写前空闲约 `20.002–20.010 ms`，A/B 范围相同，`route_same=1`、`pa=0`、`final_safe=1`，QMI 与 `0x55` 对照均通过自身门禁。**这只是在主控数字输入侧观察到地址阶段 NACK，不证明 U3 焊盘的 SDA/SCL 电平、上升沿、RESET 或芯片本体是根因。**本镜像刻意把目标后读回延至 PREP 后约 31 ms；`read01` 反而在约 `160–177 µs` 返回 `INVALID_RESPONSE`，`target_verified=0`，因此**本轮不能写“锁存 `0xC3` 已有效读回”**；上一镜像的有效读回属于不同测试时序。原始日志 `.tmp/hi-yaka-c0-target-retry-boot1-20261125.log`（SHA-256 `31708A317F5FABAB754FF030BCBEBD714C5B6EE0A7D5D8DA62CCDEB10AE1936D`）与 `...-boot2-20261125.log`（SHA-256 `BAEAA5F71780453A90A4B5F507F44B4F33828037CD1B0A223B538B8F61AA848D`）。本次实板目标定位证据有效，但未修复首写失败。实验后原 GPIO fast 应用 SHA-256 `09E9B9A9C60B508C6E9CF17B3AB4E88007E730E6CC43E39BA40D892AD6C77BD1` 已按 115200 no-stub 写回 COM5 的 OTA0 `0x20000`，设备哈希校验通过。45 秒启动日志 `.tmp/hi-yaka-c0-target-retry-rollback-boot-20261125.log`（SHA-256 `B1448D7CE77E6B8B4E65DE62C97A4AD9439E135AD629A786B2A69482FBD8FC16`）记录五色 API 均 `ESP_OK`、28.46 秒 READY，仍有 494 条 `retry 2/3`、22 次写耗尽、18 次 `draw_bitmap io tx param failed`；烧录日志 `.tmp/hi-yaka-c0-target-retry-rollback-flash-20261125.log`（SHA-256 `D4A75E574646D08902721F7AF5F9DA81D16C9BC9EC66E1D817812ED5ADD701CE`）。本次无实屏目视或温度确认，不能写成显示正常，M2 仍未验收。

**PN532 悬空绑带的归因边界（离线核查）：**NXP [PN532 C106 应用笔记 AN10609_3](https://community.nxp.com/pwmxy87654/attachments/pwmxy87654/nfc/4394/1/PN532C106_Application%2BNote_v1.2.pdf) §2.2.1 将 I²C 模式定义为 `I0=1、I1=0`；§2.4.4 要求两脚直接接 DVDD 或 GND，并列明共用的 MOSI/SDA/HSU_TX 脚在 HSU 为推挽输出、I²C 为开漏、SPI 为输入。c0 样机的 R73/R74 已拆，模式脚现悬空，因此后续共享 I²C 现象存在模式不确定的混杂；**若进入 HSU，还可能在 SDA 上产生驱动争用，但尚无波形或模式实测，不能写成已发生**。2026-11-04 原绑带仍在时已有 PCA 写入失败，悬空不是全部历史故障的必要条件，亦不能单独判为 PCA 根因。用户恢复配合后，须先完全断电并确定 PN532 模式绑带，再用 c0 做硬件归因或 M2 验收；当前不做物理操作，M2 仍未验收。

**跨器件软件 I²C 旁证（2026-11-25 回查历史实验）：**同一 c0 样机在 2026-11-20 的 ES7210 隔离测试中，GPIO10/11 改由开漏软件 I²C 驱动、不启用 ESP-IDF 硬件 I²C 控制器：约 50 ms 空闲后首笔状态寄存器读取在后 15 轮均停于写地址 NACK，随后两项身份寄存器均 15/15 读到有效值；另一受限可逆写回测试的两次复位中，目标写首笔均在写地址 NACK、第二笔得到 ACK 和目标值读回。见[ES7210 控制面实板证据](2026-11-20-es7210-control-ab-evidence.md)的 R、U 组。它说明“共享总线的所有类似瞬态都只由 IDF 硬件 I²C 控制器造成”与既有观察不符；但 ES7210 与 PCA9557 是不同器件，软件实现、事务和时间条件也不同，**不能据此认定两项故障同源，更不能替代 U3 焊盘的波形测量**。本轮 Windows 已不枚举 c0 的 COM5，未继续烧录；下次对照须先确认板卡身份和安全条件。

**重试时间语义（离线代码核查）：**主线和阶段 B2b 的 PCA 写重试均调用 `vTaskDelay(pdMS_TO_TICKS(1))`；本轮 ESP32-C6 构建为 `CONFIG_FREERTOS_HZ=100`，[ESP-IDF v6.1 的换算宏](https://github.com/espressif/esp-idf/blob/v6.1/components/freertos/FreeRTOS-Kernel/include/freertos/projdefs.h#L39-L44)按整数运算得到 `1×100÷1000=0 tick`。因此这里不保证真正退避 1 ms，至多让出调度；它可能影响连续重试形态，**不能解释重试之前的空闲首访失败，也不是已证根因**。尚未就此实施修复 A/B；用户醒来前不做实板操作，M2 仍开放。

本轮原始串口日志仅存工作区 `.tmp/`：`hi-yaka-c0-old-lcd-exact-boot{1..3}-20261123.log`、`hi-yaka-c0-full-gpio-replay-boot{1,2}-20261123.log`、`hi-yaka-c0-parlio-samesource-replay-boot{1,2}-20261123.log`，以及上述 1 MHz 日志。所有次数均为该 c0 板的用户视觉观察，不能与 c4 的 PCA 计数或另一板历史画面混算。M2 仍为未验收。

同一轮 ACT_B 人工按键未出现可确认的串口事件，也**不能直接归因 PCA9557 错误**：按键扫描走独立的 ADC/GPIO 路径。两次捕获到的 `ladder=0 mV decoded=-1 key_a=1 key_b=1` 是无效采样窗口；源码在 ADC `ReadMillivolts` 失败时保留默认 0 mV 并令解码值为 `-1`，故这**不是实测按键电压为 0 V**。按键是否未跨去抖、采样失败或事件日志丢失仍须分辨，不能据这两帧给按键或 PCA9557 下根因结论。证据：工作区 `.tmp/hi-yaka-c4-main-lab-runtime-actb-20261123.log` 与 `...-actb-retry-20261123.log`；代码 `main/boards/chatchat/hi-yaka/hi_yaka_input.cc`。

## 3. 排查分析

### 3.1 因果链

```text
PCA9557 IO0 写 1 后仅进入高阻态
→ LCD_BL 没有外部上拉
→ R86 继续把 Q10 栅极拉低
→ Q10 截止、背光 LED 无电流
→ 屏幕完全无背光
```

### 3.2 已确认事实

- NXP 原厂资料确认 PCA9557 IO0 是开漏输出，不能主动输出高电平。
- 当前原理图和 PCB 中，`LCD_BL` 通过 R85 接 Q10 栅极，R86 将栅极下拉，但没有上拉电阻。
- 实测 R85 两端接近 0 V、R84 压差接近零，与“Q10 未导通”一致。
- 最新 PCB 数据确认 R85、C118、Q10 和 FPC1 均位于底层；R85.1 为 `LCD_BL`，C118.2 为 `SYS_3V3`，两点距离约 5.6 mm，可实施小型飞线补救。

### 3.3 精确模组逐脚复核

本轮以 `H0160S002 V1` 原厂规格书逐脚核对，不再使用通用 ST7789 模组假设：

| FPC 脚 | 规格书要求 | 当前连接 | 结论 |
|-|-|-|-|
| 1 VCC | 2.4–3.3 V | `SYS_3V3` | 合规 |
| 2/4 GND | 电源地 | GND | 合规 |
| 3 IOVCC | 1.65–3.3 V | `SYS_3V3` | 合规；符号名 `VDDIO1.8V` 具有误导性，但 3.3 V 在规格内 |
| 5 LEDA | 背光阳极 | `SYS_3V3` | 拓扑可工作，但裕量取决于 LED Vf |
| 6 LEDK | 背光阴极 | R84 10 Ω → Q10 低边开关 | 控制拓扑正确；IO0 缺上拉导致 Q10 实际不导通 |
| 7 RESET | 低有效复位 | PCA9557 IO1 / `LCD_RST` | IO1 为普通推挽口，电气可用；固件必须给出有效低脉冲 |
| 8 CS | 片选 | PCA9557 IO6 / `LCD_CS` | 电气可用 |
| 9 SDA | 4SPI 数据 | GPIO6，网名误写 `IO6_LCD_CLK` | 铜线功能可用；固件必须把 GPIO6 配为 MOSI/SDA |
| 10 SCL | 4SPI 时钟 | GPIO7，网名误写 `IO7_LCD_DIN` | 铜线功能可用；固件必须把 GPIO7 配为 SCLK/SCL |
| 11 TE | 撕裂同步，可不使用 | 未连接 | 允许，不是无背光根因 |
| 12 DC | 数据/命令选择 | PCA9557 IO7 / `LCD_DC` | 电气可用 |

除背光控制外，未发现供电电压、GND、RESET、CS、DC 或 4SPI 物理脚位的硬件断链。GPIO6/7 的网络名称写反会增加固件集成成本，但不是铜线接反：实际应固定为 `GPIO6=数据、GPIO7=时钟`。

### 3.4 背光供电的第二层风险

规格书给出背光为三颗并联白光 LED，总额定电流约 60 mA、Vf 约 2.8–3.3 V。当前采用 3.3 V 直供并串 10 Ω：

- Vf=2.8 V 时理论总电流约 50 mA；
- Vf=3.2 V 时约 10 mA；
- Vf 接近 3.3 V 时电流接近 0。

因此 10 Ω 方案能点亮部分批次，但亮度随 Vf、温度和 3.3 V 压降变化很大，不能精确保证 60 mA。它不是当前“R84 压差为零、Q10 栅极为零”的直接根因，却可能在补上 IO0 上拉后表现为亮度偏低或批次差异。下一版优先采用带使能的恒流背光驱动，或至少按实物 Vf 重新定阻并验证最差电压、温升和亮度。

### 3.5 仍需确认的独立风险

| 独立风险 | 当前判断与证据 | 置信度 | 最短判别方法 |
|---|---|---|---|
| 屏内背光 LED 或 FPC 接触异常 | 一块飞线样机背光已亮，削弱背光 LED 完全开路假设；未测背光电流与其它 FPC 脚 | 中/未知 | 测 R84 压降，并另行验证数据与控制脚。 |
| ST7789 数据、复位、CS 或时钟链异常 | 低速 GPIO 版已显示画面，说明存在可工作路径；PARLIO 版未获得目视画面 | 中 | 同板对照两种传输，并核对边界和持续刷新。 |
| 3.3 V + 10 Ω 背光亮度不足 | Vf 范围宽，理论电流约 0–50 mA | 中 | 补救后测 R84 压降/总电流，并在低电量与升温条件下观察亮度。 |
| LCD 超时和 LVGL 看门狗 | 出现在换入样机后段，不能解释上电即无背光 | 中，属于独立问题 | 先恢复背光，再单独记录 SPI/PARLIO 与任务时序。 |

## 4. 解决方法

### 4.1 当前 PCBA 不重新制板条件下的人工补救

该方案只用于已经生产的 PCBA，无需重新制板、切断走线或返厂回流；需要维修人员在显微镜下增加一只电阻和约 6 mm 漆包线。用户已报告一块样机飞线后背光亮起，但未记录具体焊点、阻值和电压；下图仍是**推荐接法**，不把它冒充已核实的实装值。

```text
C118.2（SYS_3V3）
        │
      2.2 kΩ
        │
R85.1（LCD_BL，PCA9557 一侧）
```

操作边界：

1. 完全断电后施工；优先从 R85.1 接到 C118.2，不焊 0.5 mm 间距的 FPC 相邻脚。
2. 不得接到 R85.2；R85.2 是 Q10 栅极侧，错误落点会削弱关断电平。
3. 不得用裸导线把 `LCD_BL` 直接短到 3.3 V；IO0 拉低时会造成电源与下拉管冲突。
4. 验证通过后用 UV 胶或电子胶固定电阻和漆包线，避免振动脱焊。

预计电平：IO0 释放时，结合 R85=1 kΩ、R86=10 kΩ，Q10 栅极约为 `3.3 × 10 / (2.2 + 1 + 10) ≈ 2.5 V`；IO0 拉低时栅极应接近 0 V。该补丁可能使背光在固件初始化前短暂亮起，因此只作为现有 PCBA 的人工补救，不作为正式量产设计。

### 4.2 下一版永久修复

优先改用经完整引脚资源核对的推挽 GPIO 控制 Q10，保留默认下拉，确保上电和固件未启动时背光关闭。若必须继续使用 IO0，应重新设计外部逻辑，使“上电默认关闭”和“运行时可控”同时成立。背光供电优先改为可控恒流方案；若保留串阻，必须以最差 Vf、最低 3.3 V、目标亮度和电阻功耗重新定值，并同步更新原理图、PCB、BOM 与固件安全态。

### 4.3 禁止采用的做法

- 不把 IO0 裸线直连 3.3 V；
- 不在 FPC 相邻脚上盲焊或用探针滑动测量；
- 不把 PCA9557 寄存器读回或 LCD 写入 API 无报错当作画面通过。

## 5. 验证与关闭标准

- [ ] 断电确认 3.3 V 对地无新增低阻短路；
- [ ] 背光 OFF 时 R85.1 接近 0 V、Q10 栅极低于约 0.2 V、背光熄灭；
- [ ] 背光 ON 时 R85.1 接近 3.3 V、Q10 栅极约 2.4–2.6 V、R84 出现合理压降且无异常发热；
- [ ] 显示全屏红、绿、蓝、白、黑和文字图案，确认 FPC、ST7789、方向与颜色；
- [ ] 重复冷启动、按键、TF/NFC/I²C 并发和持续亮屏，未复现失控；
- [ ] 保存样机编号、返修照片、测量值、固件 SHA 和复测记录后，才可重新评估 M2 与发布门禁。

## 6. 证据来源

- [首板 LCD、PN532 与麦克风专项诊断](2026-11-04-lcd-nfc-mic-investigation.md)：PCA9557 寄存器、R84/R85 实测和 FPC 背光链路；
- [换入样机复测](2026-11-05-replacement-board-retest.md)：另一块实板无背光、LCD 超时和 LVGL 看门狗；
- [飞线后 LCD 与模块复测](2026-11-12-lcd-flywire-and-module-retest.md)：背光飞线、PARLIO 对照及 GPIO 横屏当次目视结果；
- [共享 I²C 同板对照证据](2026-11-12-pca-audio-shared-i2c-ab-evidence.md)：同镜像黑屏与复位后恢复、PCA9557 重试统计及无屏幕对照；
- [微雪 ESP32-C6 屏幕传输对照](2026-11-25-waveshare-c6-lcd-transport-comparison.md)：另一开发板的慢速 GPIO、快速 GPIO 与 SPI2 同屏实测速度和目视结果；PARLIO 待测，不能据此归因 HiYaKa 的 PCA9557 故障；
- 2026-11-23 c0 原始日志（仅本地工作区 `.tmp/`）：`hi-yaka-c0-lcd-lab-boot1-20261123.log`（原始 PARLIO，在 TF 前 abort）；`hi-yaka-c0-tf-inserted-warm-{1,2,3}-20261123.log`（插卡后旧无屏 3/3）；`hi-yaka-c0-headless-same-revision-boot{1..6}-20261123.log`（插卡后同修复版无屏 6/6）；`hi-yaka-c0-lcd-graceful-boot1-20261123.log`（容错 PARLIO，TF 挂载与 READY）；`hi-yaka-c0-lcd-gpio-ab-boot2-20261123.log`（慢速 GPIO，TF 挂载与 READY；其后用户已在同镜像重复启动中确认画面）。镜像 SHA-256 依次为原始 PARLIO `3D334D10A8596AA5A018F6652C510E094AFF2BE43D3804D1B8C60AF65B35D15B`、旧无屏 `9702027529E8A1BCE6F63DA4F6BE8C6821B169F907A65F5CA74E8D0C2F70C622`、容错 PARLIO `9B3CE08D9B0F02E8900C5EDF3D94D3AFA10C37EAE6029A4DB47312B09B79D609`、慢速 GPIO `ACE56A4D9B035568A53F87379A72B22FBD1E7859E7D80C0108BFD3546A51250C`；后两者源码基线 `33e3469`，旧无屏镜像源码基线 `3242189`，慢速 GPIO 另含 profile 提交 `367fe12`。
- 最新 EasyEDA PCB 活体数据：R85.1=`LCD_BL`、C118.2=`SYS_3V3`、底层点位距离约 5.6 mm；
- [硬件连接与固件接口表](../hardware/HARDWARE_MODULE_INTERFACES.md)：PCA9557、Q10、FPC1 网表合同；
- [NXP PCA9557 数据手册](https://www.nxp.com/docs/en/data-sheet/PCA9557.pdf)：IO0 开漏输出特性；
- `H0160S002 V1` 原厂规格书：12 Pin 定义、VCC/IOVCC 范围、4SPI 与背光 Vf/If；
- [固件 PRD](../governance/FIRMWARE_PRD.md)：显示里程碑仍未通过。

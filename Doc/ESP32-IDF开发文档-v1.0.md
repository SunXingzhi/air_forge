# 焊接废气净化装置 · ESP32-S3 / ESP-IDF 开发文档

| 项 | 内容 |
|---|---|
| 文档编号 | DEV-AIRFORGE-001 |
| 版本 | v1.1（文件名沿用 v1.0） |
| 日期 | 2026-10-25 |
| 需求基线 | `Doc/需求文档-人机交互与硬件选型-v1.0.md`（REQ-AIRFORGE-001） |
| 硬件基线 | ESP32-S3-DevKitC-1（N16R8）+ P169H002-CTP（ST7789T3 + CST816T） |
| 软件基线 | ESP-IDF **v6.1.0**（`C:\esp\v6.1\esp-idf`），已确认 target=`esp32s3` |
| 文档范围 | 软件架构、任务模型、驱动与适配层、配置/分区、构建调试、测试与里程碑；含厂商 P169H002 例程的引脚提取与移植说明。**不含电路级设计** |

---

## 0. 阅读结论（先看这一节）

1. **当前工程只是空模板**：`main/main.c` 仅打印 Hello World；[sdkconfig](file:///c:/Users/q1325/Documents/Project/air_forge/sdkconfig) 仍是默认值（2 MB Flash、单 APP 分区、PSRAM 关闭、CPU 160 MHz），与 N16R8 模组（16 MB Flash + 8 MB Octal PSRAM）不符，**任何 UI/显存工作开始前必须先修正配置**。
2. **实际 IDF 为 v6.1.0**，该版本已移除 legacy `driver/i2c.h`，必须使用 `driver/i2c_master.h` 新 API；`esp_lcd` 已内置，但 **ST7789/CST816T 驱动未内置**，Component Registry 组件对 v6.1 的兼容性未验证，建议自带最小驱动（见 §7）。
3. **屏幕 FPC 引脚已从手册确认**（18 pin）：VDD 典型 **3.0 V**、VDDIO 1.65–3.3 V、背光只引出 **LEDK 阴极**、触摸四线（TP_SCL/SDA/RST/INT）与屏共用一根 FPC。背光不能用 GPIO 直驱，需低边开关 + 限流；3.3 V 直供 VDD 已超手册上限（3.2 V），需 3.0 V 或降额确认。
4. **BOM 只有 1 只 MS1100 + 1 只 GP2Y1014AU**，物理上无法同时得到「处理前 / 处理后」两组读数，**P0 验收项无法达成**；须补第二套或修改验收口径（见 B-02）。
5. **两只模拟传感器均为 5 V 供电、输出可达 4.5 V**，ESP32-S3 ADC 上限约 3.3 V，必须加分压 + RC 滤波（新增硬件缺口 H-08）。
6. 风机为感性负载且 MOS 板是否支持高频 PWM 未知；**续流二极管、栅极驱动、24 V→5 V、FPC 座、电源自锁均未采购**，这几项是联调硬门槛。
7. **转接板出现“只有 MISO、没有 MOSI”与两种接口定义都不符**：CST816T 是 I2C（TP_SDA/TP_SCL），屏是 4 线 SPI 单根双向 SDA（pin 10，无独立 MISO）。必须先做通断核对（§6.2.1）；若板子是 SPI 触摸控制器（如 XPT2046）转接板，则与本屏不兼容。
8. **厂商例程已分析并移植**：厂商 `ESP32-LCD-IDF/P169H002` 提供完整 ST7789 初始化序列、CST816T 寄存器表和坐标偏移（Y+5）；已移植到 `components/p169h002/`，`idf.py build` 通过（`air_forge.bin` 251 KB）。厂商 target 是 ESP32-C3，GPIO3/19 不能直接抄到 S3，已重映射（§6.2、§7.6）。

> 结论：**软件架构、点屏、触摸、UI 适配、按键/灯、状态机、NVS、控制台可以立即开工；「处理前后对比、过热保护、模拟采集、风机调速、整机上电」在硬件补齐前只能做桩（stub）与单元验证。**

---

## 1. 依据、范围与术语

- 需求文档 §2 的 CR-01~CR-04 已生效：触摸屏为主输入/唯一信息出口；OLED_UI 为 UI 逻辑框架；物理控件仅 2 键 2 灯；编码器/情绪键取消。
- 本文件把需求落到 **ESP-IDF 工程结构与接口** 一级，不展开寄存器时序与电路参数。
- 术语：
  - **OLED_UI**：`bdth-7777777/OLED_UI`，纯 C99、1 bpp 帧缓冲、四键输入模型、20 ms tick。
  - **适配层（port）**：把 1 bpp 帧缓冲映射到 ST7789 RGB565，把触摸手势映射为虚拟四键。
  - **业务快照（snapshot）**：传感器/风机/滤芯状态的只读结构体，供 UI 每帧读取。

---

## 2. 当前工程基线核查

| 项 | 现状 | 目标 | 动作 |
|---|---|---|---|
| Target | `esp32s3`（`sdkconfig` L628） | `esp32s3` | 保持 |
| IDF | v6.1.0（`build/project_description.json`） | 锁定 v6.1.0 | 写进 README/构建脚本，禁止随手升级 |
| Flash | 2 MB，DIO 80 MHz | 16 MB，QIO 80 MHz | 改 `sdkconfig.defaults` |
| 分区 | 单 APP（`partitions_singleapp.csv`） | 双 OTA + coredump + storage | 新增 `partitions.csv` |
| PSRAM | 关闭 | Octal 80 MHz，8 MB | 开启 `CONFIG_SPIRAM*` |
| CPU | 160 MHz | 240 MHz | 改主频 |
| FreeRTOS tick | 100 Hz | 1000 Hz | 改 `CONFIG_FREERTOS_HZ` |
| 代码 | `components/p169h002/` 已落地厂商 demo（M1/M2 代码路径） | 按 §3.2 拆分组件 | 先实机验证点屏/触摸，再重构 |
| 构建产物 | `air_forge.bin` 0x3D520（251 KB），单 APP 分区 1 MB，余 76% | 16 MB + 双 OTA | 配置改完后 `fullclean` 重建 |

> 注意：`sdkconfig` 是生成物。正确做法是把目标配置写进 `sdkconfig.defaults`，删除 `sdkconfig` 后重新 `idf.py reconfigure`，避免手工改 `sdkconfig` 被覆盖。

---

## 3. 软件架构

### 3.1 分层

```
┌─────────────────────────────────────────────────────────────┐
│ app 层：启动自检 / 设备状态机 / 事件分发 / 故障仲裁           │
├──────────────┬──────────────┬──────────────┬────────────────┤
│ ui_service   │ sensor_svc   │ fan/filter   │ alarm/led/pwr  │
│ (OLED_UI)    │ (ADC 采样)   │ (档位/寿命)   │ (弹窗/灯/息屏) │
├──────────────┴──────────────┴──────────────┴────────────────┤
│ port 层：ui_port(1bpp→RGB565、脏矩形) / touch→虚拟四键        │
├─────────────────────────────────────────────────────────────┤
│ driver 层：st7789 / cst816t / adc_frontend / fan_ledc / keys  │
├─────────────────────────────────────────────────────────────┤
│ ESP-IDF v6.1：esp_lcd / i2c_master / adc_oneshot / ledc /     │
│ nvs_flash / esp_timer / FreeRTOS / esp_console                │
└─────────────────────────────────────────────────────────────┘
```

设计原则：

- **UI 单线程**：OLED_UI 的所有状态与绘制只在 `ui_task` 中访问，避免其已知的指针/静态变量问题在并发下放大。
- **驱动不碰业务**：驱动只提供 `read/write` 与回调，档位、寿命、告警策略都在 service 层。
- **服务通过事件与快照交互**：高频数据走快照（双缓冲 + 互斥），低频状态变化走事件队列。
- **所有阻塞操作有超时**：I2C/ADC/NVS 均设上限，失败只降级不瘫痪。

### 3.2 组件目录（建议）

```
components/
  bsp/            board_pins.h（唯一引脚真相）、板级初始化、电源保持
  display/        st7789.c（init/window/draw_bitmap）、backlight.c
  touch/          cst816t.c（i2c_master）、touch_gesture.c
  ui/             oled_ui/（vendor，固定 commit）、ui_port.c、pages/
  sensors/        adc_frontend.c、ms1100.c、gp2y1014.c、ntc.c、sensor_service.c
  fan/            fan_ctrl.c（PWM/开关双后端）、fan_auto.c
  filter/         filter_life.c
  alarm/          alarm.c、led_pattern.c
  settings/       settings.c（NVS 封装）
  power/          power_mgr.c（息屏、软关机、唤醒）
  app/            app_state.c、app_events.c、self_test.c
main/             main.c（只负责 create/start，不放业务）
```

每个组件独立 `CMakeLists.txt`；OLED_UI 作为 vendored 组件放在 `components/ui/oled_ui/`，**不改逻辑，只改 port 宏**。

> **已落地**：`components/p169h002/` 是厂商 ESP32-LCD-IDF 例程的移植版，含 ST7789 驱动、GB2312 字库、CST816T 驱动和点屏/触摸 demo。先以单组件完成 M1/M2 实机验证，再按上表拆成 `display/`、`touch/`。

---

## 4. 运行时模型

### 4.1 任务划分

| 任务 | 优先级 | 栈 | 核心 | 触发 | 职责 |
|---|---|---|---|---|---|
| `app_task` | 9 | 5 KB | 0 | 事件队列 | 状态机、故障仲裁、页面切换命令 |
| `ui_task` | 7 | 10 KB | 1 | 20 ms tick + flush 完成信号 | 调 `OLED_UI_InterruptHandler`、渲染、刷屏 |
| `sensor_task` | 6 | 5 KB | 0 | 10 ms `vTaskDelayUntil` | GP2Y 脉冲采样、1 s 聚合、预热/异常判定 |
| `touch_task` | 6 | 4 KB | 1 | INT 下降沿 / 20 ms 轮询 | 读坐标、手势识别、健康心跳 |
| `fan_task` | 5 | 3 KB | 0 | 队列 | 软启动/降档、互锁、PWM 输出 |
| `led_task` | 4 | 3 KB | 0 | 20 ms | 状态灯/电源灯图案、呼吸 |
| `nvs_task` | 3 | 3 KB | 0 | 队列 + 10 min 定时 | 合并写 NVS，降低擦写 |
| `console_task` | 2 | 4 KB | 0 | UART0 | `esp_console` 调试命令（可选，开发期默认开） |

约束：

- `ui_task` 是 **唯一** 允许调用 OLED_UI 的任务；20 ms 定时器回调只做 `xTaskNotifyGive(ui_task)`，不在 `esp_timer` 回调里跑 UI（回调运行在 esp_timer 任务，阻塞会拖垮其他定时器）。
- 所有 ISR 只 `xQueueSendFromISR` / `vTaskNotifyGiveFromISR`，不做 I2C、ADC、日志。
- 若 `ui_task` 因刷屏错过 20 ms tick，按累计 tick 数连续调用 handler（设上限），保持动画/停留时间以真实时间为准。上一帧 flush 未完成时跳过本帧渲染，避免队列堆积。
- 任务栈用 `uxTaskGetStackHighWaterMark()` 在联调期实测后收敛，初始值仅作上限参考。
- 任务看门狗（TWDT）订阅 `app_task`、`ui_task`、`sensor_task`，超时 5 s 触发 panic + coredump；**不允许在低优先级循环里喂狗**。

### 4.2 核心亲和性

- **Core 1**：UI 渲染 + SPI 刷屏 + 触摸。屏刷是长事务，放同一核减少跨核缓存争用。
- **Core 0**：业务、采样、NVS、控制台。ADC/DMA 中断由驱动处理，不额外占核。
- 不强制绑定 `touch_task` 到 Core 1 的 INT 中断（GPIO ISR 可在任一核），但事件消费固定在 Core 1。

### 4.3 数据流

```
GP2Y/MS1100/NTC ──adc_oneshot──► sensor_task ──► snapshot(双缓冲) ──► ui_task
                                     │
                                     └──事件(异常/预热完成)──► app_task ──► alarm
按键/触摸 ──► 事件队列 ──► app_task ──► fan_task / power / ui 命令
风机/滤芯 ──► 快照 ──► 状态栏
```

### 4.4 应用事件

```c
typedef enum {
    APP_EVT_PWR_KEY_SHORT,     // 息屏 ↔ 唤醒
    APP_EVT_PWR_KEY_LONG,      // 3 s 软关机
    APP_EVT_FN_KEY_SHORT,      // 停→弱→中→强→停
    APP_EVT_FN_KEY_LONG,       // 直达滤芯页
    APP_EVT_TOUCH_GESTURE,     // 携带 gesture + 坐标
    APP_EVT_FAN_CHANGED,       // 档位生效
    APP_EVT_SENSOR_READY,      // 预热完成
    APP_EVT_SENSOR_FAULT,      // 通道异常
    APP_EVT_OVERHEAT,          // 过热
    APP_EVT_FILTER_DUE,        // ≤15% 或 =0
    APP_EVT_TOUCH_LOST,        // 触摸通信 30 s 失败
    APP_EVT_SCREEN_TIMEOUT,    // 息屏
} app_evt_id_t;
```

---

## 5. 状态机与故障仲裁

### 5.1 设备状态

| 状态 | 进入条件 | 行为 | 退出 |
|---|---|---|---|
| `SELF_TEST` | 上电/复位 | 屏、触摸、NVS、PSRAM、ADC 合理性自检，≤3 s | 通过→`STANDBY`；失败→`FAULT` |
| `STANDBY` | 自检通过 | 风机停；屏可息屏；状态灯蓝呼吸 | 触摸/按键/风量>停→`RUNNING`；长按电源→`SHUTDOWN` |
| `RUNNING` | 风量>停 | 按档位输出；状态灯蓝常亮 | 风量=停且 30 s 无操作→`STANDBY` |
| `FAULT` | 过热等硬故障 | 风机强制停、红闪、弹窗；拒绝再启动风机 | 故障清除并复位→`STANDBY` |
| `SHUTDOWN` | 长按电源 3 s | 停风机→存 NVS→关背光→屏睡眠→释放电源保持 | 断电；若保持电路未动作则 `deep sleep` 兜底 |

### 5.2 故障优先级与处置

| 优先级 | 事件 | 是否停风机 | 状态灯 | 屏幕 | 恢复条件 |
|---|---|---|---|---|---|
| 1 | 过热（>45 ℃ 持续 3 s） | **是**，先降档再停 | 红快闪 4 Hz | 弹窗 + 灰化数据 | <40 ℃ 持续 30 s 且手动复位 |
| 2 | 传感器异常（超量程/断线/60 s 无变化） | 否 | 红常亮/快闪 | 弹窗 + 该通道置灰 | 读数恢复合理区间 10 s |
| 3 | 通信异常（I2C 触摸/屏） | 否 | 红慢闪 | 文字提示 + 降级 | 通信恢复 |
| 4 | 滤芯到期（≤15% / 0） | 否 | 琥珀常亮/快闪 | 弹窗，24 h 再提醒 | 更换并复位 |
| — | 触摸失效（30 s 无响应） | 否 | 红慢闪 | 「触摸异常，请使用功能键」 | 触摸恢复 |

> 需求 §3.5 把「传感器/通信/滤芯提醒」都列进故障态，§3.7 又要求传感器异常不停机。本设计按 §3.7 执行：**只有过热进入 `FAULT` 并停风机**，其余为警告态；此口径需在评审中确认（见 B-06）。

### 5.3 风量语义

| 档位 | 占空比（建议） | 物理功能键可达 | 说明 |
|---|---|---|---|
| 停 | 0% | 是 | 输出关断，EN 拉低 |
| 弱 | 30% | 是 | 软启动 300 ms |
| 中 | 60% | 是 | 软启动 300 ms |
| 强 | 100% | 是 | 软启动 500 ms |
| 自动 | 由算法输出 | **否**（仅触摸） | 入口浓度 + 迟滞 + 30 s 驻留 |

- 物理键循环严格为 停→弱→中→强→停，不进入自动（R-UI-02）。
- 自动档：取 PM 与 VOC 各自映射档位的较大值；阈值带 ±10% 迟滞，切换最小间隔 30 s，防抖。
- 风机软启动用 `ledc_set_fade_with_time()`；若 MOS 板不支持高频 PWM，则切换为「4 档开关 + 延时」后端，接口不变。

---

## 6. 硬件接口与引脚

### 6.1 屏幕 FPC（P169H002-CTP，手册 §2.0 / §6.0 确认）

| Pin | 符号 | 功能 | 备注 |
|---|---|---|---|
| 1 | GND | 地 | |
| 2 | LEDK | 背光阴极 | **只引出阴极**，阳极在模组内接 VDD；必须低边开关 + 限流 |
| 3 | VDD | 模拟/逻辑电源 | 典型 **3.0 V**，手册最大 3.2 V |
| 4 | VDDIO | IO 电源 | 1.65–3.3 V；图纸标注 1.8 V，**须按实物/供应商确认** |
| 5 | — | 电源（与 3/4 同域，按图纸核对） | |
| 6 | GND | 地 | |
| 7 | D/C | 数据/命令 | |
| 8 | CS | 片选 | |
| 9 | SCL | SPI 时钟 | |
| 10 | SDA | SPI 数据（写方向即 MOSI） | 4 线 SPI 只有这一根双向数据线，**没有独立 MISO** |
| 11 | RESET | 屏复位 | |
| 12 | GND | 地 | |
| 13 | TP_SCL | 触摸 I2C SCL | CST816T |
| 14 | TP_SDA | 触摸 I2C SDA | 与 pin10 是两条独立网络，**禁止并线** |
| 15 | TP_TRST | 触摸复位 | |
| 16 | TP_TINT | 触摸中断 | 低有效（待手册确认） |
| 17 | TP_VDD | 触摸电源 | 3.0 V 域 |
| 18 | GND | 地 | |

要点：

- 模组没有 TE 引脚、没有 IM0/IM1（固定 4 线 SPI）。
- VDD 3.0 V 典型、3.2 V 最大：DevKitC-1 的 3.3 V 轨已越上限，建议 **3.0 V LDO** 或与供应商确认可 3.3 V 工作（B-04）。
- 背光 3 颗 LED、典型 2.8 V / 45 mA：3.0 V 供电时裕量仅 0.2 V，需低边 MOSFET/NPN + 限流电阻或恒流 sink，**禁止 GPIO 直驱**。
- 经 14 pin 转接板时，板内已有背光驱动，`PWR` 为**高有效使能**（厂商例程 `LCD_BLK_Set()` 验证）；不要再按裸 LEDK 做低边接法，除非实测证明 PWR 直连阴极。
- FPC 为 0.5 mm 间距 18 pin，需转接座（H-05）。
- **两根 “SDA” 不是同一根线**：pin 10 是屏的 4 线 SPI 数据线（MCU 驱动，功能等同 MOSI），pin 14 是触摸 I2C 的 SDA。转接板若把两者并在一起，或只引出其中一根，都不能直接使用。
- 转接板必须 **18 pin 1:1 引出**，且 TP_SDA/TP_SCL 保持双向。若板上出现 “只有 MISO、没有 MOSI”，先按 §6.2.1 核对，不要直接接线。

### 6.2 SoC 引脚建议（**待确认**，非最终接线）

| 功能 | 信号 | 建议 GPIO | 电气/说明 |
|---|---|---|---|
| LCD | SCLK | 12 | 3.3 V 逻辑；10 MHz 先起，稳定后试 40 MHz |
| LCD | MOSI | 11 | |
| LCD | CS | 13 | |
| LCD | DC | 14 | |
| LCD | RST | 15 | |
| LCD | PWR/BL | 16 | 厂商例程 BLK 高有效；先 GPIO 高开，确认转接板驱动后可 LEDC 调光 |
| Touch | SDA | 8 | I2C 400 kHz，4.7 kΩ 上拉 |
| Touch | SCL | 9 | |
| Touch | INT | 10 | 低有效，10 kΩ 上拉 |
| Touch | RST | 18 | |
| ADC1 | MS1100_OUT | 1 | ADC1_CH0，**2:1 分压 + RC** |
| ADC1 | PM2.5_OUT | 2 | ADC1_CH1，**2:1 分压 + RC** |
| ADC1 | NTC | 4 | ADC1_CH3，10 kΩ 分压 |
| ADC1 | 差压（预留） | 5 | ADC1_CH4 |
| ADC1 | 备用 | 6 | ADC1_CH5 |
| PM2.5 | LED_DRV | 21 | NPN 基极（1 kΩ），LED 串 150 Ω |
| Fan | PWM | 39 | LEDC 20 kHz，10 bit |
| Fan | EN | 40 | 与 PWM 同板时做使能/方向 |
| Keys | PWR_KEY | 41 | 低有效，内部上拉 + 100 nF |
| Keys | FN_KEY | 42 | 同上 |
| LED | PWR_LED | 47 | 串限流电阻 |
| LED | STAT_LED | 48 | 若 DevKitC-1 板载 WS2812 占用 48，则改 38 或复用板载灯 |
| Buzzer | BUZZ | 7 | 无源蜂鸣器 + 三极管（P2，占用原 ADC1_CH6） |
| 调试 | UART0 TX/RX | 43/44 | 保留，勿占 |
| USB | D-/D+ | 19/20 | USB-Serial-JTAG，保留 |

**禁用/避让**：GPIO0、3、45、46（strapping）；GPIO19/20（USB）；GPIO26–37（Flash + Octal PSRAM）；GPIO43/44（UART0）。GPIO39–42 属 JTAG 引脚，用作风机/按键后仍可用 USB-Serial-JTAG 调试。

**厂商例程引脚（ESP32-C3，仅作对照，不要直接抄到 S3）**

来源：`Doc/touch_screen/.../P169H002/main/Inc/lcd_init.h`、`cst816.h`。厂商工程 target 为 `esp32c3`、4 MB Flash、DIO、SPI 10 MHz。

| 信号 | 厂商 C3 | 本项目 S3 | 重映射原因 |
|---|---|---|---|
| LCD SCK | 2 | 12 | S3 可用 |
| LCD DSA/MOSI | 3 | 11 | **S3 GPIO3 是 strapping**，不能照抄 |
| LCD RST | 19 | 15 | **S3 GPIO19 是 USB D-**，不能照抄 |
| LCD DC | 18 | 14 | S3 可用 |
| LCD CS | 7 | 13 | S3 可用 |
| LCD BL/PWR | 1 | 16 | 厂商以 BLK 高电平开背光 |
| Touch SDA | 4 | 8 | S3 可用 |
| Touch SCL | 5 | 9 | S3 可用 |
| Touch RST | 10 | 18 | S3 可用 |
| Touch INT | 6 | 10 | 厂商声明未使用，本项目预留 |
| MISO | -1 | -1 | 4 线 SPI 无 MISO |

厂商触摸参数：地址 `0x15`、坐标寄存器 `0x03/0x04` 与 `0x05/0x06`、Y 偏移 `+5`、芯片 ID 寄存器 `0xA7`、自动睡眠 `0xF9=5 s`。厂商用软件模拟 I2C（SDA/SCL 位翻转）；本项目改用 ESP-IDF `i2c_master` 硬件外设。

> 该表是软件侧建议，**必须以 DevKitC-1 具体版本原理图 + FPC 转接板实际走线为准**再定稿（B-01）。

### 6.2.1 转接板核对（针对“只有 MISO、没有 MOSI”）

先判断这块板子属于哪一类：

| 现象 | 判断 | 处理 |
|---|---|---|
| 18 pin 全引出，“MISO”与 FPC pin 10 导通 | 丝印把 4 线 SPI 的单根双向 SDA 写成了 MISO | 该焊盘按 **SDA/MOSI** 使用；MCU 的 MISO 不接 |
| 只有 4/6 pin，丝印含 CS/SCLK/MISO（或 T_CS/T_DO） | 这是 **SPI 触摸控制器**（如 XPT2046/ADS7843）转接板 | 与 CST816T 不兼容，不能使用；换板或飞线 |
| 有 SDA/SCL 但串了单向缓冲/电平转换 | I2C 无法完成“先写寄存器地址再读” | 不能用；换双向电平转换或直连 |
| “MISO”与 FPC pin 14 导通、且 pin 10 未引出 | 只引出了触摸 I2C 的 SDA，且命名错误 | 可当 I2C SDA 用，但必须确认双向；缺 TP_SCL 则不可用 |

核对步骤：

1. 万用表通断档，逐针对照 FPC pin 1–18 与转接板排针，做一张对应表（重点：pin 9、10、13、14、15、16）。
2. 看板上是否有 74LVC/电平转换、单向缓冲、串联电阻；I2C 的 SDA/SCL 必须双向且有上拉。
3. 若“MISO”实际连 pin 14，说明板子把触摸 SDA 命名成 MISO；它只能当 I2C SDA，不能当 SPI MISO。
4. 若转接板缺 pin 13（TP_SCL）或 pin 14（TP_SDA），直接飞线到 FPC 座对应引脚，绕过转接板的触摸部分。

> 结论：**CST816T 没有 MOSI/MISO**（它是 I2C）；屏的 4 线 SPI 也**没有独立 MISO**（只有 pin 10 一根双向 SDA）。任何“只有 MISO”的转接板，都必须先证明它对应的是哪一根线，不能按 SPI 直觉接线。

#### 14 pin 转接板映射（2×7，按原理图 PZ254V-12-14P 确认）

**这块板不是 1×14 单排，而是 2×7 双排**：奇数脚在右列，偶数脚在左列。用户最初给出的 `TRST TINT TMISO GND PWR SCK DC TSCL BUSY TSDA VCC CS DSA RST` 是“右列从上到下 + 左列从上到下”的**标签阅读顺序**，不是物理脚 1→14 的顺序。按单排顺序接线会整体错位。

| 排针 | 列 | 原理图网络 | 功能 | 接 ESP32 |
|---|---|---|---|---|
| 1 | 右 | TP_TRST | 触摸复位 | GPIO18 |
| 2 | 左 | TP_SCL | 触摸 I2C SCL | **GPIO9** |
| 3 | 右 | TP_TINT | 触摸中断 | GPIO10 |
| 4 | 左 | RT_BUSY | 电阻屏 BUSY，**NC** | 不接 |
| 5 | 右 | RT_MISO | 电阻屏 MISO，**NC** | 不接 |
| 6 | 左 | TP_SDA | 触摸 I2C SDA | **GPIO8** |
| 7 | 右 | GND | 地 | GND |
| 8 | 左 | VCC | 3.0–3.2 V | 3.0 V LDO |
| 9 | 右 | LCD_PWR | 背光使能 | GPIO16（高有效） |
| 10 | 左 | LCD_CS | 屏片选 | GPIO13 |
| 11 | 右 | LCD_SCK | 屏 SPI 时钟 | GPIO12 |
| 12 | 左 | LCD_SDA | 屏数据（MOSI） | GPIO11 |
| 13 | 右 | LCD_DC | 命令/数据 | GPIO14 |
| 14 | 左 | LCD_RST | 屏复位 | GPIO15 |

原理图要点：

- 绿 X / 红菱形只出现在 **RT_BUSY（4）** 和 **RT_MISO（5）** 上，表示这两脚 NC；它们是电阻屏/SPI 触摸变体预留，CST816T（I2C）不用。`TP_SCL/TP_SDA/TP_TRST/TP_TINT` 都是正常连接的。
- 原理图里没有上拉、下拉、三极管或电平转换，说明这是一块**无源转接板**；I2C 上拉、背光限流/驱动都要由外部或屏模组自己提供。
- **DSA（LCD_SDA，12）与 TSDA（TP_SDA，6）是两条独立数据线，禁止并线；SCK（11）与 TSCL（2）也是两条独立时钟。**
- **PWR 已由厂商例程定性为高有效背光使能**（`LCD_BLK_Set()` 拉高开背光）；但本转接板无背光驱动，若实测 `LCD_PWR` 直连 LED 阳极/阴极，需按实际极性加驱动，不能直接 GPIO 灌 45 mA。
- 接线前先确认排针 **1 脚标记**（通常在 TP_TRST 这一端）；如果从另一端数，奇偶列会全部对调。

### 6.3 新增硬件缺口（本文档发现，需求文档未列）

| 编号 | 缺口 | 后果 | 建议 |
|---|---|---|---|
| H-08 | 5 V 模拟输出未分压 | 直接接 ADC 会过压损坏 | 每通道 10 k/10 k 分压 + 1 k/100 nF RC + 钳位 |
| H-09 | 只有 1 套传感器 | 无法同时测「处理前/后」 | 补 1×MS1100 + 1×GP2Y1014AU |
| H-10 | 屏 VDD 3.0 V / 背光驱动 | 3.3 V 越上限；裸屏 GPIO 直驱会烧 | 3.0 V LDO；裸屏低边恒流，14 pin 转接板按 PWR 高有效使能 |
| H-11 | 按键/LED/蜂鸣器不在采购清单 | 物理交互无法验收 | 补 2 按键 + 2 灯 + 蜂鸣器 |

---

## 7. 驱动设计

### 7.1 显示：ST7789T3（先 spi_master，后迁 esp_lcd）

**当前落地（M1）**：直接移植厂商 ESP32-LCD-IDF 例程的最小 ST7789 驱动，用 `spi_master` 轮询发送，10 MHz、mode 0、half-duplex、`MISO=-1`。初始化序列已由厂商例程验证，代码在 `components/p169h002/src/lcd_init.c`：`0x11` sleep out → `0x36=0x00`（竖屏）→ `0x3A=0x05`（RGB565）→ porch/gamma/power → `0x21` 反色开 → `0x29` display on；窗口写入 `0x2A/0x2B/0x2C`，竖屏 `y+20`。

**后续（UI 阶段）**：迁移到 `esp_lcd` 的 SPI panel IO + DMA + 脏矩形，保留同一套初始化序列。迁移前先用彩条在实机确认 BGR/反色/偏移。

```c
// display/st7789.h
esp_err_t st7789_init(const st7789_config_t *cfg);      // SPI 总线 + panel_io + init sequence
esp_err_t st7789_set_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);
esp_err_t st7789_draw_bitmap(uint16_t x0, uint16_t y0, uint16_t w, uint16_t h, const void *rgb565);
esp_err_t st7789_sleep(bool on);
esp_err_t st7789_backlight_set(uint8_t percent);         // 0..100
```

关键参数与验证点：

| 项 | 建议值 | 说明 |
|---|---|---|
| SPI 主机 | SPI2_HOST | 专用总线，不与其它设备共享 |
| 时钟 | 10 MHz 起步（厂商例程值），稳定后试 40 MHz | 80 MHz 超 ST7789 常规规格，需实测误码 |
| 模式 | SPI mode 0 | |
| DMA | `trans_queue_depth = 10` | 刷屏走 DMA，配 `on_color_trans_done` 回调 |
| 命令/参数位宽 | 8/8 | `lcd_cmd_bits=8`、`lcd_param_bits=8` |
| 颜色 | RGB565，`MADCTL` 注意 BGR 位 | 红蓝反了就是 BGR/RGB 位错 |
| 纵向偏移 | `Y_OFFSET = 20` | 厂商例程同样 `y+20`；**用彩条实测**，偏移错会上下错位 |
| 反色 | `0x21` 开 | 厂商序列开启；若黑白反了先查这里 |
| 初始化序列 | 厂商 P169H002 例程 | 已移植到 `lcd_init.c`，不再猜时序 |
| 分辨率差异 | 本项目按 240×280 | 厂商按 `LCD_H=284` 写；多出的 4 行是否在可见区外需实测，本项目先取 280 |

注意：

- `esp_lcd_new_panel_io_spi()` 只负责命令/数据切换、CS/DC 与 DMA 队列；窗口命令需自行发 `CASET(0x2A)`、`RASET(0x2B)`、`RAMWR(0x2C)`。
- 背光 PWM 用 LEDC 20 kHz，10 bit；3 档亮度映射 30/60/100%，开关加 200 ms 渐变，避免突变。
- 息屏 = 背光关 + `SLPIN(0x10)`；唤醒 = `SLPOUT(0x11)` + 120 ms 延时 + 背光渐亮。
- 屏与触摸共用 FPC，**排线松动会同时表现为花屏 + 触摸失效**，自检要能区分。

### 7.2 触摸：CST816T（I2C，**新驱动 API**）

厂商例程用 GPIO 位翻转模拟 I2C（`iic_hal.c`，2 µs 延时）。本项目移植时改为 IDF v6.1 的 `i2c_master` 硬件外设，地址与寄存器沿用厂商值：

```c
i2c_master_bus_config_t bus = {
    .i2c_port = I2C_NUM_0,
    .sda_io_num = BSP_TOUCH_PIN_SDA,   // GPIO8
    .scl_io_num = BSP_TOUCH_PIN_SCL,   // GPIO9
    .clk_source = I2C_CLK_SRC_DEFAULT,
    .glitch_ignore_cnt = 7,
    .flags.enable_internal_pullup = true,
};
i2c_new_master_bus(&bus, &s_bus);

i2c_device_config_t dev = {
    .dev_addr_length = I2C_ADDR_BIT_LEN_7,
    .device_address = 0x15,            // 厂商例程确认
    .scl_speed_hz = 400000,
};
i2c_master_bus_add_device(s_bus, &dev, &s_dev);
```

从厂商例程提取的寄存器表：

| 寄存器 | 名称 | 用途 |
|---|---|---|
| `0x01` | GestureID | 手势：0x01 下滑 / 0x02 上滑 / 0x03 左滑 / 0x04 右滑 / 0x05 单击 / 0x0B 双击 / 0x0C 长按 |
| `0x02` | FingerNum | 手指数量；`0xFF` 表示睡眠 |
| `0x03/0x04` | X 高位/低位 | `X = ((b0 & 0x0F) << 8) | b1` |
| `0x05/0x06` | Y 高位/低位 | `Y = (((b2 & 0x0F) << 8) | b3) + 5` |
| `0xA7` | ChipID | 通信/健康自检 |
| `0xE5` | SleepMode | 写 `0x03` 进入睡眠 |
| `0xF9` | AutoSleepTime | 厂商写 5（秒） |

本项目驱动接口（`components/p169h002/include/cst816.h`）：

```c
esp_err_t cst816_init(void);
esp_err_t cst816_read_touch(cst816_touch_t *out);   // 一次读 0x02..0x06，含坐标/按下
esp_err_t cst816_read_gesture(uint8_t *gesture);   // 0x01..0x0C
esp_err_t cst816_read_chip_id(uint8_t *id);        // 0xA7，健康心跳用
```

要点：

- 触摸坐标 `Y+5` 来自厂商例程（`TOUCH_OFFSET_Y 5`）；屏幕本身 `y+20`，两者不要混淆。
- 厂商声明了 `TOUCH_INT_PIN=6` 但实际未使用，靠轮询坐标；本项目把 INT 接到 GPIO10 仅作预留，demo 仍是 10 ms 轮询。
- 厂商 demo 判断 `FingerNum > 0` 会把睡眠值 `0xFF` 误当成按下；移植版已把 `0xFF` 归零。
- 坐标需做镜像/交换标定（`swap_xy`、`mirror_x`、`mirror_y`），与屏幕旋转一起验证。
- **触摸失效判定不能用「30 s 没人触摸」**，否则正常待机就误报；应每 500 ms 读一次芯片 ID/状态寄存器，连续 30 s 通信失败才置 `APP_EVT_TOUCH_LOST`。

### 7.3 模拟前端（MS1100 / GP2Y1014AU / NTC）

统一使用 `adc_oneshot` + 曲线拟合校准：

```c
adc_oneshot_unit_init_cfg_t init = { .unit_id = ADC_UNIT_1 };
adc_oneshot_new_unit(&init, &s_adc);
adc_oneshot_chan_cfg_t ch = {
    .atten = ADC_ATTEN_DB_12,        // 约 0–3.3 V
    .bitwidth = ADC_BITWIDTH_12,
};
adc_oneshot_config_channel(s_adc, ADC_CHANNEL_0, &ch);
adc_cali_curve_fitting_config_t cal = {
    .unit_id = ADC_UNIT_1, .chan = ADC_CHANNEL_0,
    .atten = ADC_ATTEN_DB_12, .bitwidth = ADC_BITWIDTH_12,
};
adc_cali_create_scheme_curve_fitting(&cal, &s_cali);
```

**GP2Y1014AU 采样时序**：

- 手册要求脉冲周期 10 ms、脉宽 0.32 ms、LED 点亮后 0.28 ms 采样。
- **正式方案**：LEDC 输出 100 Hz / 3.2%（320 µs）驱动 LED；用 GPTimer 记录脉冲边沿时间戳，`sensor_task` 按时间戳补偿到边沿 +280 µs 再读 ADC。**ADC oneshot 不是 ISR 安全 API，禁止在定时器中断里直接调用。**
- **软件点验路径**（仅调试）：`LED_DRV` 拉高 → `esp_rom_delay_us(280)` → 读 ADC → 拉低。注意 oneshot 转换需数十至上百 µs，实际脉宽会超过 320 µs 上限，不能作为量产时序。
- 1 s 聚合 100 个点，去极值后取均值。

**换算（占位，待手册/标定）**：

- GP2Y：传感器输出电压 `Vout = adc_voltage × divider_ratio`；粉尘浓度 `mg/m³ = k × Vout + b`，Sharp 常用近似 `0.17 × Vout − 0.1`，**必须用 NVS 中的 k/b 标定值覆盖**，并钳位到 ≥0。
- MS1100：`Rs = (Vc − Vout) / Vout × RL`（Vc=5 V，RL 按模块），`R0` 为洁净空气基准，`ppm = a × (Rs/R0)^b`，a/b 待手册；本期 UI 标注「气态综合（TVOC 等效）」，不分甲醛/苯。
- NTC 10K B3950：`T = 1 / (1/T0 + ln(R/R0)/B) − 273.15`，10 k 上拉分压。

**滤波与有效性**：

- 每通道 8 点去极值均值；1 s 发布一次。
- 合理性区间：ADC 原始码不在 `[50, 4000]` 连续 3 s → 通道异常；风机运行中 60 s 方差低于阈值 → 卡死异常。
- 预热：MS1100 180 s、GP2Y 60 s，期间 UI 显示「预热中」，不显示数值。
- 净化效率 `η = (Cin − Cout) / Cin × 100%`，入口低于噪声门限（PM < 5 µg/m³ 或 VOC < 0.05 ppm）时显示「—」。

### 7.4 风机（LEDC）

```c
typedef enum { FAN_STOP = 0, FAN_LOW, FAN_MID, FAN_HIGH, FAN_AUTO } fan_level_t;
esp_err_t fan_init(void);
esp_err_t fan_set_level(fan_level_t level);   // 内部做软启动与互锁
fan_level_t fan_get_level(void);
void fan_force_stop(void);                    // 过热/关机用，绕过普通互锁
```

- LEDC：20 kHz、10 bit、定时器 0、通道 0；占空比 0/30/60/100%。
- 软启动 300–500 ms；启动失败（无转速反馈）暂无法检测，风机规格书到位后补转速/电流反馈。
- 复用 `EN` 做硬关断；关机顺序严格为 **PWM 归零 → EN 拉低 → 延时 200 ms**。
- 过热联锁：`fan_force_stop()` 后置 `overheat_latched`，任何手动/自动请求都被拒绝，直到温度恢复且用户确认。

### 7.5 按键、指示灯、蜂鸣器

- 按键 10 ms 扫描、30 ms 消抖、3 s 长按；电源键 10 s 强制断电由**硬件自锁电路**完成，固件只负责 3 s 软关机（H-06）。
- 按键扫描放在 `app_task` 的 10 ms 定时分支，不单独占任务；或由 `esp_timer` 通知。
- 指示灯用 20 ms 图案引擎（`led_pattern.c`），支持常亮/呼吸/双闪/快闪；三色灯按需求 §3.6，单色灯按闪烁编码。
- 蜂鸣器 P2：短鸣 80 ms 用于触摸反馈，告警时 3 短鸣；音量不可调。

### 7.6 厂商 P169H002 例程移植（已落地）

源目录：`Doc/touch_screen/产品资料/ESP32/程序源码/ESP32-LCD-IDF/ESP32-LCD/ESP32-LCD/P169H002`。

| 厂商文件 | 本项目位置 | 处理 |
|---|---|---|
| `lcd_init.c/h` | `components/p169h002/src/lcd_init.c`、`include/lcd_init.h` | 重写为 S3 引脚，保留初始化序列；SPI 10 MHz |
| `lcd.c/h` | `components/p169h002/src/lcd.c`、`include/lcd.h` | 原样移植：填充/线/圆/ASCII/汉字/彩条/灰度 |
| `lcdfont.h` | `components/p169h002/include/lcdfont.h` | 原样移植（GB2312 点阵）；对 `unterminated-string-initialization` 按警告处理 |
| `cst816.c/h` | `components/p169h002/src/cst816.c`、`include/cst816.h` | 保留寄存器/坐标解析，I2C 改为 `i2c_master` |
| `iic_hal.c/h` | 不保留 | 由硬件 I2C 替代 |
| `lcd_demo.c/h` | `components/p169h002/src/demo.c`、`include/demo.h` | 重写：标题 → 纯色 → 彩条 → 灰度 → 触摸画板 |
| `pic.h`（2.1 MB logo） | 未移植 | 避免 2.1 MB rodata 占用 app 分区；需要时再加 |
| `main.c` | `main/main.c` | 只调用 `p169h002_demo()` |

构建结果（2026-10-25）：

- target `esp32s3`，ESP-IDF v6.1.0，`idf.py build` 通过。
- `air_forge.bin` 0x3D520（251,168 B）；单 APP 分区 1 MB，余 76%。

**实机 bring-up 记录（2026-10-26，COM9，ESP32-S3 rev0.2）**：

- 烧录成功；LCD 初始化日志正常：`240x280, y_offset=20, SPI=10 MHz`，引脚 `SCLK=12 MOSI=11 CS=13 DC=14 RST=15 BL=16`。
- CST816T：`chip id read failed: ESP_ERR_INVALID_RESPONSE`；I2C 全地址扫描（0x08–0x77）无设备；把 SDA/SCL 交换后重试仍无设备。
- **TRST 已排除**：新固件（含线测）在 COM10 复测结果为 `pull-up=1 pull-down=0`（高阻，正常），`TRST level after release: 1`（复位已释放）。旧日志的 `TRST=0` 确认是 `GPIO_MODE_OUTPUT` 关闭输入缓冲导致的回读失真，不是硬件短路。
- **原理图确认转接板是 2×7（PZ254V-12-14P）**，不是 1×14：TP_TRST=1、TP_SCL=2、TP_TINT=3、RT_BUSY=4(NC)、RT_MISO=5(NC)、TP_SDA=6、GND=7、VCC=8、LCD_PWR=9、LCD_CS=10、LCD_SCK=11、LCD_SDA=12、LCD_DC=13、LCD_RST=14。按单排 1→14 接线会错位。
- **但 I2C 仍无 ACK**：全地址扫描无设备，交换 SDA/SCL 后也无设备。软件侧（SPI/GPIO/I2C 初始化、寄存器读写、扫描）基本排除，问题收敛到触摸物理层：2×7 触摸组接线、TP_VCC/GND、TP_SCL/TP_SDA、上拉或触摸 IC 本身。
- 仍待实机确认：LCD 是否实际显示（用于判断显示组是否接对）、TP_VCC 是否为 3.0 V、触摸 IC 是否为 CST816T、触摸坐标。

移植时修正的点：

1. 厂商 C3 的 GPIO3/19 在 S3 上是 strapping/USB，已重映射（§6.2）。
2. 厂商 demo 判断 `FingerNum > 0` 会把睡眠值 `0xFF` 当成按下，已修正为 `0xFF → 0`。
3. 厂商 demo 多处 `LCD_Fill(0,0,W,H,...)` 越界 1 行/列，移植版统一用 `W-1/H-1`。
4. 厂商 GB2312 字库不认 UTF-8 字面量；移植版用 GB2312 字节数组显示“浦洋液晶”。

---

## 8. UI 方案：OLED_UI 适配层

### 8.1 适配边界

```
┌──────────────────────────────────────────────┐
│ pages/：P0–P5、W 弹窗（业务与文案）           │
├──────────────────────────────────────────────┤
│ OLED_UI 逻辑层（vendor，固定 commit）         │
│ MenuPage / MenuItem / MenuWindow / 动画       │
├───────────────────┬──────────────────────────┤
│ ui_port_display   │ ui_port_input            │
│ 1bpp → RGB565     │ 触摸 → 虚拟四键           │
│ 脏矩形 + DMA      │ 单击/滑动/双击/长按        │
├───────────────────┴──────────────────────────┤
│ st7789 / cst816t                              │
└──────────────────────────────────────────────┘
```

只允许修改 OLED_UI 的三个 port 点：

1. `OLED_Update()` → `ui_port_flush(fb, w, h)`；
2. 四键读取宏 → `ui_port_read_keys()` 返回 `UP/DOWN/ENTER/BACK` 位图；
3. 20 ms tick → `ui_task` 调用 `OLED_UI_InterruptHandler()`。

### 8.2 显示适配

| 项 | 值 |
|---|---|
| 分辨率宏 | `OLED_WIDTH=240`、`OLED_HEIGHT=280` |
| 1 bpp 帧缓冲 | 240×280/8 = **8.4 KB**（内部 RAM） |
| 上一帧副本 | 8.4 KB（用于差分） |
| RGB565 条带缓冲 | 240×24×2 = **11.5 KB × 2**（内部 DMA 可访问） |
| 可选整帧 | 134.4 KB（PSRAM，仅截图/调试） |
| 刷新策略 | 逐 8 行带比较，合并为脏矩形；按条带 `draw_bitmap` |
| 目标帧率 | ≥25 fps（脏面积 <60% 时） |

带宽核算（SPI 40 MHz，含命令开销）：

- 整帧 134.4 KB ≈ 27 ms → 理论上限 ~37 fps，实际受脏矩形与队列开销影响；
- 典型页面（状态栏 + 数值变化）脏面积 <20%，单帧 <6 ms，25 fps 有余量；
- 若 40 MHz 下不稳，先降到 30 MHz 再查排线/地回流，**不要直接上 80 MHz**。

颜色映射：1 bpp 的 1/0 映射到前景/背景。为保留彩色观感，适配层按页面状态选调色板：

| 场景 | 背景 | 前景 |
|---|---|---|
| 常规页面 | `#000000` | `#00E5FF` |
| 滤芯提醒 | `#000000` | `#FFB000` |
| 故障/过热 | `#000000` | `#FF3B30` |
| 反色光标 | 前背景互换 | |

### 8.3 触摸 → 虚拟四键

OLED_UI 只有 `UP/DOWN/ENTER/BACK`，因此映射必须遵循「先定位、再确认」：

| 手势 | 映射 |
|---|---|
| 点击数列表项 | 按 y 计算行号 → 发送 `UP/DOWN` 使光标到该行 → 发送 `ENTER` |
| 上/下滑 | 连续 `UP/DOWN`（按滑动距离折算步数） |
| 左滑 | `BACK` |
| 双击（主页面） | 直接调用风量循环（等价功能键），不走四键 |
| 长按 1.5 s（主页面空白） | 打开 `P5 设置` |
| 返回区（左上角 44×44） | `BACK` |

实现要点：

- 需要一份「页面布局注册表」（每页行高、可见行数、首行索引），因为 OLED_UI 不保证暴露几何信息；由 `pages/` 在进入页面时登记。
- 虚拟按键按下保持 2 个 tick（40 ms）再释放，保证 OLED_UI 采样到；**一次手势只产生一个语义事件**。
- 命中区 ≥44×44 px、相邻可点击元素间距 ≥8 px；列表行高按 48 px 设计，状态栏图标区同样满足最小命中。
- 触摸事件在 `touch_task` 识别，`ui_task` 消费；不做跨任务直接改 UI 状态。
- 触摸反馈：命中即反色，≤100 ms；滑动过程不触发点击（位移阈值 12 px）。

### 8.4 页面落地

| 页面 | 实现方式 | 说明 |
|---|---|---|
| P0 启动页 | `MenuPage EMPTY` + 自定义绘制 | Logo + 自检进度，≤3 s，可触摸跳过 |
| P1 主页面 | 自定义绘制 + `TILES` 入口 | 状态栏 32 px；2×2 图标；底部时长 |
| P2 风量 | `MenuPage LIST` | 5 项，当前项反色；切换后 2 s 提示窗 |
| P3 滤芯寿命 | `MenuPage LIST` + 进度条 | 4 条寿命、剩余天数、长按复位 |
| P4 实时数据 | 自定义绘制 | 前后数值 + 效率 + 60 s 迷你趋势（60 点环形缓冲） |
| P5 设置 | `MenuPage LIST` | 亮度/息屏/单位/校准/恢复出厂/关于 |
| W 弹窗 | `MenuWindow` | 档位、换芯、传感器、过热、恢复出厂、关机确认 |

字体与主题：

- 正文 16 px（拉丁/数字）、标题 20 px；中文内置 12×12，正文中文可读性偏弱，**建议补 16×16 中文子集**（按实际文案裁剪，放 Flash rodata）。
- 中文标题用 12×12 做 2× 整数放大（24 px）作为过渡方案；正式版再换 16×16。
- 所有文案集中到 `pages/strings.c`，便于换字体与做中英切换。

### 8.5 风险与止损门槛

| 风险 | 措施 |
|---|---|
| OLED_UI 在 ESP32 上有已知指针 bug | 固定 commit（记录 SHA）；开启 stack protector、heap poisoning、coredump；连续翻页 1 h 压测 |
| 帧率不足 | 脏矩形 + 双条带 DMA；先 40 MHz，必要时降分辨率策略（不做缩放） |
| 触摸映射复杂 | 先做「列表页 tap = 当前光标 ENTER」的最小可用版，再补按行定位 |
| 字体版权 | 使用 SIL OFL 字体（如 Noto Sans SC）子集，记录来源与许可 |

**方案切换门槛**（沿用需求 §4.4）：若适配层稳定后仍 <15 fps，或 100 次操作误触 >3 次，则切换 **LVGL v9 + `esp_lvgl_adapter`**；切换前必须核对组件对 IDF v6.1 的依赖范围。

---

## 9. 业务服务

### 9.1 传感器服务

- 采样：GP2Y 100 Hz 脉冲、MS1100 与 NTC 1 Hz；1 s 发布快照。
- 预热：上电 180 s（MS1100）/60 s（GP2Y），UI 显示倒计时；预热期间不参与告警与效率计算。
- 异常：超量程、断线（ADC 近 0/满量程）、长时间无变化；置灰对应通道。
- 效率：入口/出口 60 s 滑动平均后计算；入口低于门限显示「—」。
- **双点测量依赖第二套传感器**（B-02）；在补齐前，`out` 通道恒为无效，UI 显示「未配置」。

### 9.2 滤芯寿命

| 级 | 默认容量（小时，待确认） | 说明 |
|---|---|---|
| 一级 金属网 | 300 | 可水洗，复位即满 |
| 二级 中效滤棉 | 200 | |
| 三级 A 活性炭（甲醛） | 150 | |
| 三级 B 活性炭（苯系物） | 150 | |

- 消耗速率 = 运行分钟 × 档位权重（弱 0.6 / 中 1.0 / 强 1.4 / 自动按实际档位）。
- `剩余% = 100 × (1 − 已用等效分钟 / (容量小时×60))`；剩余天数按「日均使用 4 h」估算并标注「估算」。
- 阈值：≤15% 提醒（琥珀常亮）；=0 到期（琥珀快闪）；每 24 h 再提醒一次，可「稍后提醒」。
- 持久化：每 10 min 或状态切换时由 `nvs_task` 合并写入，掉电最多丢 10 min。

### 9.3 告警服务

| 事件 | 触发 | 呈现 | 抑制 |
|---|---|---|---|
| 换芯提醒 | 任一 ≤15% 或 0 | 弹窗 + 琥珀 | 24 h 内可稍后 |
| 传感器预热 | 上电未稳定 | 「预热中」占位 | 否 |
| 传感器异常 | 超量程/断线/无变化 | 弹窗 + 红 + 置灰 | 否 |
| 过热 | >45 ℃ 3 s | 降档→停风机 + 红闪 + 弹窗 | 否 |
| 触摸失效 | 触摸通信 30 s 失败 | 红慢闪 + 文字提示 | 否 |

### 9.4 电源与息屏

- 息屏：无操作 60 s（可设 30/60/常亮）；背光渐灭 + `SLPIN`。
- 唤醒：任意触摸或按键唤醒；**唤醒的第一个事件被吞掉**，不触发业务。
- 软关机：停风机 → 写 NVS → 关背光 → 屏睡眠 → 释放电源保持；若保持电路未生效，进入 `deep sleep` 并配置电源键 `ext1` 唤醒兜底。
- 电源灯：上电且主控运行常亮；软关机前熄灭。状态灯在软关机后全灭。

### 9.5 NVS 规划

| namespace | 键 | 类型 | 说明 |
|---|---|---|---|
| `cfg` | `fan_lvl` / `bright` / `screen_to` / `unit` / `auto_profile` | u8 | 用户设置 |
| `filter` | `min_metal` / `min_mid` / `min_carb_a` / `min_carb_b` | u32 | 已用等效分钟 |
| `cal` | `ms_r0` / `pm_k` / `pm_b` / `adc_off` | f32/u16 | 标定值 |
| `stat` | `total_min` / `boot_cnt` | u32 | 累计运行 |
| `sys` | `filter_reset_ts` / `warn_ts` | u32 | 提醒抑制时间 |

写入策略：只在 `nvs_task` 中写；每次 `nvs_set_*` 后批量 `nvs_commit()`；禁止在 UI 任务或定时器回调中写。

---

## 10. sdkconfig 与分区

### 10.1 `sdkconfig.defaults`（按 IDF v6.1 键名）

```ini
# Flash / 启动
CONFIG_ESPTOOLPY_FLASHSIZE_16MB=y
CONFIG_ESPTOOLPY_FLASHSIZE="16MB"
CONFIG_ESPTOOLPY_FLASHMODE_QIO=y
CONFIG_ESPTOOLPY_FLASHFREQ_80M=y
CONFIG_PARTITION_TABLE_CUSTOM=y
CONFIG_PARTITION_TABLE_CUSTOM_FILENAME="partitions.csv"

# CPU / PSRAM（N16R8）
CONFIG_ESP32S3_DEFAULT_CPU_FREQ_240=y
CONFIG_SPIRAM=y
CONFIG_SPIRAM_MODE_OCT=y
CONFIG_SPIRAM_SPEED_80M=y
CONFIG_SPIRAM_USE_MALLOC=y
CONFIG_SPIRAM_MALLOC_ALWAYSINTERNAL=16384
CONFIG_SPIRAM_MALLOC_RESERVE_INTERNAL=32768

# RTOS / 看门狗 / 崩溃
CONFIG_FREERTOS_HZ=1000
CONFIG_ESP_TASK_WDT_EN=y
CONFIG_ESP_TASK_WDT_TIMEOUT_S=5
CONFIG_ESP_TASK_WDT_PANIC=y
CONFIG_ESP_COREDUMP_ENABLE_TO_FLASH=y

# 日志 / 编译
CONFIG_LOG_DEFAULT_LEVEL_INFO=y
CONFIG_LOG_MAXIMUM_LEVEL_DEBUG=y
CONFIG_ESP_MAIN_TASK_STACK_SIZE=4096
CONFIG_COMPILER_OPTIMIZATION_PERF=y
CONFIG_COMPILER_STACK_CHECK_MODE_STRONG=y

# 开发期安全兜底（发布前复核）
CONFIG_HEAP_POISONING_COMPREHENSIVE=y
```

说明：

- QIO 若在样机上不稳，退 `DIO`；先保证能启动，再追性能。
- `CONFIG_SPIRAM_MODE_OCT` 会占用 GPIO33–37，引脚规划必须避开。
- `CONFIG_COMPILER_OPTIMIZATION_PERF` 为 UI 帧率服务；若 Flash 紧张再评估 `-Os`。
- heap poisoning 有性能开销，发布版关闭。

### 10.2 `partitions.csv`（16 MB，双 OTA）

```csv
# Name,   Type, SubType,  Offset,   Size,     Flags
nvs,      data, nvs,      0x9000,   0x6000,
otadata,  data, ota,      0xF000,   0x2000,
phy_init, data, phy,      0x11000,  0x1000,
ota_0,    app,  ota_0,    0x12000,  0x300000,
ota_1,    app,  ota_1,    0x312000, 0x300000,
coredump, data, coredump, 0x612000, 0x10000,
storage,  data, spiffs,   0x622000, 0x9DE000,
```

- APP 各 3 MB：含中文子集字体与 UI 资源后仍有余量；不足时先裁字体，不改分区。
- `storage` 约 9.8 MB：放日志导出、字体/图片资源、OTA 下载缓存（若后续做 HTTPS OTA）。
- `coredump` 64 KB：panic 后 `idf.py coredump-info` 可直接定位。
- 若确定不做 OTA，可退单 APP 并把空间并入 storage；**但需求预留 Wi-Fi 上报，建议保留双 OTA**。

---

## 11. 构建、烧录与调试

当前 shell 中 `idf.py` 不在 PATH。**不要用 `C:\esp\v6.1\esp-idf\export.ps1`**：它指向不存在的 `C:\Users\q1325\.espressif\python_env\idf6.1_py3.13_env`，会报 “Python virtual environment not found”。本机实际由 EIM 安装，正确入口是：

```powershell
. C:\Espressif\tools\Microsoft.v6.1.PowerShell_profile.ps1
idf.py --version          # 应输出 ESP-IDF v6.1

# 首次应用新配置（sdkconfig 是生成物，建议删除后重建）
Remove-Item .\sdkconfig -ErrorAction SilentlyContinue
idf.py fullclean
idf.py reconfigure
idf.py build

# 烧录与监视（端口按实际修改）
idf.py -p COM5 -b 921600 flash monitor
```

EIM 环境细节：`IDF_PATH=C:\esp\v6.1\esp-idf`、`IDF_TOOLS_PATH=C:\Espressif`、Python venv=`C:\Espressif\tools\python\v6.1\venv`。

**已实测**：2026-10-25 用上述入口执行 `idf.py build` 通过，生成 `build/air_forge.bin` 0x3D520（251,168 B）。当前 `sdkconfig` 仍是 2 MB / 单 APP / PSRAM 关闭，所以构建产物按 1 MB app 分区校验；切到 16 MB 双 OTA 前需要按 §10 改配置并 `fullclean`。

调试要点：

| 场景 | 命令/手段 |
|---|---|
| 查看分区与 Flash 占用 | `idf.py size`、`idf.py size-components` |
| 崩溃定位 | panic 后 `idf.py coredump-info` + `idf.py coredump-debug` |
| 堆/栈水位 | 控制台命令 `heap`、`tasks` |
| 日志级别 | `log <tag> <level>` 运行时调整，无需重烧 |
| 点屏自检 | 彩条 + 四角标记 + 红绿蓝顺序检查（同时验证 BGR/偏移） |
| SPI 波形 | 40 MHz 下先看 CS/DC 时序与地回流，再考虑提速 |

控制台命令（`esp_console`，UART0 115200）：

```
status            # 状态机、档位、温度、滤芯%
fan 0|1|2|3|4     # 强制档位（自动=4）
sensor            # 原始 ADC、电压、换算值、有效性
filter            # 四路寿命，filter reset <idx>
log <tag> <lvl>   # 动态日志级别
heap / tasks      # 内存与任务水位
reboot / factory  # 重启 / 恢复出厂
```

> 控制台默认只在开发版开启；量产版通过编译开关关闭，避免误操作与安全暴露。

---

## 12. 内存与性能预算

| 项 | 预算 | 位置 |
|---|---|---|
| 1 bpp 帧缓冲 ×2（当前/上一帧） | 16.8 KB | 内部 RAM |
| RGB565 条带 ×2 | 23 KB | 内部 DMA RAM |
| 任务栈合计（8 任务） | ~36 KB | 内部 RAM |
| NVS/驱动/协议栈 | ~60 KB | 内部 RAM |
| 整帧截图缓冲（可选） | 134.4 KB | PSRAM |
| UI 字体/资源 | 0.2–1.5 MB | Flash（storage 或 rodata） |

- 内部 RAM 保持 ≥80 KB 余量；UI 大缓冲优先 PSRAM，但 **DMA 条带必须内部 RAM**。
- 刷屏 CPU 占用目标 <15%（Core 1），传感器任务 <5%。
- 启动到 P1 可见 ≤3 s（含自检），自检失败也要在 3 s 内给出故障画面。

---

## 13. 测试与验收映射

| 需求 | 测试方法 | 通过标准 | 当前可测 |
|---|---|---|---|
| R-UI-01 电源键短按/3 s/10 s | 手动计时 | 息屏↔唤醒；3 s 软关机；10 s 硬件断电 | 软关机需 H-06 |
| R-UI-02 功能键循环 | 连按 8 次 | 停→弱→中→强→停，任意页面生效，2 s 提示 | 需按键硬件 |
| R-UI-03/04 灯语义 | 逐状态观察 | 与 §3.6 一致，电源灯不参与告警 | 需 LED |
| 触摸导航 | 全页面走查 100 次 | 无死路，误触 ≤3 次 | 是（点屏后） |
| 处理前/后 + 效率 | 标准气源/替代气源 | 主页面可见，变化可辨 | **受阻 B-02** |
| 触摸失效降级 | 拔触摸排线 | 功能键可切风量，电源键可关机，红灯提示 | 是（需按键） |
| 换芯提醒 | 注入寿命 14%/0% | 弹窗 + 琥珀；24 h 再提醒 | 是（软件注入） |
| 过热保护 | 加热 NTC / 注入温度 | 降档→停风机 + 红闪 + 弹窗 | **受阻 H-04** |
| 息屏/唤醒/亮度/单位 | 手动 | 30/60/常亮生效；唤醒不触发业务 | 是 |
| 连续翻页稳定性 | 1 h 自动翻页 | 无重启、无内存泄漏（heap 曲线平稳） | 是 |
| 掉电续存 | 运行 11 min 后断电 | 滤芯累计误差 ≤10 min | 是 |

---

## 14. 实施里程碑

| 阶段 | 内容 | 完成判据 |
|---|---|---|
| M0 基线 | 修正 sdkconfig/分区/PSRAM/主频，工程组件化 | `idf.py build` 通过；`esp_psram_get_size()` 返回 8 MB |
| M1 点屏 | ST7789 初始化、彩条、窗口刷新、背光调光 | 颜色/方向/偏移正确；整帧 ≥30 fps |
| M2 触摸 | CST816T 通信、坐标标定、手势 | 点击/滑动/左滑识别正确，误触可接受 |
| M3 UI 适配 | OLED_UI 移植、脏矩形、虚拟四键、P1/P2 | 1 h 翻页压测无崩溃；≥25 fps |
| M4 业务闭环 | 风机、传感器、滤芯、告警、状态机 | P0 除「双点数据」外全部可演示 |
| M5 降级与电源 | 触摸失效、过热、息屏、软关机 | 拔排线/注入故障可复现降级路径 |
| M6 工程化 | 控制台、coredump、OTA（可选）、日志策略 | 现场问题可定位；OTA 可回滚 |

**当前进度**：M1/M2 的代码路径已落地（厂商 demo 移植），已在 COM10 实机复测；LCD 初始化日志通过，TRST 线测/释放均正常，但触摸 I2C 仍无 ACK。软件侧基本排除，当前阻塞在触摸物理层（B-15）。M0 尚未执行（sdkconfig 仍是 2 MB/单 APP/PSRAM 关闭）；M1/M2 的实机判据（颜色、偏移、坐标、背光）尚未全部验证。

---

## 15. 阻塞项与待确认清单

| 编号 | 阻塞项 | 影响 | 解除条件 |
|---|---|---|---|
| B-01 | 引脚映射/FPC 转接板未定稿 | 无法开始接线与驱动 | 提供 DevKitC-1 版本 + 转接板原理图/走线 |
| B-02 | 只有 1 套传感器，无法双点测量 | P0「处理前/后」验收不可能 | 补 1×MS1100 + 1×GP2Y1014AU，或变更验收为「处理后 + 估算」 |
| B-03 | 模拟前端无分压 | 过压损坏 ADC | 增加 2:1 分压 + RC（H-08） |
| B-04 | 屏 VDD 3.0 V / VDDIO 电平 | 3.3 V 越上限，可能不亮/偏色 | 供应商确认 + 3.0 V LDO；**背光已由厂商例程定性为 BLK/PWR 高有效** |
| B-05 | ST7789T3/CST816T 完整手册缺失 | 只能按例程参数工作，极限值未知 | **部分解除**：厂商例程已给出 ST7789 初始化序列与 CST816T 寄存器表；仍需手册确认时序上限 |
| B-06 | 故障态口径歧义（传感器异常是否停机） | 状态机与验收争议 | 需求方确认按 §3.7（不停机） |
| B-07 | OLED_UI 具体 commit、许可证、ESP32 bug 状态 | 崩溃与返工风险 | 锁定 commit + SHA，完成 1 h 压测 |
| B-08 | 风机规格与 MOS 板 PWM 能力 | 无法确定 PWM 后端与软启动 | 风机规格书 + MOS 板原理图 + 续流评估 |
| B-09 | 24 V→5 V、FPC 座、电源自锁、按键/LED 未采购 | 无法整机上电 | 补齐 H-01/H-05/H-06/H-11 |
| B-10 | 无温度/差压传感器 | 过热保护与真实寿命不可实现 | 先补 NTC（H-04），差压列 v2 |
| B-11 | 组件对 IDF v6.1 兼容性 | 构建失败风险 | 优先自带驱动；若用 Registry 组件，核对 `idf_component.yml` 版本范围 |
| B-12 | 传感器换算系数未标定 | 读数不可信 | 用标准气源/洁净空气标定，写入 NVS |
| B-13 | 转接板丝印/通路与 FPC 定义不符（“只有 MISO”） | 触摸无法初始化或屏无法写入 | **映射已推断**（§6.2.1）；仍要做 pin1–18 通断确认，必要时飞线 TP_SDA/TP_SCL |
| B-14 | 转接板 RT_MISO(5)/RT_BUSY(4) 的用途 | 无（原理图已确认 NC） | 保持悬空，不接 ESP32 |
| B-15 | **触摸 I2C 无 ACK；TRST 已确认正常（线测 1/0，释放后 1）** | 触摸全失效；软件侧已排除 | 按 §6.2.1 的 2×7 表核对触摸组（1/2/3/6/7/8）；量 TP_VCC=3.0 V、GND 通断、TP_SCL↔GPIO9、TP_SDA↔GPIO8；加 4.7 kΩ 上拉；确认触摸 IC 为 CST816T |

**未在硬件上验证的内容**：移植 demo 的实际显示内容（已烧录并确认 LCD 初始化日志，但颜色/反色/偏移/背光尚未目视确认）、触摸 I2C（当前无 ACK，待 2×7 接线复核）、转接板 2×7 逐脚通断与 I2C 双向通路（§6.2.1）、ADC 分压后的量程、风机软启动与噪声、整机 24 V 上电时序与掉电保持。以上必须在样机到位后逐项实测，不能以代码编译通过替代。

---

## 16. 变更记录

| 版本 | 日期 | 变更 |
|---|---|---|
| v1.0 | 2026-10-25 | 首版：基于需求 v1.0、实际工程基线（IDF v6.1.0）与屏幕手册，给出架构、任务、驱动、UI 适配、配置/分区、测试与阻塞项 |
| v1.1 | 2026-10-25 | 补充厂商 `ESP32-LCD-IDF/P169H002` 例程分析：提取 C3/S3 引脚映射、ST7789 初始化序列、CST816T 寄存器表与 Y+5 偏移；PWR 定性为高有效背光使能；记录 demo 移植结果（`components/p169h002/`，251 KB，构建通过）与正确的 EIM 构建入口。文件名沿用 v1.0 |

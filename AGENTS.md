# AGENTS.md — air_forge

面向 AI/人类协作者的工程约定。**先读本文件，再按“技能路由”加载对应 Skill。**

## 项目事实

- 产品：桌面焊接废气净化装置；固件 + 像素风 UI。
- 主控：ESP32-S3-DevKitC-1（N16R8，16MB Flash + 8MB Octal PSRAM），目标 `esp32s3`。
- 屏幕：P169H002，240×280 IPS，SPI，RGB565；触摸：CST816T，I2C 地址 `0x15`。
- 框架：ESP-IDF v6.1；UI：LVGL 9.3；桌面仿真：SDL2。
- 固件与仿真**共用同一份 `ui/` 源码**，`ui/` 不依赖任何平台 API。
- 引脚只在 `components/p169h002/include/board_pins.h` 定义，禁止在其他文件硬编码 GPIO。

## 目录地图

| 路径 | 职责 |
|---|---|
| `main/` | 固件入口（`app_main`） |
| `components/p169h002/` | 屏幕 + 触摸板级驱动（SPI / I2C） |
| `ui/` | 平台无关 LVGL UI：`ui_state.c`、`ui_popup.c`、`widgets/`、`screens/` |
| `ui/assets/` | 由脚本生成的 C 资源（图标、纹理、logo） |
| `sim/` | SDL2 桌面仿真工程 |
| `resources/` | **源资源**：SVG/PNG 与生成脚本，是静态资源的唯一事实源 |
| `third_party/` | LVGL / SDL2，由 `scripts/fetch_deps.py` 恢复，**只读，不修改** |

## 构建与验证

```powershell
# 桌面仿真（Windows）
python scripts/fetch_deps.py
cmake -S sim -B sim/build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Debug
cmake --build sim/build -j 8
.\sim\build\air_forge_sim.exe

# 固件
. $IDF_PATH/export.sh
idf.py build
idf.py -p COMx flash monitor
```

- 改动 `ui/`：必须通过仿真构建；固件接入 UI 后还必须通过 `idf.py build`。
- 改动固件/驱动：必须通过 `idf.py build`，不得新增编译警告。
- 不得在未实际构建的情况下宣称“完成”。硬件行为未验证的，必须显式列出。
- `sdkconfig`/`sdkconfig.defaults` 直接编辑，不依赖 `menuconfig`。
- 代码索引：根目录 `compile_commands.json` 是 `build/` 与 `sim/build/` 的合并库（`.vscode/settings.json` 指向它）。新增源文件或重建数据库后需重新合并，否则“跳转定义”会退化为只到声明。该文件已被 `.gitignore` 忽略。

## 代码约定

- 格式化：遵循 `.clang-format`（Tab 缩进、`IndentWidth: 8`）。
- C 语言标准以工程配置为准（固件 gnu23，仿真 C11）；ESP-IDF 惯用法优先，不引入无必要的自定义抽象。
- 并发与硬件安全优先：ISR 保持最短，阻塞操作放任务，明确队列/锁/定时器所有权。
- 错误一律检查并传播 `esp_err_t`；失败路径要有可排查的 `ESP_LOGx` 上下文。

## 函数注释（硬性要求）

1. **每个函数都必须有函数头注释**：用途、输入/输出、返回值与失败语义、调用上下文（任务/ISR/回调）、并发或重入约束。
2. **函数体内必须对关键流程写准确注释**，至少覆盖：
   - 状态迁移与模式切换；
   - 硬件初始化顺序、时序/延时约束、寄存器位含义；
   - 锁、临界区、ISR 与任务的交接点；
   - 缓冲区/内存的生命周期与所有权；
   - 错误处理、重试、回滚与超时路径；
   - 非显然的魔数、坐标/尺寸换算、字节序与像素格式。
3. 不写复述代码的废话注释（如 `// 赋值`）；不为凑注释而拆碎逻辑。
4. **注释随代码同步修改**：过期、错误或与行为不符的注释按缺陷处理。
5. 注释语言使用中文，与现有代码保持一致。

## 静态资源同步（硬性要求）

静态资源包括：`resources/` 下的源文件、`ui/assets/` 下的生成代码、字体/图标/纹理/logo、`3d_model/`、`.screenshots/`。

- 生成链（只改源，不改生成物）：
  - `resources/gui_icons/*.svg` + `make_pixel_icons.py` → `ui/assets/icons/*.c` + `icon.h`
  - `resources/logo/*` + `make_logo.py` → `ui/assets/logo_icon.c`
  - `resources/fonts/*.ttf` + `build_fonts.py` → `ui/assets/fonts/lv_font_fusion_12.c`、`lv_font_fusion_8.c`
  - 噪点纹理等 → `ui/assets/ui_assets.c`
- 字库规则：中文一律用 `UI_FONT_TITLE`（12px）；`UI_FONT_SMALL`（8px）只放纯 ASCII 小字。改动中文文案后重跑 `python resources/fonts/build_fonts.py`，字符集由脚本扫描 `ui/**/*.c` 自动收集。
- **禁止手改** `*_icon.c`、`ui_assets.c`、`logo_icon.c`；必须先改脚本/源文件，再重新生成。
- 新增、重命名、删除或移动资源时，**同一提交内**同步更新：生成脚本、生成 C 文件、`icon.h`、所有引用点、README/截图（若用户可见）。
- 不得留下孤儿资源或失效引用；资源必须平台无关（C 数组，无文件 I/O），保证固件与仿真都能使用。
- 当静态资源或固定硬件事实（引脚、分辨率、Flash/PSRAM、器件型号）变化时，**同步更新本文件、README 与相关文档**。

## 技能路由

| 任务类型 | 使用的 Skill |
|---|---|
| ESP-IDF 编码、调试、烧录、LVGL 集成、panic 分析 | `esp32-firmware-engineer` |
| 通用 RTOS / 功耗 / 内存方法论 | `embedded-systems` |
| 原理图、PCB、器件选型 | `easyeda-api` |
| 数据分析、报表 | `data-analysis` |

使用 Skill 时，其流程不得放宽本文件的硬性约束；发生冲突时以本文件为准。

## 完成标准

- 按改动范围完成构建验证（仿真 / 固件 / 两者）。
- 新增或修改的函数已补齐注释；静态资源链路已同步；无失效引用。
- 汇报时说明改了什么、关键决策、验证结果，以及尚未在硬件上验证的部分。

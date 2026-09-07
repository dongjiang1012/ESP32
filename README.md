# ESP32 SW6306 电源控制台

基于 ESP32-P4-WIFI6-Touch-LCD-7B 开发板的 SW6306 数据监视与控制台项目。
界面使用 LVGL 9 构建，通过 I2C 读写 SW6306 寄存器，将 ADC、快充状态、电量计和异常事件
发布到线程安全的数据快照供 UI 读取；同时提供输出控制、Buck-Boost 配置和系统设置页面，
采用“编辑草稿 → 后台应用 → 回读确认”的交互，另有只读故障分析页辅助定位充放电异常。

第一阶段的字段映射、写入语义和实机验证边界见 [第一阶段说明](docs/phase1-control-console.md)。
界面已中文化，中文使用按当前界面字符裁剪的 Noto Sans SC 子集字库（301 个非 ASCII 字符），
正文 16 px、标题/主要数值 24 px，ASCII 字符与符号由 Montserrat 后备；字体大小、空间占用及
再生成方式见 [中文字体说明](main/ui/fonts/README.md)。

## 硬件

- ESP32-P4-WIFI6-Touch-LCD-7B（BSP 版本 3.0.1）
- EK79007 1024 × 600 MIPI-DSI 显示屏
- GT911 触摸控制器
- SW6306 芯片，I2C 地址由 `i2c_device_0x3c` 驱动配置
- USB 线，用于供电、烧录和串口监视

## 功能页面

| 页面 | 内容 |
| --- | --- |
| Monitor | “功率监控”显示 BUS 电压、电流、功率及最近 60 个有效 ADC 采样曲线；“电量计”直接显示 REG0x86–0x8C、0x94、0x99 的容量、电量与原始值，不做软件估算 |
| Output | 板内 VBUS 强制输出使能、22V 草稿快捷按钮、共享 IBUS 强制控制、输出电压、输出/充电 IBUS 限流、实时总线与 Die 温度 |
| Buck-Boost | 开关频率、共享峰值限流、Die 过温门限、M2 RDS(on)、轻载强制 PWM |
| System | 屏幕亮度、C1 的 DRP/Try.SRC、Source、Sink 角色；保留顶部下拉亮度面板 |
| Fault Analysis | 只读显示 REG0x15 事件与 REG0x2A–0x2C 的充放电异常历史、62368/DPDM/CC 保护状态和原始寄存器值 |

配置页首次进入时后台读取芯片，读取成功后才能编辑；右侧显示最后一次回读值。
数值输入使用固定小数键盘，范围和步进强制检查，不自动舍入；Apply 写入并回读，
失败后显示错误码且必须 Reload。页面对象长期保留，切换页面不会丢失草稿。

## 软件结构

```text
main/
├── main.c                 系统初始化和 FreeRTOS 任务创建
├── drivers/               I2C、寄存器访问、ADC、快充状态、电量计、故障状态、
│                          电源配置、启动配置、Type-C 模式驱动
├── services/              配置校验/应用/回读、后台配置队列、采集服务、诊断服务
├── config/                物理量类型、参数范围、配置草稿和已读回值
├── data/
│   ├── data_scheduler.c   采集任务及周期调度
│   └── data_snapshot.c    数据快照和互斥锁保护
└── ui/                    五页导航、数字键盘、亮度面板、功率/电量计图表、
                           故障分析、共用控件、中文字库
```

### 采集任务

采集任务由 `main.c` 创建和管理，任务配置集中在 `main.c`：

- 栈大小：4096
- 优先级：4
- 任务节拍：50ms

任务内部使用表驱动方式调度各数据组：

| 数据组 | 周期 |
| --- | ---: |
| 快充状态 | 100ms |
| ADC 电压、电流、功率 | 500ms |
| 电量计 | 2000ms |
| 异常事件及状态（0x15、0x2A–0x2C） | 200ms |

采集任务不操作 LVGL，也不决定 UI 刷新时机。每个 `collect_*()` 函数负责调用
驱动、计算必要的派生值，然后将成功读取的数据发布到快照。

### 数据快照

快照保存在 `data_snapshot.c` 的内部 RAM 对象中。采集任务发布数据和 UI 读取数据
时都使用同一个 FreeRTOS mutex：

- 发布成功数据时，整组数据一次性更新
- 读取失败时只记录错误，不覆盖上一次有效数据
- UI 通过 `data_snapshot_read()` 获取完整副本后再更新控件

### 配置任务

`sw6306_config_worker` 使用 4096 字节栈、优先级 3。请求和结果通过队列按值复制，
任务不操作 LVGL。UI 每 100ms 轮询结果，同一时间只允许一个配置请求。
配置组分别保存草稿、最后读回值、有效状态和未应用状态；切换页面不会丢失草稿。
任何应用失败都会使对应配置组失效，需要 Reload 后才能再次编辑。
功率曲线只在新的 ADC 采样到达时追加点，读取错误或数据超时会显示异常状态。

### 诊断串口命令

固件启动后由 `sw6306_diagnostics` 启动一个读取串口输入的诊断任务，便于不接屏幕时
调试硬件。命令通过串口监视器输入：

| 命令 | 作用 |
| --- | --- |
| `sw status` | 打印关键寄存器（0x18、0x19、0x1C–0x1D、0x28、0x2A–0x2C、0x40–0x44、0x100–0x101、0x107–0x108、0x118、0x132）与 ADC 全量读数 |
| `sw 12v` | 将输出电压强制为 12 V 并启动放电（调试用） |
| `sw off` | 清除强制电压，保持物理输出关闭 |

诊断任务只读串口输入，不操作 LVGL。默认输出保持关闭，只有显式执行 `sw 12v` 才会
强制高压，调试时不要接普通 USB 设备测试。

## 构建

项目使用 ESP-IDF 5.5.5，目标芯片为 ESP32-P4。

在 ESP-IDF 环境中执行：

```text
idf.py set-target esp32p4
idf.py build
```

Windows 环境也可以使用项目自带脚本：

```text
esp.bat build
```

`esp.bat` 支持 `build` / `flash` / `monitor` / `all` / `clean` / `menuconfig` 六个动作。

## 烧录和监视

修改 `esp.bat` 中的 `ESP_PORT` 为实际串口号，然后执行：

```text
esp.bat flash
esp.bat monitor
```

一键完成构建、烧录并打开监视：

```text
esp.bat all
```

也可以直接使用 ESP-IDF 命令：

```text
idf.py -p PORT flash monitor
```

## 分区与默认配置

自定义分区表（`partitions.csv`）：factory 应用分区 8 MiB、storage SPIFFS 数据分区 7 MiB。
`sdkconfig.defaults` 中开启 SPIRAM（200 MHz XIP）、自定义分区表、性能优化编译等默认配置。

## 其他目录

- `build/`：ESP-IDF 构建输出，可重新生成
- `managed_components/`：组件管理器下载的依赖组件（BSP 3.0.1、LVGL 9.5.0 等，见 `main/idf_component.yml`）
- `tmp/`：字体生成缓存、npm 缓存、手册截图等临时文件，不参与固件编译，可随时清理
- `docs/archive/legacy-work/`：历史整理脚本、清单和 SW6306 参考图片，不参与固件编译

## Pull Request 工作流

功能开发应在独立分支上进行，推送到 GitHub 后创建 Pull Request，再进行代码审查和合并。

# 中文字库

界面使用 Noto Sans SC 常规字重（400）。两套字库按当前固件的 C/C++ UI 字符串裁剪；当前包含
**301 个非 ASCII 字符**。这样可显著降低固件体积；每次新增或修改中文界面文本后，必须重新运行
字体生成与校验脚本。ASCII 字符、数值和常用符号由 Montserrat 后备字体显示。

| 字体 | 用途 | 位图 |
| --- | --- | --- |
| ui_font_cn_16.c | 正文、按钮、说明、单选项 | 16 px，2 bpp，未压缩 |
| ui_font_cn_24.c | 标题、主要数值、键盘 | 24 px，2 bpp，未压缩 |

2 bpp 提供四级抗锯齿灰度。RLE 是位图数据压缩，不是删减字符。
Montserrat 16/24 作为图标等缺字的后备字体；页面文案使用中文，V/A/kHz、IBUS、PWM 等单位与技术缩写保留。
源字体版权和 SIL Open Font License 1.1 见同目录 OFL.txt。

生成的 C 源码总共约 38.6 MB，源文件中的十六进制文本和注释不等于固件占用。
当前子集的字体数据估算约 **0.06 MiB**。最终占用以 ESP-IDF 链接/分区检查结果为准；当前 factory
分区为 8 MiB。20 px 的用途统一使用 24 px 字体，避免维护第三套字库。

sdkconfig 和 sdkconfig.defaults 保留 `CONFIG_LV_FONT_FMT_TXT_LARGE=y` 与
`CONFIG_LV_USE_FONT_COMPRESSED=y`，以便未来扩展字库。当前生成的子集不依赖大字库地址空间。
普通编译直接使用已经生成的 C 文件，不需要 Node、字体转换器或 Python。

## 行高和后续扩展

ui_fonts.c 只复制很小的 lv_font_t 描述符，字形和位图数据始终共享。
正文固定使用 20 px 行高/4 px 基线，标题固定使用 28 px 行高/5 px 基线；校验脚本会确认每个当前
界面字符在这些行框内不会越界。

UI_FONT_BODY、UI_FONT_MEDIUM、UI_FONT_LARGE 是统一字体入口。
目前 MEDIUM 与 LARGE 共用 24 px。新页面应复用这些入口，避免直接指定无中文字形的 Montserrat。

## 再生成（普通编译无需执行）

工具依赖放在 tools/font-converter/，不参与固件构建。
原始字体来自本机 C:/Windows/Fonts/NotoSansSC-VF.ttf，生成的常规字重 TTF 放在 tmp/fonts/。
可以通过 prepare_chinese_font.py 的首个参数指定另一个同授权字体路径。

在工程根目录运行：

```text
npm ci --prefix tools/font-converter
python -m pip install --target tools/font-converter/python -r tools/font-converter/requirements.txt
python tools/prepare_chinese_font.py
node --max-old-space-size=4096 tools/generate_chinese_fonts.cjs
node tools/check_chinese_fonts.cjs
python tools/check_phase1_static.py
```

font_manifest.json 记录已生成子集的码点、字号、位深与转换器版本。
check_chinese_fonts.cjs 解析实际生成的 C 映射表，核对当前页面字符、行高，并估算数据大小。
preview_chinese_fonts.cjs 可生成字体栅格样张，它不等同于 LVGL 实机截图。
上述工具不运行固件编译器、不烧录、不读写 SW6306。

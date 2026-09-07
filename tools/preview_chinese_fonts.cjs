// Inspect the actual rasterizer/bit depth and horizontal line metrics without a firmware build.
const fs = require('fs');
const path = require('path');
const converter = path.join(__dirname, 'font-converter/node_modules/lv_font_conv');
const ft = require(path.join(converter, 'lib/freetype'));
const {PNG} = require(path.join(converter, 'node_modules/pngjs'));
const fontData = fs.readFileSync(path.join(__dirname, '../tmp/fonts/NotoSansSC-Regular.ttf'));
const png = new PNG({width: 1024, height: 430});
for (let i = 0; i < png.data.length; i += 4) png.data.set([16, 24, 32, 255], i);
const layouts = [
    {size:24, x:24, y:20, text:'电源控制中心    实时监控    输出控制    升降压配置    系统设置'},
    {size:24, x:24, y:76, text:'输出控制                 升降压配置'},
    {size:16, x:24, y:125, text:'强制控制输出电压          5.000 V       输出 IBUS 限流       2.000 A'},
    {size:16, x:24, y:168, text:'开关工作频率          300 kHz     芯片内部过温门限          120 ℃'},
    {size:16, x:24, y:211, text:'M2 导通电阻          2.5 mΩ      双向（优先供电）      供电端      受电端'},
    {size:16, x:24, y:254, text:'重新读取        放弃修改        应用输出配置        载入芯片复位值到草稿'},
    {size:16, x:24, y:297, text:'请输入数字，最多保留 3 位小数。   数值须在允许范围内，并符合调整步进。'},
    {size:24, x:24, y:352, text:'取消     确认     清空     退格     确认数值'},
];
(async () => {
    await ft.init();
    for (const item of layouts) {
        const face = ft.fontface_create(fontData, item.size);
        const line = item.size === 16 ? 20 : 28;
        const baseline = item.size === 16 ? 4 : 5;
        let x = item.x;
        for (const char of item.text) {
            const g = ft.glyph_render(face, char.codePointAt(0), {});
            for (let row = 0; row < g.height; row++) for (let col = 0; col < g.width; col++) {
                const px = Math.round(x + g.x + col), py = item.y + line - baseline - g.y + row;
                if (px < 0 || px >= png.width || py < 0 || py >= png.height) continue;
                const alpha = Math.round(g.pixels[row][col] * 3 / 255) / 3;
                const index = (py * png.width + px) * 4;
                [231,240,245].forEach((v,i) => png.data[index+i] = Math.round(png.data[index+i]*(1-alpha)+v*alpha));
            }
            x += Math.round(g.advance_x);
        }
        ft.fontface_destroy(face);
    }
    ft.destroy();
    fs.writeFileSync(path.join(__dirname, '../tmp/fonts/chinese-font-preview.png'), PNG.sync.write(png));
})().catch(error => { console.error(error); process.exitCode = 1; });

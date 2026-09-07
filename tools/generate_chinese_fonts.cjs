// Run after prepare_chinese_font.py. No firmware compiler is invoked.
const fs = require('fs');
const path = require('path');
const root = path.resolve(__dirname, '..');
const converter = path.join(__dirname, 'font-converter/node_modules/lv_font_conv');
const opentype = require(path.join(converter, 'node_modules/opentype.js'));
const cli = require(path.join(converter, 'lib/cli'));
const source = path.join(root, 'tmp/fonts/NotoSansSC-Regular.ttf');
const font = opentype.loadSync(source);
// Scan project strings and keep only non-ASCII characters actually used by the UI.
const needed = new Set();
function collectFiles(directory) {
    return fs.readdirSync(directory, {withFileTypes: true}).flatMap(entry => {
        const file = path.join(directory, entry.name);
        if (file === path.join(root, 'main/ui/fonts')) return [];
        if (entry.isDirectory()) return collectFiles(file);
        return /\.(c|h)$/.test(entry.name) ? [file] : [];
    });
}
for (const file of collectFiles(path.join(root, 'main'))) {
    const text = fs.readFileSync(file, 'utf8');
    for (const match of text.matchAll(/"(?:\\.|[^"\\])*"/g)) {
        const value = JSON.parse(match[0]);
        for (const character of value) {
            if (character.codePointAt(0) > 127) needed.add(character.codePointAt(0));
        }
    }
}
const codes = [...needed].sort((a, b) => a - b);
const ranges = [];
for (let i = 0; i < codes.length;) {
    let first = codes[i], last = first;
    while (++i < codes.length && codes[i] === last + 1) last = codes[i];
    ranges.push(`${first}-${last}`);
}

(async () => {
    for (const size of [16, 24]) {
        console.log(`Generating ${size}px, ${codes.length} used Chinese/punctuation characters, 2 bpp...`);
        await cli.run(['--font', source, '--range', ranges.join(','), '--size', String(size),
            '--bpp', '2', '--no-kerning', '--no-compress', '--format', 'lvgl', '--lv-include', 'lvgl.h',
            '--lv-fallback', `lv_font_montserrat_${size}`,
            '--lv-font-name', `ui_font_cn_${size}`, '--output',
            path.join(root, `main/ui/fonts/ui_font_cn_${size}.c`)]);
        console.log(`Finished ${size}px`);
    }
    const metadata = {source: 'Noto Sans SC', weight: 400, unicodeCharacters: codes.length,
        sizes: [16, 24], bpp: 2, compression: 'none', subset: true,
        converter: require(path.join(converter, 'package.json')).version, codepoints: codes};
    fs.writeFileSync(path.join(root, 'main/ui/fonts/font_manifest.json'), JSON.stringify(metadata, null, 2) + '\n');
})().catch(error => { console.error(error); process.exitCode = 1; });

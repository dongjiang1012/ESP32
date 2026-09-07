// Read-only verification of generated glyph maps and every current UI string.
const fs = require('fs');
const path = require('path');
const root = path.resolve(__dirname, '..');
const directory = path.join(root, 'main/ui/fonts');
const manifest = JSON.parse(fs.readFileSync(path.join(directory, 'font_manifest.json'), 'utf8'));
const files = fs.readdirSync(path.join(root, 'main/ui')).filter(x => x.endsWith('.c'))
    .map(x => path.join(root, 'main/ui', x));
files.push(path.join(root, 'main/config/sw6306_config_validate.c'));
const needed = new Set();
for (const file of files) {
    const source = fs.readFileSync(file, 'utf8');
    for (const match of source.matchAll(/"(?:\\.|[^"\\])*"/g)) {
        const value = JSON.parse(match[0]);
        for (const character of value) if (character.codePointAt(0) > 127) needed.add(character.codePointAt(0));
    }
}
let total = 0;
for (const size of manifest.sizes) {
    const bytes = fs.readFileSync(path.join(directory, `ui_font_cn_${size}.c`));
    const source = new TextDecoder('utf-8', {fatal: true}).decode(bytes).replace(/\/\*[\s\S]*?\*\//g, '');
    const arrays = new Map();
    let dataSize = 0;
    for (const m of source.matchAll(/(?:static\s+)?(?:LV_ATTRIBUTE_LARGE_CONST\s+)?(?:static\s+)?const\s+uint(8|16)_t\s+(\w+)\[\]\s*=\s*\{([\s\S]*?)\};/g)) {
        const values = [...m[3].matchAll(/0x[\da-f]+|\b\d+\b/gi)].map(x => Number(x[0]));
        arrays.set(m[2], values);
        dataSize += values.length * Number(m[1]) / 8;
    }
    if (!arrays.has('glyph_bitmap')) throw Error('Bitmap not found');
    const glyphs = [...source.matchAll(/\.bitmap_index\s*=\s*(\d+),\s*\.adv_w\s*=\s*(\d+),\s*\.box_w\s*=\s*(\d+),\s*\.box_h\s*=\s*(\d+),\s*\.ofs_x\s*=\s*(-?\d+),\s*\.ofs_y\s*=\s*(-?\d+)/g)]
        .map(m => m.slice(1).map(Number));
    const map = new Map();
    let mapCount = 0;
    for (const m of source.matchAll(/\.range_start\s*=\s*(\d+),\s*\.range_length\s*=\s*(\d+),\s*\.glyph_id_start\s*=\s*(\d+),\s*\.unicode_list\s*=\s*(\w+),\s*\.glyph_id_ofs_list\s*=\s*(\w+),\s*\.list_length\s*=\s*(\d+),\s*\.type\s*=\s*(\w+)/g)) {
        mapCount++;
        const start = Number(m[1]), count = Number(m[2]), glyph = Number(m[3]);
        if (m[7] === 'LV_FONT_FMT_TXT_CMAP_FORMAT0_TINY') {
            for (let i = 0; i < count; i++) map.set(start + i, glyph + i);
        } else if (m[7] === 'LV_FONT_FMT_TXT_CMAP_FORMAT0_FULL') {
            const offsets = arrays.get(m[5]);
            offsets.forEach((offset, i) => { if (i === 0 || offset !== 0) map.set(start + i, glyph + offset); });
        } else if (m[7] === 'LV_FONT_FMT_TXT_CMAP_SPARSE_TINY') {
            const offsets = arrays.get(m[4]);
            if (offsets.length !== Number(m[6])) throw Error('Cmap length mismatch');
            offsets.forEach((offset, i) => map.set(start + offset, glyph + i));
        } else throw Error(`Unsupported verification cmap: ${m[7]}`);
    }
    const missing = manifest.codepoints.filter(c => !map.has(c));
    if (missing.length || map.size !== manifest.unicodeCharacters) throw Error(`Generated subset cmap mismatch at ${size}px`);
    if (glyphs.length !== manifest.unicodeCharacters + 1) throw Error('Glyph descriptor count mismatch');
    for (const c of needed) {
        if (!map.has(c)) throw Error(`UI character missing: ${String.fromCodePoint(c)}`);
        const g = glyphs[map.get(c)];
        const lineHeight = size === 16 ? 20 : 28;
        const baseline = size === 16 ? 4 : 5;
        const top = lineHeight - baseline - g[3] - g[5];
        if (top < 0 || top + g[3] > lineHeight) throw Error(`UI glyph would exceed line metrics: ${String.fromCodePoint(c)}, ${size}px`);
    }
    dataSize += glyphs.length * 16 + mapCount * 24 + 128;
    total += dataSize;
    console.log(`PASS ${size}px: ${map.size} mapped characters, ${needed.size} UI characters covered; approx. ${dataSize} font-data bytes.`);
}
const previousBinary = path.join(root, 'build/lvgl_demo_v9.bin');
console.log(`Font data estimate: ${(total / 1048576).toFixed(2)} MiB / 8 MiB app partition.`);
if (fs.existsSync(previousBinary)) {
    // The binary may already contain the fonts after a user build, so this
    // comparison is informational and must not double-count as a hard failure.
    const oldFirmware = fs.statSync(previousBinary).size;
    console.log(`Existing binary: ${(oldFirmware / 1048576).toFixed(2)} MiB; fonts plus that binary: ${((total + oldFirmware) / 1048576).toFixed(2)} MiB (may double-count fonts after rebuilding).`);
}
if (total >= 8 * 1024 * 1024) throw Error('Fonts alone exceed the existing app partition');
console.log('This is not a linked firmware size; no firmware compiler was run.');

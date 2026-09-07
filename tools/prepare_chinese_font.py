"""Create a regular-weight instance without removing any Unicode glyphs."""
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools/font-converter/python"))
from fontTools.ttLib import TTFont
from fontTools.varLib.instancer import instantiateVariableFont

source = Path(sys.argv[1]) if len(sys.argv) > 1 else Path("C:/Windows/Fonts/NotoSansSC-VF.ttf")
output = ROOT / "tmp/fonts/NotoSansSC-Regular.ttf"
output.parent.mkdir(parents=True, exist_ok=True)
font = TTFont(source)
before = set(font.getBestCmap())
if "fvar" in font:
    font = instantiateVariableFont(font, {"wght": 400}, inplace=True)
assert set(font.getBestCmap()) == before, "Unexpected character loss"
font.save(output)
print(f"Regular weight: {output}; {len(before)} Unicode characters retained", flush=True)

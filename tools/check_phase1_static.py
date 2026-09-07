"""Read-only pre-build checks. Does not invoke a compiler or touch hardware.

Requires pycparser. This checks C grammar with stand-in external typedefs, NOT
type correctness, preprocessing, linking, memory use or actual device behavior.
"""
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools/font-converter/python"))
from pycparser import c_parser, c_ast
MAIN = ROOT / "main"
errors = []


def check(condition, message):
    if not condition:
        errors.append(message)


def strip_comments(text):
    token = r'"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'|/\*[\s\S]*?\*/|//[^\n]*'
    return re.sub(token, lambda m: "\n" * m[0].count("\n") if m[0].startswith(("/*", "//")) else m[0], text)


cmake = (MAIN / "CMakeLists.txt").read_text(encoding="utf-8")
listed = re.findall(r"[\w/]+\.c\b", cmake)
actual = {p.relative_to(MAIN).as_posix() for p in MAIN.rglob("*.c")}
check(len(listed) == len(set(listed)), "Duplicate CMake source entry")
check(set(listed) == actual, f"CMake mismatch: missing={actual-set(listed)}, absent={set(listed)-actual}")

lv_headers = "\n".join(strip_comments(p.read_text(encoding="utf-8", errors="replace"))
                       for p in (ROOT / "managed_components/lvgl__lvgl").rglob("*.h"))
lv_headers += "\n" + "\n".join(strip_comments(p.read_text(encoding="utf-8")) for p in MAIN.rglob("*.h"))


class Calls(c_ast.NodeVisitor):
    def __init__(self):
        self.calls = []

    def visit_FuncCall(self, node):
        if isinstance(node.name, c_ast.ID):
            self.calls.append((node.name.name, len(node.args.exprs) if node.args else 0))
        self.generic_visit(node)


files = sorted((MAIN / "ui").glob("*.c")) + sorted((MAIN / "config").glob("*.c"))
files += [MAIN / "drivers/sw6306_power_config.c", MAIN / "services/sw6306_config_worker.c",
          MAIN / "services/sw6306_config_service.c", MAIN / "main.c"]
all_calls = set()
for path in files:
    source = strip_comments(path.read_text(encoding="utf-8-sig"))
    source = re.sub(r"^\s*#.*$", "", source, flags=re.M)
    types = set(re.findall(r"\b[A-Za-z_]\w*_t\b", source)) | {"bool", "QueueHandle_t"}
    prefix = "\n".join(f"typedef int {name};" for name in sorted(types)) + "\n"
    try:
        ast = c_parser.CParser().parse(prefix + source, filename=str(path.relative_to(ROOT)))
    except Exception as exc:
        errors.append(f"C grammar: {exc}")
        continue
    calls = Calls()
    calls.visit(ast)
    all_calls.update((name, count) for name, count in calls.calls if name.startswith("lv_"))

for name, count in sorted(all_calls):
    signatures = re.findall(r"\b" + re.escape(name) + r"\s*\(([^()]*)\)\s*(?:LV_FORMAT_ATTRIBUTE\([^)]*\)\s*)?(?:;|\{)", lv_headers)
    # Macro helpers, e.g. lv_pct(), may not have a function declaration.
    if not signatures:
        check(re.search(r"#\s*define\s+" + re.escape(name) + r"\b", lv_headers) is not None,
              f"LVGL API not found: {name}")
        continue
    accepted = False
    for signature in signatures:
        args = signature.strip()
        expected = 0 if args in ("", "void") else len(args.split(","))
        accepted |= count >= expected - 1 if "..." in args else count == expected
    check(accepted, f"LVGL argument count: {name} called with {count}, declarations={signatures}")

sdkconfig = (ROOT / "sdkconfig").read_text(encoding="utf-8")
for feature in ["BUTTON", "BUTTONMATRIX", "CHART", "LABEL", "SLIDER", "SWITCH", "TEXTAREA"]:
    check(f"CONFIG_LV_USE_{feature}=y" in sdkconfig, f"LVGL feature disabled: {feature}")
for font in [16, 20, 24]:
    check(f"CONFIG_LV_FONT_MONTSERRAT_{font}=y" in sdkconfig, f"LVGL font disabled: {font}")

# Keep the four-layer boundaries visible: UI only submits service requests.
for path in (MAIN / "ui").glob("*.c"):
    source = strip_comments(path.read_text(encoding="utf-8"))
    check(re.search(r"\b(?:sw6306_register_\w+|sw6306_power_(?:read|write)_\w+|i2c_master_\w+)\s*\(", source) is None,
          f"UI bypasses service boundary: {path.name}")

if errors:
    print("\n".join(errors))
    sys.exit(1)
print(f"PASS: {len(listed)} CMake sources, {len(files)} C grammar checks, {len(all_calls)} LVGL call signatures.")
print("PASS: required LVGL widgets/fonts enabled; UI does not access chip registers directly.")
print("No compiler, linker, simulator or hardware tests were run.")

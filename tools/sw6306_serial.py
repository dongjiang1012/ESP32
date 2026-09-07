"""Send a diagnostic command without toggling the board's reset lines."""
import argparse
from pathlib import Path
import sys
import time
import serial

p = argparse.ArgumentParser()
p.add_argument("command", nargs="?", default="sw status")
p.add_argument("--port", default="COM7")
p.add_argument("--seconds", type=float, default=5)
p.add_argument("--log")
a = p.parse_args()
sys.stdout.reconfigure(encoding="utf-8", errors="replace")
s = serial.Serial()
s.port, s.baudrate, s.timeout = a.port, 115200, 0.1
s.dtr = s.rts = False
s.open()
chunks = []
try:
    if a.command:
        s.write((a.command + "\n").encode("ascii"))
    end = time.monotonic() + a.seconds
    while time.monotonic() < end:
        chunk = s.read(4096)
        if chunk:
            chunks.append(chunk)
            print(chunk.decode("utf-8", errors="replace"), end="", flush=True)
finally:
    s.close()
if a.log:
    Path(a.log).write_bytes(b"".join(chunks))

"""Generate the review preview using the exact firmware pixel renderer."""
from pathlib import Path
import base64
import json
import struct
import subprocess
import tempfile
import zlib

ROOT = Path(__file__).resolve().parents[1]


def png(raw, width, height, scale=1):
    rows = []
    for y in range(height):
        row = b"".join(bytes([v]) * scale for v in raw[y * width : (y + 1) * width])
        rows.extend([b"\x00" + row] * scale)

    def chunk(kind, value):
        return (struct.pack(">I", len(value)) + kind + value
                + struct.pack(">I", zlib.crc32(kind + value) & 0xFFFFFFFF))

    return (b"\x89PNG\r\n\x1a\n"
            + chunk(b"IHDR", struct.pack(">IIBBBBB", width * scale, height * scale, 8, 0, 0, 0, 0))
            + chunk(b"IDAT", zlib.compress(b"".join(rows))) + chunk(b"IEND", b""))


with tempfile.TemporaryDirectory(prefix="walkies-preview-") as tmp:
    binary = str(Path(tmp) / "preview")
    subprocess.run(["cc", "-std=c11", "-Wall", "-Wextra", "-Werror",
                    "-Iboards/shields/walkies", "tools/walkies_preview.c",
                    "boards/shields/walkies/render.c", "-o", binary], cwd=ROOT, check=True)
    modes = {}
    for mode, wpm, count in [("rest", 0, 2), ("walk", 25, 512), ("run", 65, 512)]:
        frames = []
        for step in range(count):
            raw = subprocess.check_output([binary, str(step), str(wpm)]).split(b"\n", 3)[3]
            frames.append(base64.b64encode(png(raw, 148, 160)).decode())
            if mode == "walk" and step == 0:
                (ROOT / "docs/walkies-preview.png").write_bytes(png(raw, 148, 160, 4))
        modes[mode] = frames

html = '''<!doctype html><meta charset="utf-8"><title>Walkies · Pixel preview</title>
<style>
body{background:#eeeae2;color:#242720;font:16px system-ui;text-align:center;margin:32px 16px}
h1{margin-bottom:8px}p{max-width:620px;margin:16px auto;line-height:1.5}
.labels{display:flex;justify-content:space-between;width:592px;max-width:90vw;margin:24px auto 10px}
.labels span{width:46%}img{width:592px;max-width:90vw;image-rendering:pixelated;box-shadow:0 3px 20px #0001}
button{font:inherit;margin:6px;padding:9px 18px;border:1px solid #aaa;border-radius:8px;background:#fff;cursor:pointer}
button[aria-pressed=true]{background:#283b2d;color:white}.controls{margin-top:22px}
</style><h1>Walkies</h1><p>A little park for your keyboard.</p>
<div class="labels"><span>LEFT · STATUS</span><span>RIGHT · WALKIES</span></div>
<img id="screens" alt="Firmware pixel preview of the status screen and Luna in a park">
<div class="controls"><button data-mode="rest">Rest</button><button data-mode="walk">Walk</button><button data-mode="run">Run</button><button id="pause">Pause</button></div>
<p>Actual 68 × 160 pixel renderer, enlarged 4×. Battery readings are examples.
The park scrolls only while typing; Luna wags her tail while resting. Nothing has been pushed or flashed.</p>
<script>
const frames=FRAMES;let mode='walk',i=0,playing=true,timer;
const screen=document.getElementById('screens');
function show(){screen.src='data:image/png;base64,'+frames[mode][i]}
function loop(){clearTimeout(timer);timer=setTimeout(()=>{if(playing){i=(i+1)%frames[mode].length;show()}loop()},mode==='rest'?300:(mode==='run'?150:300))}
function choose(value){mode=value;i=0;document.querySelectorAll('[data-mode]').forEach(b=>b.setAttribute('aria-pressed',b.dataset.mode===mode));show();loop()}
document.querySelectorAll('[data-mode]').forEach(b=>b.onclick=()=>choose(b.dataset.mode));
document.getElementById('pause').onclick=function(){playing=!playing;this.textContent=playing?'Pause':'Play'};
choose('walk');
</script>'''.replace("FRAMES", json.dumps(modes, separators=(",", ":")))
(ROOT / "docs/walkies-preview.html").write_text(html)
print("Updated docs/walkies-preview.png and docs/walkies-preview.html")

# Walkies

Both halves use `shield: walkies`. The left shows its battery percentage,
active layer number, and USB/BT connection status. Typing speed is not
displayed; it only controls the dog's pose and animation speed.

The right shows its own battery and the original 32 × 22 Luna in a scrolling park
with rounded hills, trees, flowers and an outlined sun. A subtle four-beat bob
adds lift to the existing walk/run poses, preserving the original face and
ears. Luna sits when
typing stops, walks below 40 WPM, and runs at 40 WPM and above. Walking frames
update every 300 ms; running frames every 150 ms. The scene only scrolls when
the dog moves. While resting, Luna alternates the original two sitting frames
every 300 ms (matching flashed commit `e9b4edc`), keeping the tail wag without scrolling the scenery. A lost
connection stops walking and shows LINK LOST. The existing sleep timeout
still powers down the keyboard after inactivity.

`sync.c` uses ZMK's existing split behavior invocation to send typing speed
at approximately 1 Hz. These messages only affect display state, never HID
reports or keymap state. Both halves must run this firmware for the animation
to respond to typing on either half. No host helper or additional module is
required. Messages stop after typing ceases, and stale data expires after
2.5 seconds. Because the last packet can follow the last key by up to two
seconds, the dog can take roughly 4.5 seconds to settle.

The 15-minute sleep timeout and working orientation are retained in
`config/blelulu.conf`. Display polling skips redraws when the framebuffer is
unchanged. Battery life has not been measured with this animation.

## Rendering and preview

`render.c` produces a 68 × 160 monochrome framebuffer without LVGL or ZMK
dependencies. `screen.c` maps it into the panel's 160 × 68 framebuffer.
Run `python3 tools/render_preview.py` to regenerate
`docs/walkies-preview.png` and `docs/walkies-preview.html` from
this same renderer. The HTML lets you select rest, walk or run and pause the
preview. Each moving scene loops over its full 512-step repeat period.
Actual movement depends on keyboard activity.

The Luna bitmaps are reused from the previous left-screen implementation.
See `LICENSE.luna` and `luna_frames.h` for attribution. The park and status
widgets are drawn locally in `render.c`.

Run the renderer checks from the repository root:

```sh
cc -std=c11 -Wall -Wextra -Werror -fsanitize=undefined \
  -Iboards/shields/walkies tests/walkies_render_test.c \
  boards/shields/walkies/render.c -o /tmp/walkies-test
/tmp/walkies-test
```

After GitHub Actions passes, flash `walkies-blelulu_left-zmk.uf2` to the left
and `walkies-blelulu_right-zmk.uf2` to the right. Check orientation, both
battery readings, layer changes, typing on each half, animation stopping,
and reconnecting after sleep. Keep the previous UF2 files for rollback.

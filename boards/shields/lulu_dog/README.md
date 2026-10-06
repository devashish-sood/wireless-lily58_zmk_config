# Left nice!view: battery and Luna

Select `shield: lulu_dog` for `blelulu_left` in `build.yaml`.
The right half continues to use the upstream `nice_view` shield.

The screen shows the left battery percentage, USB/Bluetooth status, Luna,
and the active layer. Luna sits at 0 WPM, walks below 40 WPM, and runs at
40 WPM or above. Two frames alternate every 300 ms while ZMK is active;
animation stops when ZMK becomes idle. The keyboard sleeps after 15 minutes
of inactivity, configured in `boards/boardsource/blelulu/blelulu.conf`.

The rendering helpers and widget event handling are adapted from the MIT
licensed `invrtd/zmk` branch `nice!view-180`, under
`app/boards/shields/nice_view`. Rotation still follows
`CONFIG_NICE_VIEW_ROTATE_180`.

The six 32-by-22 Luna sprite frames are adapted from
`mctechnology17/zmk-nice-oled`,
`boards/shields/nice_oled/assets/luna_images.c`, using the horizontal,
unrotated bitmaps. See `LICENSE.luna` for the upstream MIT license.
They are drawn at twice their original size without requiring an LVGL
image-format conversion.

After a successful GitHub Actions build, flash
`lulu_dog-blelulu_left-zmk.uf2` to the left half. Verify the orientation,
battery percentage, animation while typing, and pause after idle on hardware.

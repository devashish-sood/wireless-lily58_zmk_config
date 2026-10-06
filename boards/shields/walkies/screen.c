/* SPDX-License-Identifier: MIT */
#include <lvgl.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zmk/battery.h>
#include "render.h"
#include "sync.h"

#if IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
#include <zmk/ble.h>
#include <zmk/endpoints.h>
#include <zmk/keymap.h>
#include <zmk/usb.h>
#else
#include <zmk/split/bluetooth/peripheral.h>
#endif

static struct walkies_state state;
static uint8_t image[WALKIES_BYTES];
static uint8_t previous[WALKIES_BYTES];
static uint8_t pixels[LV_CANVAS_BUF_SIZE(160,68,8,LV_DRAW_BUF_STRIDE_ALIGN)];
static bool first_frame=true;

static void update(lv_timer_t *timer) {
    lv_obj_t *canvas=lv_timer_get_user_data(timer);
    state.battery=zmk_battery_state_of_charge();
#if IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
    state.layer=zmk_keymap_highest_layer_active();
    state.usb=zmk_endpoint_get_selected().transport==ZMK_TRANSPORT_USB;
    state.connected=state.usb?zmk_usb_is_hid_ready():zmk_ble_active_profile_is_connected();
    state.paired=state.usb || !zmk_ble_active_profile_is_open();
#else
    state.connected=zmk_split_bt_peripheral_is_connected();
    state.moving=walkies_typing(&state.wpm) && state.connected;
    state.rest_frame=(k_uptime_get_32()/300)%2;
    lv_timer_set_period(timer,state.moving?(state.wpm>=40?150:300):300);
    if (state.moving) state.step++;
#endif
    walkies_render(image,&state,IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL));
    if (!first_frame && memcmp(image,previous,sizeof(image))==0) return;
    memcpy(previous,image,sizeof(image));
    first_frame=false;
    const uint32_t stride=lv_draw_buf_width_to_stride(160,LV_COLOR_FORMAT_L8);
    for (unsigned y=0;y<WALKIES_HEIGHT;y++) {
        for (unsigned x=0;x<WALKIES_WIDTH;x++) {
#if IS_ENABLED(CONFIG_NICE_VIEW_ROTATE_180)
            unsigned panel_x=159-y, panel_y=x;
#else
            unsigned panel_x=y, panel_y=67-x;
#endif
            pixels[panel_y*stride+panel_x]=walkies_pixel(image,x,y)?0:255;
        }
    }
    lv_obj_invalidate(canvas);
}

lv_obj_t *zmk_display_status_screen(void) {
    lv_obj_t *screen=lv_obj_create(NULL);
    lv_obj_t *canvas=lv_canvas_create(screen);
    lv_canvas_set_buffer(canvas,pixels,160,68,LV_COLOR_FORMAT_L8);
    lv_obj_set_pos(canvas,0,0);
    lv_timer_t *timer=lv_timer_create(update,250,canvas);
    update(timer);
    return screen;
}

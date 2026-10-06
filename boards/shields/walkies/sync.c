/* SPDX-License-Identifier: MIT
 * One display-only message per second while typing, over the existing split
 * behavior transport. No new GATT service, pairing, or host software.
 */
#define DT_DRV_COMPAT custom_walkies_sync
#include <zephyr/kernel.h>
#include <zephyr/sys/atomic.h>
#include <drivers/behavior.h>
#include <zmk/behavior.h>
#include "sync.h"

#define WALKIES_MAGIC 0x57414C4B
static atomic_t last_update;
static atomic_t speed;
static atomic_t received;

#if IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
#include <zmk/event_manager.h>
#include <zmk/events/keycode_state_changed.h>
#include <zmk/split/central.h>
#include <zmk/wpm.h>

static void send_activity(struct k_work *work);
K_WORK_DELAYABLE_DEFINE(activity_work, send_activity);

bool walkies_typing(uint8_t *wpm) {
    *wpm = zmk_wpm_get_state();
    return atomic_get(&received) &&
           (uint32_t)(k_uptime_get_32() - (uint32_t)atomic_get(&last_update)) < 2500;
}

static void send_activity(struct k_work *work) {
    uint8_t wpm;
    if (!walkies_typing(&wpm)) return;
    struct zmk_behavior_binding binding = {
        .behavior_dev = DEVICE_DT_NAME(DT_NODELABEL(walkies_sync)),
        .param1 = MAX(1, wpm),
        .param2 = WALKIES_MAGIC,
    };
    struct zmk_behavior_binding_event event = {.timestamp = k_uptime_get()};
    for (uint8_t i=0;i<ZMK_SPLIT_CENTRAL_PERIPHERAL_COUNT;i++) {
        // Display updates are disposable. A missing peer must not affect typing.
        (void)zmk_split_central_invoke_behavior(i, &binding, event, true);
    }
    k_work_schedule(&activity_work, K_SECONDS(1));
}

static int activity_listener(const zmk_event_t *eh) {
    const struct zmk_keycode_state_changed *ev=as_zmk_keycode_state_changed(eh);
    if (ev && ev->state) {
        atomic_set(&last_update,(atomic_val_t)k_uptime_get_32());
        atomic_set(&received,1);
        // schedule() preserves an existing deadline, bounding traffic to 1 Hz.
        k_work_schedule(&activity_work,K_NO_WAIT);
    }
    return ZMK_EV_EVENT_BUBBLE;
}
ZMK_LISTENER(walkies_activity,activity_listener);
ZMK_SUBSCRIPTION(walkies_activity,zmk_keycode_state_changed);
#else
bool walkies_typing(uint8_t *wpm) {
    *wpm=(uint8_t)atomic_get(&speed);
    return atomic_get(&received) &&
           (uint32_t)(k_uptime_get_32() - (uint32_t)atomic_get(&last_update)) < 2500;
}
#endif

static int receive_activity(struct zmk_behavior_binding *binding,
                            struct zmk_behavior_binding_event event) {
    if (binding->param2 != WALKIES_MAGIC) return -EINVAL;
    atomic_set(&speed,MIN(binding->param1,255));
    atomic_set(&last_update,(atomic_val_t)k_uptime_get_32());
    atomic_set(&received,1);
    return ZMK_BEHAVIOR_OPAQUE;
}

static const struct behavior_driver_api api={
    .locality=BEHAVIOR_LOCALITY_EVENT_SOURCE,
    .binding_pressed=receive_activity,
};
BEHAVIOR_DT_INST_DEFINE(0,NULL,NULL,NULL,NULL,POST_KERNEL,
                        CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,&api);

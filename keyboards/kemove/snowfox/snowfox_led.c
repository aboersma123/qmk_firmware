#include "snowfox.h"
#include "snowfox_ble.h"
#include "math.h"
const SPIConfig spi1Config = {
  .clock_divider = 1, // No Division
  .clock_prescaler = 24, // To 2MHz
  .clock_rate = 4, // Divide 2 again to be 1MHz
  .data_size = 8, // 8 bits per transfer
};

uint8_t led_brightness = 0xFF;

static mutex_t led_profile_mutex;

typedef void (*led_udpate_cb_t)(void);
typedef void (*led_action_up_t)(void);
typedef void (*led_action_dn_t)(void);

typedef struct {
    led_udpate_cb_t update;
    // led_action_up_t up;
    // led_action_dn_t dn;
} led_profile_t;

// ========= BEGIN PROFILES =========

void led_profile_cycle_update(void) {
    static HsvColor currentColor = {.h = 0, .s = 0xFF, .v = 0xFF};
    currentColor.h += 1;
    currentColor.v = led_brightness;
    RgbColor rgb = HsvToRgb(currentColor);
    for (int i = 0; i < 61; ++i)
    {
        sled_set_color(i, rgb.r, rgb.g, rgb.b);
    }
}

void led_profile_static_update(void) {
    static HsvColor currentColor = {.h = 0, .s = 0x00, .v = 0xFF};
    RgbColor rgb = HsvToRgb(currentColor);
    currentColor.v = led_brightness;
    for (int i = 0; i < 61; ++i)
    {
        sled_set_color(i, rgb.r, rgb.g, rgb.b);
    }
}

/* Cycle all odd led while keeping the even leds static */
void led_profile_cycle_update_half(void) {
    static HsvColor currentColor = {.h = 0, .s = 0xFF, .v = 0xFF};
    currentColor.h += 1;
    currentColor.v = led_brightness;
    RgbColor rgb = HsvToRgb(currentColor);
    for (int i = 0; i < 61; i += 2)
    {
        sled_set_color(i, rgb.r, rgb.g, rgb.b);
    }
}

/* Cycle odd leds in one direction and even leds in reverse */
void led_profile_cycle_update_bidirectional(void) {
    static HsvColor currentColor = {.h = 0, .s = 0xFF, .v = 0xFF};
    static HsvColor currentColor_reverse = {.h = 0, .s = 0xFF, .v = 0xFF};

    currentColor.h += 1;
    currentColor_reverse.h -= 1;
    currentColor.v = led_brightness;
    currentColor_reverse.v = led_brightness;
    RgbColor rgb = HsvToRgb(currentColor);
    RgbColor rgb_reverse = HsvToRgb(currentColor_reverse);
    for (int i = 0; i < 61; ++i)
    {
        if (i % 2) {
            sled_set_color(i, rgb.r, rgb.g, rgb.b);
        }
        else {
            sled_set_color(i, rgb_reverse.r, rgb_reverse.g, rgb_reverse.b);
        }
    }
}

/* Move from even color to a rainbow patern and back */
void led_profile_cycle_update_scale(void) {
    static int t = 0;
    const int bounce = 0xFF / 2;
    HsvColor currentColor = {.h = 0, .s = 0xFF, .v = 0xFF};
    currentColor.v = led_brightness;
    t += 1;
    if (t > 0xFF) t = 0;
    int r = abs((unsigned long) t - bounce);
    for (int i = 0; i < 61; ++i)
    {
        currentColor.h = (int) ((float) i * r) / 30;
        RgbColor rgb = HsvToRgb(currentColor);
        sled_set_color(i, rgb.r, rgb.g, rgb.b);
    }
}
// ========= END PROFILES ===========

const led_profile_t led_profiles[] = {
    {led_profile_cycle_update},
    {led_profile_static_update},
    {led_profile_cycle_update_half},
    {led_profile_cycle_update_bidirectional},
    {led_profile_cycle_update_scale},
};

static uint8_t current_profile = 0;
const uint8_t num_profiles = sizeof(led_profiles) / sizeof(led_profiles[0]);
static bool led_active = false;

void snowfox_early_led_init(void) {
    sled_early_init();
    chMtxObjectInit(&led_profile_mutex);
}

THD_WORKING_AREA(waLEDThread, 128);
THD_FUNCTION(LEDThread, arg) {
    (void)arg;
    chRegSetThreadName("LEDThread");
    sled_init();
    sled_off();
    while(1) {
        chThdSleepMilliseconds(50);
        chMtxLock(&led_profile_mutex); // Begin Critical Section
        led_udpate_cb_t updateFn = led_profiles[current_profile].update;
        chMtxUnlock(&led_profile_mutex); // End Critical Section
        (*updateFn)();
        sled_update_matrix();
    }
}

void snowfox_led_next(void) {
    chMtxLock(&led_profile_mutex);
    current_profile = (current_profile + 1) % num_profiles;
    chMtxUnlock(&led_profile_mutex);
}

void suspend_power_down_kb(void) {
#if ENABLE_SLEEP_LED == TRUE
    if (led_active && !snowfox_ble_is_active()) {
        sled_off();
    }
#endif
}

void suspend_wakeup_init_kb(void) {
#if ENABLE_SLEEP_LED == TRUE
    if (led_active) {
        sled_on();
    }
#endif
}

void snowfox_led_on(void) {
    if (led_active) {
        snowfox_led_next();
    } else {
        sled_on();
        led_active = true;
    }
}

void snowfox_led_off(void) {
    sled_off();
    led_active = false;
}

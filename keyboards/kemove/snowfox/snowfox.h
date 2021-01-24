#ifndef __SNOWFOX_H__
#define __SNOWFOX_H__

#include "quantum.h"
#include "ch.h"
#include "hal.h"
#include "sled17341.h"

#define SF_LED_STEP 0x40

enum SNOWFOX_KEYCODES {
    SNOWFOX_LED_ON = SAFE_RANGE,
    SNOWFOX_LED_OFF,
    SNOWFOX_LED_BUP,
    SNOWFOX_LED_BDN,
    SNOWFOX_BLE_CONN,
    SNOWFOX_BLE_DISCOVER,
    SNOWFOX_BLE_DISCONN,
    SNOWFOX_SAFE_RANGE,
};

#define OVERRIDE __attribute__ ((noinline))

extern THD_WORKING_AREA(waLEDThread, 128);
THD_FUNCTION(LEDThread, arg);

extern SerialConfig serialCfg;
extern thread_t *bleThread;

extern const SPIConfig spi1Config;
extern uint8_t led_brightness;

#endif

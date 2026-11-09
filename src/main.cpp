/**
 * main.cpp — Snake EX2 AVR application: hardware setup and the main loop.
 *
 * The screens (src/app/screen_*.cpp) are
 *
 *   title      full-screen title art, 3 s or key; after a game it shows the final
 *              score (plus two trophies for a new best) and waits for a key
 *   menu       [Continue] / New game / High scores / Settings / Instructions
 *   credits    shown by "Instructions"
 *   game       Classic or Extreme, using the saved EEPROM settings. Back (Pause) saves the
 *              game to EEPROM and returns to the menu; Continue restores it and waits for Fire
 *
 * The old hardware self-test is no longer run at boot: the drivers are covered
 * by the host tests and have been hardware-verified. Keeping boot on the title
 * also satisfies the cold-boot <2 s requirement.
 */

#include <Arduino.h>
#include <avr/io.h>

#include "config.h"
#include "hal/gpio.h"
#include "drivers/st7735.h"
#include "drivers/buttons.h"
#include "drivers/battery.h"
#include "drivers/buzzer.h"
#include "drivers/eeprom_store.h"
#include "drivers/stack_measure.h"
#include "app/screens.h"
#include "ui/chrome.h"

void setup() {
#if STACK_MEASURE
    /* Paint free SRAM before any driver/game work; the credits screen
     * reports the low-water mark.  Release builds compile this out. */
    stack_measure_begin();
#endif

    // Disable JTAG to free PORTC (PC2–PC5). ATmega644P/PA moves JTD to MCUCR;
    // the ATmega32 fallback used MCUCSR.
#if defined(__AVR_ATmega644A__) || defined(__AVR_ATmega644__) || \
    defined(__AVR_ATmega644P__) || defined(__AVR_ATmega644PA__)
    MCUCR = (1 << JTD);
    MCUCR = (1 << JTD);
#else
    MCUCSR = (1 << JTD);
    MCUCSR = (1 << JTD);
#endif

    gpio_init();
    st7735_init();
    buttons_init();
    buzzer_init();
    eeprom_store_init();                 // writes/repairs defaults on first boot

    st7735_backlight(eeprom_get_settings()->backlight);
    screen_sync_silent();
    chrome_set_battery(battery_percent(battery_read_mv()));
    screen_title_open();
}

void loop() {
    uint8_t raw = buttons_poll();

    uint8_t btn;
    btn_event_t ev;
    while ((ev = buttons_get_event(&btn)) != BTN_EVENT_NONE) screen_on_button(btn, ev);

    screen_tick(millis(), raw);

    static uint32_t next_battery_ms = BATTERY_POLL_MS;
    if ((int32_t)(millis() - next_battery_ms) >= 0) {
        next_battery_ms += BATTERY_POLL_MS;
        chrome_set_battery(battery_percent(battery_read_mv()));
    }

    if (buzzer_is_playing()) buzzer_update();
    delay(2);
}

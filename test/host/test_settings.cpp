/* Host test: settings screen state, labels, value changes, and partial repaint. */
#include "ui/settings.h"
#include "render/text.h"
#include "drivers/eeprom_store.h"
#include "mock_display.h"
#include <stdio.h>
#include <string.h>

/* Mock the EEPROM storage for settings (referenced by the avr/eeprom.h mock
 * used when eeprom_store.cpp is compiled). */
uint8_t mock_eeprom[1024];
unsigned mock_eeprom_writes;

/* Mock millis for the Arduino mock. */
uint32_t mock_millis = 0;

static uint16_t before[160][128];

int main() {
    settings_init();
    CHECK(settings_selected() == SETTING_MODE);

    /* Move down through all items and wrap */
    settings_move(1);
    CHECK(settings_selected() == SETTING_MAZE);
    settings_move(1);
    CHECK(settings_selected() == SETTING_SPEED);
    settings_move(1);
    CHECK(settings_selected() == SETTING_SOUND);
    settings_move(1);
    CHECK(settings_selected() == SETTING_MODE);

    /* Move up wraps */
    settings_move(-1);
    CHECK(settings_selected() == SETTING_SOUND);

    /* Full screen draw: wallpaper + chrome, four rows, "Save" and "Back" soft keys */
    settings_init();
    settings_draw();
    CHECK(dark_count(96, 1, 32, 12) > 25);           /* status bar: the clock digits */
    CHECK(count_color(4, 18, 50, 7, 0x0000) > 15);            /* "Settings" title */
    for (int i = 0; i < 4; i++) {
        int y = 30 + i * 20;
        CHECK(non_bg(4, y + 4, 40, 12, fb[y + 2][122]) > 20);  /* label ink on the row */
    }
    CHECK(fb[30][0] == 0x03DB && fb[49][0] == 0x02F8);        /* first row is the selected band */
    CHECK(fb[40][119] != fb[40][120]);                         /* band is 120 wide */
    CHECK(count_color(4, 34, 40, 12, 0xFFFF) > 20);            /* selected label is white */
    CHECK(count_color(50, 144, 28, 12, 0x0010) > 20);          /* "Save", centred */
    CHECK(count_color(100, 148, 24, 7, 0x0010) > 10);          /* "Back" bottom-right */
    CHECK(fb[100][124] == 0x7C7A && fb[35][123] == 0xFF5A);    /* scrollbar track + thumb */
    CHECK(count_color(60, 34, 56, 12, 0xFFFF) > 20);           /* value text */

    /* Adjusting a value repaints only that row (settings_draw_value). */
    memcpy(before, fb, sizeof fb);
    fill_screen_calls = 0;
    settings_adjust(1);               /* CLASSIC -> EXTREME */
    settings_draw_value(settings_selected());
    CHECK(fill_screen_calls == 0);
    CHECK(memcmp(before, fb, sizeof fb) != 0);
    {
        int changed_left = 0;                                  /* label side untouched */
        for (int y = 0; y < 160; y++)
            for (int x = 0; x < 128; x++)
                if (before[y][x] != fb[y][x] && (x < 55 || y < 30 || y >= 50)) changed_left++;
        CHECK(changed_left == 0);
    }
    memcpy(before, fb, sizeof fb);
    settings_draw();
    CHECK(memcmp(before, fb, sizeof fb) == 0);

    /* Edits are not stored until settings_save(); re-init drops them. */
    CHECK(eeprom_get_settings()->mode == MODE_CLASSIC);
    settings_init();
    settings_adjust(1);
    settings_move(2); settings_adjust(1);
    settings_init();
    settings_save();
    CHECK(eeprom_get_settings()->mode == MODE_CLASSIC);
    settings_adjust(1);
    settings_save();                  /* no popup: just stores */
    CHECK(eeprom_get_settings()->mode == MODE_EXTREME);
    settings_adjust(1);
    settings_save();
    CHECK(eeprom_get_settings()->mode == MODE_CLASSIC);

    /* Moving the selection repaints two rows + thumb */
    settings_init();
    settings_draw();
    settings_move(1);
    settings_draw_row(SETTING_MODE);
    settings_draw_row(settings_selected());
    memcpy(before, fb, sizeof fb);
    settings_draw();
    CHECK(memcmp(before, fb, sizeof fb) == 0);

    /* Speed increments and clamps */
    settings_init();
    settings_move(2); /* SPEED */
    settings_draw();
    settings_adjust(1);
    settings_draw_value(settings_selected());
    /* second adjust should increment again */
    settings_adjust(1);
    settings_draw_value(settings_selected());
    /* going down should decrement */
    settings_adjust(-1);
    settings_draw_value(settings_selected());
    /* clamp at 0 */
    for (int i = 0; i < 10; i++) settings_adjust(-1);
    settings_draw_value(settings_selected());
    /* clamp at max */
    for (int i = 0; i < 10; i++) settings_adjust(1);
    settings_draw_value(settings_selected());

    /* Speed wraps 5 -> 0 and 0 -> 5 (drawn value is the digit in the selected row) */
    settings_init();
    settings_move(2);
    for (int i = 0; i < 6; i++) settings_adjust(1);        /* full cycle back to the start */
    settings_draw();
    memcpy(before, fb, sizeof fb);
    for (int i = 0; i < 5; i++) settings_adjust(1);        /* now at 5 */
    settings_adjust(1);                                    /* wraps to the start */
    settings_draw();
    CHECK(memcmp(before, fb, sizeof fb) == 0);
    settings_adjust(-1);                                   /* 0 -> 5 */
    settings_draw();
    CHECK(memcmp(before, fb, sizeof fb) != 0);
    settings_adjust(1);
    settings_draw();
    CHECK(memcmp(before, fb, sizeof fb) == 0);

    /* Sound toggle */
    settings_init();
    settings_move(3); /* SOUND */
    settings_draw();
    settings_adjust(1);  /* toggle */
    settings_draw_value(settings_selected());
    settings_adjust(1);  /* toggle back */
    settings_draw_value(settings_selected());

    /* Maze cycles */
    settings_init();
    settings_move(1); /* MAZE */
    for (int i = 0; i < 7; i++) {
        settings_adjust(1);
        settings_draw_value(settings_selected());
    }
    for (int i = 0; i < 7; i++) {
        settings_adjust(-1);
        settings_draw_value(settings_selected());
    }

    printf(fails ? "settings: %d FAILED\n" : "settings: all passed\n", fails);
    return fails != 0;
}
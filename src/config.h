/**
 * config.h — Snake EX2 AVR Port: Central Configuration
 *
 * Every constant used by the port has a named reference to its original
 * source (decompiled code, docs/reference.md, or BACKLOG.MD).
 *
 * Do not change these values without cross-referencing the original and
 * the 128×160 adaptation documented in BACKLOG.MD §1.5.
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>

/* ── Display ─────────────────────────────────────────────────────────────── */

/** ST7735 panel resolution (target: 128×160, never scale or letterbox). */
#define TFT_WIDTH  128   // BACKLOG.MD §1.5, §2.5
#define TFT_HEIGHT 160

/** Panel variant offsets (set per actual module; defaults for common variant). */
#define TFT_OFFSET_X  0  // adjust after color-bar test (M1/H2)
#define TFT_OFFSET_Y  0
#define TFT_BGR       0  // 0=RGB, 1=BGR — set after color-bar test
#define TFT_INVERT    0  // set after color-bar test

/** Hardware SPI pins (ATmega644P/PA; same pinout as the retired ATmega32). BACKLOG.MD §2.3. */
#define LCD_CS   PB4
#define LCD_DC   PB3
#define LCD_RST  PB2
#define LCD_BL   PC1

/* ── Geometry ────────────────────────────────────────────────────────────── */
/** Derived 128×160 layout. BACKLOG.MD §1.5.
 *
 * Original 128×128 layout has playfield 126×108 at (1,0), HUD at (0,108).
 * Target 128×160 layout has playfield 126×138 at (1,1), HUD at (0,139),
 * 1px black borders at y=0 and y=159.
 */

/** Grid dimensions. Original: 21×18. Target: 21×23. BACKLOG.MD §1.1, §1.5. */
#define FIELD_COLS 21   // original: SnakeCanvas, ResourceDefs
#ifndef FIELD_ROWS
#define FIELD_ROWS 23   // derived: BACKLOG.MD §1.5 maze extension rule (host tests build with 18 to compare with the original)
#endif

#define CELL_SIZE        6   // pixels per cell side. BACKLOG.MD §1.1
#define FIELD_CELLS      (FIELD_COLS * FIELD_ROWS)  // 483

/** Playfield pixel area. */
#define FIELD_WIDTH_PX   (FIELD_COLS * CELL_SIZE)   // 126
#define FIELD_HEIGHT_PX  (FIELD_ROWS * CELL_SIZE)   // 138

/** Playfield background colour 0xEEDDBB (b.java A(): setColor(-1122885)), RGB565. Black only during lights-out. */
#define COLOR_FIELD_BG   0xEEF7

/** Playfield screen origin. Pixel coords are translated by (1,1). */
#define FIELD_ORIGIN_X   1
#define FIELD_ORIGIN_Y   1

/** HUD position. BACKLOG.MD §1.5 derived-layout table, Appendix B. */
#define HUD_X            0
#define HUD_Y            139   // original: 108
#define HUD_WIDTH        128
#define HUD_HEIGHT       20

/**
 * HUD contents, verified against b.java (draw): digits are 8x9 cells, pitch 8;
 * score at (6,114), bonus at (69,114), icon slot at (110,111) in the 128x128 original,
 * i.e. y + 31 in the derived layout (x unchanged). NB: an earlier note had score x=39;
 * that is the game-over screen (Snake.java), not the in-game HUD.
 */
#define SCORE_X          6
#define SCORE_Y          145   // original: 114
#define SCORE_DIGITS     6
#define SCORE_DIGIT_PITCH 8

#define BONUS_X          69
#define BONUS_Y          145   // original: 114
#define BONUS_DIGITS     4

/** Power-up icon slot: fixed position; the strip is clipped to icon (id-1). */
#define POWERUP_ICON_X   110
#define POWERUP_ICON_Y   142   // original: 111
#define POWERUP_ICON_SIZE 13

/** Title screen: 128×128 art at (0,16), centered vertically. BACKLOG.MD §1.2, D10. */
#define TITLE_X          0
#define TITLE_Y          16
#define TITLE_WIDTH      128
#define TITLE_HEIGHT     128
#define TITLE_TOP_COLOR    0xD612   // title row 0 colour (213,193,148)
#define TITLE_BOTTOM_COLOR 0x62EA   // title row 127 colour (98,95,83)

/* ── Snake start state ───────────────────────────────────────────────────── */
/** Identical to original. BACKLOG.MD §1.1, Appendix B. */
#define START_LENGTH     6
#define START_HEAD_COL   7     // cell column (0-based)
#define START_HEAD_ROW   9     // cell row (0-based)
#define START_HEAD_X     (START_HEAD_COL * CELL_SIZE)  // 42 px
#define START_HEAD_Y     (START_HEAD_ROW * CELL_SIZE)  // 54 px
#define START_DIR        8   // = DIR_RIGHT; b.java: j=0x28, low nibble 8 (head x += 6)

/** Initial food. BACKLOG.MD §1.1. */
#define START_FOOD_COL   4
#define START_FOOD_ROW   4
#define START_FOOD_X     (START_FOOD_COL * CELL_SIZE)  // 24 px
#define START_FOOD_Y     (START_FOOD_ROW * CELL_SIZE)  // 24 px

/* ── Direction constants ─────────────────────────────────────────────────── */
/**
 * Match the original direction bits. Verified in b.java head-move switch:
 * 1: y-=6 (up), 4: y+=6 (down), 2: x-=6 (left), 8: x+=6 (right).
 * Stored direction bytes also carry 0x10 (grown segment) and 0x20/0x40 (turn flags).
 * Defined before START_DIR use; keep this block above it.
 */
#define DIR_UP    1
#define DIR_LEFT  2
#define DIR_DOWN  4
#define DIR_RIGHT 8
#define DIR_MASK  0x0F

/* ── Timing ───────────────────────────────────────────────────────────────── */
/** 15 fps. BACKLOG.MD §1.1 (original programmer's notes); §4.4. */
#define TARGET_FPS       15
#define FRAME_PERIOD_MS  (1000 / TARGET_FPS)  // ~66.67 ms

/**
 * Movement cadence: framesPerCell = 2 + (8 - speedIndex).
 * speedIndex 0..5 → framesPerCell 10,9,8,7,6,5.  BACKLOG.MD §9, Appendix B.
 */
#define SPEED_FRAMES_BASE 2
#define SPEED_OFFSET      8
#define SPEED_INDEX_MIN   0
#define SPEED_INDEX_MAX   5
#define SPEED_INDEX_COUNT 6

/** Speed power-up effects. b.java c() method. */
#define SPEED_UP_DELTA   2   // subtract, clamp >= 1
#define SLOW_DOWN_DELTA  2   // add, clamp <= 10
#define SPEED_MIN_FRAMES 1
#define SPEED_MAX_FRAMES 10

/** Collision/death animation. b.java — 40 engine ticks. BACKLOG.MD §1.1. */
#define DEATH_TICKS      40

/** Active power-up duration range. b.java: ba = 25 + rand%50. */
#define POWERUP_DURATION_BASE 25
#define POWERUP_DURATION_RAND 50

/**
 * Power-ups exist only in Extreme mode. Spawn is attempted when FOOD IS EATEN
 * (not per tick): each empty slot spawns with 1/16 chance, id = 1 + rand%10,
 * timer 25 + rand%50. Verified b.java ~1620.
 */
#define POWERUP_SPAWN_CHANCE 16   // (rand & 0xF) == 15

/* ── Snake mechanics ──────────────────────────────────────────────────────── */

/** Normal food score = speedIndex + 1. b.java: Aa(n). BACKLOG.MD §1.1. */
#define FOOD_SCORE_BASE  1

/** Bonus food adds its countdown value. BACKLOG.MD §1.1, Appendix B. */
/**
 * Bonus boost (ID 5): the next 3 bonus foods start with +24 on their countdown
 * timer (b.java ~1590); bonus timer is otherwise 25 + rand%50.
 */
#define BONUS_BOOST_EXTRA 24

/** Snakes per wrap: x wraps at 0↔126, y wraps at 0↔138. BACKLOG.MD §1.1, App.B. */
#define WRAP_X_MIN  0
#define WRAP_X_MAX  FIELD_WIDTH_PX   // 126 → x + 126 or x - 126
#define WRAP_Y_MIN  0
#define WRAP_Y_MAX  FIELD_HEIGHT_PX  // 138 → y + 138 or y - 138

/** Tongue range: 2 cells ahead. b.java draw method. BACKLOG.MD §1.1. */
#define TONGUE_RANGE 2

/* ── Extreme campaign ─────────────────────────────────────────────────────── */
/** Portal threshold: score > levelStartScore + speedIndex*10 + levelNumber*20. */
/** BACKLOG.MD §1.1, Appendix B. */
#define PORTAL_SPEED_MULT  10
#define PORTAL_LEVEL_MULT  20

/** Maze cycle in Extreme mode: -1,0,1,2,3,4. b.java, BACKLOG.MD §1.1. */
#define EXTREME_MAZE_COUNT 6

/** Snake.J is the instructions/tutorial sequence (U5), not campaign progress:
 *  62 entries = 55 text pages + seven demo games (-1..-7); -8 is the high-score demo. */
#define TUTORIAL_SCRIPT_COUNT 62
#define TUTORIAL_DEMO_COUNT   8

/** Portal shrink: one segment per tick until length <= 2, then reset. */
#define PORTAL_SHRINK_MIN_LENGTH 2

/* ── Food ─────────────────────────────────────────────────────────────────── */
/** Food spawn retry: 8 attempts, then fallback scan. b.java, BACKLOG.MD §1.1. */
#define FOOD_SPAWN_RETRIES 8

/** Moving food timer: Ea = (15 + rand%15) * ((8 - speedIndex>>1) + 1). b.java. */
#define MOVING_FOOD_TIMER_MIN 15

/* ── Coin rush ────────────────────────────────────────────────────────────── */
/** Map overlay flag for coin rush cells. b.java. */
#define MAP_FLAG_SNAKE   0x10   // snake occupancy (transient)
#define MAP_FLAG_COIN    0x20   // coin rush overlay

/* ── Power-up inventory ───────────────────────────────────────────────────── */
/** 10 power-up types. b.java: c() method. BACKLOG.MD Appendix B. */
#define POWERUP_TYPE_COUNT 10
#define POWERUP_SLOTS       4   // Ba[4][4] in original
#define POWERUP_EXPIRED_ID  15  // Aa=15 means no active power-up

/** Power-up icon strip layout (SnakeCanvas.E, 130×13). */
#define POWERUP_STRIP_ICON_WIDTH 13
#define POWERUP_STRIP_ICON_COUNT 10

/* ── Audio ────────────────────────────────────────────────────────────────── */
/** 13 sounds (12 + 1 synthetic beep). BACKLOG.MD §1.3. */
#define SFX_COUNT         13
#define SFX_BEEP_INDEX    12
#define SFX_BEEP_FREQ_HZ  1760
#define SFX_BEEP_DURATION_MS 100

/**
 * playSFX indices, verified against b.java c() and Snake.java:
 *  0 = speed-up power-up (ID 1; also ID 4 wall-stamp fallback)
 *  1 = slow-down power-up (ID 2)
 *  2 = wall-ghost power-up (ID 3)
 *  3 = wall-stamp power-up success (ID 4)
 *  4 = shrink power-up (ID 6)
 *  5 = grow power-up (ID 7)
 *  6 = moving-food power-up (ID 9)
 *  7 = bonus-boost power-up (ID 5)
 *  8 = coin-rush power-up (ID 8)
 *  9 = lights-out power-up (ID 10)
 * 10 = death / collision
 * 11 = new high score (Snake.java game-over)
 * 12 = synthetic 1760 Hz beep: played on eating normal food and bonus food
 * Coin pickups play no sound.
 */

/* ── Maze wall tiles ──────────────────────────────────────────────────────── */
/**
 * wall_strip.png: 120×6, 20 tiles of 6×6.
 * The game uses tiles 1..15 (tile IDs 0..14 in the extracted index).
 * Backlog §1.2. Tile hex values in maze maps correspond to wall tile index+1.
 */
#define WALL_TILE_COUNT   15   // tiles used in gameplay (indices 0..14)
#define WALL_STRIP_TILES  20   // total tiles in the strip
#define WALL_TILE_SIZE     6

/* ── EEPROM layout ────────────────────────────────────────────────────────── */
/** BACKLOG.MD §3.3. */
#define EEPROM_MAGIC      0x53324558   // "S2EX"
#define EEPROM_VERSION    1
#define EEPROM_MAGIC_OFF  0
#define EEPROM_SETTINGS_OFF 8
#define EEPROM_HIGH_CLASSIC_OFF 32
#define EEPROM_HIGH_EXTREME_OFF 48
#define EEPROM_RESUME_OFF 64
#define EEPROM_SIZE       1024

/* ── HUD digit font ───────────────────────────────────────────────────────── */
/** digits.png: 77×9, 11 glyphs × 7 px. BACKLOG.MD §1.2. */
#define DIGIT_GLYPH_WIDTH  7
#define DIGIT_GLYPH_HEIGHT 9
#define DIGIT_GLYPH_COUNT  11   // digits 0..9 + one extra

/* ── Resource extraction offsets ──────────────────────────────────────────── */
/** From d blob at specified offset in BACKLOG.MD §1.2 and docs/reference.md. */
#define D_OFFSET_MAZES        0     // 2268 bytes — 6×378 original bytes
#define D_OFFSET_TITLE        2268  // 3616 bytes — title.png 128×128
#define D_OFFSET_DIGITS       5884  // 1280 bytes — digits.png 77×9
#define D_OFFSET_HUD          7164  // 1833 bytes — hud.png 128×20
#define D_OFFSET_G            8997  // 287 bytes — g.png 11×11
#define D_OFFSET_G_CAP        9284  // 403 bytes — G.png 60×21
#define D_OFFSET_POWERUP_STRIP 9687 // 1440 bytes — powerup_strip.png 130×13
#define D_OFFSET_F            11127 // 184 bytes — f.png 11×11
#define D_OFFSET_WALL_STRIP   11311 // 309 bytes — wall_strip.png 120×6
#define D_OFFSET_SPRITES      11620 // 3982 bytes — 61 sprite records
#define D_SIZE                15602

/* ── Flash budget ─────────────────────────────────────────────────────────── */
/** BACKLOG.MD §3.1. */
#define FLASH_TOTAL_KB   32
#define FLASH_TARGET_KB  30   // leave 2 KB reserve

/* ── RAM budget ───────────────────────────────────────────────────────────── */
/** BACKLOG.MD §3.2. */
#define RAM_TOTAL_B      2048   // 2 KB
#define RAM_FIELD_MAP    483    // uint8_t[483] maze map
#define RAM_DIR_HISTORY  483    // uint8_t[483] direction history
#define RAM_STACK_MIN    256    // minimum stack margin

/* ── Feature flags ────────────────────────────────────────────────────────── */
/** BACKLOG.MD §4.7, §10 Decisions. */
#define FEATURE_TITLE_ART     1   // full title art in flash (~8.9 KB); 0 = small built-in logo
#define FEATURE_PASSIVE_BUZZER 0  // set to 1 when passive piezo is wired (D3)
#define FEATURE_EXTREME_CAMPAIGN 1 // D5: implement all 60 stages
#define FEATURE_RESUME_SAVE   0   // set to 1 if it fits (D7)

#endif /* CONFIG_H */
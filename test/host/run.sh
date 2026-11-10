#!/bin/sh
# Host unit tests (no hardware). Run from project root: sh test/host/run.sh
set -e
cd "$(dirname "$0")/../.."
OUT=${TMPDIR:-/tmp}/snake_host_tests; mkdir -p $OUT
F="-Wall -Wextra -I test/host -I src"
g++ $F test/host/test_eeprom_store.cpp src/drivers/eeprom_store.cpp -o $OUT/t_eeprom && $OUT/t_eeprom
g++ $F test/host/test_buttons.cpp src/drivers/buttons.cpp -o $OUT/t_buttons && $OUT/t_buttons
g++ $F test/host/test_buzzer.cpp src/drivers/buzzer.cpp src/assets/sounds_gen.cpp -o $OUT/t_buzzer && $OUT/t_buzzer
g++ $F test/host/test_battery.cpp src/drivers/battery.cpp -o $OUT/t_battery && $OUT/t_battery
g++ $F test/host/test_stack_measure.cpp src/drivers/stack_measure.cpp -o $OUT/t_stack && $OUT/t_stack
# UI tests share one "chrome" source set: the screen chrome, list widget, sprites, text, assets.
CHROME="src/ui/chrome.cpp src/ui/list.cpp src/render/sprite.cpp src/assets/statusbar.cpp src/assets/wallpaper.cpp src/render/image.cpp src/render/text.cpp src/assets/assets_gen.cpp"
STORE="src/drivers/eeprom_store.cpp src/assets/maze.cpp"
HS="src/ui/highscores.cpp src/render/hud.cpp src/assets/trophy.cpp"
g++ $F test/host/test_menu.cpp src/ui/menu.cpp $CHROME -o $OUT/t_menu && $OUT/t_menu
g++ $F test/host/test_credits.cpp src/ui/credits.cpp $CHROME -o $OUT/t_credits && $OUT/t_credits
g++ $F test/host/test_settings.cpp src/ui/settings.cpp src/ui/menu.cpp $CHROME $STORE -o $OUT/t_settings && $OUT/t_settings
g++ $F test/host/test_highscores.cpp $HS src/ui/menu.cpp $CHROME $STORE -o $OUT/t_highscores && $OUT/t_highscores
g++ $F test/host/test_transitions.cpp $HS src/ui/settings.cpp src/ui/menu.cpp src/ui/credits.cpp $CHROME $STORE -o $OUT/t_transitions && $OUT/t_transitions
g++ $F test/host/test_screens.cpp src/app/screen.cpp src/app/screen_menu.cpp src/app/screen_scores.cpp src/app/screen_credits.cpp src/app/screen_settings.cpp $HS src/ui/settings.cpp src/ui/menu.cpp src/ui/credits.cpp $CHROME $STORE -o $OUT/t_screens && $OUT/t_screens
g++ $F test/host/test_savegame.cpp src/game/savegame.cpp src/game/game.cpp src/game/rng.cpp src/drivers/eeprom_store.cpp -o $OUT/t_savegame && $OUT/t_savegame
g++ $F -I test/host test/host/test_snake_sprites.cpp src/render/snake_sprites.cpp src/assets/assets_gen.cpp -o $OUT/t_spr && $OUT/t_spr
g++ $F test/host/test_game_init.cpp src/game/game.cpp src/assets/maze.cpp src/assets/assets_gen.cpp -o $OUT/t_init && $OUT/t_init
g++ $F test/host/test_tongue.cpp src/game/game.cpp src/assets/maze.cpp src/assets/assets_gen.cpp -o $OUT/t_tongue && $OUT/t_tongue
g++ $F test/host/test_powerup.cpp src/game/game.cpp -o $OUT/t_powerup && $OUT/t_powerup
g++ $F test/host/test_extreme.cpp src/game/game.cpp -o $OUT/t_extreme && $OUT/t_extreme
g++ $F test/host/test_playfield.cpp src/game/game.cpp src/render/playfield.cpp src/render/field.cpp src/render/sprite.cpp src/render/snake_sprites.cpp src/assets/assets_gen.cpp src/assets/maze.cpp -o $OUT/t_pf && $OUT/t_pf
# Q5: pixel-count frame budget at 4 MHz SPI (normal frames < 50 ms; full redraws reported).
g++ $F test/host/bench_frame.cpp src/game/game.cpp src/render/playfield.cpp src/render/field.cpp src/render/sprite.cpp src/render/snake_sprites.cpp src/render/hud.cpp src/render/image.cpp src/render/text.cpp src/assets/trophy.cpp src/assets/assets_gen.cpp src/assets/maze.cpp -o $OUT/bench_frame && $OUT/bench_frame

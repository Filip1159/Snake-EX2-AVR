/* Replays Extreme-mode sandbox traces (tools/oracle, build/oracle/traces_pu) against the C++
 * core, including every G7 power-up field: active id/timer, slots, coin bitmap, wall stamp,
 * moving-food timer and lights-out state. Built with -DFIELD_ROWS=18. */
#include "game/game.h"
#include "game/rng.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <vector>
#include <dirent.h>

/* java.util.Random */
static uint64_t jseed;
static void jr_seed(int64_t s) { jseed = ((uint64_t)s ^ 0x5DEECE66DULL) & ((1ULL << 48) - 1); }
static int32_t jr_next32() { jseed = (jseed * 0x5DEECE66DULL + 0xBULL) & ((1ULL << 48) - 1); return (int32_t)(jseed >> 16); }
extern "C" int32_t game_rand_int(void) { return jr_next32(); }

static std::vector<uint8_t> d_blob;
static uint8_t campaign_map[FIELD_CELLS];
static int total_ticks, files, fails;

static int8_t maze_loader(uint8_t index, uint8_t *buf) {
    if (index >= 6) return -1;
    memcpy(buf, &d_blob[(size_t)index * FIELD_CELLS], FIELD_CELLS);
    return 0;
}

static std::vector<std::string> split(const std::string &s, char c) {
    std::vector<std::string> v; size_t i = 0;
    for (;;) { size_t j = s.find(c, i); if (j == std::string::npos) { v.push_back(s.substr(i)); break; } v.push_back(s.substr(i, j - i)); i = j + 1; }
    return v;
}

static int coin_count(const game_t *g) {
    int n = 0;
    for (int i = 0; i < FIELD_CELLS; i++) n += (g->coins[i >> 4] >> (i & 15)) & 1;
    return n;
}

static bool run(const std::string &path, const std::string &name) {
    int speed; long seed;
    if (sscanf(name.c_str(), "extreme_s%d_r%ld", &speed, &seed) != 2 &&
        sscanf(name.c_str(), "campaign_s%d_r%ld", &speed, &seed) != 2) return true;
    FILE *f = fopen(path.c_str(), "r");
    if (!f) { printf("cannot open %s\n", path.c_str()); return false; }
    jr_seed(seed);
    static game_t g;
    game_init_mode(&g, (uint8_t)speed, -1, nullptr, true);
    /* Traces in this directory are either the G7 sandbox (Java z=1e9, no portals) or
     * natural campaign play (z=0). The directory basename decides; both start with
     * "extreme_" or "campaign_" respectively. */
    bool campaign = name.rfind("campaign_", 0) == 0;
    if (!campaign) g.level_start_score = 1000000000u;
    game_set_maze_source(&g, campaign_map, maze_loader);
    char line[4096];
    int tick = 0;
    while (fgets(line, sizeof line, f)) {
        auto c = split(line, ',');
        if (c.size() < 59) continue;            /* trailing comma adds one empty field */
        int keys = atoi(c[1].c_str());
        game_tick(&g, (uint8_t)keys);
        total_ticks++;
        auto bad = [&](const char *what, long got, long want) {
            printf("MISMATCH %s tick %d: %s got %ld want %ld\n", name.c_str(), tick, what, got, want);
            fclose(f); return false; };
        /* base engine state */
        if (g.len != (unsigned)atoi(c[2].c_str())) return bad("len", g.len, atoi(c[2].c_str()));
        if (g.qx != atoi(c[3].c_str())) return bad("Q", g.qx, atoi(c[3].c_str()));
        if (g.qy != atoi(c[4].c_str())) return bad("r", g.qy, atoi(c[4].c_str()));
        if (g.dir != (atoi(c[5].c_str()) & 0xFF)) return bad("j", g.dir, atoi(c[5].c_str()));
        if ((long)g.score != atol(c[6].c_str())) return bad("score", g.score, atol(c[6].c_str()));
        if (g.death != atoi(c[7].c_str())) return bad("Ja", g.death, atoi(c[7].c_str()));
        if (g.food_x != atoi(c[8].c_str())) return bad("Da", g.food_x, atoi(c[8].c_str()));
        if (g.food_y != atoi(c[9].c_str())) return bad("ea", g.food_y, atoi(c[9].c_str()));
        if (g.bonus != atoi(c[10].c_str())) return bad("fa", g.bonus, atoi(c[10].c_str()));
        if (g.bonus_x != atoi(c[11].c_str())) return bad("ga", g.bonus_x, atoi(c[11].c_str()));
        if (g.bonus_y != atoi(c[12].c_str())) return bad("Ga", g.bonus_y, atoi(c[12].c_str()));
        if (g.bonus_type != atoi(c[13].c_str())) return bad("Fa", g.bonus_type, atoi(c[13].c_str()));
        if (g.cnt_b != atoi(c[14].c_str())) return bad("b", g.cnt_b, atoi(c[14].c_str()));
        if (g.cnt_c != atoi(c[15].c_str())) return bad("c", g.cnt_c, atoi(c[15].c_str()));
        if (g.cnt_C != atoi(c[16].c_str())) return bad("C", g.cnt_C, atoi(c[16].c_str()));
        if (g.per_ma != atoi(c[18].c_str())) return bad("ma", g.per_ma, atoi(c[18].c_str()));
        if (g.tongue != atoi(c[19].c_str())) return bad("J", g.tongue, atoi(c[19].c_str()));

        /* segments */
        const std::string &seg = c[20];
        if (seg.size() != g.len * 2) return bad("seglen", seg.size() / 2, g.len);
        for (unsigned i = 0; i < g.len; i++) {
            unsigned v = strtoul(seg.substr(i * 2, 2).c_str(), nullptr, 16);
            if (g.a[i] != v) { printf("MISMATCH %s tick %d: a[%u] got %02X want %02X\n", name.c_str(), tick, i, g.a[i], v); fclose(f); return false; }
        }

        /* events */
        uint16_t mask = 0; bool over = false;
        for (auto &e : split(c[21], '|')) {
            int id; if (sscanf(e.c_str(), "sfx%d", &id) == 1) mask |= SFXBIT(id);
            if (e.find("state-123") != std::string::npos) over = true;
        }
        if (g.sfx_mask != mask) return bad("sfx", g.sfx_mask, mask);
        if (g.game_over != over) return bad("game_over", g.game_over, over);

        /* G7 power-up state */
        if (g.pu_active != atoi(c[17].c_str())) {
            printf("  head %d,%d dir %u tongue %d\n", g.qx, g.qy, g.dir, g.tongue);
            for (int i = 0; i < 4; i++)
                printf("  slot%d ours id=%u life=%u px=%u,%u | orig id=%s life=%s px=%s,%s\n", i,
                       g.pu[i].id, g.pu[i].life, g.pu[i].x, g.pu[i].y,
                       c[31 + i * 4].c_str(), c[32 + i * 4].c_str(),
                       c[33 + i * 4].c_str(), c[34 + i * 4].c_str());
            return bad("Aa", g.pu_active, atoi(c[17].c_str()));
        }
        if (g.pu_timer != atoi(c[22].c_str())) return bad("ba", g.pu_timer, atoi(c[22].c_str()));
        if (g.pu_saved_period != atoi(c[23].c_str())) return bad("na", g.pu_saved_period, atoi(c[23].c_str()));
        if (g.move_food_timer != atoi(c[24].c_str())) return bad("Ea", g.move_food_timer, atoi(c[24].c_str()));
        if ((int)g.lights_out != atoi(c[25].c_str())) return bad("U", g.lights_out, atoi(c[25].c_str()));
        if ((long)g.lights_timer != atol(c[26].c_str())) return bad("v", g.lights_timer, atol(c[26].c_str()));
        if (g.per_Ma != atoi(c[27].c_str())) return bad("Ma", g.per_Ma, atoi(c[27].c_str()));
        if (g.per_La != atoi(c[28].c_str())) return bad("La", g.per_La, atoi(c[28].c_str()));
        if (g.per_Na != atoi(c[29].c_str())) return bad("Na", g.per_Na, atoi(c[29].c_str()));
        if (g.cnt_B != atoi(c[30].c_str())) return bad("B", g.cnt_B, atoi(c[30].c_str()));
        for (int i = 0; i < 4; i++) {
            int b = 31 + i * 4;
            if (g.pu[i].id   != atoi(c[b + 0].c_str())) return bad("slot id", g.pu[i].id, atoi(c[b + 0].c_str()));
            if (g.pu[i].life != atoi(c[b + 1].c_str())) return bad("slot life", g.pu[i].life, atoi(c[b + 1].c_str()));
            if (g.pu[i].x    != atoi(c[b + 2].c_str())) return bad("slot x", g.pu[i].x, atoi(c[b + 2].c_str()));
            if (g.pu[i].y    != atoi(c[b + 3].c_str())) return bad("slot y", g.pu[i].y, atoi(c[b + 3].c_str()));
        }
        int T = atoi(c[47].c_str()), u = atoi(c[48].c_str());
        if (g.stamp_col != T) return bad("T", g.stamp_col, T);
        if (g.stamp_row != u) return bad("u", g.stamp_row, u);
        if ((int)g.stamp_active != (atoi(c[17].c_str()) == 4))
            return bad("stamp_active", g.stamp_active, atoi(c[17].c_str()) == 4);

        /* coin bitmap: 95 hex nibbles, LSB first */
        const std::string &hex = c[49];
        if (hex.size() != (FIELD_CELLS + 3) / 4)
            return bad("coinhex len", hex.size(), (FIELD_CELLS + 3) / 4);
        for (int i = 0; i < FIELD_CELLS; i++) {
            int ch = hex[i >> 2];
            int nib = (ch >= 'A') ? ch - 'A' + 10 : ch - '0';
            int want = (nib >> (i & 3)) & 1;
            int got = (g.coins[i >> 4] >> (i & 15)) & 1;
            if (got != want) {
                printf("MISMATCH %s tick %d: coin[%d] got %d want %d (count %d)\n",
                       name.c_str(), tick, i, got, want, coin_count(&g));
                fclose(f); return false;
            }
        }

        /* wall-stamp cells (tile values at T..T+2, u..u+2) */
        for (int r = 0; r < 3; r++) {
            for (int col = 0; col < 3; col++) {
                int tcol = T + col, trow = u + r;
                int want = atoi(c[50 + r * 3 + col].c_str());
                int got = 0;
                if (tcol >= 0 && tcol < FIELD_COLS && trow >= 0 && trow < FIELD_ROWS)
                    got = game_tile_at(&g, (uint8_t)tcol, (uint8_t)trow);
                if (got != want) {
                    printf("MISMATCH %s tick %d: stamp[%d,%d] got %d want %d (T=%d u=%d)\n",
                           name.c_str(), tick, tcol, trow, got, want, T, u);
                    fclose(f); return false;
                }
            }
        }

        /* G8 campaign columns (only natural-campaign traces have them). */
        if (c.size() >= 65) {
            if (g.level != (uint16_t)atoi(c[59].c_str())) return bad("x", g.level, atoi(c[59].c_str()));
            if (g.level_start_score != (uint32_t)strtoul(c[60].c_str(), nullptr, 10))
                return bad("z", (long)g.level_start_score, strtoul(c[60].c_str(), nullptr, 10));
            if (g.portal_state != atoi(c[61].c_str())) return bad("Y", g.portal_state, atoi(c[61].c_str()));
            if (g.portal_x != atoi(c[62].c_str())) return bad("X", g.portal_x, atoi(c[62].c_str()));
            if (g.portal_y != atoi(c[63].c_str())) return bad("y", g.portal_y, atoi(c[63].c_str()));
            int da = atoi(c[64].c_str());
            if (g.maze != da) return bad("da", g.maze, da);
            if (da < 0) {
                if (g.map != nullptr) return bad("map-null", 1, 0);
            } else if (!g.map ||
                       memcmp(g.map, &d_blob[(size_t)da * FIELD_CELLS], FIELD_CELLS) != 0) {
                return bad("map", 0, 1);
            }
        }
        tick++;
    }
    fclose(f);
    files++;
    return true;
}

int main(int argc, char **argv) {
    const char *dir = argc > 1 ? argv[1] : "build/oracle/traces_pu";
    FILE *fd = fopen("reference/d", "rb");
    if (!fd) { printf("reference/d missing\n"); return 2; }
    int ch; while ((ch = fgetc(fd)) != EOF) d_blob.push_back((uint8_t)ch);
    fclose(fd);
    DIR *dp = opendir(dir);
    if (!dp) { printf("no power-up traces: run python3 tools/oracle/build_oracle.py --pu-traces\n"); return 2; }
    std::vector<std::string> names;
    while (dirent *e = readdir(dp)) { std::string n = e->d_name; if (n.size() > 4 && n.substr(n.size() - 4) == ".csv") names.push_back(n); }
    closedir(dp);
    for (auto &n : names) if (!run(std::string(dir) + "/" + n, n)) fails++;
    printf("%s: %d traces, %d ticks compared, %d failed\n",
           fails ? "powerup_oracle FAILED" : "powerup_oracle: all passed", files, total_ticks, fails);
    return fails != 0;
}

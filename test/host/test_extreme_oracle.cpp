/* Replays Extreme campaign traces (tools/oracle, build/oracle/traces_campaign) against the
 * C++ core. The Java driver (mode 3) forces a portal as soon as it is eligible and puts the
 * head on it, so every trace cycles through all six mazes and the shrink-to-2 transition.
 * Built with -DFIELD_ROWS=18 to compare with the original 21x18 engine. */
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
static uint8_t map_buf[FIELD_CELLS];
static int total_ticks, files, fails;

static std::vector<std::string> split(const std::string &s, char c) {
    std::vector<std::string> v; size_t i = 0;
    for (;;) { size_t j = s.find(c, i); if (j == std::string::npos) { v.push_back(s.substr(i)); break; } v.push_back(s.substr(i, j - i)); i = j + 1; }
    return v;
}

static int8_t maze_loader(uint8_t index, uint8_t *buf) {
    if (index >= 6) return -1;
    memcpy(buf, &d_blob[(size_t)index * FIELD_CELLS], FIELD_CELLS);
    return 0;
}

static bool run(const std::string &path, const std::string &name) {
    int speed; long seed;
    if (sscanf(name.c_str(), "campaign_s%d_r%ld", &speed, &seed) != 2) return true;
    FILE *f = fopen(path.c_str(), "r");
    if (!f) { printf("cannot open %s\n", path.c_str()); return false; }
    jr_seed(seed);
    static game_t g;
    game_init_mode(&g, (uint8_t)speed, -1, nullptr, true);
    game_set_maze_source(&g, map_buf, maze_loader);

    char line[4096];
    int tick = 0;
    while (fgets(line, sizeof line, f)) {
        auto c = split(line, ',');
        if (c.size() < 65) continue;

        /* Same harness injection as Oracle mode 3. */
        if (g.portal_state == 0) {
            g.score = g.level_start_score + (uint32_t)speed * 10u +
                      (uint32_t)g.level * 20u + 1u;
            g.cnt_c = (uint8_t)(g.per_La - 1);
            g.cnt_b = 0;
        } else if (g.portal_state == 1) {
            g.qx = g.portal_x;
            g.qy = g.portal_y;
            g.cnt_c = (uint8_t)(g.per_La - 1);
            g.cnt_b = 0;
        }

        int keys = atoi(c[1].c_str());
        game_tick(&g, (uint8_t)keys);
        total_ticks++;
        auto bad = [&](const char *what, long got, long want) {
            printf("MISMATCH %s tick %d: %s got %ld want %ld\n", name.c_str(), tick, what, got, want);
            fclose(f); return false; };

        /* G8 campaign state first, so a level/portal divergence is reported directly. */
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
        if (g.pu_active != atoi(c[17].c_str())) return bad("Aa", g.pu_active, atoi(c[17].c_str()));
        if (g.per_ma != atoi(c[18].c_str())) return bad("ma", g.per_ma, atoi(c[18].c_str()));
        if (g.tongue != atoi(c[19].c_str())) return bad("J", g.tongue, atoi(c[19].c_str()));

        const std::string &seg = c[20];
        if (seg.size() != g.len * 2) return bad("seglen", seg.size() / 2, g.len);
        for (unsigned i = 0; i < g.len; i++) {
            unsigned v = strtoul(seg.substr(i * 2, 2).c_str(), nullptr, 16);
            if (g.a[i] != v) {
                printf("MISMATCH %s tick %d: a[%u] got %02X want %02X\n", name.c_str(), tick, i, g.a[i], v);
                fclose(f); return false;
            }
        }

        tick++;
        if (g.game_over) break;
    }
    fclose(f);
    files++;
    return true;
}

int main(int argc, char **argv) {
    const char *dir = argc > 1 ? argv[1] : "build/oracle/traces_campaign";
    FILE *fd = fopen("reference/d", "rb");
    if (!fd) { printf("reference/d missing\n"); return 2; }
    int ch; while ((ch = fgetc(fd)) != EOF) d_blob.push_back((uint8_t)ch);
    fclose(fd);
    DIR *dp = opendir(dir);
    if (!dp) { printf("no traces: run python3 tools/oracle/build_oracle.py --campaign-traces\n"); return 2; }
    std::vector<std::string> names;
    while (dirent *e = readdir(dp)) {
        std::string n = e->d_name;
        if (n.size() > 4 && n.substr(n.size() - 4) == ".csv") names.push_back(n);
    }
    closedir(dp);
    for (auto &n : names) if (!run(std::string(dir) + "/" + n, n)) fails++;
    printf("%s: %d traces, %d ticks compared, %d failed\n",
           fails ? "extreme_oracle FAILED" : "extreme_oracle: all passed",
           files, total_ticks, fails);
    return fails != 0;
}

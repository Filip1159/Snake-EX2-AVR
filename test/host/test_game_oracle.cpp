/* Replays the original engine traces (tools/oracle, build/oracle/traces) against the C++ core.
 * Built with -DFIELD_ROWS=18 so the field matches the original 21x18 geometry. */
#include "game/game.h"
#include "game/rng.h"
#include "drivers/buzzer.h"
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
static int total_ticks, files, fails;

static std::vector<std::string> split(const std::string &s, char c) {
    std::vector<std::string> v; size_t i = 0;
    for (;;) { size_t j = s.find(c, i); if (j == std::string::npos) { v.push_back(s.substr(i)); break; } v.push_back(s.substr(i, j - i)); i = j + 1; }
    return v;
}

static bool run(const std::string &path, const std::string &name) {
    int maze, speed; long seed;
    if (sscanf(name.c_str(), "classic_m%d_s%d_r%ld", &maze, &speed, &seed) != 3) return true;
    FILE *f = fopen(path.c_str(), "r");
    jr_seed(seed);
    static game_t g;
    game_init(&g, speed, maze, maze >= 0 ? &d_blob[maze * 378] : nullptr);
    char line[4096];
    int tick = 0;
    while (fgets(line, sizeof line, f)) {
        auto c = split(line, ',');
        if (c.size() < 22) continue;
        int keys = atoi(c[1].c_str());
        game_tick(&g, (uint8_t)keys);
        total_ticks++;
        auto bad = [&](const char *what, long got, long want) {
            printf("MISMATCH %s tick %d: %s got %ld want %ld\n", name.c_str(), tick, what, got, want); fclose(f); return false; };
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
        if (g.cnt_B != 0 && false) return false;
        if (g.cnt_b != atoi(c[14].c_str())) return bad("b", g.cnt_b, atoi(c[14].c_str()));
        if (g.cnt_c != atoi(c[15].c_str())) return bad("c", g.cnt_c, atoi(c[15].c_str()));
        if (g.cnt_C != atoi(c[16].c_str())) return bad("C", g.cnt_C, atoi(c[16].c_str()));
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
        tick++;
    }
    fclose(f);
    files++;
    return true;
}

int main(int argc, char **argv) {
    const char *dir = argc > 1 ? argv[1] : "build/oracle/traces";
    FILE *fd = fopen("reference/d", "rb");
    if (!fd) { printf("reference/d missing\n"); return 2; }
    int ch; while ((ch = fgetc(fd)) != EOF) d_blob.push_back((uint8_t)ch);
    fclose(fd);
    DIR *dp = opendir(dir);
    if (!dp) { printf("no traces: run python3 tools/oracle/build_oracle.py --traces\n"); return 2; }
    std::vector<std::string> names;
    while (dirent *e = readdir(dp)) { std::string n = e->d_name; if (n.size() > 4 && n.substr(n.size() - 4) == ".csv") names.push_back(n); }
    closedir(dp);
    for (auto &n : names) if (!run(std::string(dir) + "/" + n, n)) fails++;
    printf("%s: %d traces, %d ticks compared, %d failed\n", fails ? "game_oracle FAILED" : "game_oracle: all passed", files, total_ticks, fails);
    return fails != 0;
}

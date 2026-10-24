#include "render/playfield.h"
#include "render/field.h"
#include "render/sprite.h"
#include "render/snake_sprites.h"
#include "drivers/st7735.h"
#include "config.h"

#define PF_MAX_CENTERS 48

typedef struct { uint8_t col, row; } pf_cell_t;

static void cell_of(int16_t px, int16_t py, uint8_t *col, uint8_t *row) {
    *col = game_col(px);
    *row = game_row(py);
}

static uint8_t add_cell(pf_cell_t *list, uint8_t n, uint8_t col, uint8_t row) {
    for (uint8_t i = 0; i < n; i++)
        if (list[i].col == col && list[i].row == row) return n;
    if (n < PF_MAX_CENTERS) { list[n].col = col; list[n].row = row; return (uint8_t)(n + 1); }
    return n;
}

/** Add the nine cells of a 3x3 wall-stamp block. */
static uint8_t add_stamp_cells(pf_cell_t *list, uint8_t n, uint8_t col, uint8_t row) {
    for (uint8_t r = 0; r < 3; r++)
        for (uint8_t c = 0; c < 3; c++) {
            int cc = (int)col + c, rr = (int)row + r;
            if (cc < FIELD_COLS && rr < FIELD_ROWS)
                n = add_cell(list, n, (uint8_t)cc, (uint8_t)rr);
        }
    return n;
}

static void cell_ahead(const game_t *g, int16_t *px, int16_t *py);

static void draw_base(const game_t *g, uint8_t col, uint8_t row) {
    uint8_t x = (uint8_t)(FIELD_ORIGIN_X + col * CELL_SIZE);
    uint8_t y = (uint8_t)(FIELD_ORIGIN_Y + row * CELL_SIZE);
    if (g->lights_out) {
        st7735_fill_rect(x, y, CELL_SIZE, CELL_SIZE, 0x0000);
        return;
    }
    field_draw_cell(col, row, game_tile_at(g, col, row));
}

/** Overlay the active 3x3 wall stamp (game_tile_at already includes zero cells, which
 *  erase any original wall under the pattern). */
static void draw_stamp(const game_t *g) {
    if (!g->stamp_active) return;
    for (uint8_t r = 0; r < 3; r++)
        for (uint8_t c = 0; c < 3; c++) {
            uint8_t col = (uint8_t)(g->stamp_col + c);
            uint8_t row = (uint8_t)(g->stamp_row + r);
            if (col < FIELD_COLS && row < FIELD_ROWS)
                field_draw_cell(col, row, game_tile_at(g, col, row));
        }
}

/** Draw every item layer at one cell: coin, food, bonus, power-up slot (original order). */
static void draw_overlay(const game_t *g, uint8_t col, uint8_t row) {
    int16_t x = FIELD_ORIGIN_X + col * CELL_SIZE;
    int16_t y = FIELD_ORIGIN_Y + row * CELL_SIZE;
    if (g->pu_active == 8 && game_has_coin(g, col, row)) {
        sprite_draw(x - 1, y - 1, spr_food_frame(0, 0));
    }
    if (g->food_x >= 0 && g->food_x / CELL_SIZE == col && g->food_y / CELL_SIZE == row) {
        sprite_draw(x - 1, y - 1, spr_food_frame(0, 0));
    }
    if (g->bonus > 0 && g->bonus_x / CELL_SIZE == col && g->bonus_y / CELL_SIZE == row) {
        sprite_draw(x - 1, y - 1, spr_food_frame(g->bonus_type, g->bonus_frame));
    }
    for (uint8_t i = 0; i < POWERUP_SLOTS; i++) {
        if (g->pu[i].id >= 1 && g->pu[i].id <= POWERUP_TYPE_COUNT &&
            g->pu[i].x / CELL_SIZE == col && g->pu[i].y / CELL_SIZE == row) {
            sprite_draw(x - 1, y - 1, SPR_POWERUP_ITEM);
        }
    }
    /* Portal: 10x10 `c` sprite at cell top-left - (2,2) (b.java: X+1-2, y-2). */
    if (game_portal_visible(g) &&
        g->portal_x / CELL_SIZE == col && g->portal_y / CELL_SIZE == row) {
        sprite_draw(x - 2, y - 2, SPR_MARKER);
    }
}

/** Tongue: drawn over the cells ahead (before the snake, like b.java). */
static void draw_tongue(const game_t *g) {
    int16_t ax, ay;
    cell_ahead(g, &ax, &ay);
    spr_pick_t t = snake_tongue_sprite(g->a[g->len - 1]);
    sprite_draw((int16_t)(ax + t.dx), (int16_t)(ay + t.dy), t.id);
}

/** Add the three cells a tongue can cover (it reaches 1..3 cells ahead incl. the tip). */
static uint8_t add_tongue_cells(pf_cell_t *list, uint8_t n,
                                int16_t hx, int16_t hy, uint8_t dir) {
    int8_t dx = 0, dy = 0;
    switch (dir & 0x0F) {
        case DIR_UP:    dy = -CELL_SIZE; break;
        case DIR_DOWN:  dy =  CELL_SIZE; break;
        case DIR_LEFT:  dx = -CELL_SIZE; break;
        default:        dx =  CELL_SIZE; break;
    }
    int16_t x = hx, y = hy;
    for (uint8_t k = 0; k < 3; k++) {
        x += dx; y += dy;
        if (x <= 0) x += FIELD_WIDTH_PX;
        if (x >= FIELD_WIDTH_PX) x -= FIELD_WIDTH_PX;
        if (y <= 0) y += FIELD_HEIGHT_PX;
        if (y >= FIELD_HEIGHT_PX) y -= FIELD_HEIGHT_PX;
        n = add_cell(list, n, game_col(x), game_row(y));
    }
    return n;
}

static void draw_segment(const game_t *g, uint16_t i, uint8_t col, uint8_t row, bool open) {
    int16_t x = FIELD_ORIGIN_X + col * CELL_SIZE;
    int16_t y = FIELD_ORIGIN_Y + row * CELL_SIZE;
    if (i == g->len - 1) {
        spr_pick_t p = snake_head_sprite(g->a[i], open);
        sprite_draw(x + p.dx, y + p.dy, p.id);
    } else if (i == 0) {
        uint8_t id = snake_tail_sprite(g->a[0], g->a[1]);
        sprite_draw(x, y, id);
    } else {
        uint8_t id = snake_body_sprite(g->a[i], g->a[i + 1]);
        sprite_draw(x, y, id);
    }
}

/** Cell in front of the head (wrapped), i.e. original (k,l). */
static void cell_ahead(const game_t *g, int16_t *px, int16_t *py) {
    int16_t x = g->qx, y = g->qy;
    switch (g->dir & 0x0F) {
        case DIR_UP:    y -= CELL_SIZE; break;
        case DIR_DOWN:  y += CELL_SIZE; break;
        case DIR_LEFT:  x -= CELL_SIZE; break;
        case DIR_RIGHT: x += CELL_SIZE; break;
    }
    if (x <= 0) x += FIELD_WIDTH_PX;
    if (x >= FIELD_WIDTH_PX) x -= FIELD_WIDTH_PX;
    if (y <= 0) y += FIELD_HEIGHT_PX;
    if (y >= FIELD_HEIGHT_PX) y -= FIELD_HEIGHT_PX;
    *px = x; *py = y;
}

static void step_back(int16_t *x, int16_t *y, uint8_t d) {
    switch (d & 0x0F) {
        case DIR_UP:    *y += CELL_SIZE; break;
        case DIR_DOWN:  *y -= CELL_SIZE; break;
        case DIR_LEFT:  *x += CELL_SIZE; break;
        case DIR_RIGHT: *x -= CELL_SIZE; break;
    }
    if (*x <= 0) *x += FIELD_WIDTH_PX;
    if (*x >= FIELD_WIDTH_PX) *x -= FIELD_WIDTH_PX;
    if (*y <= 0) *y += FIELD_HEIGHT_PX;
    if (*y >= FIELD_HEIGHT_PX) *y -= FIELD_HEIGHT_PX;
}

/** Is the cell within two cells of a changed centre? (Item sprites overhang 1 px, and
 *  the centre's 3x3 repaint reaches one cell out, so ±2 must trigger an item redraw.) */
static bool near(const pf_cell_t *list, uint8_t n, uint8_t col, uint8_t row) {
    for (uint8_t i = 0; i < n; i++) {
        int8_t dc = (int8_t)(list[i].col - col);
        int8_t dr = (int8_t)(list[i].row - row);
        if (dc >= -2 && dc <= 2 && dr >= -2 && dr <= 2) return true;
    }
    return false;
}

static void cache_sync(const game_t *g, playfield_cache_t *c) {
    c->food_x = g->food_x; c->food_y = g->food_y; c->food_valid = (g->food_x >= 0);
    c->bonus_x = g->bonus_x; c->bonus_y = g->bonus_y; c->bonus_type = g->bonus_type;
    c->bonus_frame = g->bonus_frame;
    c->bonus_valid = g->bonus > 0;
    c->tongue = g->tongue > 0;
    c->tongue_x = g->qx; c->tongue_y = g->qy; c->tongue_dir = g->dir;
    for (uint8_t i = 0; i < POWERUP_SLOTS; i++) {
        bool on = g->pu[i].id >= 1 && g->pu[i].id <= POWERUP_TYPE_COUNT;
        c->pu_id[i] = on ? g->pu[i].id : 0;
        c->pu_x[i] = g->pu[i].x;
        c->pu_y[i] = g->pu[i].y;
    }
    c->coin_rush = g->pu_active == 8;
    c->lights_out = g->lights_out;
    c->stamp_active = g->stamp_active;
    c->stamp_col = g->stamp_col; c->stamp_row = g->stamp_row; c->stamp_pat = g->stamp_pat;
    c->portal_valid = game_portal_visible(g);
    c->portal_x = g->portal_x; c->portal_y = g->portal_y;
    c->portal_state = g->portal_state;
}

void playfield_draw_all(const game_t *g, playfield_cache_t *c) {
    sprite_set_clip(FIELD_ORIGIN_X, FIELD_ORIGIN_Y,
                    FIELD_ORIGIN_X + FIELD_WIDTH_PX, FIELD_ORIGIN_Y + FIELD_HEIGHT_PX);
    if (g->lights_out) {
        st7735_fill_screen(0x0000);
    } else if (g->map) {
        field_draw_map(g->map);
        draw_stamp(g);
    } else {
        st7735_fill_screen(0x0000);
        st7735_fill_rect(FIELD_ORIGIN_X, FIELD_ORIGIN_Y,
                         FIELD_WIDTH_PX, FIELD_HEIGHT_PX, COLOR_FIELD_BG);
        draw_stamp(g);
    }

    /* coin-rush overlay: the original scans every cell for Ca & 0x20 */
    if (g->pu_active == 8) {
        for (uint8_t row = 0; row < FIELD_ROWS; row++)
            for (uint8_t col = 0; col < FIELD_COLS; col++)
                if (game_has_coin(g, col, row))
                    sprite_draw((int16_t)(FIELD_ORIGIN_X + col * CELL_SIZE - 1),
                                (int16_t)(FIELD_ORIGIN_Y + row * CELL_SIZE - 1),
                                spr_food_frame(0, 0));
    }
    if (g->food_x >= 0) draw_overlay(g, game_col(g->food_x), game_row(g->food_y));
    if (g->bonus > 0) draw_overlay(g, game_col(g->bonus_x), game_row(g->bonus_y));
    for (uint8_t i = 0; i < POWERUP_SLOTS; i++)
        if (g->pu[i].id >= 1 && g->pu[i].id <= POWERUP_TYPE_COUNT)
            draw_overlay(g, game_col(g->pu[i].x), game_row(g->pu[i].y));
    if (game_portal_visible(g))
        draw_overlay(g, game_col(g->portal_x), game_row(g->portal_y));

    if (g->tongue > 0) draw_tongue(g);

    int16_t ax, ay;
    cell_ahead(g, &ax, &ay);
    bool coin_ahead = (g->pu_active == 8 && game_has_coin(g, game_col(ax), game_row(ay)));
    bool open = coin_ahead ||
                (ax == g->food_x && ay == g->food_y) ||
                (g->bonus > 0 && ax == g->bonus_x && ay == g->bonus_y);
    int16_t x = g->qx, y = g->qy;
    for (int16_t i = (int16_t)g->len - 1; i >= 0; i--) {
        draw_segment(g, (uint16_t)i, game_col(x), game_row(y), open);
        step_back(&x, &y, g->a[i]);
    }

    c->head_open = open;
    cache_sync(g, c);
    sprite_reset_clip();
}

bool playfield_update(const game_t *g, playfield_cache_t *c) {
    bool coin_now = (g->pu_active == 8);

    /* Only a real background change (lights-out, explicit restart/death redraw) needs
     * the whole screen. Wall stamps and coin-rush start/end are local cell changes. */
    if (g->full_redraw || g->lights_out != c->lights_out) {
        playfield_draw_all(g, c);
        return true;
    }

    sprite_set_clip(FIELD_ORIGIN_X, FIELD_ORIGIN_Y,
                    FIELD_ORIGIN_X + FIELD_WIDTH_PX, FIELD_ORIGIN_Y + FIELD_HEIGHT_PX);
    pf_cell_t centers[PF_MAX_CENTERS];
    uint8_t cn = 0;

    /* Wall stamp appeared, moved or expired: repaint the old and new 3x3 blocks.
     * The snake is redrawn head-to-tail below, so cells it overlaps stay correct. */
    bool stamp_changed = (g->stamp_active != c->stamp_active) ||
        (g->stamp_active && (g->stamp_col != c->stamp_col ||
                             g->stamp_row != c->stamp_row ||
                             g->stamp_pat != c->stamp_pat));
    if (stamp_changed) {
        if (c->stamp_active)
            cn = add_stamp_cells(centers, cn, c->stamp_col, c->stamp_row);
        if (g->stamp_active)
            cn = add_stamp_cells(centers, cn, g->stamp_col, g->stamp_row);
    }

    /* Extreme portal appeared or disappeared (state 1 -> 2 keeps the same sprite). */
    bool portal_now = game_portal_visible(g);
    bool portal_changed = (portal_now != c->portal_valid) ||
        (portal_now && (g->portal_x != c->portal_x || g->portal_y != c->portal_y));
    if (portal_changed && c->portal_valid) {
        uint8_t col, row; cell_of(c->portal_x, c->portal_y, &col, &row);
        cn = add_cell(centers, cn, col, row);
    }
    if (portal_now && portal_changed)
        cn = add_cell(centers, cn, game_col(g->portal_x), game_row(g->portal_y));

    /* items that appeared, moved or expired */
    if (c->food_valid && (c->food_x != g->food_x || c->food_y != g->food_y)) {
        uint8_t col, row; cell_of(c->food_x, c->food_y, &col, &row);
        cn = add_cell(centers, cn, col, row);
    }
    if (g->food_x >= 0 && (g->food_x != c->food_x || g->food_y != c->food_y))
        cn = add_cell(centers, cn, game_col(g->food_x), game_row(g->food_y));

    bool bvalid = g->bonus > 0;
    bool bonus_changed = (bvalid != c->bonus_valid) ||
                         (bvalid && (c->bonus_x != g->bonus_x || c->bonus_y != g->bonus_y ||
                                     c->bonus_type != g->bonus_type ||
                                     c->bonus_frame != g->bonus_frame));
    if (bonus_changed && c->bonus_valid) {
        uint8_t col, row; cell_of(c->bonus_x, c->bonus_y, &col, &row);
        cn = add_cell(centers, cn, col, row);
    }
    if (bvalid && bonus_changed)
        cn = add_cell(centers, cn, game_col(g->bonus_x), game_row(g->bonus_y));

    /* power-up slots: picked up, expired, or (never in the original) moved */
    for (uint8_t i = 0; i < POWERUP_SLOTS; i++) {
        bool on = g->pu[i].id >= 1 && g->pu[i].id <= POWERUP_TYPE_COUNT;
        if (c->pu_id[i] &&
            (!on || g->pu[i].x != c->pu_x[i] || g->pu[i].y != c->pu_y[i])) {
            cn = add_cell(centers, cn, game_col(c->pu_x[i]), game_row(c->pu_y[i]));
        }
        if (on && (!c->pu_id[i] || g->pu[i].x != c->pu_x[i] || g->pu[i].y != c->pu_y[i]))
            cn = add_cell(centers, cn, game_col(g->pu[i].x), game_row(g->pu[i].y));
    }

    /* coin scan placed or erased one coin this frame */
    if (g->coin_added >= 0) {
        uint16_t idx = (uint16_t)g->coin_added;
        cn = add_cell(centers, cn, (uint8_t)(idx % FIELD_COLS), (uint8_t)(idx / FIELD_COLS));
    }
    if (g->coin_removed >= 0) {
        uint16_t idx = (uint16_t)g->coin_removed;
        cn = add_cell(centers, cn, (uint8_t)(idx % FIELD_COLS), (uint8_t)(idx / FIELD_COLS));
    }

    int16_t hx_ahead, hy_ahead;
    cell_ahead(g, &hx_ahead, &hy_ahead);
    bool coin_ahead = coin_now && game_has_coin(g, game_col(hx_ahead), game_row(hy_ahead));
    bool head_open = coin_ahead ||
                     (hx_ahead == g->food_x && hy_ahead == g->food_y) ||
                     (g->bonus > 0 && hx_ahead == g->bonus_x && hy_ahead == g->bonus_y);
    if (head_open != c->head_open)
        cn = add_cell(centers, cn, game_col(g->qx), game_row(g->qy));

    bool tongue_now = (g->tongue > 0);

    /* The tongue can change without a successful move (it retracts on the tick the snake
     * dies), so erase/redraw it on state changes as well as on movement. It only changes
     * position on movement ticks; between them it stays on screen untouched. */
    if (c->tongue && (!tongue_now || g->moved))
        cn = add_tongue_cells(centers, cn, c->tongue_x, c->tongue_y, c->tongue_dir);
    if (tongue_now && (!c->tongue || g->moved))
        cn = add_tongue_cells(centers, cn, g->qx, g->qy, g->dir);

    /* cells vacated by the snake / newly covered ground */
    if (g->moved) {
        /* every cell whose segment sprite changes needs a fresh underlay: head, old head
         * (the 11x11 head sprite overhangs its cell), the new tail cell and the freed one */
        int16_t hx = g->qx, hy = g->qy, ox = hx, oy = hy;
        if (g->len >= 2) step_back(&ox, &oy, g->a[g->len - 1]);
        cn = add_cell(centers, cn, game_col(hx), game_row(hy));
        if (g->len >= 2) cn = add_cell(centers, cn, game_col(ox), game_row(oy));
        if (g->tail_removed) {
            int16_t tx = hx, ty = hy;
            for (int16_t i = (int16_t)g->len - 1; i > 0; i--) step_back(&tx, &ty, g->a[i]);
            cn = add_cell(centers, cn, game_col(tx), game_row(ty));
            uint8_t col, row; cell_of(g->old_tail_x, g->old_tail_y, &col, &row);
            cn = add_cell(centers, cn, col, row);
        }
        if (g->food_eaten) {
            uint8_t col, row; cell_of(g->eaten_x, g->eaten_y, &col, &row);
            cn = add_cell(centers, cn, col, row);
        }
        if (g->bonus_eaten && c->bonus_valid) {
            uint8_t col, row; cell_of(c->bonus_x, c->bonus_y, &col, &row);
            cn = add_cell(centers, cn, col, row);
        }
    }
    /* type 6 removed a second tail cell before the normal move */
    if (g->shrink_removed)
        cn = add_cell(centers, cn, game_col(g->shrink_x), game_row(g->shrink_y));

    /* Repaint a 3x3 block around every changed cell: sprites overhang their cell by up to
     * 2 px. Drawing a cell twice is harmless, so expand directly instead of keeping a
     * capped list (the tongue can add several adjacent centres). */
    for (uint8_t i = 0; i < cn; i++) {
        for (int8_t dr = -1; dr <= 1; dr++) {
            for (int8_t dc = -1; dc <= 1; dc++) {
                int col = centers[i].col + dc, row = centers[i].row + dr;
                if (col >= 0 && col < FIELD_COLS && row >= 0 && row < FIELD_ROWS) {
                    draw_base(g, (uint8_t)col, (uint8_t)row);
                    draw_overlay(g, (uint8_t)col, (uint8_t)row);
                }
            }
        }
    }

    /* An item sprite overhangs its cell by 1 px; if the repaint touched or came within
     * one cell of it, redraw the item. The snake is redrawn as a whole below. */
    if (g->food_x >= 0) {
        uint8_t col = game_col(g->food_x), row = game_row(g->food_y);
        if (near(centers, cn, col, row)) draw_overlay(g, col, row);
    }
    if (g->bonus > 0) {
        uint8_t col = game_col(g->bonus_x), row = game_row(g->bonus_y);
        if (near(centers, cn, col, row)) draw_overlay(g, col, row);
    }
    for (uint8_t i = 0; i < POWERUP_SLOTS; i++) {
        if (g->pu[i].id >= 1 && g->pu[i].id <= POWERUP_TYPE_COUNT) {
            uint8_t col = game_col(g->pu[i].x), row = game_row(g->pu[i].y);
            if (near(centers, cn, col, row)) draw_overlay(g, col, row);
        }
    }
    /* The 10x10 portal overhangs its cell by 2 px in every direction. */
    if (portal_now) {
        uint8_t col = game_col(g->portal_x), row = game_row(g->portal_y);
        if (near(centers, cn, col, row)) draw_overlay(g, col, row);
    }
    /* A coin two cells away can still overhang 1 px into a repainted block. */
    if (coin_now) {
        for (uint8_t i = 0; i < cn; i++) {
            for (int8_t dr = -2; dr <= 2; dr++) {
                for (int8_t dc = -2; dc <= 2; dc++) {
                    if (dc > -2 && dc < 2 && dr > -2 && dr < 2) continue;
                    int col = centers[i].col + dc, row = centers[i].row + dr;
                    if (col >= 0 && col < FIELD_COLS && row >= 0 && row < FIELD_ROWS &&
                        game_has_coin(g, (uint8_t)col, (uint8_t)row))
                        draw_overlay(g, (uint8_t)col, (uint8_t)row);
                }
            }
        }
    }

    /* Redraw the whole snake head-to-tail (same order as playfield_draw_all()).  Segment
     * sprites are drawn in index order, so a redrawn head can be overwritten by a tail
     * sprite further down the list; redrawing every segment guarantees the exact same
     * stack of sprites as a full redraw while the field itself stays dirty-cell based. */
    if (cn) {
        if (tongue_now) draw_tongue(g);
        int16_t x = g->qx, y = g->qy;
        for (int16_t i = (int16_t)g->len - 1; i >= 0; i--) {
            draw_segment(g, (uint16_t)i, game_col(x), game_row(y), head_open);
            step_back(&x, &y, g->a[i]);
        }
    }

    c->head_open = head_open;
    cache_sync(g, c);
    sprite_reset_clip();
    return false;
}

# Snake EX2 sprite mapping (verified against decompiled b.java, task A5)

Source: `reference/decompiled/.../b.java` draw routine (lines ~470-760), turn flags (~1326-1392),
sprite loading `SnakeCanvas.a()`. Implemented in `src/render/snake_sprites.cpp`; independently
re-transcribed in `tools/gen_sprite_vectors.py` and compared exhaustively by `test/host/test_snake_sprites.cpp`.

## Record order in `d` (offset 11620)
`B0..B41` (6x6 x30, 9x9 x6, 11x11 x6), then `b[0..7]` animation groups of 1,1,1,2,2,2,2,4 frames (8x8),
then `c` 10x10, `C` 8x8, `d` 19x21, `D` 32x18.

## 4 bpp pixel order
`SnakeCanvas.a(DataInputStream, ...)` decodes one byte per two pixels: the **low nibble is the
first** (left) pixel, then the byte is shifted right by 4 for the next pixel. The nibble stream
continues across row boundaries (rows are not byte-padded). `tools/extract_snake_ex2.py` follows
this order; `test_snake_sprites` pins a few decoded pixels (B0 body, B30/B36 eye) so a high-first
decode cannot slip back in.

## Roles
| Sprite | Role |
|---|---|
| `b[0][0]` (P0_0) | normal food and coin-rush overlay, 8x8, drawn at cell (-1,-1) |
| `b[Fa][frame]`, Fa=1..7 | bonus food types; Fa>2 in Extreme is rotated by its move direction |
| `C` | power-up item on the field, 8x8 at (-1,-1) |
| `c` | stage-marker object (Y==1/2 stages), 10x10 at (-2,-2) |
| `f.png` | tongue 11x11 (rotated 0/90/180/270 by head direction) |
| `d`, `D` | high-score decoration (unused; trophy `d` is in `src/assets/trophy.cpp`) |
| B0 right, B1 left, B2 up, B5 down | straight body (B3 = up / B4 = down, counter-clockwise stripe phase) |
| B6..B9 | corners; B10, B11 = counter-clockwise variants of B6, B7 |
| B12..B17 | fat (food inside) straight: 0->12 1->13 2->14 5->17; ccw: 14->15, 17->16 |
| B18..B21 | fat corners: 6->18 7->19 8->20 9->21; ccw: 18->22, 19->23 |
| B24..B29 | tail: right 24, left 25, up 26, down 29; ccw up->27, down->28 |
| B30..B35 | head, mouth closed, 9x9 |
| B36..B41 | head, mouth open (food/coin in next cell), 11x11 |

## Segment byte `a[i]` (a[0] = tail, a[len-1] = head)
- low nibble: direction UP=1 LEFT=2 DOWN=4 RIGHT=8 (movement that entered the cell)
- 0x10: fat segment (swallowed food)
- 0x20 / 0x40: last turn was clockwise / counter-clockwise; inherited along straight runs
  (stripe phase). Start byte is 0x28.

## Selection
- Body (0<i<len-1): base by `a[i]` (R0 L1 U2 D5); m=(a[i]|a[i+1])&15, n=a[i+1]&15:
  m=3: n==L?7:8; m=9: n==R?6:9; m=6: n==L?9:6; m=12: n==R?8:7. Then fat remap, then ccw remap.
- Tail: by `a[1]` (U26 D29 L25 R24), then ccw remap if `a[0]&0x60==0x40`.
- Head, closed (cw = flag 0x20): U cw?32@(-3,-3):33@(0,-3); D cw?35@(0,0):34@(-3,0); L 31@(-3,-1); R 30@(0,-1).
- Head, open: U cw?38@(-4,-5):39@(-1,-5); D cw?41@(-1,0):40@(-4,0); L 37@(-5,-2); R 36@(0,-2).
- Draw order: head first, then toward the tail (later segments overdraw earlier ones).
- Original redraws only head (3 segs) and tail (2 segs) per tick unless a full redraw is flagged.
- Snake is hidden on alternate 4-tick windows while dying (`Ja/4%2`).

## Tongue (`f.png`, G6)
- `f.png` is 11×11 with a transparent background, red body and black outline, pointing **right**.
- `tools/convert_assets.py` emits `build/assets/tongue.json` with four 4 bpp rotations:
  `TONGUE_R` (0°), `TONGUE_U` (90° CCW), `TONGUE_L` (180°), `TONGUE_D` (270° CCW);
  transparent palette index 0. `gen_assets.py` appends them at indices 59–62 (`SPRITE_COUNT` 63).
- Drawn **before** the head sprite, with its top-left at the cell one ahead of the head (`k,l`) plus
  the original `o/O` offsets:
  | Direction | Offset `(dx,dy)` |
  |---|---|
  | right | (3, −2) |
  | left | (−6, −2) |
  | up | (−1, −7) if the head byte has `0x20`, else (−2, −7) |
  | down | (−2, 1) if the head byte has `0x20`, else (−1, 1) |
- The bitmap extends slightly past the two-cell reach (right and up spill 1–2 px into the third
  cell), so the renderer repaints up to three cells ahead when the tongue appears/disappears.
- Lifetime: one movement cadence. `J` is set to 1 on the movement tick and decremented on the next;
  the original has no lash sound. A successful grab uses the normal eat beep (SFX 12).

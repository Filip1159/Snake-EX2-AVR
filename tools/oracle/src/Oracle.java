package com.nokia.mid.appl.snake;

import java.lang.reflect.Field;
import java.nio.file.*;
import java.util.*;
import javax.microedition.lcdui.Image;

/** Headless driver around the decompiled engine class b. Emits one CSV line per tick. */
public class Oracle {
    static Object get(Object o, String n) throws Exception { Field f = b.class.getDeclaredField(n); f.setAccessible(true); return f.get(o); }
    static void set(Object o, String n, Object v) throws Exception { Field f = b.class.getDeclaredField(n); f.setAccessible(true); f.set(o, v); }
    static void setb(Object o, String n, int v) throws Exception { Field f = b.class.getDeclaredField(n); f.setAccessible(true); f.setByte(o, (byte) v); }
    static int gi(Object o, String n) throws Exception { Object v = get(o, n); return v instanceof Number ? ((Number) v).intValue() : (((Boolean) v) ? 1 : 0); }

    public static void main(String[] a) throws Exception {
        // args: dfile mode(0 classic,1 extreme) maze(-1..5) speed(0..5) seed inputSeed ticks pressProb
        byte[] d = Files.readAllBytes(Paths.get(a[0]));
        int mode = Integer.parseInt(a[1]), maze = Integer.parseInt(a[2]), speed = Integer.parseInt(a[3]);
        long seed = Long.parseLong(a[4]), iseed = Long.parseLong(a[5]);
        int ticks = Integer.parseInt(a[6]); double prob = Double.parseDouble(a[7]);

        b.D = new byte[6][378];
        for (int m = 0; m < 6; m++) System.arraycopy(d, m * 378, b.D[m], 0, 378);
        SnakeCanvas.H = Image.createImage(128, 128);
        SnakeCanvas.F = Image.createImage(128, 20);
        SnakeCanvas.e = new Image[15]; for (int i = 0; i < 15; i++) SnakeCanvas.e[i] = Image.createImage(6, 6);
        Snake sn = new Snake();
        sn.I = (byte) speed;
        b.w = (byte) (mode >= 1 ? 1 : 0);
        b.V = (byte) (mode == 2 ? -1 : maze);
        b g = new b(sn, true, 1);
        if (mode == 2) set(g, "z", 1000000000);   /* sandbox: never open a portal / advance a stage */
        set(g, "d", new Random(seed));
        Random in = new Random(iseed);

        StringBuilder sb = new StringBuilder();
        for (int t = 0; t < ticks; t++) {
            /* Mode 3: deterministic campaign harness. Each level gets a portal as soon
             * as it is eligible; when the portal is waiting, put the head on it and
             * force the next c==0 tick so entry/shrink/level reset are exercised. */
            if (mode == 3) {
                int Y = gi(g, "Y");
                if (Y == 0) {
                    int z = gi(g, "z"), x = gi(g, "x");
                    set(g, "H", z + speed * 10 + x * 20 + 1);
                    setb(g, "c", gi(g, "La") - 1);
                    setb(g, "b", 0);
                } else if (Y == 1) {
                    setb(g, "Q", gi(g, "X"));
                    setb(g, "r", gi(g, "y"));
                    setb(g, "c", gi(g, "La") - 1);
                    setb(g, "b", 0);
                }
            }
            /* The real game runs the draw pass between engine ticks; it clears consumed slot
             * ids (15) and the "erase HUD icon" marker Aa=15. The headless driver must mimic
             * that so slot reuse / repeated pickups match the handheld. */
            if (gi(g, "Aa") == 15) setb(g, "Aa", 0);
            int[][] bs = (int[][]) get(g, "Ba");
            for (int i = 0; i < 4; i++) if (bs[i][0] == 15) { bs[i][0] = 0; bs[i][1] = 0; }

            Arrays.fill(sn.K, false);
            if (prob <= -2) {
                // survival bot: head for food/bonus, avoid walls and body (incl. the engine's 2-cell look-ahead)
                int hx = gi(g, "Q") / 6, hy = gi(g, "r") / 6, A = gi(g, "A");
                byte[] seg = (byte[]) get(g, "a");
                byte[] ca = (byte[]) get(g, "Ca");
                boolean[] occ = new boolean[21 * 18];
                int px = gi(g, "Q"), py = gi(g, "r");
                for (int i = A - 1; i >= 1; i--) {   // exclude the tail cell (index 0)
                    occ[(px / 6) + (py / 6) * 21] = true;
                    switch (seg[i] & 15) { case 1: py += 6; break; case 4: py -= 6; break; case 2: px += 6; break; case 8: px -= 6; break; }
                    px = ((px % 126) + 126) % 126; py = ((py % 108) + 108) % 108;
                }
                int tx = gi(g, "Da") / 6, ty = gi(g, "ea") / 6;
                if (gi(g, "fa") > 0 && in.nextInt(2) == 0) { tx = gi(g, "ga") / 6; ty = gi(g, "Ga") / 6; }
                if (prob == -3) {
                    int[][] slots = (int[][]) get(g, "Ba");
                    int bd = Integer.MAX_VALUE, bx = -1, by = -1;
                    for (int i = 0; i < 4; i++) {
                        if (slots[i][0] == 0 || slots[i][0] == 15) continue;
                        int sx = slots[i][2] / 6, sy = slots[i][3] / 6;
                        int dist = Math.abs(sx - hx) + Math.abs(sy - hy);
                        if (dist < bd) { bd = dist; bx = sx; by = sy; }
                    }
                    if (bx >= 0) { tx = bx; ty = by; }
                }
                int cur = gi(g, "j") & 15;
                int[] dx = {-1, 1, 0, 0}, dy = {0, 0, -1, 1};
                int[] dirbit = {2, 8, 1, 4};
                int best = -1, bestScore = Integer.MAX_VALUE;
                for (int k = 0; k < 4; k++) {
                    if ((dirbit[k] | cur) == 5 + 0 && false) continue;
                    if (cur == 1 && dirbit[k] == 4 || cur == 4 && dirbit[k] == 1 || cur == 2 && dirbit[k] == 8 || cur == 8 && dirbit[k] == 2) continue;
                    int x1 = ((hx + dx[k]) % 21 + 21) % 21, y1 = ((hy + dy[k]) % 18 + 18) % 18;
                    int x2 = ((x1 + dx[k]) % 21 + 21) % 21, y2 = ((y1 + dy[k]) % 18 + 18) % 18;
                    boolean bad = occ[x1 + y1 * 21] || occ[x2 + y2 * 21] || (ca[x1 + y1 * 21] & 15) > 0;
                    int score = Math.abs(tx - x1) + Math.abs(ty - y1) + (bad ? 1000 : 0) + in.nextInt(3);
                    if (score < bestScore) { bestScore = score; best = k; }
                }
                if (best >= 0 && in.nextInt(100) >= 3) sn.K[best] = true;
            } else if (prob < 0) {
                // greedy bot toward food (or bonus food while it exists), with 8% random jitter
                int hx = gi(g, "Q"), hy = gi(g, "r");
                int tx = gi(g, "Da"), ty = gi(g, "ea");
                if (gi(g, "fa") > 0 && in.nextInt(2) == 0) { tx = gi(g, "ga"); ty = gi(g, "Ga"); }
                int dx = tx - hx, dy = ty - hy, k = -1;
                if (in.nextInt(100) < 8) k = in.nextInt(4);
                else if (dx != 0 && (dy == 0 || in.nextBoolean())) k = dx < 0 ? 0 : 1;
                else if (dy != 0) k = dy < 0 ? 2 : 3;
                if (k >= 0) sn.K[k] = true;
            } else if (in.nextDouble() < prob) {
                int k = in.nextInt(4);            // 0 left 1 right 2 up 3 down
                if (mode >= 1 && in.nextInt(8) == 0) k = 4;   // fire/tongue only in extreme
                sn.K[k] = true;
            } else if (in.nextDouble() < prob / 4) {
                sn.K[in.nextBoolean() ? 5 : 6] = true;   // relative turn
            }
            int keys = 0;
            for (int k = 0; k < 7; k++) if (sn.K[k]) keys |= 1 << k;
            sn.log.clear();
            g.b();
            int A = gi(g, "A");
            byte[] arr = (byte[]) get(g, "a");
            sb.setLength(0);
            sb.append(t).append(',').append(keys).append(',').append(A).append(',').append(gi(g, "Q")).append(',').append(gi(g, "r")).append(',')
              .append(gi(g, "j")).append(',').append(gi(g, "H")).append(',').append(gi(g, "Ja")).append(',')
              .append(gi(g, "Da")).append(',').append(gi(g, "ea")).append(',')
              .append(gi(g, "fa")).append(',').append(gi(g, "ga")).append(',').append(gi(g, "Ga")).append(',').append(gi(g, "Fa")).append(',')
              .append(gi(g, "b")).append(',').append(gi(g, "c")).append(',').append(gi(g, "C")).append(',')
              .append(gi(g, "Aa")).append(',').append(gi(g, "ma")).append(',').append(gi(g, "J")).append(',');
            for (int i = 0; i < A; i++) sb.append(String.format("%02X", arr[i] & 0xFF));
            sb.append(',').append(String.join("|", sn.log));
            /* power-up columns (G7), appended so existing classic parsers still work */
            sb.append(',').append(gi(g, "ba")).append(',').append(gi(g, "na")).append(',')
              .append(gi(g, "Ea")).append(',').append(gi(g, "U")).append(',').append(gi(g, "v")).append(',')
              .append(gi(g, "Ma")).append(',').append(gi(g, "La")).append(',').append(gi(g, "Na")).append(',')
              .append(gi(g, "B")).append(',');
            int[][] slots = (int[][]) get(g, "Ba");
            for (int i = 0; i < 4; i++)
                sb.append(slots[i][0]).append(',').append(slots[i][1]).append(',')
                  .append(slots[i][2]).append(',').append(slots[i][3]).append(',');
            int T = gi(g, "T"), u = gi(g, "u");
            sb.append(T).append(',').append(u).append(',');
            byte[] ca = (byte[]) get(g, "Ca");
            for (int i = 0; i < 378; i += 4) {
                int v = 0;
                for (int k = 0; k < 4 && i + k < 378; k++)
                    if ((ca[i + k] & 0x20) != 0) v |= 1 << k;
                sb.append(String.format("%X", v));
            }
            sb.append(',');
            for (int r = 0; r < 3; r++)
                for (int c = 0; c < 3; c++) {
                    int x = T + c, y = u + r;
                    int v = (x >= 0 && x < 21 && y >= 0 && y < 18) ? (ca[x + y * 21] & 0x0F) : 0;
                    sb.append(v).append(',');
                }
            /* G8 campaign columns: level x, level-start score z, portal Y/X/y, maze da */
            sb.append(gi(g, "x")).append(',').append(gi(g, "z")).append(',')
              .append(gi(g, "Y")).append(',').append(gi(g, "X")).append(',')
              .append(gi(g, "y")).append(',').append(gi(g, "da"));
            System.out.println(sb);
            if (gi(g, "Ja") == 0 && sn.log.contains("state-123")) break;
        }
    }
}

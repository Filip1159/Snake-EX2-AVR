"""Helpers shared by the asset generators: RGB565 packing, C array formatting, RLE."""


def rgb565(r, g, b):
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)


def fmt(vals, width, per_line):
    lines = []
    for i in range(0, len(vals), per_line):
        lines.append("    " + ", ".join(f"0x{v:0{width}X}" for v in vals[i:i + per_line]) + ",")
    return "\n".join(lines)


def encode_rle(idx):
    out = bytearray()
    i = 0
    n = len(idx)
    while i < n:
        j = i
        while j + 1 < n and idx[j + 1] == idx[i] and j - i < 127:
            j += 1
        if j > i:
            out += bytes((0x80 | (j - i), idx[i]))
            i = j + 1
        else:
            k = i
            while k + 1 < n and k - i < 126 and not (k + 2 < n and idx[k + 1] == idx[k + 2]):
                k += 1
            out.append(k - i)
            out += bytes(idx[i:k + 1])
            i = k + 1
    return bytes(out)


def decode_rle(data, count):
    out = []
    i = 0
    while i < len(data):
        t = data[i]
        i += 1
        if t & 0x80:
            out += [data[i]] * ((t & 0x7F) + 1)
            i += 1
        else:
            out += list(data[i:i + t + 1])
            i += t + 1
    assert len(out) == count, (len(out), count)
    return out

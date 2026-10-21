// cnfont.h - draws Chinese characters and pinyin from font.bin (made by tools/make_font.py).
// Pure C++ (no Arduino), so the PC tests and screen previews use exactly the same code as the board.
//
//   CnFonts f;  cn_font_open(blob, len, &f);              blob = the "font" flash partition, memory mapped
//   cn_draw(&f.big, x, y, "你好", -1, put_pixel);          returns the width drawn
//   cn_wrap_next(&f.small, text, 192, true, &n)           the first line that fits in 192 px (n bytes); returns the rest
#pragma once
#include <stdint.h>
#include <stddef.h>
#include <string.h>

struct CnFace {
  const uint8_t *entries = nullptr;   // count x 12 bytes, sorted by codepoint
  const uint8_t *bitmaps = nullptr;
  uint32_t count = 0;
  uint8_t height = 0;
};
struct CnFonts { CnFace big, small, huge, latin; };   // huge = 36 px characters, latin = 26 px pinyin/English (fall back)
struct CnGlyph { uint16_t adv, bw; const uint8_t *bits; };

#define CN_BOX_ADV(f) ((f)->height * 2 / 3)        // a character the font does not have is drawn as a small box

static inline uint32_t cn_rd32(const uint8_t *p) { return p[0] | (p[1] << 8) | (p[2] << 16) | ((uint32_t)p[3] << 24); }
static inline uint16_t cn_rd16(const uint8_t *p) { return (uint16_t)(p[0] | (p[1] << 8)); }

// next codepoint from a UTF-8 string; 0 at the end; 0xFFFD for a broken byte (and moves on by one)
static inline uint32_t cn_utf8_next(const char **ps)
{
  const uint8_t *s = (const uint8_t *)*ps;
  if (!*s) return 0;
  uint32_t c = s[0]; int n = 0;
  if (c < 0x80) n = 0;
  else if ((c & 0xE0) == 0xC0) { c &= 0x1F; n = 1; }
  else if ((c & 0xF0) == 0xE0) { c &= 0x0F; n = 2; }
  else if ((c & 0xF8) == 0xF0) { c &= 0x07; n = 3; }
  else { *ps += 1; return 0xFFFD; }
  for (int i = 1; i <= n; i++) {
    if ((s[i] & 0xC0) != 0x80) { *ps += 1; return 0xFFFD; }
    c = (c << 6) | (s[i] & 0x3F);
  }
  *ps += n + 1;
  return c;
}

static inline bool cn_face_at(const uint8_t *p, size_t avail, CnFace *f)
{
  if (avail < 12 || memcmp(p, "PFNT", 4) != 0) return false;
  f->count = cn_rd32(p + 4);
  f->height = p[8];
  f->entries = p + 12;
  f->bitmaps = f->entries + (size_t)f->count * 12;
  return (size_t)(f->bitmaps - p) <= avail && f->height > 0;
}

static inline bool cn_font_open(const uint8_t *blob, size_t len, CnFonts *out)
{
  if (!blob || len < 16 || memcmp(blob, "PFN2", 4) != 0 || cn_rd32(blob + 4) < 2) return false;
  uint32_t nf = cn_rd32(blob + 4);
  uint32_t o0 = cn_rd32(blob + 8), o1 = cn_rd32(blob + 12);
  if (o0 >= len || o1 >= len) return false;
  if (!cn_face_at(blob + o0, len - o0, &out->big) || !cn_face_at(blob + o1, len - o1, &out->small)) return false;
  out->huge = out->big; out->latin = out->small;
  if (nf >= 3 && len >= 20) {
    uint32_t o2 = cn_rd32(blob + 16);
    if (o2 >= len || !cn_face_at(blob + o2, len - o2, &out->huge)) out->huge = out->big;
  }
  if (nf >= 4 && len >= 24) {
    uint32_t o3 = cn_rd32(blob + 20);
    if (o3 >= len || !cn_face_at(blob + o3, len - o3, &out->latin)) out->latin = out->small;
  }
  return true;
}

static inline bool cn_glyph(const CnFace *f, uint32_t cp, CnGlyph *g)
{
  if (!f || !f->entries) return false;
  uint32_t lo = 0, hi = f->count;
  while (lo < hi) {                                  // binary search by codepoint
    uint32_t mid = (lo + hi) / 2;
    const uint8_t *e = f->entries + (size_t)mid * 12;
    uint32_t c = cn_rd32(e);
    if (c == cp) { g->adv = cn_rd16(e + 4); g->bw = cn_rd16(e + 6); g->bits = f->bitmaps + cn_rd32(e + 8); return true; }
    if (c < cp) lo = mid + 1; else hi = mid;
  }
  return false;
}

static inline int cn_char_adv(const CnFace *f, uint32_t cp)
{
  CnGlyph g;
  return cn_glyph(f, cp, &g) ? g.adv : CN_BOX_ADV(f);
}

// width of the first nbytes of s (nbytes = -1: all of it)
static inline int cn_text_width(const CnFace *f, const char *s, int nbytes)
{
  const char *end = nbytes < 0 ? s + strlen(s) : s + nbytes;
  int w = 0;
  while (s < end && *s) w += cn_char_adv(f, cn_utf8_next(&s));
  return w;
}

typedef void (*cn_px_fn)(int x, int y);

static inline int cn_draw(const CnFace *f, int x, int y, const char *s, int nbytes, cn_px_fn px)
{
  const char *end = nbytes < 0 ? s + strlen(s) : s + nbytes;
  int x0 = x;
  while (s < end && *s) {
    uint32_t cp = cn_utf8_next(&s);
    CnGlyph g;
    if (cn_glyph(f, cp, &g)) {
      int rb = (g.bw + 7) / 8;
      for (int r = 0; r < f->height; r++)
        for (int c = 0; c < g.bw; c++)
          if (g.bits[r * rb + (c >> 3)] & (0x80 >> (c & 7))) px(x + c, y + r);
      x += g.adv;
    } else if (cp != ' ') {                          // missing: a small hollow box
      int b = CN_BOX_ADV(f) - 2, top = y + f->height - b - 2;
      for (int i = 0; i < b; i++) { px(x + 1 + i, top); px(x + 1 + i, top + b - 1); px(x + 1, top + i); px(x + b, top + i); }
      x += CN_BOX_ADV(f);
    } else x += CN_BOX_ADV(f);
  }
  return x - x0;
}

// punctuation that must not start a line
static inline bool cn_is_closing(uint32_t cp)
{
  static const uint32_t list[] = { 0xFF01, 0xFF0C, 0x3002, 0xFF1F, 0x3001, 0xFF1B, 0xFF1A, 0x201D, 0x2019, 0xFF09, 0x300B,
                                   0x3011, 0x2026, '!', ',', '.', '?', ';', ':', ')' };
  for (uint32_t c : list) if (c == cp) return true;
  return false;
}

// Finds the first line of s that fits in max_w pixels. at_spaces = break only between words (pinyin, English);
// otherwise break between any two characters (Chinese). *nbytes = the line's length in bytes. Returns where the next
// line starts (spaces after the break are skipped). A word too long for a line is cut so the text always moves on.
static inline const char *cn_wrap_next(const CnFace *f, const char *s, int max_w, bool at_spaces, int *nbytes)
{
  while (*s == ' ') s++;
  const char *p = s, *last_break = nullptr, *prev = s;
  int w = 0;
  while (*p) {
    const char *q = p;
    uint32_t cp = cn_utf8_next(&q);
    int a = cn_char_adv(f, cp);
    if (cp == ' ') last_break = p;
    if (w + a > max_w && p > s) {
      const char *cut = (at_spaces && last_break && last_break > s) ? last_break : p;
      if (!at_spaces && cn_is_closing(cp) && prev > s) cut = prev;   // never start a line with ！ or ，: take a character with it
      *nbytes = (int)(cut - s);
      while (*cut == ' ') cut++;
      return cut;
    }
    w += a; prev = p; p = q;     // prev = where the last character on the line starts
  }
  *nbytes = (int)(p - s);
  return p;
}

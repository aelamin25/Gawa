// faces.h - Pengu the penguin: drawing helpers, moods, expressions and animation frames.
// Pure C/C++ so it can also be compiled on a PC to preview the 200x200 image.
//
//   draw_face(mood, sketch, variant, frame, line, put_pixel)
//     mood    : one of the 22 moods in mood_t below: which little animation plays (tears, hearts, Zs, puffs ...)
//     sketch  : which of the 99 pencil drawings in sketches.h to show (SK_HOPEFUL, SK_JEALOUS ...), or -1 for the mood's usual one
//     variant : 0..FACE_VARIANTS-1, different extras for the same mood
//     frame   : 0..ANIM_FRAMES-1, one step of that mood's little animation
//     line    : up to 3 rows of big text under the penguin (or NULL)
#pragma once
#include <stdint.h>
#include <string.h>
#include <math.h>
#include "sketches.h"

#define FACE_W 200
#define FACE_H 200

typedef void (*face_px_fn)(int x, int y);

typedef enum {
  MOOD_IDLE = 0, MOOD_LISTENING, MOOD_HAPPY, MOOD_SAD, MOOD_THINKING, MOOD_SLEEP, MOOD_ANGRY,
  MOOD_SCARED, MOOD_GRUMPY, MOOD_GRATEFUL, MOOD_PROUD, MOOD_CURIOUS, MOOD_CONFIDENT, MOOD_PEACEFUL,
  MOOD_MISCHIEVOUS, MOOD_THOUGHTFUL, MOOD_RELIEF, MOOD_SHOCKED, MOOD_DISGUSTED, MOOD_NERVOUS, MOOD_TIRED, MOOD_SHY,
  MOOD_COUNT
} mood_t;
#define FACE_VARIANTS 3
#define ANIM_FRAMES   4

// ---------------------------------------------------------------- pixel sink with optional scale/offset
static face_px_fn g_px;
static float g_scale = 1.0f;
static int g_ox = 0, g_oy = 0;
static inline void px(int x, int y) {
  x = (int)(x * g_scale) + g_ox;
  y = (int)(y * g_scale) + g_oy;
  if (x >= 0 && y >= 0 && x < FACE_W && y < FACE_H) g_px(x, y);
}

// ---------------------------------------------------------------- primitives
static inline int inside_ellipse(int x, int y, int cx, int cy, int rx, int ry) {
  float dx = (float)(x - cx) / rx, dy = (float)(y - cy) / ry;
  return dx * dx + dy * dy <= 1.0f;
}
static void fill_circle(int cx, int cy, int r) {
  for (int y = -r; y <= r; y++)
    for (int x = -r; x <= r; x++)
      if (x * x + y * y <= r * r) px(cx + x, cy + y);
}
static void ring(int cx, int cy, int r, int thick) {
  int r2o = r * r, ri = r - thick, r2i = ri * ri;
  for (int y = -r; y <= r; y++)
    for (int x = -r; x <= r; x++) {
      int d = x * x + y * y;
      if (d <= r2o && d > r2i) px(cx + x, cy + y);
    }
}
static void ellipse_outline(int cx, int cy, int rx, int ry, int thick) {
  for (int y = -ry; y <= ry; y++)
    for (int x = -rx; x <= rx; x++)
      if (inside_ellipse(cx + x, cy + y, cx, cy, rx, ry) && !inside_ellipse(cx + x, cy + y, cx, cy, rx - thick, ry - thick))
        px(cx + x, cy + y);
}
// Part of a ring between two angles (degrees, 0 = right, 90 = down).
static void arc(int cx, int cy, int r, int thick, int a0, int a1) {
  int r2o = r * r, ri = r - thick, r2i = ri * ri;
  for (int y = -r; y <= r; y++)
    for (int x = -r; x <= r; x++) {
      int d = x * x + y * y;
      if (d > r2o || d <= r2i) continue;
      int ang = (int)(atan2((double)y, (double)x) * 180.0 / 3.14159265);
      if (ang < 0) ang += 360;
      if (a0 <= a1 ? (ang >= a0 && ang <= a1) : (ang >= a0 || ang <= a1)) px(cx + x, cy + y);
    }
}
static void thick_line(int x0, int y0, int x1, int y1, int r) {
  int dx = x1 > x0 ? x1 - x0 : x0 - x1, dy = y1 > y0 ? y1 - y0 : y0 - y1;
  int sx = x0 < x1 ? 1 : -1, sy = y0 < y1 ? 1 : -1, err = dx - dy;
  for (;;) {
    fill_circle(x0, y0, r);
    if (x0 == x1 && y0 == y1) break;
    int e2 = 2 * err;
    if (e2 > -dy) { err -= dy; x0 += sx; }
    if (e2 < dx)  { err += dx; y0 += sy; }
  }
}
static void fill_rect(int x, int y, int w, int h) {
  for (int j = 0; j < h; j++)
    for (int i = 0; i < w; i++) px(x + i, y + j);
}
static void fill_triangle_down(int cx, int top, int half_w, int h) {   // beak
  for (int j = 0; j < h; j++) {
    int w = half_w - (half_w * j) / h;
    for (int i = -w; i <= w; i++) px(cx + i, top + j);
  }
}

// ---------------------------------------------------------------- 5x7 font
static const uint8_t FONT[][5] = {
  {0x7E,0x11,0x11,0x11,0x7E}, {0x7F,0x49,0x49,0x49,0x36}, {0x3E,0x41,0x41,0x41,0x22}, {0x7F,0x41,0x41,0x22,0x1C},
  {0x7F,0x49,0x49,0x49,0x41}, {0x7F,0x09,0x09,0x09,0x01}, {0x3E,0x41,0x49,0x49,0x7A}, {0x7F,0x08,0x08,0x08,0x7F},
  {0x00,0x41,0x7F,0x41,0x00}, {0x20,0x40,0x41,0x3F,0x01}, {0x7F,0x08,0x14,0x22,0x41}, {0x7F,0x40,0x40,0x40,0x40},
  {0x7F,0x02,0x0C,0x02,0x7F}, {0x7F,0x04,0x08,0x10,0x7F}, {0x3E,0x41,0x41,0x41,0x3E}, {0x7F,0x09,0x09,0x09,0x06},
  {0x3E,0x41,0x51,0x21,0x5E}, {0x7F,0x09,0x19,0x29,0x46}, {0x46,0x49,0x49,0x49,0x31}, {0x01,0x01,0x7F,0x01,0x01},
  {0x3F,0x40,0x40,0x40,0x3F}, {0x1F,0x20,0x40,0x20,0x1F}, {0x3F,0x40,0x38,0x40,0x3F}, {0x63,0x14,0x08,0x14,0x63},
  {0x07,0x08,0x70,0x08,0x07}, {0x61,0x51,0x49,0x45,0x43},
  {0x3E,0x51,0x49,0x45,0x3E}, {0x00,0x42,0x7F,0x40,0x00}, {0x42,0x61,0x51,0x49,0x46}, {0x21,0x41,0x45,0x4B,0x31},
  {0x18,0x14,0x12,0x7F,0x10}, {0x27,0x45,0x45,0x45,0x39}, {0x3C,0x4A,0x49,0x49,0x30}, {0x01,0x71,0x09,0x05,0x03},
  {0x36,0x49,0x49,0x49,0x36}, {0x06,0x49,0x49,0x29,0x1E},
  {0x00,0x00,0x00,0x00,0x00}, {0x00,0x00,0x5F,0x00,0x00}, {0x02,0x01,0x51,0x09,0x06}, {0x00,0x60,0x60,0x00,0x00},
  {0x00,0x36,0x36,0x00,0x00}, {0x08,0x08,0x08,0x08,0x08}, {0x00,0x05,0x03,0x00,0x00}, {0x00,0x50,0x30,0x00,0x00},
};
static int glyph_index(char c) {
  if (c >= 'a' && c <= 'z') c -= 32;
  if (c >= 'A' && c <= 'Z') return c - 'A';
  if (c >= '0' && c <= '9') return 26 + (c - '0');
  switch (c) { case '!': return 37; case '?': return 38; case '.': return 39; case ':': return 40;
               case '-': return 41; case '\'': return 42; case ',': return 43; }
  return 36;
}
static void draw_text(int x, int y, const char *s, int scale) {
  float keep_s = g_scale; int keep_x = g_ox, keep_y = g_oy;
  g_scale = 1.0f; g_ox = 0; g_oy = 0;
  for (; *s; s++) {
    const uint8_t *g = FONT[glyph_index(*s)];
    for (int col = 0; col < 5; col++)
      for (int row = 0; row < 7; row++)
        if (g[col] & (1 << row)) fill_rect(x + col * scale, y + row * scale, scale, scale);
    x += 6 * scale;
  }
  g_scale = keep_s; g_ox = keep_x; g_oy = keep_y;
}
static int text_width(const char *s, int scale) { return (int)strlen(s) * 6 * scale - scale; }
static void draw_text_centered(int y, const char *s, int scale) { draw_text((FACE_W - text_width(s, scale)) / 2, y, s, scale); }

static int draw_wrapped(int y0, const char *text, int scale, int max_rows, int cols, int row_h) {
  int row = 0;
  const char *p = text;
  while (*p && row < max_rows) {
    while (*p == ' ') p++;
    int n = (int)strlen(p);
    int take = n <= cols ? n : cols;
    if (n > cols) { int cut = cols; while (cut > 0 && p[cut] != ' ') cut--; if (cut > 0) take = cut; }
    char buf[40];
    if (take > 39) take = 39;
    memcpy(buf, p, take); buf[take] = 0;
    draw_text_centered(y0 + row * row_h, buf, scale);
    p += take; row++;
  }
  return row;
}
static void draw_line_big(const char *s) {
  if (strlen(s) <= 16) { draw_text_centered(166, s, 2); return; }
  draw_wrapped(150, s, 2, 3, 16, 17);
}

// ---------------------------------------------------------------- the penguin (pencil sketches)
// The faces are real pencil drawings (sketches.h), 200x150, shown above the text. The little
// animations are drawn on top: bounce, extra tears, steam, a bobbing question mark, growing Zs.

static void draw_sketch(int which, int ox, int oy) {
  const uint8_t *d = SKETCHES[which];
  for (int y = 0; y < SKETCH_H; y++) {
    int yy = y + oy;
    if (yy < 0 || yy >= SKETCH_H) continue;                  // never draw into the text area
    const uint8_t *row = d + y * (SKETCH_W / 8);
    for (int x = 0; x < SKETCH_W; x++)
      if (row[x >> 3] & (0x80 >> (x & 7))) px(x + ox, yy);
  }
}
// small overlays
static void heart(int cx, int cy)        { fill_circle(cx - 3, cy - 2, 3); fill_circle(cx + 3, cy - 2, 3); for (int j = 0; j <= 5; j++) for (int i = -(5 - j); i <= 5 - j; i++) px(cx + i, cy + j); }
static void sparkle(int x, int y, int r) { thick_line(x - r, y, x + r, y, 0); thick_line(x, y - r, x, y + r, 0); px(x - 1, y - 1); px(x + 1, y + 1); px(x - 1, y + 1); px(x + 1, y - 1); }
static void tear(int x, int y)           { fill_circle(x, y, 3); px(x, y - 4); px(x, y - 5); px(x - 1, y - 3); px(x + 1, y - 3); }
static void music_note(int x, int y)     { fill_circle(x, y, 3); thick_line(x + 3, y, x + 3, y - 12, 0); thick_line(x + 3, y - 12, x + 9, y - 10, 0); }
static void puff(int x, int y, int r)    { ring(x, y, r, 1); ring(x + r, y - 2, r - 1, 1); ring(x - r + 1, y - 1, r - 2 > 1 ? r - 2 : 1, 1); }
static void qmark(int x, int y)          { draw_text(x, y, "?", 2); }
static void big_z(int x, int y, int s)   { draw_text(x, y, "Z", s); }

static void rain(int x, int y, int n)    { for (int i = 0; i < n; i++) { int xx = x + i * 14, yy = y + (i * 7) % 20; px(xx, yy); px(xx, yy + 1); px(xx, yy + 2); px(xx, yy + 3); } }
static void sweat_drop(int x, int y)     { fill_circle(x, y, 2); px(x, y - 3); px(x, y - 4); }

// The penguin in a mood.
//   mood    = which little animation plays on top (tears, hearts, Zs, puffs ...)
//   sketch  = which drawing to show (SK_...), or -1 for the mood's usual drawing
//   variant = which extras, frame = animation step
static int g_sketch = -1;
#define SK(x) (g_sketch >= 0 ? g_sketch : (x))
static void draw_pet(mood_t mood, int sketch, int variant, int frame, float scale, int ox, int oy) {
  g_scale = scale; g_ox = ox; g_oy = oy;
  g_sketch = (sketch >= 0 && sketch < SK_COUNT) ? sketch : -1;
  variant %= FACE_VARIANTS; frame %= ANIM_FRAMES;
  static const int bounce[ANIM_FRAMES] = { 0, -4, 0, -2 };
  static const int wobble[ANIM_FRAMES] = { -2, 2, -1, 1 };
  int dy = bounce[frame], wx = wobble[frame];

  switch (mood) {
    case MOOD_HAPPY:
      if (variant == 0)      { draw_sketch(SK(SK_EXCITED), 0, dy); if (frame & 1) { sparkle(30, 20, 4); sparkle(176, 60, 3); } }
      else if (variant == 1) { draw_sketch(SK(SK_HAPPY), 0, dy); heart(178, 40 - frame * 3); if (frame >= 2) heart(22, 70 - (frame - 2) * 4); }
      else                   { draw_sketch(SK(SK_SILLY), 0, dy); music_note(176, 30 - (frame & 1) * 3); if (frame >= 2) music_note(20, 50); }
      break;
    case MOOD_SAD:
      draw_sketch(SK(SK_SAD), 0, 0);
      if (variant == 0 || variant == 2) tear(34, 96 + frame * 6);
      if (variant == 1 || variant == 2) tear(166, 90 + ((frame + 2) % 4) * 6);
      break;
    case MOOD_ANGRY:
      draw_sketch(SK(SK_ANGRY), wx, 0);
      if (frame >= 1) puff(176, 36 - frame * 2, 4 + frame);
      if (frame >= 2 && variant != 1) puff(24, 40 - frame, 3 + frame);
      break;
    case MOOD_THINKING:                       // confused: not sure, did not understand
      draw_sketch(SK(SK_CONFUSED), 0, 0);
      if (variant != 2) qmark(170, 18 - frame * 3);
      if (variant == 1 && frame >= 2) qmark(12, 30);
      break;
    case MOOD_THOUGHTFUL:
      draw_sketch(SK(variant == 1 ? SK_THOUGHTFUL2 : SK_THOUGHTFUL), 0, 0);
      if (frame >= 1) fill_circle(170, 30, 2); if (frame >= 2) fill_circle(178, 20, 3); if (frame >= 3) ring(188, 8, 6, 1);
      break;
    case MOOD_LISTENING:
      draw_sketch(SK(SK_SURPRISED), wx, 0);
      break;
    case MOOD_SLEEP:
      draw_sketch(SK(SK_SLEEPY), 0, 0);
      if (frame >= 1) big_z(164, 10, 2);
      if (frame >= 2) big_z(180, -2, 2);
      if (frame >= 3) big_z(150, 24, 1);
      break;
    case MOOD_SCARED:
      draw_sketch(SK(SK_SCARED), wx, 0);
      if (frame & 1) sweat_drop(176, 40);
      break;
    case MOOD_GRUMPY:
      draw_sketch(SK(SK_GRUMPY), 0, 0);
      rain(150, 10 + frame * 4, 3);
      break;
    case MOOD_GRATEFUL:
      draw_sketch(SK(SK_GRATEFUL), 0, dy);
      heart(180, 44 - frame * 4); if (frame >= 2) heart(20, 60 - (frame - 2) * 5);
      break;
    case MOOD_PROUD:
      draw_sketch(SK(SK_PROUD), 0, dy);
      if (frame & 1) { sparkle(176, 30, 4); sparkle(24, 50, 3); }
      break;
    case MOOD_CURIOUS:
      draw_sketch(SK(SK_CURIOUS), 0, 0);
      if (variant != 2) qmark(168, 14 - frame * 3);
      break;
    case MOOD_CONFIDENT:
      draw_sketch(SK(SK_CONFIDENT), 0, dy);
      if (frame & 1) sparkle(172, 24, 5);
      break;
    case MOOD_PEACEFUL:
      draw_sketch(SK(SK_PEACEFUL), 0, frame == 2 ? -2 : 0);
      break;
    case MOOD_MISCHIEVOUS:
      draw_sketch(SK(SK_MISCHIEVOUS), wx, 0);
      if (frame & 1) sparkle(176, 36, 3);
      break;
    case MOOD_RELIEF:
      draw_sketch(SK(SK_RELIEF), 0, 0);
      sweat_drop(176, 30 + frame * 8);
      break;
    case MOOD_SHOCKED:
      draw_sketch(SK(SK_SHOCKED), wx, 0);
      break;
    case MOOD_DISGUSTED:
      draw_sketch(SK(SK_DISGUSTED), wx, 0);
      break;
    case MOOD_NERVOUS:
      draw_sketch(SK(SK_NERVOUS), wx, 0);
      sweat_drop(170, 30 + frame * 6); if (frame >= 2) sweat_drop(26, 40 + (frame - 2) * 6);
      break;
    case MOOD_TIRED:
      draw_sketch(SK(SK_TIRED), 0, 0);
      if (frame >= 2) big_z(170, 14, 1); if (frame >= 3) big_z(180, 2, 1);
      break;
    case MOOD_SHY:
      draw_sketch(SK(SK_SHY), 0, 0);
      if (frame & 1) heart(180, 36);
      break;
    default:   // idle: the calm smiling face, with a tiny breath on frame 2
      draw_sketch(SK(SK_HAPPY), 0, frame == 2 ? -2 : 0);
      break;
  }
  g_scale = 1.0f; g_ox = 0; g_oy = 0; g_sketch = -1;
}

static bool g_chat_dot = false;   // true while Pengu is listening: a small "(((" ear mark top-left
static int  g_chat_pulse = 2;     // how many arcs the ear mark shows (1..3): it pulses while you talk
static bool g_heart = false;      // true for the "I HEARD YOU" moment: a little heart top-right

static void draw_face(mood_t mood, int sketch, int variant, int frame, const char *line, face_px_fn put_pixel) {
  g_px = put_pixel;
  draw_pet(mood, sketch, variant, frame, 1.0f, 0, 0);
  if (line && *line) draw_line_big(line);
  if (g_chat_dot) {                                                   // "(((" listening mark
    fill_circle(12, 12, 1);
    if (g_chat_pulse >= 1) arc(12, 12, 4, 2, 100, 260);
    if (g_chat_pulse >= 2) arc(12, 12, 8, 2, 100, 260);
    if (g_chat_pulse >= 3) arc(12, 12, 12, 2, 100, 260);
  }
  if (g_heart) { heart(188, 116); heart(176, 130); }                  // "I heard you": bottom right, clear of the mood extras
}

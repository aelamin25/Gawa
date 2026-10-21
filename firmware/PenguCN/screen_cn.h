// screen_cn.h - the Chinese screen. Pengu stays big in the middle; the strip underneath shows ONE thing at a time and
// softly fades between them: the characters, then the pinyin, then the English, then round again.
// Pure C++ (uses faces.h + cnfont.h), so tools/test_cn/preview_cn.cpp renders the same frames to PNGs on a PC.
//
//   +----------------------------+
//   |(((                       S |   ear mark while it is your turn; a tiny status word top right
//   |      [Pengu, full size]    |   the pencil sketch, moved up 6 px to make room
//   |                            |
//   |           你好！            |   the strip (y 140..193): as big as fits on one line (characters 36/24 px, pinyin
//   |                            |   and English 26/20 px)
//   |            • ○ ○           |   which of the three you are looking at
//   +----------------------------+
// "Fading" on e-paper: the old text in a 50% dot pattern, then the new one in 50%, then the new one solid.
#pragma once
#include "faces.h"
#include "cnfont.h"

#define CN_MARGIN    4
#define CN_WIDTH     (FACE_W - 2 * CN_MARGIN)
#define CN_PET_LIFT  6          // the sketches start 6 rows down: lift them so the strip gets 60 px
#define CN_STRIP_Y   140
#define CN_STRIP_H   53         // 140..193, the dots sit below
#define CN_DOTS_Y    197

enum CnPart { CN_HANZI = 0, CN_PINYIN = 1, CN_ENGLISH = 2 };
enum CnInk  { CN_INK_HALF = 2, CN_INK_FULL = 4 };

static face_px_fn cn_sink = nullptr;
static int cn_ink = CN_INK_FULL;
static void cn_px_inked(int x, int y)
{
  if (x < 0 || y < 0 || x >= FACE_W || y >= FACE_H) return;
  if (cn_ink == CN_INK_HALF && ((x + y) & 1)) return;      // a checkerboard: the text looks grey, half there
  cn_sink(x, y);
}

// lays out up to max_lines of text, centred; draw = false only measures. Returns the total height.
static int cn_lines(const CnFace *f, int y, const char *text, bool at_spaces, int max_lines, int gap, bool draw)
{
  const char *p = text; int h = 0;
  for (int line = 0; line < max_lines && p && *p; line++) {
    int nb = 0;
    const char *next = cn_wrap_next(f, p, CN_WIDTH, at_spaces, &nb);
    if (draw) cn_draw(f, (FACE_W - cn_text_width(f, p, nb)) / 2, y + h, p, nb, cn_px_inked);
    h += f->height + gap;
    p = next;
  }
  return h - gap;
}

static int cn_count_lines(const CnFace *f, const char *text, bool at_spaces)
{
  int n = 0; const char *p = text;
  while (p && *p && n < 9) { int nb; p = cn_wrap_next(f, p, CN_WIDTH, at_spaces, &nb); n++; }
  return n;
}

// which face a part uses: as big as fits on one line. Characters 36 px (else 24 px), pinyin and English 26 px (else 20 px)
static const CnFace *cn_face_for(const CnFonts *fonts, CnPart part, const char *text)
{
  if (part != CN_HANZI) return cn_count_lines(&fonts->latin, text, true) <= 1 ? &fonts->latin : &fonts->small;
  return cn_count_lines(&fonts->huge, text, false) <= 1 ? &fonts->huge : &fonts->big;
}

// the strip: one part, centred vertically, in full or half ink
static void cn_strip(const CnFonts *fonts, CnPart part, const char *text, int ink)
{
  if (!text || !*text) return;
  const CnFace *f = cn_face_for(fonts, part, text);
  if (!f->height) return;                            // no font flashed
  bool spaces = part != CN_HANZI;
  int max_lines = CN_STRIP_H / f->height; if (max_lines < 1) max_lines = 1; if (max_lines > 2) max_lines = 2;
  int h = cn_lines(f, 0, text, spaces, max_lines, 1, false);
  cn_ink = ink;
  cn_lines(f, CN_STRIP_Y + (CN_STRIP_H - h) / 2, text, spaces, max_lines, 1, true);
  cn_ink = CN_INK_FULL;
}

// The whole screen.
//   part/text: what the strip shows. ink: CN_INK_FULL or CN_INK_HALF (a step of the fade).
//   parts: how many of the three exist yet (1 until the pinyin + English arrive) -> the dots.
static void draw_card(const CnFonts *fonts, mood_t mood, int frame, CnPart part, const char *text, int ink,
                      int parts, bool listening, const char *corner, face_px_fn put_pixel)
{
  g_px = put_pixel; cn_sink = put_pixel;
  g_chat_dot = false; g_heart = false;
  draw_pet(mood, -1, 0, frame, 1.0f, 0, -CN_PET_LIFT);
  cn_strip(fonts, part, text, ink);
  if (parts > 1) {                                   // • ○ ○  which one is showing
    int x0 = FACE_W / 2 - (parts - 1) * 6;
    for (int i = 0; i < parts; i++) {
      if (i == (int)part) fill_circle(x0 + i * 12, CN_DOTS_Y, 2);
      else ring(x0 + i * 12, CN_DOTS_Y, 2, 1);
    }
  }
  if (listening) {                                   // the "(((" ear mark, top left: your turn
    fill_circle(12, 14, 2);
    arc(12, 14, 6, 2, 100, 260); arc(12, 14, 11, 2, 100, 260); arc(12, 14, 16, 2, 100, 260);
  }
  if (corner && *corner) draw_text(FACE_W - text_width(corner, 1) - 2, 2, corner, 1);
}

// a fade between two parts is 3 quick e-paper updates: old at half ink, new at half ink, new solid
static void draw_fade_step(const CnFonts *fonts, mood_t mood, int step, CnPart from, const char *from_text,
                           CnPart to, const char *to_text, int parts, bool listening, face_px_fn put_pixel)
{
  if (step == 0)      draw_card(fonts, mood, 0, from, from_text, CN_INK_HALF, parts, listening, "", put_pixel);
  else if (step == 1) draw_card(fonts, mood, 0, to, to_text, CN_INK_HALF, parts, listening, "", put_pixel);
  else                draw_card(fonts, mood, 0, to, to_text, CN_INK_FULL, parts, listening, "", put_pixel);
}

// the big English words of v13 (rest screen, errors, setup): full-size Pengu + up to 3 rows of 5x7 text
static void draw_en(mood_t mood, int frame, const char *line, const char *corner, face_px_fn put_pixel)
{
  g_chat_dot = false; g_heart = false;
  draw_face(mood, -1, 0, frame, line, put_pixel);
  if (corner && *corner) draw_text(FACE_W - text_width(corner, 1) - 2, 2, corner, 1);
}

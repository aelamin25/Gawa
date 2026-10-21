// preview_cn.cpp - renders Pengu's screens (200x200) to PGM files, exactly as the board draws them.
// g++ -std=c++17 -I../../firmware/PenguCN preview_cn.cpp -o preview_cn && ./preview_cn ../../build/font.bin out_dir
#include <cstdio>
#include <cstring>
#include <vector>
#include <string>
#include "screen_cn.h"

static uint8_t img[FACE_H][FACE_W];
static void put(int x, int y) { if (x >= 0 && y >= 0 && x < FACE_W && y < FACE_H) img[y][x] = 1; }

static void save(const std::string &path)
{
  FILE *f = fopen(path.c_str(), "wb");
  fprintf(f, "P5\n%d %d\n255\n", FACE_W, FACE_H);
  for (int y = 0; y < FACE_H; y++) for (int x = 0; x < FACE_W; x++) fputc(img[y][x] ? 0 : 255, f);
  fclose(f);
}

int main(int argc, char **argv)
{
  if (argc < 3) { printf("usage: preview_cn font.bin out_dir\n"); return 2; }
  FILE *f = fopen(argv[1], "rb"); if (!f) return 2;
  std::vector<uint8_t> blob; int c; while ((c = fgetc(f)) != EOF) blob.push_back((uint8_t)c); fclose(f);
  CnFonts fonts; if (!cn_font_open(blob.data(), blob.size(), &fonts)) { printf("bad font\n"); return 1; }
  std::string out = argv[2];

  // one line, the way it plays: characters -> fade -> pinyin -> fade -> English
  const char *h = "你好！", *p = "nǐ hǎo!", *e = "Hello!";
  int k = 0;
  auto frame = [&](const char *name) { save(out + "/" + std::to_string(k++) + "_" + name + ".pgm"); };
  memset(img, 0, sizeof(img)); draw_card(&fonts, MOOD_HAPPY, 0, CN_HANZI, h, CN_INK_FULL, 3, false, "", put); frame("hanzi");
  for (int s = 0; s < 3; s++) { memset(img, 0, sizeof(img)); draw_fade_step(&fonts, MOOD_HAPPY, s, CN_HANZI, h, CN_PINYIN, p, 3, false, put); frame("fade"); }
  for (int s = 0; s < 3; s++) { memset(img, 0, sizeof(img)); draw_fade_step(&fonts, MOOD_IDLE, s, CN_PINYIN, p, CN_ENGLISH, e, 3, true, put); frame("fade"); }

  // other lines: longer characters (2 rows of 24 px), long pinyin, the card not here yet
  struct { const char *name; CnPart part; const char *t; int parts; } more[] = {
    { "long_hanzi", CN_HANZI, "你叫什么名字？", 3 },
    { "long_pinyin", CN_PINYIN, "nǐ jiào shén me míng zi?", 3 },
    { "long_english", CN_ENGLISH, "What is your name?", 3 },
    { "waiting", CN_HANZI, "我是Pengu。", 1 },
  };
  for (auto &m : more) { memset(img, 0, sizeof(img)); draw_card(&fonts, MOOD_CURIOUS, 0, m.part, m.t, CN_INK_FULL, m.parts, true, "", put); frame(m.name); }
  memset(img, 0, sizeof(img)); draw_en(MOOD_IDLE, 0, "PRESS THE GEAR", "", put); frame("rest");
  printf("previews written\n");
  return 0;
}

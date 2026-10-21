// test_cnfont.cpp - checks cnfont.h against the real font.bin.
// g++ -std=c++17 -I../../firmware/PenguCN test_cnfont.cpp -o test_cnfont && ./test_cnfont ../../build/font.bin
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
#include "cnfont.h"

static int fails = 0;
#define CHECK(c) do { if (!(c)) { printf("FAIL %s:%d  %s\n", __FILE__, __LINE__, #c); fails++; } } while (0)

static int pixels = 0;
static void count_px(int, int) { pixels++; }

int main(int argc, char **argv)
{
  // utf-8
  const char *s = "a你ǎ\xF0\x9F\x98\x80\xFF";
  CHECK(cn_utf8_next(&s) == 'a');
  CHECK(cn_utf8_next(&s) == 0x4F60);
  CHECK(cn_utf8_next(&s) == 0x01CE);
  CHECK(cn_utf8_next(&s) == 0x1F600);
  CHECK(cn_utf8_next(&s) == 0xFFFD);      // a broken byte
  CHECK(cn_utf8_next(&s) == 0);           // the end

  if (argc < 2) { printf("usage: test_cnfont font.bin\n"); return 2; }
  FILE *f = fopen(argv[1], "rb");
  if (!f) { printf("no font file\n"); return 2; }
  std::vector<uint8_t> blob; int c;
  while ((c = fgetc(f)) != EOF) blob.push_back((uint8_t)c);
  fclose(f);

  CnFonts fonts;
  CHECK(cn_font_open(blob.data(), blob.size(), &fonts));
  CHECK(fonts.big.height == 24);
  CHECK(fonts.small.height == 20);
  CHECK(fonts.big.count > 3755);
  CHECK(fonts.huge.height == 36);
  CHECK(fonts.huge.count == fonts.big.count);
  CHECK(cn_text_width(&fonts.huge, "你好", -1) == 72);
  CHECK(fonts.latin.height == 26);

  CnGlyph g;
  CHECK(cn_glyph(&fonts.big, 0x4F60, &g));          // 你
  CHECK(g.adv == 24);
  CHECK(cn_glyph(&fonts.big, 0x996D, &g));          // 饭
  CHECK(cn_glyph(&fonts.big, 0xFF01, &g));          // ！
  CHECK(cn_glyph(&fonts.big, 'P', &g));
  CHECK(cn_glyph(&fonts.small, 0x01CE, &g));        // ǎ
  CHECK(cn_glyph(&fonts.small, 0x00FC, &g));        // ü
  CHECK(cn_glyph(&fonts.latin, 0x01CE, &g));        // ǎ, big
  CHECK(!cn_glyph(&fonts.big, 0x1F600, &g));        // no emoji

  CHECK(cn_text_width(&fonts.big, "你好", -1) == 48);
  CHECK(cn_text_width(&fonts.big, "\xF0\x9F\x98\x80", -1) == CN_BOX_ADV(&fonts.big));

  pixels = 0;
  int w = cn_draw(&fonts.big, 0, 0, "你好", -1, count_px);
  CHECK(w == 48);
  CHECK(pixels > 60);                                // something was drawn

  // wrapping: 10 characters -> 8 on the first line, 2 on the second
  const char *line = "我们吃饭吧我们吃饭吧";
  int nbytes = 0;
  const char *rest = cn_wrap_next(&fonts.big, line, 192, false, &nbytes);
  CHECK(nbytes == 24);
  rest = cn_wrap_next(&fonts.big, rest, 192, false, &nbytes);
  CHECK(nbytes == 6);
  CHECK(*rest == 0);

  // pinyin breaks at spaces, never inside a word
  const char *py = "wǒ men chī fàn ba wǒ men chī fàn ba wǒ men";
  rest = cn_wrap_next(&fonts.small, py, 192, true, &nbytes);
  CHECK(nbytes > 0 && (py[nbytes] == ' ' || py[nbytes] == 0));
  CHECK(cn_text_width(&fonts.small, py, nbytes) <= 192);
  CHECK(*rest != ' ');                               // the space is skipped

  // a word longer than the line still makes progress
  rest = cn_wrap_next(&fonts.small, "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa", 192, true, &nbytes);
  CHECK(nbytes > 0 && nbytes < 70);

  printf(fails ? "%d FAILED\n" : "all cnfont tests passed\n", fails);
  return fails ? 1 : 0;
}

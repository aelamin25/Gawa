// list_lines.cpp - prints every line Pengu can show on screen, one per line, so make_speech.py can record them.
// From the main folder:   g++ -std=c++17 -w -I firmware/Pengu tools/list_lines.cpp -o /tmp/list_lines && /tmp/list_lines > tools/speech_lines.txt
#include <stdio.h>
#include <string.h>
#include "phrases.h"
int main() {
  printf("WAKING UP...\n");
  for (int f = 0; f < (int)FAMILY_COUNT; f++)
    for (int r = 0; r < FAMILIES[f].nreplies; r++) {
      bool has = strstr(FAMILIES[f].replies[r], "%s") != NULL;
      int reps = has ? FAMILIES[f].ntopics : 1;
      for (int t = 0; t < reps; t++) { char b[128]; snprintf(b, 128, FAMILIES[f].replies[r], FAMILIES[f].topics[t]); if (*b) printf("%s\n", b); }
    }
  for (int k = 2; k < 6; k++) printf("%s\n%s\n", ANSWERS[k].yes, ANSWERS[k].no);
  const char **extra[] = { R_LISTEN, R_HEARD_YOU, R_HEARD_END, R_HEARD_SAD, R_HEARD_GOOD, R_NOTHING };
  int cnt[] = { N(R_LISTEN), N(R_HEARD_YOU), N(R_HEARD_END), N(R_HEARD_SAD), N(R_HEARD_GOOD), N(R_NOTHING) };
  for (int i = 0; i < 6; i++) for (int j = 0; j < cnt[i]; j++) printf("%s\n", extra[i][j]);
  return 0;
}

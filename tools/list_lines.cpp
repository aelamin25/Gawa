// list_lines.cpp - prints every line Pengu can show on screen with its feeling tag, one per line (tag TAB line),
// so make_speech.py can record each one in the right tone of voice.
// From the main folder:   g++ -std=c++17 -w -I firmware/Pengu tools/list_lines.cpp -o /tmp/list_lines && /tmp/list_lines > tools/speech_lines.txt
#include <stdio.h>
#include <string.h>
#include "phrases.h"
// the feeling each face/animation stands for, as a word the voice can act
static const char *tag_of(mood_t m) {
  switch (m) {
    case MOOD_HAPPY: return "happy";        case MOOD_PROUD: return "proud";       case MOOD_SAD: return "sad";
    case MOOD_THINKING: return "thoughtful"; case MOOD_SLEEP: return "sleepy";     case MOOD_ANGRY: return "angry";
    case MOOD_SCARED: return "scared";      case MOOD_GRUMPY: return "grumpy";     case MOOD_GRATEFUL: return "loving";
    case MOOD_CURIOUS: return "curious";    case MOOD_CONFIDENT: return "confident"; case MOOD_PEACEFUL: return "calm";
    case MOOD_MISCHIEVOUS: return "playful"; case MOOD_THOUGHTFUL: return "thoughtful"; case MOOD_RELIEF: return "relieved";
    case MOOD_SHOCKED: return "excited";    case MOOD_DISGUSTED: return "grumpy";  case MOOD_NERVOUS: return "nervous";
    case MOOD_TIRED: return "sleepy";       case MOOD_SHY: return "shy";           case MOOD_LISTENING: return "curious";
    default: return "calm";
  }
}
int main() {
  printf("sleepy\tWAKING UP...\n");
  for (int f = 0; f < (int)FAMILY_COUNT; f++)
    for (int r = 0; r < FAMILIES[f].nreplies; r++) {
      bool has = strstr(FAMILIES[f].replies[r], "%s") != NULL;
      int reps = has ? FAMILIES[f].ntopics : 1;
      const char *tag = tag_of(FAMILIES[f].mood);
      if (FAMILIES[f].ask == Q_PARTY) tag = "excited";          // a party question is always said with a bounce
      for (int t = 0; t < reps; t++) { char b[128]; snprintf(b, 128, FAMILIES[f].replies[r], FAMILIES[f].topics[t]); if (*b) printf("%s\t%s\n", tag, b); }
    }
  const char *yes_tag[] = { "", "", "loving", "sleepy", "curious", "excited" };
  const char *no_tag[]  = { "", "", "calm",   "calm",   "calm",    "happy" };
  for (int k = 2; k < 6; k++) printf("%s\t%s\n%s\t%s\n", yes_tag[k], ANSWERS[k].yes, no_tag[k], ANSWERS[k].no);
  struct { const char **list; int n; const char *tag; } extra[] = {
    { R_LISTEN, N(R_LISTEN), "curious" }, { R_HEARD_YOU, N(R_HEARD_YOU), "thoughtful" }, { R_HEARD_END, N(R_HEARD_END), "calm" },
    { R_HEARD_SAD, N(R_HEARD_SAD), "loving" }, { R_HEARD_GOOD, N(R_HEARD_GOOD), "happy" }, { R_NOTHING, N(R_NOTHING), "calm" } };
  for (auto &e : extra) for (int j = 0; j < e.n; j++) printf("%s\t%s\n", e.tag, e.list[j]);
  return 0;
}

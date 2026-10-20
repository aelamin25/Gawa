// phrases.h - what Gawa understands and what it says back.
//
// Gawa learns PATTERNS, not just fixed sentences. A "family" is:
//     prefix + one of the TOPICS + one of the ENDINGS
// e.g. "my " + "cat" + " died"  ->  "my cat died", "my dog died", "my cat is sick", ...
// The reply can use the topic word with %s:  "SO SORRY ABOUT YOUR %s" -> "SO SORRY ABOUT YOUR CAT".
//
// Each family also says which face Gawa makes and whether it ENDS the conversation (like "thank you" or "stop"
// on a Google speaker). Any sentence containing "gawa" can start a conversation on its own.
//
// Rules: plain English, no contractions (write "I am", not "I'm"). The chip holds about 400 sentences in total.
#pragma once
#include "faces.h"

struct Family {
  mood_t       mood;
  const char  *prefix;        // may be ""
  const char **topics;  int ntopics;
  const char **endings; int nendings;   // at least one; "" means no ending
  const char **replies; int nreplies;   // %s = the topic word in capitals
  bool         ends_chat;     // true: after this reply Gawa stops listening (you must say "Hey Gawa" again)
};

#define N(a) (int)(sizeof(a) / sizeof(a[0]))
static const char *E_NONE[] = { "" };

// ================================================================ wake words
static const char *T_WAKE[]   = { "hey gawa", "hi gawa", "hello gawa", "okay gawa" };
static const char *R_LISTEN[] = { "I AM LISTENING", "YES? GO ON", "I AM ALL EARS", "TELL ME" };

// ================================================================ talking TO Gawa (works any time)
static const char *T_HOWRU[]  = { "how are you", "hey how are you", "hi how are you", "how are you gawa", "gawa how are you", "hey gawa how are you",
                                  "how are you doing", "how is it going", "are you okay", "are you okay gawa", "what is up", "how was your day" };
static const char *R_HOWRU[]  = { "GREAT! HOW WAS YOUR DAY? GOOD OR BAD?", "HAPPY YOU ARE HERE. GOOD DAY OR BAD DAY?", "ALL GOOD! AND YOUR DAY, GOOD OR BAD?" };
static const char *T_NAME[]   = { "what is your name", "who are you", "what are you" };
static const char *R_NAME[]   = { "I AM GAWA, YOUR PENGUIN", "GAWA! NICE TO MEET YOU", "A PENGUIN CALLED GAWA" };
static const char *T_LIKEME[] = { "do you like me", "do you love me", "are we friends", "am I your friend" };
static const char *R_LIKEME[] = { "I LIKE YOU A LOT", "YOU ARE MY FAVOURITE", "BEST FRIENDS, OBVIOUSLY" };
static const char *T_LOVEYOU[] = { "I love you", "I love you gawa", "you are the best", "you are cute", "good penguin" };
static const char *R_LOVEYOU[] = { "I LOVE YOU TOO", "OH STOP IT. I AM BLUSHING", "YOU MADE MY DAY" };
static const char *T_DOING[]  = { "what are you doing", "what are you up to", "are you bored", "are you awake" };
static const char *R_DOING[]  = { "WAITING FOR YOU", "WATCHING THE ROOM", "THINKING ABOUT FISH", "JUST BEING A PENGUIN" };
static const char *T_SLEEPYQ[] = { "are you sleepy", "are you tired", "do you sleep" };
static const char *R_SLEEPYQ[] = { "A BIT. BUT I AM HERE", "PENGUINS NAP A LOT", "ONLY WHEN YOU GO QUIET" };
static const char *T_JOKE[]   = { "tell me a joke", "make me laugh", "say something funny", "do a trick" };
static const char *R_JOKE[]   = { "I ONLY KNOW PENGUIN JOKES", "WHAT DO I WEAR? A TUX. ALWAYS", "I TRIED TO FLY ONCE. ONCE" };
static const char *T_THANKS[] = { "thank you", "thank you gawa", "thanks", "thanks gawa", "I am grateful", "I appreciate you" };
static const char *R_THANKS[] = { "ANY TIME, FRIEND", "THAT IS WHAT I AM FOR", "YOU ARE WELCOME" };
static const char *T_MORNING[] = { "good morning", "good morning gawa", "morning gawa", "I am home", "I am back", "hello" };
static const char *R_MORNING[] = { "HELLO FRIEND! GOOD DAY OR BAD DAY?", "GOOD TO SEE YOU! HOW WAS IT, GOOD OR BAD?", "I MISSED YOU! GOOD DAY OR BAD?" };
static const char *T_NIGHT[]  = { "good night", "good night gawa", "night night", "I am going to bed", "see you tomorrow", "goodbye gawa" };
static const char *R_NIGHT[]  = { "NIGHT NIGHT", "SLEEP WELL FRIEND", "SEE YOU TOMORROW" };
static const char *T_STOP[]   = { "stop gawa", "okay stop", "that is all", "I have finished", "bye gawa", "okay bye" };
static const char *R_STOP[]   = { "OKAY. I AM HERE IF YOU NEED ME", "BYE FOR NOW", "OKAY, FRIEND" };

// ================================================================ answers to "good day or bad day?"
static const char *T_DAY_ANS_GOOD[] = { "pretty good", "it was good", "really good", "it was great", "it was amazing", "a good day", "good day", "very good" };
static const char *R_DAY_ANS_GOOD[] = { "YAY! GLAD IT WAS GOOD", "A GOOD ONE! LOVE THAT", "GOOD DAYS ARE THE BEST" };
static const char *T_DAY_ANS_BAD[]  = { "not great", "it was bad", "pretty bad", "it was awful", "it was terrible", "a bad day", "bad day", "not good" };
static const char *R_DAY_ANS_BAD[]  = { "OH NO. BIG HUG", "SORRY IT WAS BAD. I AM HERE", "TOMORROW IS A NEW ONE" };
static const char *T_DAY_ANS_MEH[]  = { "it was okay", "it was fine", "so so", "pretty normal", "a busy day", "it was boring", "same as usual" };
static const char *R_DAY_ANS_MEH[]  = { "OKAY DAYS COUNT TOO", "FAIR ENOUGH. I AM HERE", "A NORMAL DAY IS FINE BY ME" };

// ================================================================ someone or something is lost / hurt (sad)
static const char *T_LOVED[] = { "cat", "dog", "hamster", "pet", "gran", "grandad", "mum", "dad", "brother", "sister", "friend" };
static const char *E_LOSS[]  = { " died", " is sick", " is in hospital" };
static const char *R_LOSS[]  = { "SO SORRY ABOUT YOUR %s", "OH NO. POOR %s", "YOUR %s. I AM SO SORRY", "THAT IS SO HARD. BIG HUG" };

// ================================================================ lost / broke / failed (sad)
static const char *T_THINGS[] = { "phone", "keys", "wallet", "job", "game", "money" };
static const char *R_LOSTIT[] = { "OH NO, YOUR %s!", "LOSING YOUR %s IS ROUGH", "UGH. I FEEL THAT" };
static const char *T_BROKE[]  = { "phone", "arm", "leg", "bike", "laptop", "heart" };
static const char *R_BROKE[]  = { "OUCH, YOUR %s!", "OH NO, NOT YOUR %s", "THAT IS BAD LUCK. SORRY" };
static const char *T_TESTS[]  = { "exam", "test", "driving test", "interview", "audition" };
static const char *R_FAILED[] = { "YOUR %s. THAT STINGS", "SORRY ABOUT THE %s", "I AM HERE. THAT IS HARD" };

// ================================================================ good news (happy / proud)
static const char *R_PASSED[] = { "YOU PASSED YOUR %s!", "YES! THE %s IS DONE!", "SO PROUD OF YOU", "YOU DID IT! WOO" };
static const char *T_GOT[]    = { "the job", "a new job", "a promotion", "an A", "a puppy", "a kitten", "a gift", "engaged", "paid today" };
static const char *R_GOT[]    = { "WOW, %s! AMAZING", "YAY! TELL ME MORE", "THAT IS GREAT NEWS", "BEST NEWS ALL DAY" };
static const char *T_WON[]    = { "the game", "my game", "the match", "the race", "a prize", "the final" };
static const char *R_WON[]    = { "YOU WON %s!", "CHAMPION! WOO", "SO PROUD OF YOU", "HIGH FIVE!" };
static const char *T_AM_HAPPY[] = { "happy", "so happy", "really happy", "excited", "so excited", "glad", "in love", "having fun" };
static const char *R_HAPPY[]  = { "YAY! TELL ME MORE", "THAT IS GREAT!", "SO HAPPY FOR YOU", "I LOVE THAT", "HIGH FIVE!", "YOU DESERVE IT" };
static const char *T_DAY_GOOD[] = { "good", "great", "amazing", "fun", "lovely", "brilliant", "perfect" };
static const char *R_DAY_GOOD[] = { "A %s DAY! LOVE IT", "YAY! WHAT WAS BEST?", "THAT MAKES ME HAPPY TOO" };
static const char *T_HAPPY_SINGLE[] = { "good news", "I have good news", "she said yes", "he said yes", "best day ever", "it went well", "it went really well",
                                        "I feel good", "I feel great", "I made a new friend", "it is my birthday", "I love this", "we are going on holiday" };
static const char *T_PROUD[]  = { "I am proud of myself", "I did it", "I finished it", "I made it", "I got it done", "I am so proud", "I nailed it" };
static const char *R_PROUD[]  = { "LOOK AT YOU GO!", "I KNEW YOU COULD", "SO PROUD. CHEST OUT", "YOU EARNED THAT" };

// ================================================================ feelings (one face each)
static const char *T_AM_SAD[]  = { "sad", "so sad", "really sad", "upset", "lonely", "hurt", "homesick", "heartbroken", "feeling low" };
static const char *R_SAD[]     = { "OH NO. I AM HERE", "THAT SOUNDS HARD", "I AM SORRY FRIEND", "BIG HUG FROM ME", "YOU ARE NOT ALONE" };
static const char *T_FEEL_SAD[] = { "sad", "bad", "awful", "terrible", "lonely", "rubbish", "down", "low", "empty", "useless" };
static const char *R_FEEL_SAD[] = { "FEELING %s IS HARD", "I AM HERE. BIG HUG", "THAT SOUNDS HEAVY", "YOU ARE NOT ALONE" };
static const char *T_DAY_BAD[]  = { "bad", "hard", "awful", "terrible", "rough", "horrible" };
static const char *R_DAY_BAD[]  = { "A %s DAY. I AM SORRY", "ROUGH ONE. I AM HERE", "YOU MADE IT THROUGH" };
static const char *T_MISS[]     = { "mum", "dad", "gran", "friend", "home", "them", "her", "him", "family" };
static const char *R_MISS[]     = { "MISSING %s IS HARD", "I BET YOU ARE MISSED TOO", "THAT ACHE IS LOVE" };
static const char *T_SAD_SINGLE[] = { "bad news", "I have bad news", "nobody came", "we broke up", "I messed up", "I got dumped", "it went badly", "I am all alone", "I feel left out" };

static const char *T_ANGRY[]  = { "angry", "so angry", "furious", "annoyed", "so annoyed", "mad", "fed up", "cross", "frustrated" };
static const char *R_ANGRY[]  = { "GRR! WHO DID WHAT?", "THAT IS NOT FAIR", "I AM ON YOUR SIDE", "THAT SOUNDS ROUGH. I AM HERE" };
static const char *T_ANGRY_SINGLE[] = { "they shouted at me", "I had a fight", "I got in trouble", "that is not fair", "I hate this", "I have had enough" };

static const char *T_SCARED[] = { "scared", "so scared", "afraid", "terrified", "frightened" };
static const char *T_SCARED_OF[] = { "the dark", "spiders", "the dentist", "tomorrow", "being alone" };
static const char *R_SCARED[] = { "I AM RIGHT HERE WITH YOU", "YOU ARE NOT ALONE IN THIS", "SCARY. I AM NOT GOING ANYWHERE" };
static const char *R_SCARED_OF[] = { "%s. SCARY. I AM HERE", "%s. I AM RIGHT HERE WITH YOU", "THAT IS A BIG FEAR. BIG HUG" };

static const char *T_GRUMPY[] = { "grumpy", "in a bad mood", "moody", "cranky", "irritable", "not in the mood" };
static const char *R_GRUMPY[] = { "GRUMPY DAY. SAME", "WANT TO GRUMBLE TOGETHER?", "I WILL JUST SIT HERE WITH YOU" };
static const char *T_GRUMPY_SINGLE[] = { "leave me alone", "ugh what a day", "everything is annoying", "I do not want to talk" };

static const char *T_CURIOUS[] = { "guess what", "what do you think", "tell me something", "I have a question", "what is new", "I found something", "I learned something" };
static const char *R_CURIOUS[] = { "OOH, WHAT?", "TELL ME EVERYTHING", "I AM CURIOUS NOW", "GO ON, GO ON" };

static const char *T_CONFIDENT[] = { "I can do this", "I got this", "I am ready", "wish me luck", "I am confident", "today is the day", "watch me" };
static const char *R_CONFIDENT[] = { "YOU HAVE GOT THIS", "GO GET IT!", "I BELIEVE IN YOU", "SHOW THEM" };

static const char *T_PEACEFUL[] = { "calm", "relaxed", "at peace", "chilled", "okay", "fine" };
static const char *R_PEACEFUL[] = { "LOVELY. ENJOY THE QUIET", "PEACEFUL SUITS YOU", "I AM CALM TOO" };
static const char *T_PEACEFUL_SINGLE[] = { "it is quiet today", "I had a rest", "I feel better", "nothing much happened", "just a normal day" };

static const char *T_MISCHIEF[] = { "I have a secret", "guess what I did", "I did something naughty", "I did something silly", "do not tell anyone", "I was cheeky" };
static const char *R_MISCHIEF[] = { "OOH. SPILL IT", "I WILL NEVER TELL", "HEHE. GO ON", "MY BEAK IS SEALED" };

static const char *T_THINK_ABOUT[] = { "mum", "dad", "work", "school", "money", "the future", "my job", "life", "tomorrow", "moving", "love" };
static const char *R_THINK_ABOUT[] = { "HMM, %s. GO ON", "BIG THOUGHTS ABOUT %s", "TELL ME MORE", "I AM THINKING TOO" };
static const char *T_SHOULD_I[]    = { "go", "stay", "quit", "move", "call them", "say yes", "give up" };
static const char *R_SHOULD_I[]    = { "BIG CHOICE. I AM LISTENING", "HMM. THAT IS A BIG ONE", "I AM WITH YOU EITHER WAY" };
static const char *T_THINK_SINGLE[] = { "I am thinking", "just thinking", "thinking out loud", "let me think", "I wonder", "I have been thinking", "I have an idea", "I need a plan", "what a day" };
static const char *R_THINK[]        = { "HMM, TELL ME MORE", "GO ON, I AM HERE", "INTERESTING...", "TAKE YOUR TIME", "WHAT ELSE?" };
static const char *T_CONFUSED[]     = { "I am not sure", "what should I do", "I do not know", "I do not know what to do", "I cannot decide", "I am confused", "I do not get it" };
static const char *R_CONFUSED[]     = { "HMM. LET US THINK", "CONFUSING. I AM HERE", "NOT SURE IS OKAY" };

static const char *T_RELIEF[] = { "phew it is over", "it is over", "I am relieved", "that went okay", "it is done", "that was close", "I made it through" };
static const char *R_RELIEF[] = { "PHEW! BIG BREATH OUT", "YOU DID IT. PHEW", "THAT WAS A LOT. WELL DONE" };

static const char *T_SHOCKED[] = { "you will not believe this", "oh my god", "no way", "I cannot believe it", "something crazy happened", "big news" };
static const char *R_SHOCKED[] = { "WHAT?! TELL ME", "NO WAY. GO ON", "I AM ON THE EDGE OF MY SEAT" };

static const char *T_DISGUST[] = { "that is gross", "that is disgusting", "yuck gross", "that was nasty" };
static const char *T_HATE[]    = { "mondays", "homework", "traffic", "my job", "the rain" };
static const char *R_DISGUST[] = { "EWW. AGREED", "YUCK. I FELT THAT", "GROSS. TELL ME ANYWAY" };
static const char *R_HATE[]    = { "%s. THE WORST", "UGH, %s. SAME", "%s. I GET IT" };

static const char *T_NERVOUS[] = { "nervous", "anxious", "worried", "stressed", "so nervous", "panicking", "on edge" };
static const char *R_NERVOUS[] = { "I AM HERE. YOU ARE NOT ALONE", "YOU HAVE DONE HARD THINGS BEFORE", "THAT IS A LOT TO CARRY" };
static const char *T_NERVOUS_SINGLE[] = { "I have an exam tomorrow", "I have an interview tomorrow", "big day tomorrow", "I cannot stop worrying", "what if it goes wrong" };

static const char *T_TIRED[] = { "tired", "so tired", "exhausted", "sleepy", "worn out", "shattered", "drained" };
static const char *R_TIRED[] = { "I WILL KEEP YOU COMPANY", "YOU DID ENOUGH TODAY", "TIRED IS ALLOWED" };
static const char *T_TIRED_SINGLE[] = { "I need sleep", "long day", "I cannot sleep", "I need a nap", "I am going to rest" };

static const char *T_SHY[] = { "I like someone", "I have a crush", "I am embarrassed", "I am shy", "I said something silly", "I am blushing", "someone likes me" };
static const char *R_SHY[] = { "OOH. TELL ME MORE", "THAT IS SWEET", "WE ALL BLUSH SOMETIMES", "CUTE. WHO?" };

// ================================================================ the families (order = family number)
//  mood,            prefix,                 topics,                    endings,          replies,                  ends_chat
static const Family FAMILIES[] = {
  { MOOD_LISTENING,  "",                     T_WAKE, N(T_WAKE),         E_NONE, 1,        R_LISTEN, N(R_LISTEN), false },  // family 0 = wake, must stay first
  { MOOD_HAPPY,      "",                     T_HOWRU, N(T_HOWRU),       E_NONE, 1,        R_HOWRU, N(R_HOWRU), false },
  { MOOD_PROUD,      "",                     T_NAME, N(T_NAME),         E_NONE, 1,        R_NAME, N(R_NAME), false },
  { MOOD_SHY,        "",                     T_LIKEME, N(T_LIKEME),     E_NONE, 1,        R_LIKEME, N(R_LIKEME), false },
  { MOOD_SHY,        "",                     T_LOVEYOU, N(T_LOVEYOU),   E_NONE, 1,        R_LOVEYOU, N(R_LOVEYOU), false },
  { MOOD_CURIOUS,    "",                     T_DOING, N(T_DOING),       E_NONE, 1,        R_DOING, N(R_DOING), false },
  { MOOD_TIRED,      "",                     T_SLEEPYQ, N(T_SLEEPYQ),   E_NONE, 1,        R_SLEEPYQ, N(R_SLEEPYQ), false },
  { MOOD_MISCHIEVOUS,"",                     T_JOKE, N(T_JOKE),         E_NONE, 1,        R_JOKE, N(R_JOKE), false },
  { MOOD_GRATEFUL,   "",                     T_THANKS, N(T_THANKS),     E_NONE, 1,        R_THANKS, N(R_THANKS), true },
  { MOOD_HAPPY,      "",                     T_MORNING, N(T_MORNING),   E_NONE, 1,        R_MORNING, N(R_MORNING), false },
  { MOOD_SLEEP,      "",                     T_NIGHT, N(T_NIGHT),       E_NONE, 1,        R_NIGHT, N(R_NIGHT), true },
  { MOOD_HAPPY,      "",                     T_DAY_ANS_GOOD, N(T_DAY_ANS_GOOD), E_NONE, 1, R_DAY_ANS_GOOD, N(R_DAY_ANS_GOOD), false },
  { MOOD_SAD,        "",                     T_DAY_ANS_BAD, N(T_DAY_ANS_BAD),   E_NONE, 1, R_DAY_ANS_BAD, N(R_DAY_ANS_BAD), false },
  { MOOD_PEACEFUL,   "",                     T_DAY_ANS_MEH, N(T_DAY_ANS_MEH),   E_NONE, 1, R_DAY_ANS_MEH, N(R_DAY_ANS_MEH), false },
  { MOOD_SAD,        "my ",                  T_LOVED, N(T_LOVED),       E_LOSS, N(E_LOSS), R_LOSS, N(R_LOSS), false },
  { MOOD_SAD,        "I lost my ",           T_THINGS, N(T_THINGS),     E_NONE, 1,        R_LOSTIT, N(R_LOSTIT), false },
  { MOOD_SAD,        "I broke my ",          T_BROKE, N(T_BROKE),       E_NONE, 1,        R_BROKE, N(R_BROKE), false },
  { MOOD_SAD,        "I failed my ",         T_TESTS, N(T_TESTS),       E_NONE, 1,        R_FAILED, N(R_FAILED), false },
  { MOOD_PROUD,      "I passed my ",         T_TESTS, N(T_TESTS),       E_NONE, 1,        R_PASSED, N(R_PASSED), false },
  { MOOD_HAPPY,      "I got ",               T_GOT, N(T_GOT),           E_NONE, 1,        R_GOT, N(R_GOT), false },
  { MOOD_PROUD,      "I won ",               T_WON, N(T_WON),           E_NONE, 1,        R_WON, N(R_WON), false },
  { MOOD_HAPPY,      "I am ",                T_AM_HAPPY, N(T_AM_HAPPY), E_NONE, 1,        R_HAPPY, N(R_HAPPY), false },
  { MOOD_HAPPY,      "today was ",           T_DAY_GOOD, N(T_DAY_GOOD), E_NONE, 1,        R_DAY_GOOD, N(R_DAY_GOOD), false },
  { MOOD_HAPPY,      "",                     T_HAPPY_SINGLE, N(T_HAPPY_SINGLE), E_NONE, 1, R_HAPPY, N(R_HAPPY), false },
  { MOOD_PROUD,      "",                     T_PROUD, N(T_PROUD),       E_NONE, 1,        R_PROUD, N(R_PROUD), false },
  { MOOD_SAD,        "I am ",                T_AM_SAD, N(T_AM_SAD),     E_NONE, 1,        R_SAD, N(R_SAD), false },
  { MOOD_SAD,        "I feel ",              T_FEEL_SAD, N(T_FEEL_SAD), E_NONE, 1,        R_FEEL_SAD, N(R_FEEL_SAD), false },
  { MOOD_SAD,        "today was ",           T_DAY_BAD, N(T_DAY_BAD),   E_NONE, 1,        R_DAY_BAD, N(R_DAY_BAD), false },
  { MOOD_SAD,        "I miss ",              T_MISS, N(T_MISS),         E_NONE, 1,        R_MISS, N(R_MISS), false },
  { MOOD_SAD,        "",                     T_SAD_SINGLE, N(T_SAD_SINGLE), E_NONE, 1,    R_SAD, N(R_SAD), false },
  { MOOD_ANGRY,      "I am ",                T_ANGRY, N(T_ANGRY),       E_NONE, 1,        R_ANGRY, N(R_ANGRY), false },
  { MOOD_ANGRY,      "",                     T_ANGRY_SINGLE, N(T_ANGRY_SINGLE), E_NONE, 1, R_ANGRY, N(R_ANGRY), false },
  { MOOD_SCARED,     "I am ",                T_SCARED, N(T_SCARED),     E_NONE, 1,        R_SCARED, N(R_SCARED), false },
  { MOOD_SCARED,     "I am scared of ",      T_SCARED_OF, N(T_SCARED_OF), E_NONE, 1,      R_SCARED_OF, N(R_SCARED_OF), false },
  { MOOD_GRUMPY,     "I am ",                T_GRUMPY, N(T_GRUMPY),     E_NONE, 1,        R_GRUMPY, N(R_GRUMPY), false },
  { MOOD_GRUMPY,     "",                     T_GRUMPY_SINGLE, N(T_GRUMPY_SINGLE), E_NONE, 1, R_GRUMPY, N(R_GRUMPY), false },
  { MOOD_CURIOUS,    "",                     T_CURIOUS, N(T_CURIOUS),   E_NONE, 1,        R_CURIOUS, N(R_CURIOUS), false },
  { MOOD_CONFIDENT,  "",                     T_CONFIDENT, N(T_CONFIDENT), E_NONE, 1,      R_CONFIDENT, N(R_CONFIDENT), false },
  { MOOD_PEACEFUL,   "I am ",                T_PEACEFUL, N(T_PEACEFUL), E_NONE, 1,        R_PEACEFUL, N(R_PEACEFUL), false },
  { MOOD_PEACEFUL,   "",                     T_PEACEFUL_SINGLE, N(T_PEACEFUL_SINGLE), E_NONE, 1, R_PEACEFUL, N(R_PEACEFUL), false },
  { MOOD_MISCHIEVOUS,"",                     T_MISCHIEF, N(T_MISCHIEF), E_NONE, 1,        R_MISCHIEF, N(R_MISCHIEF), false },
  { MOOD_THOUGHTFUL, "I am thinking about ", T_THINK_ABOUT, N(T_THINK_ABOUT), E_NONE, 1,  R_THINK_ABOUT, N(R_THINK_ABOUT), false },
  { MOOD_THOUGHTFUL, "should I ",            T_SHOULD_I, N(T_SHOULD_I), E_NONE, 1,        R_SHOULD_I, N(R_SHOULD_I), false },
  { MOOD_THOUGHTFUL, "",                     T_THINK_SINGLE, N(T_THINK_SINGLE), E_NONE, 1, R_THINK, N(R_THINK), false },
  { MOOD_THINKING,   "",                     T_CONFUSED, N(T_CONFUSED), E_NONE, 1,        R_CONFUSED, N(R_CONFUSED), false },
  { MOOD_RELIEF,     "",                     T_RELIEF, N(T_RELIEF),     E_NONE, 1,        R_RELIEF, N(R_RELIEF), false },
  { MOOD_SHOCKED,    "",                     T_SHOCKED, N(T_SHOCKED),   E_NONE, 1,        R_SHOCKED, N(R_SHOCKED), false },
  { MOOD_DISGUSTED,  "",                     T_DISGUST, N(T_DISGUST),   E_NONE, 1,        R_DISGUST, N(R_DISGUST), false },
  { MOOD_DISGUSTED,  "I hate ",              T_HATE, N(T_HATE),         E_NONE, 1,        R_HATE, N(R_HATE), false },
  { MOOD_NERVOUS,    "I am ",                T_NERVOUS, N(T_NERVOUS),   E_NONE, 1,        R_NERVOUS, N(R_NERVOUS), false },
  { MOOD_NERVOUS,    "",                     T_NERVOUS_SINGLE, N(T_NERVOUS_SINGLE), E_NONE, 1, R_NERVOUS, N(R_NERVOUS), false },
  { MOOD_TIRED,      "I am ",                T_TIRED, N(T_TIRED),       E_NONE, 1,        R_TIRED, N(R_TIRED), false },
  { MOOD_TIRED,      "",                     T_TIRED_SINGLE, N(T_TIRED_SINGLE), E_NONE, 1, R_TIRED, N(R_TIRED), false },
  { MOOD_SHY,        "",                     T_SHY, N(T_SHY),           E_NONE, 1,        R_SHY, N(R_SHY), false },
  { MOOD_PEACEFUL,   "",                     T_STOP, N(T_STOP),         E_NONE, 1,        R_STOP, N(R_STOP), true },
};
#define FAMILY_COUNT N(FAMILIES)
// find a family by its topic list, e.g. family_of(T_HOWRU)
static int family_of(const char **topics) { for (int i = 0; i < (int)FAMILY_COUNT; i++) if (FAMILIES[i].topics == topics) return i; return -1; }
#define FAMILY_WAKE 0

// command id = family * 64 + topic   (so one id tells us both the family and the word heard)
#define CMD_ID(family, topic) ((family) * 64 + (topic))
#define CMD_FAMILY(id) ((id) / 64)
#define CMD_TOPIC(id)  ((id) % 64)

// ================================================================ what Gawa says when it is unsure
static const char *R_DIDNT_HEAR[] = { "MY EARS! ONE MORE TIME?", "I MISSED THAT. AGAIN?", "HMM? SAY IT AGAIN FOR ME" };
static const char *R_GIVE_UP[]    = { "I DID NOT GET IT, BUT I AM HERE", "STILL HERE. BIG HUG", "WE CAN TRY AGAIN LATER" };
static const char *R_NOTHING[]    = { "I AM HERE WHEN YOU NEED ME", "OKAY. I AM AROUND" };
static const char *LINE_IDLE = "SAY HEY GAWA";

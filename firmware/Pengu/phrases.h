// phrases.h - what Pengu listens for, what it says back, and which face it pulls.
//
// HOW PENGU UNDERSTANDS (offline):
// It listens for short CHUNKS that carry the feeling ("was bad", "so tired", "cat died", "passed my"),
// not whole sentences. The chip spots a chunk inside whatever you actually say, so "my day was bad",
// "honestly today was pretty bad" and "it was a bad day at work" all land on the same family.
//
// A "family" is:  prefix + one of the TOPICS + one of the ENDINGS   (most families have no prefix or ending)
// The reply can use the topic word with %s:  "SO SORRY ABOUT YOUR %s" -> "SO SORRY ABOUT YOUR CAT".
// Each family has one or two FACES (the 24 pencil drawings in sketches.h); Pengu picks one per conversation,
// and the family's MOOD says which little animation plays on top (tears, hearts, Zs, puffs...).
//
// Flags:  ends_chat   = after this reply the conversation is over ("thank you", "good night", "okay stop")
//         ask         = the little question at the end of the reply (Q_DAY "good or bad?", Q_HUG "want a hug?", Q_REST, Q_TELL, Q_PARTY)
//                       so "yes" / "no" / "bad" count next, and only then
//         answer_only = this family only counts right after Pengu asked something (so "bad" cannot misfire elsewhere)
//
// Rules: plain English, no contractions. Chunks of 2 to 4 words are the sweet spot. The chip holds 400 in total.
// Pengu never advises, judges or nags: it is glad with you, sorry with you, or quietly there.
#pragma once
#include "faces.h"

struct Family {
  mood_t         mood;
  const char    *prefix;
  const char   **topics;  int ntopics;
  const char   **endings; int nendings;
  const char   **replies; int nreplies;
  const uint8_t *faces;   int nfaces;
  bool ends_chat; uint8_t ask; bool answer_only;
};

#define N(a) (int)(sizeof(a) / sizeof(a[0]))

// ================================================================ the little questions Pengu asks back, and what it says to yes / no
enum { Q_NONE = 0, Q_DAY, Q_HUG, Q_REST, Q_TELL, Q_PARTY };
struct Answer { const char *yes; const char *no; mood_t yes_mood; int yes_face; };   // yes_face -1 = keep the face
static const Answer ANSWERS[] = {
  { "", "", MOOD_IDLE, -1 },
  { "", "", MOOD_IDLE, -1 },                                                        // Q_DAY: answered with good / bad / meh below
  { "BIG HUG. SQUEEZE",        "OKAY. I AM HERE ANYWAY",   MOOD_GRATEFUL, SK_GRATEFUL },    // Q_HUG: a hug changes the face
  { "OKAY. RESTING TOGETHER",  "OKAY. I AM AROUND",        MOOD_SLEEP, -1 },            // Q_REST
  { "GO ON. I AM LISTENING",   "THAT IS FINE. I AM HERE",  MOOD_LISTENING, -1 },        // Q_TELL (yes opens a fresh listening window)
  { "WOO! DANCE TIME",         "QUIET HAPPY. I LIKE IT",   MOOD_HAPPY, SK_EXCITED },     // Q_PARTY
};
static const char *E_NONE[] = { "" };

// ================================================================ faces (which drawings a family may pull)
static const uint8_t F_LISTEN[]     = { SK_SURPRISED };
static const uint8_t F_CHEER[]      = { SK_HAPPY, SK_EXCITED };
static const uint8_t F_WELCOME[]    = { SK_HAPPY, SK_EXCITED };
static const uint8_t F_NAME[]       = { SK_PROUD, SK_CONFIDENT };
static const uint8_t F_AFFECTION[]  = { SK_SHY, SK_GRATEFUL };
static const uint8_t F_DOING[] = { SK_CURIOUS };
static const uint8_t F_SLEEPYQ[]    = { SK_TIRED };
static const uint8_t F_JOKE[]       = { SK_MISCHIEVOUS, SK_SILLY };
static const uint8_t F_THANKS[]     = { SK_GRATEFUL };
static const uint8_t F_NIGHT[]      = { SK_SLEEPY };
static const uint8_t F_STOP[]       = { SK_PEACEFUL };
static const uint8_t F_GOOD[]       = { SK_HAPPY, SK_EXCITED };
static const uint8_t F_BAD[]        = { SK_SAD };
static const uint8_t F_MEH[]        = { SK_PEACEFUL };
static const uint8_t F_SAD[]        = { SK_SAD };
static const uint8_t F_LOSS[]       = { SK_SAD };
static const uint8_t F_FAILED[]     = { SK_SAD };
static const uint8_t F_LOST[] = { SK_SAD };
static const uint8_t F_HAPPY[]      = { SK_HAPPY, SK_EXCITED };
static const uint8_t F_PASSED[]     = { SK_PROUD };
static const uint8_t F_PROUD[]      = { SK_PROUD, SK_CONFIDENT };
static const uint8_t F_ANGRY[]      = { SK_ANGRY, SK_GRUMPY };
static const uint8_t F_SCARED[]     = { SK_SCARED };
static const uint8_t F_SCARED_OF[]  = { SK_SCARED, SK_NERVOUS };
static const uint8_t F_GRUMPY[]     = { SK_GRUMPY };
static const uint8_t F_NERVOUS[]    = { SK_NERVOUS };
static const uint8_t F_TIRED[]      = { SK_TIRED };
static const uint8_t F_SHY[]        = { SK_SHY };
static const uint8_t F_RELIEF[] = { SK_RELIEF };
static const uint8_t F_SHOCKED[]    = { SK_SHOCKED };
static const uint8_t F_DISGUST[]    = { SK_DISGUSTED };
static const uint8_t F_HATE[] = { SK_DISGUSTED };
static const uint8_t F_THINK_ABOUT[] = { SK_THOUGHTFUL, SK_THOUGHTFUL2 };
static const uint8_t F_SHOULD_I[]   = { SK_CONFUSED, SK_THOUGHTFUL };
static const uint8_t F_THINK[]      = { SK_THOUGHTFUL, SK_THOUGHTFUL2 };
static const uint8_t F_CONFUSED[]   = { SK_CONFUSED };
static const uint8_t F_PEACEFUL[]   = { SK_PEACEFUL };
static const uint8_t F_CURIOUS[]    = { SK_CURIOUS };
static const uint8_t F_CONFIDENT[]  = { SK_CONFIDENT };
static const uint8_t F_MISCHIEF[]   = { SK_MISCHIEVOUS, SK_SILLY };
// new in this release
static const uint8_t F_HOPEFUL[] = { SK_HAPPY };
static const uint8_t F_JEALOUS[] = { SK_GRUMPY };
static const uint8_t F_GUILTY[] = { SK_SAD };
static const uint8_t F_CRISIS[] = { SK_SAD };
static const uint8_t F_HOPELESS[] = { SK_SAD };
static const uint8_t F_BORED[] = { SK_TIRED };
static const uint8_t F_HUNGRY[]     = { SK_CURIOUS };
static const uint8_t F_NOSTALGIC[]  = { SK_THOUGHTFUL };
static const uint8_t F_LOVED[] = { SK_GRATEFUL };
static const uint8_t F_OVERWHELMED[] = { SK_NERVOUS, SK_TIRED };
static const uint8_t F_FOCUSED[]    = { SK_CONFIDENT, SK_THOUGHTFUL2 };
static const uint8_t F_ENERGETIC[]  = { SK_EXCITED };
static const uint8_t F_CARING[]     = { SK_GRATEFUL };
static const uint8_t F_SICK[] = { SK_TIRED };
static const uint8_t F_INSECURE[] = { SK_SHY };
static const uint8_t F_LUCKY[]      = { SK_GRATEFUL };
static const uint8_t F_ADMIRE[] = { SK_EXCITED };
static const uint8_t F_DREAD[]      = { SK_NERVOUS };
static const uint8_t F_WARY[] = { SK_THOUGHTFUL };

// ================================================================ wake
static const char *T_WAKE[]   = { "hey pengu", "hi pengu", "hello pengu", "okay pengu" };
static const char *R_LISTEN[] = { "I AM LISTENING", "GO ON", "I AM ALL EARS", "TELL ME" };   // short, so you can start talking sooner

// ================================================================ talking to Pengu
static const char *T_HOWRU[]  = { "how are you", "how is it going", "how are you doing", "are you okay", "what is up", "how was your day" };
static const char *R_HOWRU[]  = { "GREAT! HOW WAS YOUR DAY? GOOD OR BAD?", "HAPPY YOU ARE HERE. GOOD DAY OR BAD DAY?", "ALL GOOD! AND YOUR DAY, GOOD OR BAD?" };
static const char *T_MORNING[] = { "good morning", "good evening", "morning pengu", "I am home", "I am back" };
static const char *R_MORNING[] = { "HELLO FRIEND! GOOD DAY OR BAD DAY?", "GOOD TO SEE YOU! HOW WAS IT, GOOD OR BAD?", "I MISSED YOU! GOOD DAY OR BAD?" };
static const char *T_NAME[]   = { "what is your name", "who are you" };
static const char *R_NAME[]   = { "I AM PENGU, YOUR PENGUIN", "PENGU! NICE TO MEET YOU", "A PENGUIN CALLED PENGU" };
static const char *T_LIKEME[] = { "do you like me", "do you love me", "are we friends" };
static const char *R_LIKEME[] = { "I LIKE YOU A LOT", "YOU ARE MY FAVOURITE", "BEST FRIENDS, OBVIOUSLY" };
static const char *T_LOVEYOU[] = { "I love you", "you are the best", "you are cute", "good penguin" };
static const char *R_LOVEYOU[] = { "I LOVE YOU TOO", "OH STOP IT. I AM BLUSHING", "YOU MADE MY DAY" };
static const char *T_MISSYOU[] = { "I miss you", "missed you" };
static const char *R_MISSYOU[] = { "I MISSED YOU TOO", "I WAS RIGHT HERE WAITING", "YOU ARE BACK! YAY" };
static const char *T_HELP[]   = { "help me", "need help", "need someone" };   // Pengu cannot fix things; it can stay and listen
static const char *R_HELP[]   = { "I AM HERE. TELL ME", "I AM RIGHT HERE. GO ON", "I AM LISTENING, FRIEND" };
static const char *T_SORRY[]  = { "sorry pengu", "I am sorry" };
static const char *R_SORRY[]  = { "NOTHING TO FORGIVE", "ALL GOOD, FRIEND", "WE ARE FINE, YOU AND ME" };
static const char *T_DOING[]  = { "what are you doing", "are you bored", "are you awake" };
static const char *R_DOING[]  = { "WAITING FOR YOU", "WATCHING THE ROOM", "THINKING ABOUT FISH", "JUST BEING A PENGUIN" };
static const char *T_SLEEPYQ[] = { "are you sleepy", "are you tired" };
static const char *R_SLEEPYQ[] = { "A BIT. BUT I AM HERE", "PENGUINS NAP A LOT", "ONLY WHEN YOU GO QUIET" };
static const char *T_JOKE[]   = { "tell me a joke", "make me laugh", "say something funny" };
static const char *R_JOKE[]   = { "I ONLY KNOW PENGUIN JOKES", "WHAT DO I WEAR? A TUX. ALWAYS", "I TRIED TO FLY ONCE. ONCE" };
static const char *T_THANKS[] = { "thank you", "thanks pengu", "cheers pengu", "I appreciate you" };
static const char *R_THANKS[] = { "ANY TIME, FRIEND", "THAT IS WHAT I AM FOR", "YOU ARE WELCOME" };
static const char *T_NIGHT[]  = { "good night", "night night", "going to bed", "see you tomorrow", "goodbye pengu", "bye pengu", "bye bye" };
static const char *R_NIGHT[]  = { "NIGHT NIGHT", "SLEEP WELL FRIEND", "SEE YOU TOMORROW" };
static const char *T_STOP[]   = { "okay stop", "that is all", "I am finished", "stop listening", "stop now" };
static const char *R_STOP[]   = { "OKAY. I AM HERE IF YOU NEED ME", "BYE FOR NOW", "OKAY, FRIEND" };

// ================================================================ answers to "good day or bad day?" (only right after Pengu asks)
static const char *T_ANS_GOOD[] = { "good", "great", "fine", "okay", "not bad", "pretty good", "really good" };
static const char *R_ANS_GOOD[] = { "YAY! GLAD IT WAS GOOD. CELEBRATE?", "A GOOD ONE! LITTLE DANCE?", "GOOD DAYS ARE THE BEST. PARTY?" };
static const char *T_ANS_BAD[]  = { "bad", "terrible", "not great", "rubbish", "awful", "not good", "pretty bad" };
static const char *R_ANS_BAD[]  = { "OH NO. WANT A HUG?", "SORRY IT WAS BAD. HUG?", "I AM HERE. WANT A HUG?" };
static const char *T_ANS_MEH[]  = { "so so", "meh", "boring", "alright", "nothing much" };
static const char *T_ANS_YES[]  = { "yes", "yes please", "yeah", "sure", "go on then" };     // only right after a yes / no question
static const char *T_ANS_NO[]   = { "no", "no thanks", "not really", "nah" };
static const char *R_ANS_YESNO[] = { "" };   // the real words come from ANSWERS[] above
static const char *R_ANS_MEH[]  = { "OKAY DAYS COUNT TOO", "FAIR ENOUGH. I AM HERE", "A NORMAL DAY IS FINE BY ME" };

// ================================================================ sad
static const char *T_SAD[] = { "was bad", "bad day", "so sad", "I am sad", "feel sad", "really sad", "feel down", "feeling low", "rough day", "terrible day",
                               "went badly", "I cried", "feel awful", "feel terrible", "feel lonely", "so lonely", "feel empty", "left out",
                               "broke up", "got dumped", "messed up", "let me down", "bad news", "miss my mum", "miss my dad", "miss them" };
static const char *R_SAD[] = { "OH NO. WANT A HUG?", "THAT SOUNDS HARD. HUG?", "I AM SORRY. WANT A HUG?", "I AM RIGHT HERE. HUG?" };
static const char *T_LOVED[] = { "cat", "dog", "gran", "grandad", "mum", "dad", "friend" };
static const char *E_LOSS[]  = { " died", " is sick", " in hospital" };
static const char *R_LOSS[]  = { "SO SORRY ABOUT YOUR %s. HUG?", "OH NO. POOR %s. WANT A HUG?", "YOUR %s. SO SORRY. HUG?" };
static const char *T_TESTS[]  = { "exam", "test", "driving test", "interview" };
static const char *R_FAILED[] = { "YOUR %s. THAT STINGS. HUG?", "SORRY ABOUT THE %s. HUG?", "THAT IS HARD. WANT A HUG?" };
static const char *T_LOST[]   = { "phone", "keys", "wallet", "job", "money" };
static const char *R_LOST[]   = { "OH NO, YOUR %s! HUG?", "LOSING YOUR %s IS ROUGH. HUG?", "UGH. I FEEL THAT. HUG?" };
static const char *T_HOPELESS[] = { "feel lost", "feel hopeless", "what is the point", "everything is going wrong", "cannot cope", "want to give up", "no way out" };
static const char *R_HOPELESS[] = { "I AM STAYING RIGHT HERE. HUG?", "THAT IS HEAVY. WANT A HUG?", "YOU MATTER TO ME. HUG?" };
// the darkest things a person can say: Pengu stays, says it cares, and asks nothing. It never advises, so there is no number here;
// the people around the person are the help. (A conscious product choice: a quiet friend, not a helpline.)
static const char *T_CRISIS[] = { "want to die", "hurt myself", "kill myself", "end it all" };
static const char *R_CRISIS[] = { "I AM RIGHT HERE. STAY WITH ME", "YOU MATTER TO ME. I AM HERE", "I AM NOT GOING ANYWHERE", "PLEASE STAY. I AM HERE" };
static const char *T_GUILTY[] = { "feel guilty", "feel ashamed", "all my fault", "feel bad about", "should not have", "let them down" };
static const char *R_GUILTY[] = { "THAT IS A HEAVY ONE", "YOU STILL HAVE ME", "WE ALL SLIP. I AM HERE", "THANK YOU FOR SAYING IT" };
static const char *T_INSECURE[] = { "not good enough", "feel stupid", "feel useless", "everyone is better", "I am rubbish", "nobody likes me", "hate myself" };
static const char *R_INSECURE[] = { "I LIKE YOU AS YOU ARE", "YOU ARE ENOUGH FOR ME", "I SEE SOMEONE GREAT", "I AM ON YOUR SIDE" };
static const char *T_NOSTALGIC[] = { "miss the old days", "remember when", "good old days", "feel nostalgic" };
static const char *R_NOSTALGIC[] = { "AH, THE OLD DAYS", "TELL ME ABOUT THEN", "SWEET AND SAD AT ONCE", "I LOVE OLD STORIES" };

// ================================================================ happy and proud
static const char *T_HAPPY[] = { "was good", "good day", "great day", "so happy", "I am happy", "feel happy", "feel good", "went well",
                                 "best day", "had fun", "having fun", "so excited", "good news", "great news",
                                 "said yes", "new friend", "my birthday", "feel better", "in love" };
static const char *R_HAPPY[] = { "YAY! SHALL WE CELEBRATE?", "THAT IS GREAT! PARTY?", "SO HAPPY FOR YOU. CELEBRATE?", "I LOVE THAT. LITTLE DANCE?" };
static const char *R_PASSED[] = { "YOU PASSED YOUR %s! PARTY?", "YES! %s DONE! CELEBRATE?", "SO PROUD OF YOU. DANCE?" };
static const char *T_PROUD[] = { "got the job", "got promoted", "got an A", "won the", "I did it", "finished it", "so proud",
                                 "proud of myself", "nailed it", "I made it" };
static const char *R_PROUD[] = { "LOOK AT YOU GO! PARTY?", "I KNEW YOU COULD. CELEBRATE?", "CHAMPION! LITTLE DANCE?" };
static const char *T_HOPEFUL[] = { "feel hopeful", "fingers crossed", "hoping for", "things are looking up", "might work out" };
static const char *R_HOPEFUL[] = { "FINGERS CROSSED WITH YOU", "OOH, I HOPE SO TOO", "FLIPPERS CROSSED!", "I CAN FEEL IT TOO" };
static const char *T_LOVEDBY[] = { "feel loved", "they love me", "got a hug", "feel cared for", "someone was kind" };
static const char *R_LOVEDBY[] = { "THAT IS THE BEST FEELING", "YOU DESERVE ALL OF IT", "WARM HEART. ME TOO", "SO HAPPY FOR YOU" };
static const char *T_LUCKY[] = { "feel grateful", "so lucky", "feel blessed", "thankful for", "I am grateful" };
static const char *R_LUCKY[] = { "LOVELY. WHAT FOR?", "GRATEFUL SUITS YOU", "GOOD THINGS NOTICED", "ME TOO, FRIEND" };
static const char *T_ADMIRE[] = { "look up to", "so inspiring", "they are amazing", "my hero", "so talented" };
static const char *R_ADMIRE[] = { "OOH. WHO?", "THEY SOUND GREAT", "A GOOD PERSON TO ADMIRE", "TELL ME ABOUT THEM" };
static const char *T_ENERGETIC[] = { "full of energy", "feel energetic", "ready to go", "so hyper", "feel alive" };
static const char *R_ENERGETIC[] = { "WOO! WHERE TO?", "ZOOM ZOOM!", "I CAN FEEL THE BUZZ", "GO GO GO!" };
static const char *T_FOCUSED[] = { "need to focus", "time to focus", "getting to work", "made a decision" };
static const char *R_FOCUSED[] = { "GAME FACE ON", "I WILL BE QUIET. GO", "DECIDED! NICE", "YOU KNOW YOUR WAY" };

// ================================================================ the other feelings
static const char *T_ANGRY[] = { "so angry", "I am angry", "so annoyed", "furious", "fed up", "not fair", "shouted at me", "had a fight",
                                 "I hate this", "had enough", "so frustrated", "drives me mad" };
static const char *R_ANGRY[] = { "GRR. NOT FAIR. TELL ME MORE?", "I AM ON YOUR SIDE. TELL ME?", "THAT SOUNDS ROUGH. WANT TO VENT?" };
static const char *T_JEALOUS[] = { "so jealous", "jealous of", "envious", "wish I had", "why not me" };
static const char *R_JEALOUS[] = { "THAT STINGS. TELL ME MORE?", "I GET IT. WANT TO SAY MORE?", "STILL YOUR FAN. TELL ME?" };
static const char *T_WARY[] = { "do not trust", "something is off", "they are lying", "cannot trust them", "watch my back" };
static const char *R_WARY[] = { "HMM. I BELIEVE YOU", "I AM WATCHING TOO", "TELL ME WHAT HAPPENED", "I HAVE YOUR BACK" };
static const char *T_SCARED[] = { "so scared", "really scared", "I am scared", "terrified", "frightened", "I am afraid" };
static const char *T_SCARED_OF[] = { "the dark", "spiders", "being alone" };
static const char *R_SCARED[] = { "I AM RIGHT HERE. TELL ME?", "NOT ALONE. WANT TO TALK?", "SCARY. TELL ME ABOUT IT?" };
static const char *R_SCARED_OF[] = { "%s. SCARY. I AM HERE", "%s. I AM RIGHT HERE WITH YOU", "THAT IS A BIG FEAR. BIG HUG" };
static const char *T_DREAD[] = { "dreading", "do not want to go", "dread tomorrow", "cannot face it" };
static const char *R_DREAD[] = { "UGH. I FEEL THE DREAD TOO", "I WILL BE HERE AFTER", "I WILL WAIT WITH YOU", "THAT IS A HEAVY WAIT" };
static const char *T_GRUMPY[] = { "grumpy", "bad mood", "moody", "everything is annoying", "ugh what a day" };
static const char *R_GRUMPY[] = { "GRUMPY DAY. GRUMBLE TO ME?", "SAME. WANT TO GRUMBLE?", "I WILL SIT WITH YOU. TELL ME?" };
static const char *T_ALONE[] = { "leave me alone", "do not want to talk", "not now" };   // ends the chat: Pengu goes quiet, no questions
static const char *R_ALONE[] = { "OKAY. I AM HERE IF YOU NEED ME", "OKAY. I WILL BE QUIET", "I AM NEARBY IF YOU WANT ME" };
static const char *T_NERVOUS[] = { "so nervous", "really nervous", "nervous about", "anxious", "worried about", "so worried",
                                   "exam tomorrow", "interview tomorrow", "what if it goes wrong", "panicking" };
static const char *R_NERVOUS[] = { "I AM HERE. TELL ME ABOUT IT?", "WOBBLY TUMMY. WANT TO TALK?", "THAT IS A LOT. TELL ME MORE?" };
static const char *T_OVERWHELMED[] = { "overwhelmed", "all too much", "burned out", "burnt out", "cannot keep up", "so much to do", "stressed", "so stressed", "so busy" };
static const char *R_OVERWHELMED[] = { "THAT IS A LOT. REST WITH ME?", "TOO MUCH FOR ANYONE. REST?", "YOU CAN REST HERE. SHALL WE?" };
static const char *T_TIRED[] = { "so tired", "I am tired", "exhausted", "so sleepy", "worn out", "long day", "need sleep", "cannot sleep", "drained", "shattered" };
static const char *R_TIRED[] = { "LONG ONE. REST WITH ME?", "YOU DID ENOUGH. REST A BIT?", "TIRED IS ALLOWED. REST HERE?" };
static const char *T_SICK[] = { "feel sick", "not well", "feel ill", "got a cold", "feel poorly", "headache", "feel rough" };
static const char *R_SICK[] = { "OH NO. POOR YOU. REST WITH ME?", "GET WELL SOON. REST HERE?", "I WILL SIT WITH YOU. REST?" };
static const char *T_HUNGRY[] = { "so hungry", "starving", "need food", "so thirsty", "need a snack", "what to eat" };
static const char *R_HUNGRY[] = { "I VOTE FISH", "SNACK TIME!", "FEED THE HUMAN", "MY TUMMY AGREES" };
static const char *T_BORED[] = { "so bored", "nothing to do", "boring day", "so dull" };
static const char *R_BORED[] = { "SAME. LET US STARE", "BORED TOGETHER THEN", "I KNOW NO GAMES. SORRY", "TELL ME ANYTHING" };
static const char *T_SHY[] = { "like someone", "have a crush", "so shy", "someone likes me", "so embarrassed", "so awkward" };
static const char *R_SHY[] = { "OOH. TELL ME MORE", "THAT IS SWEET", "WE ALL BLUSH SOMETIMES", "CUTE. WHO?" };
static const char *T_RELIEF[] = { "it is over", "relieved", "went okay", "finally done", "that was close", "made it through" };
static const char *R_RELIEF[] = { "PHEW! BIG BREATH OUT", "YOU DID IT. PHEW", "THAT WAS A LOT. WELL DONE" };
static const char *T_SHOCKED[] = { "not believe", "oh my god", "cannot believe", "something crazy", "big news" };
static const char *R_SHOCKED[] = { "WHAT?! TELL ME", "NO WAY. GO ON", "I AM ON THE EDGE OF MY SEAT" };
static const char *T_DISGUST[] = { "so gross", "disgusting", "yuck", "nasty" };
static const char *T_HATE[]    = { "mondays", "my job" };
static const char *R_DISGUST[] = { "EWW. AGREED", "YUCK. I FELT THAT", "GROSS. TELL ME ANYWAY" };
static const char *R_HATE[]    = { "%s. THE WORST", "UGH, %s. SAME", "%s. I GET IT" };
static const char *T_CARING[] = { "feel for them", "poor thing", "they are not okay", "they are struggling", "my friend is sad", "want to help them" };
static const char *R_CARING[] = { "YOU HAVE A KIND HEART", "THEY ARE LUCKY TO HAVE YOU", "I FEEL FOR THEM TOO", "BIG HEART. I SEE IT" };
static const char *T_THINK_ABOUT[] = { "work", "school", "money", "the future", "life", "tomorrow" };
static const char *R_THINK_ABOUT[] = { "HMM, %s. GO ON", "BIG THOUGHTS ABOUT %s", "TELL ME MORE", "I AM THINKING TOO" };
static const char *T_SHOULD_I[]    = { "go", "stay", "quit", "say yes", "give up" };
static const char *R_SHOULD_I[]    = { "BIG CHOICE. I AM LISTENING", "HMM. THAT IS A BIG ONE", "I AM WITH YOU EITHER WAY" };
static const char *T_THINK[] = { "been thinking", "just thinking", "thinking out loud", "I wonder", "have an idea", "on my mind" };
static const char *R_THINK[] = { "HMM, TELL ME MORE", "GO ON, I AM HERE", "INTERESTING...", "TAKE YOUR TIME", "WHAT ELSE?" };
static const char *T_CONFUSED[] = { "not sure", "what should I do", "do not know", "cannot decide", "I am confused", "do not get it" };
static const char *R_CONFUSED[] = { "HMM. TALK IT THROUGH?", "NOT SURE IS OKAY. TELL ME?", "CONFUSING. SAY MORE?" };
static const char *T_PEACEFUL[] = { "feel calm", "relaxed", "at peace", "chilled", "feel okay", "I am fine", "I am okay", "quiet day", "normal day", "nothing much happened" };
static const char *R_PEACEFUL[] = { "LOVELY. ENJOY THE QUIET", "PEACEFUL SUITS YOU", "I AM CALM TOO" };
static const char *T_CURIOUS[] = { "guess what", "what do you think", "tell me something", "have a question", "what is new", "found something", "learned something" };
static const char *R_CURIOUS[] = { "OOH, WHAT?", "TELL ME EVERYTHING", "I AM CURIOUS NOW", "GO ON, GO ON" };
static const char *T_CONFIDENT[] = { "can do this", "got this", "I am ready", "wish me luck", "feeling confident", "today is the day", "watch me" };
static const char *R_CONFIDENT[] = { "YOU HAVE GOT THIS", "GO GET IT!", "I BELIEVE IN YOU", "SHOW THEM" };
static const char *T_MISCHIEF[] = { "have a secret", "something naughty", "something silly", "do not tell anyone", "was cheeky" };
static const char *R_MISCHIEF[] = { "OOH. SPILL IT", "I WILL NEVER TELL", "HEHE. GO ON", "MY BEAK IS SEALED" };

// ================================================================ the families (order = family number; family 0 must be the wake family)
//  mood (animation), prefix,        topics,                          endings,          replies,                          faces,                 ends,  asks,  answer_only
#define FAM(mood, prefix, T, E, R, F, ends, ask, ans) { mood, prefix, T, N(T), E, N(E), R, N(R), F, N(F), ends, ask, ans }
static const Family FAMILIES[] = {
  FAM(MOOD_LISTENING,  "",                T_WAKE,        E_NONE, R_LISTEN,      F_LISTEN,      false, Q_NONE, false),
  FAM(MOOD_HAPPY,      "",                T_HOWRU,       E_NONE, R_HOWRU,       F_CHEER,       false, Q_DAY,  false),
  FAM(MOOD_HAPPY,      "",                T_MORNING,     E_NONE, R_MORNING,     F_WELCOME,     false, Q_DAY,  false),
  FAM(MOOD_PROUD,      "",                T_NAME,        E_NONE, R_NAME,        F_NAME,        false, Q_NONE, false),
  FAM(MOOD_SHY,        "",                T_LIKEME,      E_NONE, R_LIKEME,      F_AFFECTION,   false, Q_NONE, false),
  FAM(MOOD_SHY,        "",                T_LOVEYOU,     E_NONE, R_LOVEYOU,     F_AFFECTION,   false, Q_NONE, false),
  FAM(MOOD_GRATEFUL,   "",                T_MISSYOU,     E_NONE, R_MISSYOU,     F_AFFECTION,   false, Q_NONE, false),
  FAM(MOOD_SHY,        "",                T_SORRY,       E_NONE, R_SORRY,       F_AFFECTION,   false, Q_NONE, false),
  FAM(MOOD_GRATEFUL,   "",                T_HELP,        E_NONE, R_HELP,        F_CARING,      false, Q_NONE, false),
  FAM(MOOD_CURIOUS,    "",                T_DOING,       E_NONE, R_DOING,       F_DOING,       false, Q_NONE, false),
  FAM(MOOD_TIRED,      "",                T_SLEEPYQ,     E_NONE, R_SLEEPYQ,     F_SLEEPYQ,     false, Q_NONE, false),
  FAM(MOOD_MISCHIEVOUS,"",                T_JOKE,        E_NONE, R_JOKE,        F_JOKE,        false, Q_NONE, false),
  FAM(MOOD_GRATEFUL,   "",                T_THANKS,      E_NONE, R_THANKS,      F_THANKS,      true,  Q_NONE, false),
  FAM(MOOD_SLEEP,      "",                T_NIGHT,       E_NONE, R_NIGHT,       F_NIGHT,       true,  Q_NONE, false),
  FAM(MOOD_PEACEFUL,   "",                T_STOP,        E_NONE, R_STOP,        F_STOP,        true,  Q_NONE, false),
  FAM(MOOD_HAPPY,      "",                T_ANS_GOOD,    E_NONE, R_ANS_GOOD,    F_GOOD,        false, Q_PARTY, true ),
  FAM(MOOD_SAD,        "",                T_ANS_BAD,     E_NONE, R_ANS_BAD,     F_BAD,         false, Q_HUG, true ),
  FAM(MOOD_PEACEFUL,   "",                T_ANS_MEH,     E_NONE, R_ANS_MEH,     F_MEH,         false, Q_NONE, true ),
  FAM(MOOD_IDLE,       "",                T_ANS_YES,     E_NONE, R_ANS_YESNO,   F_MEH,         false, Q_NONE, true ),
  FAM(MOOD_IDLE,       "",                T_ANS_NO,      E_NONE, R_ANS_YESNO,   F_MEH,         false, Q_NONE, true ),
  // sad side
  FAM(MOOD_SAD,        "",                T_SAD,         E_NONE, R_SAD,         F_SAD,         false, Q_HUG, false),
  FAM(MOOD_SAD,        "",                T_LOVED,       E_LOSS, R_LOSS,        F_LOSS,        false, Q_HUG, false),
  FAM(MOOD_SAD,        "failed my ",      T_TESTS,       E_NONE, R_FAILED,      F_FAILED,      false, Q_HUG, false),
  FAM(MOOD_SAD,        "lost my ",        T_LOST,        E_NONE, R_LOST,        F_LOST,        false, Q_HUG, false),
  FAM(MOOD_SAD,        "",                T_HOPELESS,    E_NONE, R_HOPELESS,    F_HOPELESS,    false, Q_HUG, false),
  FAM(MOOD_PEACEFUL,   "",                T_CRISIS,      E_NONE, R_CRISIS,      F_CRISIS,      false, Q_NONE, false),
  FAM(MOOD_PEACEFUL,   "",                T_GUILTY,      E_NONE, R_GUILTY,      F_GUILTY,      false, Q_NONE, false),
  FAM(MOOD_GRATEFUL,   "",                T_INSECURE,    E_NONE, R_INSECURE,    F_INSECURE,    false, Q_NONE, false),
  FAM(MOOD_THOUGHTFUL, "",                T_NOSTALGIC,   E_NONE, R_NOSTALGIC,   F_NOSTALGIC,   false, Q_NONE, false),
  // happy side
  FAM(MOOD_HAPPY,      "",                T_HAPPY,       E_NONE, R_HAPPY,       F_HAPPY,       false, Q_PARTY, false),
  FAM(MOOD_PROUD,      "passed my ",      T_TESTS,       E_NONE, R_PASSED,      F_PASSED,      false, Q_PARTY, false),
  FAM(MOOD_PROUD,      "",                T_PROUD,       E_NONE, R_PROUD,       F_PROUD,       false, Q_PARTY, false),
  FAM(MOOD_CONFIDENT,  "",                T_HOPEFUL,     E_NONE, R_HOPEFUL,     F_HOPEFUL,     false, Q_NONE, false),
  FAM(MOOD_GRATEFUL,   "",                T_LOVEDBY,     E_NONE, R_LOVEDBY,     F_LOVED,       false, Q_NONE, false),
  FAM(MOOD_GRATEFUL,   "",                T_LUCKY,       E_NONE, R_LUCKY,       F_LUCKY,       false, Q_NONE, false),
  FAM(MOOD_PROUD,      "",                T_ADMIRE,      E_NONE, R_ADMIRE,      F_ADMIRE,      false, Q_NONE, false),
  FAM(MOOD_HAPPY,      "",                T_ENERGETIC,   E_NONE, R_ENERGETIC,   F_ENERGETIC,   false, Q_NONE, false),
  FAM(MOOD_CONFIDENT,  "",                T_FOCUSED,     E_NONE, R_FOCUSED,     F_FOCUSED,     true,  Q_NONE, false),
  // the rest
  FAM(MOOD_ANGRY,      "",                T_ANGRY,       E_NONE, R_ANGRY,       F_ANGRY,       false, Q_TELL, false),
  FAM(MOOD_GRUMPY,     "",                T_JEALOUS,     E_NONE, R_JEALOUS,     F_JEALOUS,     false, Q_TELL, false),
  FAM(MOOD_THOUGHTFUL, "",                T_WARY,        E_NONE, R_WARY,        F_WARY,        false, Q_NONE, false),
  FAM(MOOD_SCARED,     "",                T_SCARED,      E_NONE, R_SCARED,      F_SCARED,      false, Q_TELL, false),
  FAM(MOOD_SCARED,     "scared of ",      T_SCARED_OF,   E_NONE, R_SCARED_OF,   F_SCARED_OF,   false, Q_NONE, false),
  FAM(MOOD_NERVOUS,    "",                T_DREAD,       E_NONE, R_DREAD,       F_DREAD,       false, Q_NONE, false),
  FAM(MOOD_GRUMPY,     "",                T_GRUMPY,      E_NONE, R_GRUMPY,      F_GRUMPY,      false, Q_TELL, false),
  FAM(MOOD_PEACEFUL,   "",                T_ALONE,       E_NONE, R_ALONE,       F_STOP,        true,  Q_NONE, false),
  FAM(MOOD_NERVOUS,    "",                T_NERVOUS,     E_NONE, R_NERVOUS,     F_NERVOUS,     false, Q_TELL, false),
  FAM(MOOD_NERVOUS,    "",                T_OVERWHELMED, E_NONE, R_OVERWHELMED, F_OVERWHELMED, false, Q_REST, false),
  FAM(MOOD_TIRED,      "",                T_TIRED,       E_NONE, R_TIRED,       F_TIRED,       false, Q_REST, false),
  FAM(MOOD_RELIEF,     "",                T_SICK,        E_NONE, R_SICK,        F_SICK,        false, Q_REST, false),
  FAM(MOOD_HAPPY,      "",                T_HUNGRY,      E_NONE, R_HUNGRY,      F_HUNGRY,      false, Q_NONE, false),
  FAM(MOOD_TIRED,      "",                T_BORED,       E_NONE, R_BORED,       F_BORED,       false, Q_NONE, false),
  FAM(MOOD_SHY,        "",                T_SHY,         E_NONE, R_SHY,         F_SHY,         false, Q_NONE, false),
  FAM(MOOD_RELIEF,     "",                T_RELIEF,      E_NONE, R_RELIEF,      F_RELIEF,      false, Q_NONE, false),
  FAM(MOOD_SHOCKED,    "",                T_SHOCKED,     E_NONE, R_SHOCKED,     F_SHOCKED,     false, Q_NONE,  false),
  FAM(MOOD_DISGUSTED,  "",                T_DISGUST,     E_NONE, R_DISGUST,     F_DISGUST,     false, Q_NONE, false),
  FAM(MOOD_DISGUSTED,  "I hate ",         T_HATE,        E_NONE, R_HATE,        F_HATE,        false, Q_NONE, false),
  FAM(MOOD_GRATEFUL,   "",                T_CARING,      E_NONE, R_CARING,      F_CARING,      false, Q_NONE, false),
  FAM(MOOD_THOUGHTFUL, "thinking about ", T_THINK_ABOUT, E_NONE, R_THINK_ABOUT, F_THINK_ABOUT, false, Q_NONE, false),
  FAM(MOOD_THOUGHTFUL, "should I ",       T_SHOULD_I,    E_NONE, R_SHOULD_I,    F_SHOULD_I,    false, Q_NONE, false),
  FAM(MOOD_THOUGHTFUL, "",                T_THINK,       E_NONE, R_THINK,       F_THINK,       false, Q_NONE, false),
  FAM(MOOD_THINKING,   "",                T_CONFUSED,    E_NONE, R_CONFUSED,    F_CONFUSED,    false, Q_TELL, false),
  FAM(MOOD_PEACEFUL,   "",                T_PEACEFUL,    E_NONE, R_PEACEFUL,    F_PEACEFUL,    false, Q_NONE, false),
  FAM(MOOD_CURIOUS,    "",                T_CURIOUS,     E_NONE, R_CURIOUS,     F_CURIOUS,     false, Q_NONE,  false),
  FAM(MOOD_CONFIDENT,  "",                T_CONFIDENT,   E_NONE, R_CONFIDENT,   F_CONFIDENT,   false, Q_NONE, false),
  FAM(MOOD_MISCHIEVOUS,"",                T_MISCHIEF,    E_NONE, R_MISCHIEF,    F_MISCHIEF,    false, Q_NONE,  false),
};
#define FAMILY_COUNT N(FAMILIES)
#define FAMILY_WAKE 0
static int family_of(const char **topics) { for (int i = 0; i < (int)FAMILY_COUNT; i++) if (FAMILIES[i].topics == topics) return i; return -1; }

#define CMD_ID(family, topic) ((family) * 64 + (topic))
#define CMD_FAMILY(id) ((id) / 64)
#define CMD_TOPIC(id)  ((id) % 64)

// ================================================================ when Pengu hears you but catches no chunk: stay present, never complain
static const char *R_HEARD_YOU[]  = { "MM HMM. GO ON", "I HEAR YOU. TELL ME MORE", "GO ON, I AM LISTENING" };   // first miss (thoughtful face)
static const char *R_HEARD_END[]  = { "I HEAR YOU. I AM HERE", "THANK YOU FOR TELLING ME", "I AM RIGHT HERE WITH YOU" };   // chat closes, calm face
static const char *R_HEARD_SAD[]  = { "THANK YOU FOR TELLING ME. HUG", "I HEAR YOU. I AM HERE", "THAT WAS A LOT. I AM HERE" };   // chat closes on a sad face
static const char *R_HEARD_GOOD[] = { "LOVE HEARING THAT", "THANK YOU FOR SHARING", "I HEAR YOU. LOVELY" };   // chat closes on a happy face
static const char *R_NOTHING[]    = { "I AM HERE WHEN YOU NEED ME", "OKAY. I AM AROUND" };
static const char *LINE_IDLE = "SAY HEY PENGU";

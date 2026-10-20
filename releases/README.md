# Releases

Ready made files to load onto the board. Always use the newest one. Steps are in the main README.
`speech.bin` is the spoken lines on their own (already inside the full file); you only need it if you upload from Arduino IDE, then flash it at `0x620000`.

| File | What it does |
|---|---|
| `Pengu_v13_full_8MB.bin` | **Newest.** Pengu says every line it shows (292 recorded lines in `speech.bin`, packed inside this file). New flash map: always Erase Flash first. Gear listens the moment you press. |
| `Pengu_v12_full_8MB.bin` | Pengu asks back ("WANT A HUG?", "REST WITH ME?", "CELEBRATE?", "TELL ME MORE?") and understands yes and no. Power button restarts it. |
| `PENGUFILE_v11_full_8MB.bin` | The face only changes when the feeling changes, "I heard you" with a heart at the end of every chat, gear press starts a conversation, no black flash between chats. |
| `PENGUFILE_v10_full_8MB.bin` | Listens for the feeling inside any sentence ("my day was bad" works), 63 feeling families, 99 hand drawn faces, stays present when it misses. Hold the gear button for the help line. |
| `PENGUFILE_v9_full_8MB.bin` | No screen flashes mid chat, asks questions it can hear the answers to, reviewed line by line. |
| `PENGUFILE_v8_full_8MB.bin` | Conversation mode like a smart speaker. |
| `PENGUFILE_v7_full_8MB.bin` | 22 moods, 24 drawings, talk to Pengu itself. |
| `PENGUFILE_v6_full_8MB.bin` | First version with the pencil sketch faces (8 of them). |
| `PENGUFILE_v5_full_8MB.bin` | Penguin drawn in code. |
| `PENGUFILE_v4_full_8MB.bin` | Patterns instead of sentences, "say that again?". Robot face. |
| `PENGUFILE_v3_full_8MB.bin` | Cloud brain test. Needs Wi-Fi and an OpenAI key. Never tested. |
| `PENGUFILE_v2_full_8MB.bin` | 100 phrases and the name "Pengu". |
| `Tamagotchi_full_8MB.bin` | v1. Two words: happy and sad. |

Every file is the whole thing (program plus the speech model), so loading one replaces everything.

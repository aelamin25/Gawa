# Putting Gawa on GitHub (and keeping it updated)

GitHub keeps every version of a project online. This folder is already shaped like a GitHub project:
`README.md` is the front page, `docs/BUILD-LOG.md` is the diary, the code is in `firmware/Gawa/`.
We use the Mac Terminal, the same way as the himalayan-hop-agent project.

## First time (5 minutes) – already done on 20 Oct 2026

1. On github.com click the **+** (top right) → **New repository**. Name: `gawa`. Leave everything else unticked. Click **Create repository**.
2. Open **Terminal** on the Mac (press Cmd+Space, type Terminal, Enter).
3. Paste these lines one at a time and press Enter after each:

```
cd ~/Desktop/Tamagotchi
git init -b main
git add .
git commit -m "v8: Gawa so far, with build log"
git remote add origin https://github.com/aelamin25/gawa.git
git push -u origin main
```

4. If it asks who you are, run these two first, then repeat the commit line:

```
git config user.name "Ahmed Elamin"
git config user.email "235667700+aelamin25@users.noreply.github.com"
```

5. Open https://github.com/aelamin25/gawa. The README is the front page.

## Every time Gawa changes (30 seconds)

```
cd ~/Desktop/Tamagotchi
git add .
git commit -m "what changed, in a few words"
git push
```

## What stays off GitHub

`.gitignore` already skips `_to_delete/`, `Claude outputs/`, build files and any `.env`. Nothing here has a password or key.

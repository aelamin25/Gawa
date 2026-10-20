# Putting Gawa on GitHub (and keeping it updated)

GitHub is a website that keeps every version of a project. This folder is already shaped like a GitHub project:
`README.md` is the front page, `LOG.md` is the diary, the code is in `Tamagotchi/`.

## First time (10 minutes)

1. Go to https://desktop.github.com and install **GitHub Desktop**. Sign in (make a free account if you need one).
2. In GitHub Desktop: **File → Add Local Repository** → choose the `Tamagotchi` folder on your Desktop → if it says
   "this is not a git repository", click **create a repository here** → **Create Repository**.
3. Click **Publish repository** (top bar). Name it `gawa`. Untick "Keep this code private" if you want people to see it. Click **Publish**.
4. Done. Your project is at `github.com/<your name>/gawa` and the README shows on the front page.

## Every time Gawa changes (1 minute)

1. Open GitHub Desktop. It lists the changed files on the left.
2. In the box at the bottom left, type what changed in a few words (for example `v8: conversation mode`).
3. Click **Commit to main**, then **Push origin** at the top.

That is it. The LOG.md diary is what people read to follow along, so keep adding a step each time.

## What not to upload

- The `_to_delete` folder (just delete it).
- Anything with a password or API key in it. The code here has none.

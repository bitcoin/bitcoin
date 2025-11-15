
#!/usr/bin/env bash
set -euo pipefail

# Run this from the repo root (where .git lives).
if [ ! -d .git ]; then
  echo "Run this from the git repo root (where .git/ exists)." >&2
  exit 1
fi

# Strongly recommended: make sure you have a clean working tree
if ! git diff --quiet || ! git diff --cached --quiet; then
  echo "Your working tree is not clean. Commit or stash changes before running." >&2
  exit 1
fi

echo "Renaming Snailcoin -> Snailcoin strings across tracked files..."
echo

# List of literal replacements: FROM → TO
# These are all plain text (not regex); we use perl's \Q...\E to escape them.
declare -a FROM=(
  "Snailcoin Core"
  "Snailcoin"
  "SNAIL"
  ".snailcoin"
  "snailcoin.conf"
  "snailcoind"
  "snailcoin-cli"
  "snailcoin-qt"
)
declare -a TO=(
  "Snailcoin Core"
  "Snailcoin"
  "SNAIL"
  ".snailcoin"
  "snailcoin.conf"
  "snailcoind"
  "snailcoin-cli"
  "snailcoin-qt"
)

# Get tracked files only (no .git, no build/)
FILES=$(git ls-files)

for i in "${!FROM[@]}"; do
  from=${FROM[$i]}
  to=${TO[$i]}
  echo ">>> Replacing: '$from' -> '$to'"

  # Find files that actually contain this string
  # -I: skip binary files
  # We use grep to limit which files we touch.
  matches=$(grep -RIl -- "$from" $FILES || true)
  if [ -z "$matches" ]; then
    echo "    No matches found."
    continue
  fi

  # Do literal replacement with perl (safer than sed for arbitrary text)
  while IFS= read -r f; do
    perl -pi -e 's/\Q'"$from"'\E/'"$to"'/g' "$f"
  done <<< "$matches"
done

echo
echo "Done. Check 'git diff' to review all changes."

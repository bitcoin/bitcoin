#!/bin/bash

# This script will locally construct a merge commit for a pull request on a
# github repository, inspect it, sign it and optionally push it.

# The following temporary branches are created/overwritten and deleted:
# * pull/$PULL/base (the current master we're merging onto)
# * pull/$PULL/head (the current state of the remote pull request)
# * pull/$PULL/merge (github's merge)
# * pull/$PULL/local-merge (our merge)

# In case of a clean merge that is accepted by the user, the local branch with
# name $BRANCH is overwritten with the merged result, and optionally pushed.

REPO="$(git config --get githubmerge.repository)"
if [[ "d$REPO" == "d" ]]; then
  echo "ERROR: No repository configured. Use this command to set:" >&2
  echo "git config githubmerge.repository <owner>/<repo>" >&2
  echo "In addition, you can set the following variables:" >&2
  echo "- githubmerge.host (default git@github.com)" >&2
  echo "- githubmerge.branch (default master)" >&2
  echo "- githubmerge.testcmd (default none)" >&2
  exit 1
fi

HOST="$(git config --get githubmerge.host)"
if [[ "d$HOST" == "d" ]]; then
  HOST="git@github.com"
fi

BRANCH="$(git config --get githubmerge.branch)"
if [[ "d$BRANCH" == "d" ]]; then
  BRANCH="master"
fi

TESTCMD="$(git config --get githubmerge.testcmd)"

PULL="$1"

if [[ "d$PULL" == "d" ]]; then
  echo "Usage: $0 pullnumber [branch]" >&2
  exit 2
fi

if [[ "d$2" != "d" ]]; then
  BRANCH="$2"
fi

# Initialize source branches.
git checkout -q "$BRANCH"
if git fetch -q "$HOST":"$REPO" "+refs/pull/$PULL/*:refs/heads/pull/$PULL/*"; then
  if ! git log -q -1 "refs/heads/pull/$PULL/head" >/dev/null 2>&1; then
    echo "ERROR: Cannot find head of pull request #$PULL on $HOST:$REPO." >&2
    exit 3
  fi
  if ! git log -q -1 "refs/heads/pull/$PULL/merge" >/dev/null 2>&1; then
    echo "ERROR: Cannot find merge of pull request #$PULL on $HOST:$REPO." >&2
    exit 3
  fi
else
  echo "ERROR: Cannot find pull request #$PULL on $HOST:$REPO." >&2
  exit 3
fi
if git fetch -q "$HOST":"$REPO" +refs/heads/"$BRANCH":refs/heads/pull/"$PULL"/base; then
  true
else
  echo "ERROR: Cannot find branch $BRANCH on $HOST:$REPO." >&2
  exit 3
fi
git checkout -q pull/"$PULL"/base
git branch -q -D pull/"$PULL"/local-merge 2>/dev/null
git checkout -q -b pull/"$PULL"/local-merge
TMPDIR="$(mktemp -d -t ghmXXXXX)"

function cleanup() {
  git checkout -q "$BRANCH"
  git branch -q -D pull/"$PULL"/head 2>/dev/null
  git branch -q -D pull/"$PULL"/base 2>/dev/null
  git branch -q -D pull/"$PULL"/merge 2>/dev/null
  git branch -q -D pull/"$PULL"/local-merge 2>/dev/null
  rm -rf "$TMPDIR"
}

# Create unsigned merge commit.
(
  echo "Merge pull request #$PULL"
  echo ""
  git log --no-merges --topo-order --pretty='format:%h %s (%an)' pull/"$PULL"/base..pull/"$PULL"/head
)>"$TMPDIR/message"
if git merge -q --commit --no-edit --no-ff -m "$(<"$TMPDIR/message")" pull/"$PULL"/head; then
  if [ "d$(git log --pretty='format:%s' -n 1)" != "dMerge pull request #$PULL" ]; then
    echo "ERROR: Creating merge failed (already merged?)." >&2
    cleanup
    exit 4
  fi
else
  echo "ERROR: Cannot be merged cleanly." >&2
  git merge --abort
  cleanup
  exit 4
fi

# Run test command if configured.
if [[ "d$TESTCMD" != "d" ]]; then
  # Go up to the repository's root.
  while [ ! -d .git ]; do cd ..; done
  if ! $TESTCMD; then
    echo "ERROR: Running $TESTCMD failed." >&2
    cleanup
    exit 5
  fi
  # Show the created merge.
  git diff pull/"$PULL"/merge..pull/"$PULL"/local-merge >"$TMPDIR"/diff
  git diff pull/"$PULL"/base..pull/"$PULL"/local-merge
  if [[ "$(<"$TMPDIR"/diff)" != "" ]]; then
    echo "WARNING: merge differs from github!" >&2
    read -p "Type 'ignore' to continue. " -r >&2
    if [[ "d$REPLY" =~ ^d[iI][gG][nN][oO][rR][eE]$ ]]; then
      echo "Difference with github ignored." >&2
    else
      cleanup
      exit 6
    fi
  fi
  read -p "Press 'd' to accept the diff. " -n 1 -r >&2
  echo
  if [[ "d$REPLY" =~ ^d[dD]$ ]]; then
    echo "Diff accepted." >&2
  else
    echo "ERROR: Diff rejected." >&2
    cleanup
    exit 6
  fi
else
  # Verify the result.
  echo "Dropping you on a shell so you can try building/testing the merged source." >&2
  echo "Run 'git diff HEAD~' to show the changes being merged." >&2
  echo "Type 'exit' when done." >&2
  if [[ -f /etc/debian_version ]]; then # Show pull number in prompt on Debian default prompt
      export debian_chroot="$PULL"
  fi
  bash -i
  read -p "Press 'm' to accept the merge. " -n 1 -r >&2
  echo
  if [[ "d$REPLY" =~ ^d[Mm]$ ]]; then
    echo "Merge accepted." >&2
  else
    echo "ERROR: Merge rejected." >&2
    cleanup
    exit 7
  fi
fi

# Sign the merge commit.
read -p "Press 's' to sign off on the merge. " -n 1 -r >&2
echo
if [[ "d$REPLY" =~ ^d[Ss]$ ]]; then
  if [[ "$(git config --get user.signingkey)" == "" ]]; then
    echo "ERROR: No GPG signing key set, not signing. Set one using:" >&2
    echo "git config --global user.signingkey <key>" >&2
    cleanup
    exit 1
  else
    if ! git commit -q --gpg-sign --amend --no-edit; then
        echo "Error signing, exiting."
        cleanup
        exit 1
    fi
  fi
else
  echo "Not signing off on merge, exiting."
  cleanup
  exit 1
fi

# Clean up temporary branches, and put the result in $BRANCH.
git checkout -q "$BRANCH"
git reset -q --hard pull/"$PULL"/local-merge
cleanup

# Push the result.
read -p "Type 'push' to push the result to $HOST:$REPO, branch $BRANCH. " -r >&2
if [[ "d$REPLY" =~ ^d[Pp][Uu][Ss][Hh]$ ]]; then
  git push "$HOST":"$REPO" refs/heads/"$BRANCH"
fi

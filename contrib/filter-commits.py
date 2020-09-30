#!/usr/bin/env python3

import subprocess, os

initialCommit = "HEAD~"
if "INITIAL_COMMIT" in os.environ:
    initialCommit = os.environ["INITIAL_COMMIT"]

filterPaths = [
    "build_msvc",
    "contrib/filter-commits.py",
    ".gitignore"
]

subprocess.run(["git", "reset", "--soft", initialCommit])
subprocess.run(["git", "add", "--all", "."])

for p in filterPaths:
    subprocess.run(["git", "rm", "--cached", "-r", p])

subprocess.run(["git", "commit", "-m", "Auto squash commits from master since " + initialCommit])

for p in filterPaths:
    subprocess.run(["git", "add", p])

subprocess.run(["git", "commit", "-m", "Auxiliary changes"])

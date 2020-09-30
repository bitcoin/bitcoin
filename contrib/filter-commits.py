#!/usr/bin/env python3

import subprocess, os

initialCommit = "HEAD~"
if "INITIAL_COMMIT" in os.environ:
    initialCommit = os.environ["INITIAL_COMMIT"]

filterPaths = [
    "./build_msvc",
    ".gitignore",
    ".dockerignore",
    ".github",
    ".gitmodules",
    "CONTRIBUTING.md",
    "COPYING",
    "INSTALL.md",
    "README.md",
    "SECURITY.md",
    "*.Dockerfile",
    "contrib"
]

subprocess.run(["git", "reset", "--soft", initialCommit])
subprocess.run(["git", "add", "--all", "."])

for p in filterPaths:
    subprocess.run(["git", "rm", "--cached", "-r", p])

subprocess.run(["git", "commit", "-m", "Features added since " + initialCommit])

for p in filterPaths:
    subprocess.run(["git", "add", p])

subprocess.run(["git", "commit", "-m", "Branding changes"])

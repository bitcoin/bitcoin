#!/usr/bin/env python3

import subprocess, os

initialCommit = "HEAD~"
if "INITIAL_COMMIT" in os.environ:
    initialCommit = os.environ["INITIAL_COMMIT"]

whitelist = [
    "depends/packages/veriblock-pop-cpp.mk",
    "depends/packages/packages.mk"
]

blacklist = [
    "build_msvc",
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
    "contrib",
    "Makefile.am",
    "depends",
    "doc",
    "docker-compose.yml",
    "setup.iss",
    "share",
    "sonar-project.properties",
    "src/bitcoin*.rc",
    "src/bitcoin-cli.cpp",
    "src/fs.cpp",
    "src/leveldb",
    "src/qt"
]

subprocess.run(["git", "reset", "--soft", initialCommit])
subprocess.run(["git", "add", "--all", "."])

# first, remove blacklisted items
for p in blacklist:
    subprocess.run(["git", "reset", p])

# then add whitelisted
for p in whitelist:
    subprocess.run(["git", "add", p])

subprocess.run(["git", "commit", "-m", "Features added since " + initialCommit])

for p in blacklist:
    subprocess.run(["git", "add", p])

subprocess.run(["git", "commit", "-m", "Branding/Aux changes"])

#!/usr/bin/env python3

"""
Check that generated out files which use special compilation units do not
contain any disallowed sections. See #18553 for additional context.

Special instructions include:

    SSE42
    SSE41
    AVX
    AVX2
    SHANI

Disallowed sections include:

    .text.startup

Example usage:

    python3 contrib/devtools/special-instruction-check.py
"""
import glob
import os
import re
import sys

import lief #type:ignore

cwd = os.getcwd()
DISALLOWED_SECTIONS = [
    ".text.startup",
]

# Compile the regex of special instructions to search for
pattern = re.compile(r".*(SSE42|SSE41|AVX|AVX2|SHANI).*", re.IGNORECASE)

# Perform the search over file names
files = [file for file in glob.glob(f"{cwd}/**/*.o", recursive=True) if pattern.search(file)]

# Warn over stderr if no files found, but don't fail. This could be a correct
# success, or it could be that glob did not find the files (e.g. run from wrong
# root directory)
if not files:
    print(f"{__file__}: no special instruction *.o files found in {cwd}", file=sys.stderr)
    sys.exit(0)
else:
    print(f"{__file__}: checking for disallowed sections in {', '.join(files)}", file=sys.stdout)

# Parse files for disallowed sections with lief
error = False
for file in files:
    out_file = lief.parse(file)
    out_file_sections = [section.name for section in out_file.sections]
    for section in DISALLOWED_SECTIONS:
        if section in out_file_sections:
            print(f"{__file__}: ERROR {file} contains disallowed section {section}", file=sys.stderr)
            error = True

sys.exit(1) if error else sys.exit(0)

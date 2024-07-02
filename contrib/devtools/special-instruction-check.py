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

# Perform the search over file names.
files = [file for file in glob.glob(f"{cwd}/**/*.o", recursive=True) if pattern.search(file)]

# Fail if we didn't find any files to check. This is likely a runtime mistake
if not files:
    print(f"{__file__}: no special instruction *.o files found in {cwd}", file=sys.stderr)
    sys.exit(1)
else:
    print(f"{__file__}: checking for disallowed sections in {', '.join(files)}", file=sys.stdout)

# Parse files for disallowed sections
error = False
for file in files:
    binary = lief.parse(file)
    if not binary:
        print(f"ERROR: {file} was not the correct format for lief parsing")
        error = True
        continue
    out_file_sections = [section.name for section in binary.sections]
    for section in DISALLOWED_SECTIONS:
        if section in out_file_sections:
            print(f"{__file__}: ERROR {file} contains disallowed section {section}", file=sys.stderr)
            error = True

if error:
    print("ERROR: special instruction check did not complete successfully")
    sys.exit(1)
else:
    print("SUCCESS: special instruction check completed successfully")
    sys.exit(0)

#!/usr/bin/python
# Copyright (c) 2014 Wladimir J. van der Laan
# Distributed under the MIT/X11 software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
'''
Run this script from the root of the repository to update all translations from
transifex.
It will do the following automatically:

- fetch all translations using the tx tool
- post-process them into valid and committable format
  - remove invalid control characters
  - remove location tags (makes diffs less noisy)

TODO:
- auto-add new translations to the build system according to the translation process
- remove 'unfinished' translation items
'''
from __future__ import division, print_function
import subprocess
import re
import sys
import os

# Name of transifex tool
TX = 'tx'
# Name of source language file
SOURCE_LANG = 'bitcoin_en.ts'
# Directory with locale files
LOCALE_DIR = 'src/qt/locale'

def check_at_repository_root():
    if not os.path.exists('.git'):
        print('No .git directory found')
        print('Execute this script at the root of the repository', file=sys.stderr)
        exit(1)

def fetch_all_translations():
    if subprocess.call([TX, 'pull', '-f']):
        print('Error while fetching translations', file=sys.stderr)
        exit(1)

def postprocess_translations():
    print('Postprocessing...')
    for filename in os.listdir(LOCALE_DIR):
        # process only language files, and do not process source language
        if not filename.endswith('.ts') or filename == SOURCE_LANG: 
            continue
        filepath = os.path.join(LOCALE_DIR, filename)
        with open(filepath, 'rb') as f:
            data = f.read()
        # remove non-allowed control characters
        data = re.sub('[\x00-\x09\x0b\x0c\x0e-\x1f]', '', data)
        data = data.split('\n')
        # strip locations from non-origin translation
        # location tags are used to guide translators, they are not necessary for compilation
        # TODO: actually process XML instead of relying on Transifex's one-tag-per-line output format
        data = [line for line in data if not '<location' in line]
        with open(filepath, 'wb') as f:
            f.write('\n'.join(data))

if __name__ == '__main__':
    check_at_repository_root()
    fetch_all_translations()
    postprocess_translations()


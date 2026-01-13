#!/usr/bin/env python3
# Copyright (c) 2022-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
import configparser
import os
import re
import subprocess
import sys
import tempfile
import argparse

BINARIES = [
'bin/bitcoin',
'bin/bitcoind',
'bin/bitcoin-cli',
'bin/bitcoin-tx',
'bin/bitcoin-wallet',
'bin/bitcoin-util',
'bin/bitcoin-qt',
]

parser = argparse.ArgumentParser(
    formatter_class=argparse.RawDescriptionHelpFormatter,
)
parser.add_argument(
    "-s",
    "--skip-missing-binaries",
    action="store_true",
    default=False,
    help="skip generation for binaries that are not found in the build path",
)
args = parser.parse_args()

# Paths to external utilities.
git = os.getenv('GIT', 'git')
help2man = os.getenv('HELP2MAN', 'help2man')

# If not otherwise specified, get top directory from git.
topdir = os.getenv('TOPDIR')
if not topdir:
    r = subprocess.run([git, 'rev-parse', '--show-toplevel'], stdout=subprocess.PIPE, check=True, text=True)
    topdir = r.stdout.rstrip()

# Get input and output directories.
builddir = os.getenv('BUILDDIR', os.path.join(topdir, 'build'))
mandir = os.getenv('MANDIR', os.path.join(topdir, 'doc/man'))

# Check build configuration to ensure all features are enabled for complete documentation.
# Man pages should document all available options, which requires the build to have
# wallet, ZMQ, and system call support enabled.
REQUIRED_COMPONENTS = {
    'ENABLE_WALLET': 'wallet-specific options (e.g., -wallet, -disablewallet)',
    'ENABLE_ZMQ': 'ZMQ notification options (e.g., -zmqpubhashtx)',
}

config_path = os.path.join(builddir, 'test', 'config.ini')
if os.path.exists(config_path):
    config = configparser.ConfigParser()
    config.read(config_path)
    components = dict(config.items('components')) if config.has_section('components') else {}

    missing_components = []
    for component, description in REQUIRED_COMPONENTS.items():
        if component.lower() not in components:
            missing_components.append(f'  - {component}: {description}')

    if missing_components:
        print("WARNING: Build is missing components needed for complete man page documentation:", file=sys.stderr)
        for msg in missing_components:
            print(msg, file=sys.stderr)
        print("Man pages generated from this build should NOT be committed.", file=sys.stderr)
        print("To generate complete man pages, rebuild with all features enabled.", file=sys.stderr)
        print(file=sys.stderr)
else:
    print(f"WARNING: Build configuration not found at {config_path}", file=sys.stderr)
    print("Cannot verify that all features are enabled for complete documentation.", file=sys.stderr)
    print(file=sys.stderr)

# Verify that all the required binaries are usable, and extract copyright
# message in a first pass.
versions = []
for relpath in BINARIES:
    abspath = os.path.join(builddir, relpath)
    try:
        r = subprocess.run([abspath, "--version"], stdout=subprocess.PIPE, check=True, text=True)
    except IOError:
        if(args.skip_missing_binaries):
            print(f'{abspath} not found or not an executable. Skipping...', file=sys.stderr)
            continue
        else:
            print(f'{abspath} not found or not an executable', file=sys.stderr)
            sys.exit(1)
    # take first line (which must contain version)
    output = r.stdout.splitlines()[0]
    # find the version e.g. v30.99.0-ce771726f3e7
    search = re.search(r"v[0-9]\S+", output)
    assert search
    verstr = search.group(0)
    # remaining lines are copyright
    copyright = r.stdout.split('\n')[1:]
    assert copyright[0].startswith('Copyright (C)')

    versions.append((abspath, verstr, copyright))

if not versions:
    print(f'No binaries found in {builddir}. Please ensure the binaries are present in {builddir}, or set another build path using the BUILDDIR env variable.')
    sys.exit(1)

if any(verstr.endswith('-dirty') for (_, verstr, _) in versions):
    print("WARNING: Binaries were built from a dirty tree.")
    print('man pages generated from dirty binaries should NOT be committed.')
    print('To properly generate man pages, please commit your changes (or discard them), rebuild, then run this script again.')
    print()

with tempfile.NamedTemporaryFile('w', suffix='.h2m') as footer:
    # Create copyright footer, and write it to a temporary include file.
    # Copyright is the same for all binaries, so just use the first.
    footer.write('[COPYRIGHT]\n')
    footer.write('\n'.join(versions[0][2]).strip())
    # Create SEE ALSO section
    footer.write('\n[SEE ALSO]\n')
    footer.write(', '.join(s.rpartition('/')[2] + '(1)' for s in BINARIES))
    footer.write('\n')
    footer.flush()

    # Call the binaries through help2man to produce a manual page for each of them.
    for (abspath, verstr, _) in versions:
        outname = os.path.join(mandir, os.path.basename(abspath) + '.1')
        print(f'Generating {outname}…')
        subprocess.run([help2man, '-N', '--version-string=' + verstr, '--include=' + footer.name, '-o', outname, abspath], check=True)

#!/usr/bin/env python3
# Copyright (c) 2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
import os
import subprocess
import sys
import tempfile
import argparse


def _read_cmake_cache(cache_path):
    options = {}
    with open(cache_path, encoding="utf-8") as cache:
        for line in cache:
            line = line.strip()
            if not line or line.startswith("#") or line.startswith("//"):
                continue
            if "=" not in line or ":" not in line:
                continue
            lhs, value = line.split("=", 1)
            key, _key_type = lhs.split(":", 1)
            options[key] = value
    return options


def _cmake_truthy(value):
    return value.upper() in {"ON", "TRUE", "YES"} or value == "1"

def _env_truthy(var_name):
    value = os.getenv(var_name)
    return value is not None and _cmake_truthy(value)

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

required_features = [
    ("HAVE_SYSTEM", "system notifications (e.g. -alertnotify)", "ensure the build environment provides std::system (HAVE_SYSTEM=1)"),
    ("ENABLE_WALLET", "wallet functionality", "configure CMake with -DENABLE_WALLET=ON"),
    ("WITH_ZMQ", "ZMQ interface", "configure CMake with -DWITH_ZMQ=ON"),
]

cmake_cache_path = os.path.join(builddir, 'CMakeCache.txt')
if not os.path.exists(cmake_cache_path):
    print(f'ERROR: {cmake_cache_path} not found. Run this script against a CMake build directory.', file=sys.stderr)
    sys.exit(1)

if _env_truthy("SKIP_BUILD_OPTIONS_CHECK"):
    print('WARNING: skipping build option checks; generated man pages may be incomplete.', file=sys.stderr)
else:
    cmake_options = _read_cmake_cache(cmake_cache_path)
    missing_features = []
    for cmake_var, description, hint in required_features:
        value = cmake_options.get(cmake_var)
        if value is None or not _cmake_truthy(value):
            missing_features.append((cmake_var, description, hint))

    if missing_features:
        print('ERROR: binaries were not built with all interfaces required for generating complete man pages.', file=sys.stderr)
        for cmake_var, description, hint in missing_features:
            print(f'  - {cmake_var}: enables {description}; {hint}.', file=sys.stderr)
        print('Rebuild the project with the options above enabled, then rerun gen-manpages.py, or export SKIP_BUILD_OPTIONS_CHECK=1 to bypass this safeguard.', file=sys.stderr)
        sys.exit(1)

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
    verstr = r.stdout.splitlines()[0]
    # last word of line is the actual version e.g. v22.99.0-5c6b3d5b3508
    verstr = verstr.split()[-1]
    assert verstr.startswith('v')
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

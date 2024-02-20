#!/usr/bin/env python3
# Copyright (c) 2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
import os
import subprocess
import sys
import tempfile
import configparser

BINARIES = [
'build/src/bitcoind',
'build/src/bitcoin-cli',
'build/src/bitcoin-tx',
'build/src/bitcoin-wallet',
'build/src/bitcoin-util',
'build/src/qt/bitcoin-qt',
]

# Paths to external utilities.
git = os.getenv('GIT', 'git')
help2man = os.getenv('HELP2MAN', 'help2man')

# If not otherwise specified, get top directory from git.
topdir = os.getenv('TOPDIR')
if not topdir:
    r = subprocess.run([git, 'rev-parse', '--show-toplevel'], stdout=subprocess.PIPE, check=True, text=True)
    topdir = r.stdout.rstrip()

# Get input and output directories.
builddir = os.getenv('BUILDDIR', topdir)
mandir = os.getenv('MANDIR', os.path.join(topdir, 'doc/man'))

# Verify that all the required binaries are usable, and extract copyright
# message in a first pass.
versions = []
for relpath in BINARIES:
    abspath = os.path.join(builddir, relpath)
    try:
        r = subprocess.run([abspath, "--version"], stdout=subprocess.PIPE, check=True, text=True)
    except IOError:
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

if any(verstr.endswith('-dirty') for (_, verstr, _) in versions):
    print("WARNING: Binaries were built from a dirty tree.")
    print('man pages generated from dirty binaries should NOT be committed.')
    print('To properly generate man pages, please commit your changes (or discard them), rebuild, then run this script again.')
    print()

# Check enabled build options
config = configparser.ConfigParser()
conffile = os.path.join(builddir, 'build/test/config.ini')
config.read(conffile)

required_components = {
    'HAVE_SYSTEM': 'System component',
    'ENABLE_WALLET': 'Wallet functionality',
    'USE_SQLITE': 'SQLite',
    'ENABLE_CLI': 'CLI',
    'ENABLE_BITCOIN_UTIL': 'Bitcoin utility',
    'ENABLE_WALLET_TOOL': 'Wallet tool',
    'ENABLE_BITCOIND': 'Bitcoind',
    'ENABLE_EXTERNAL_SIGNER': 'External Signer',
}

enabled_components = {
    'USE_BDB': 'Berkeley DB',
    'ENABLE_FUZZ_BINARY': 'Fuzz testing binary',
    'ENABLE_ZMQ': 'ZeroMQ support',
    'ENABLE_USDT_TRACEPOINTS': 'USDT tracepoints',
}

for component, description in (required_components | enabled_components).items():
    if not config['components'].getboolean(component, fallback=False):
        print(
            "Aborting generating manpages...\n"
            f"Error: '{component}' ({description}) support is not enabled.\n"
            "Please enable it and try again."
        )
        sys.exit(1)

for component in config['components']:
    if component.upper() not in required_components and component.upper() not in enabled_components:
        print(
            "Aborting generating manpages...\n"
            f"Error: Unknown component '{component}' found in configuration.\n"
            "Please remove it and try again"
        )
        sys.exit(1)


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

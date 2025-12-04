#!/usr/bin/env python3
# Copyright (c) 2020-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Script for verifying Bitcoin Core release binaries.

This script attempts to download the sum file SHA256SUMS and corresponding
signature file SHA256SUMS.asc from bitcoincore.org and bitcoin.org and
compares them.

The sum-signature file is signed by a number of builder keys. This script
ensures that there is a minimum threshold of signatures from pubkeys that
we trust. This trust is articulated on the basis of configuration options
here, but by default is based upon local GPG trust settings.

The builder keys are available in the guix.sigs repo:

    https://github.com/bitcoin-core/guix.sigs/tree/main/builder-keys

If a minimum good, trusted signature threshold is met on the sum file, we then
download the files specified in SHA256SUMS, and check if the hashes of these
files match those that are specified. The script returns 0 if everything passes
the checks. It returns 1 if either the signature check or the hash check
doesn't pass. If an error occurs the return value is >= 2.

Logging output goes to stderr and final binary verification data goes to stdout.

JSON output can by obtained by setting env BINVERIFY_JSON=1.
"""
import argparse
import difflib
import json
import logging
import os
import subprocess
import typing as t
import re
import sys
import shutil
import tempfile
import textwrap
import urllib.request
import urllib.error
import enum
from hashlib import sha256
from pathlib import PurePath, Path

# The primary host; this will fail if we can't retrieve files from here.
HOST1 = "https://bitcoincore.org"
HOST2 = "https://bitcoin.org"
VERSIONPREFIX = "bitcoin-core-"
SUMS_FILENAME = 'SHA256SUMS'
SIGNATUREFILENAME = f"{SUMS_FILENAME}.asc"


class ReturnCode(enum.IntEnum):
    SUCCESS = 0
    INTEGRITY_FAILURE = 1
    FILE_GET_FAILED = 4
    FILE_MISSING_FROM_ONE_HOST = 5
    FILES_NOT_EQUAL = 6
    NO_BINARIES_MATCH = 7
    NOT_ENOUGH_GOOD_SIGS = 9
    BINARY_DOWNLOAD_FAILED = 10
    BAD_VERSION = 11


def set_up_logger(is_verbose: bool = True) -> logging.Logger:
    """Set up a logger that writes to stderr."""
    log = logging.getLogger(__name__)
    log.setLevel(logging.INFO if is_verbose else logging.WARNING)
    console = logging.StreamHandler(sys.stderr)  # log to stderr
    console.setLevel(logging.DEBUG)
    formatter = logging.Formatter('[%(levelname)s] %(message)s')
    console.setFormatter(formatter)
    log.addHandler(console)
    return log


log = set_up_logger()


def indent(output: str) -> str:
    return textwrap.indent(output, '  ')


def bool_from_env(key, default=False) -> bool:
    if key not in os.environ:
        return default
    raw = os.environ[key]

    if raw.lower() in ('1', 'true'):
        return True
    elif raw.lower() in ('0', 'false'):
        return False
    raise ValueError(f"Unrecognized environment value {key}={raw!r}")


VERSION_FORMAT = "<major>.<minor>[.<patch>][-rc[0-9]][-platform]"
VERSION_EXAMPLE = "22.0 or 23.1-rc1-darwin.dmg or 27.0-x86_64-linux-gnu"

def parse_version_string(version_str):
    # "<version>[-rcN][-platform]"
    version_base, _, platform = version_str.partition('-')
    rc = ""
    if platform.startswith("rc"): # "<version>-rcN[-platform]"
        rc, _, platform = platform.partition('-')
    # else "<version>" or "<version>-platform"

    return version_base, rc, platform


def download_with_wget(remote_file, local_file):
    result = subprocess.run(['wget', '-O', local_file, remote_file],
                            stderr=subprocess.STDOUT, stdout=subprocess.PIPE)
    return result.returncode == 0, result.stdout.decode().rstrip()


def download_lines_with_urllib(url) -> tuple[bool, list[str]]:
    """Get (success, text lines of a file) over HTTP."""
    try:
        return (True, [
            line.strip().decode() for line in urllib.request.urlopen(url).readlines()])
    except urllib.error.HTTPError as e:
        log.warning(f"HTTP request to {url} failed (HTTPError): {e}")
    except Exception as e:
        log.warning(f"HTTP request to {url} failed ({e})")
    return (False, [])


def verify_with_gpg(
    filename,
    signature_filename,
    output_filename: t.Optional[str] = None
) -> tuple[int, str]:
    with tempfile.NamedTemporaryFile() as status_file:
        args = [
            'gpg', '--yes', '--verify', '--verify-options', 'show-primary-uid-only', "--status-file", status_file.name,
            '--output', output_filename if output_filename else '', signature_filename, filename]

        env = dict(os.environ, LANGUAGE='en')
        result = subprocess.run(args, stderr=subprocess.STDOUT, stdout=subprocess.PIPE, env=env)

        gpg_data = status_file.read().decode().rstrip()

    log.debug(f'Result from GPG ({result.returncode}): {result.stdout.decode()}')
    log.debug(f"{gpg_data}")
    return result.returncode, gpg_data


def remove_files(filenames):
    for filename in filenames:
        os.remove(filename)


class SigData:
    """GPG signature data as parsed from GPG stdout."""
    def __init__(self):
        self.key = None
        self.name = ""
        self.trusted = False
        self.status = ""

    def __bool__(self):
        return self.key is not None

    def __repr__(self):
        return (
            "SigData(%r, %r, trusted=%s, status=%r)" %
            (self.key, self.name, self.trusted, self.status))


def parse_gpg_result(
    output: list[str]
) -> tuple[list[SigData], list[SigData], list[SigData]]:
    """Returns good, unknown, and bad signatures from GPG stdout."""
    good_sigs: list[SigData] = []
    unknown_sigs: list[SigData] = []
    bad_sigs: list[SigData] = []
    total_resolved_sigs = 0

    # Ensure that all lines we match on include a prefix that prevents malicious input
    # from fooling the parser.
    def line_begins_with(patt: str, line: str) -> t.Optional[re.Match]:
        return re.match(r'^(\[GNUPG:\])\s+' + patt, line)

    curr_sigs = unknown_sigs
    curr_sigdata = SigData()

    for line in output:
        if line_begins_with(r"NEWSIG(?:\s|$)", line):
            total_resolved_sigs += 1
            if curr_sigdata:
                curr_sigs.append(curr_sigdata)
                curr_sigdata = SigData()
            newsig_split = line.split()
            if len(newsig_split) == 3:
                curr_sigdata.name = newsig_split[2]

        elif line_begins_with(r"GOODSIG(?:\s|$)", line):
            curr_sigdata.key, curr_sigdata.name = line.split(maxsplit=3)[2:4]
            curr_sigs = good_sigs

        elif line_begins_with(r"EXPKEYSIG(?:\s|$)", line):
            curr_sigdata.key, curr_sigdata.name = line.split(maxsplit=3)[2:4]
            curr_sigs = good_sigs
            curr_sigdata.status = "expired"

        elif line_begins_with(r"REVKEYSIG(?:\s|$)", line):
            curr_sigdata.key, curr_sigdata.name = line.split(maxsplit=3)[2:4]
            curr_sigs = good_sigs
            curr_sigdata.status = "revoked"

        elif line_begins_with(r"BADSIG(?:\s|$)", line):
            curr_sigdata.key, curr_sigdata.name = line.split(maxsplit=3)[2:4]
            curr_sigs = bad_sigs

        elif line_begins_with(r"ERRSIG(?:\s|$)", line):
            curr_sigdata.key, _, _, _, _, _ = line.split()[2:8]
            curr_sigs = unknown_sigs

        elif line_begins_with(r"TRUST_(UNDEFINED|NEVER)(?:\s|$)", line):
            curr_sigdata.trusted = False

        elif line_begins_with(r"TRUST_(MARGINAL|FULLY|ULTIMATE)(?:\s|$)", line):
            curr_sigdata.trusted = True

    # The last one won't have been added, so add it now
    assert curr_sigdata
    curr_sigs.append(curr_sigdata)

    all_found = len(good_sigs + bad_sigs + unknown_sigs)
    if all_found != total_resolved_sigs:
        raise RuntimeError(
            f"failed to evaluate all signatures: found {all_found} "
            f"but expected {total_resolved_sigs}")

    return (good_sigs, unknown_sigs, bad_sigs)


def files_are_equal(filename1, filename2):
    with open(filename1, 'rb') as file1:
        contents1 = file1.read()
    with open(filename2, 'rb') as file2:
        contents2 = file2.read()
    eq = contents1 == contents2

    if not eq:
        with open(filename1, 'r') as f1, \
                open(filename2, 'r') as f2:
            f1lines = f1.readlines()
            f2lines = f2.readlines()

            diff = indent(
                ''.join(difflib.unified_diff(f1lines, f2lines)))
            log.warning(f"found diff in files ({filename1}, {filename2}):\n{diff}\n")

    return eq


def get_files_from_hosts_and_compare(
    hosts: list[str], path: str, filename: str, require_all: bool = False
) -> ReturnCode:
    """
    Retrieve the same file from a number of hosts and ensure they have the same contents.
    The first host given will be treated as the "primary" host, and is required to succeed.

    Args:
        filename: for writing the file locally.
    """
    assert len(hosts) > 1
    primary_host = hosts[0]
    other_hosts = hosts[1:]
    got_files = []

    def join_url(host: str) -> str:
        return host.rstrip('/') + '/' + path.lstrip('/')

    url = join_url(primary_host)
    success, output = download_with_wget(url, filename)
    if not success:
        log.error(
            f"couldn't fetch file ({url}). "
            "Have you specified the version number in the following format?\n"
            f"{VERSION_FORMAT} "
            f"(example: {VERSION_EXAMPLE})\n"
            f"wget output:\n{indent(output)}")
        return ReturnCode.FILE_GET_FAILED
    else:
        log.info(f"got file {url} as {filename}")
        got_files.append(filename)

    for i, host in enumerate(other_hosts):
        url = join_url(host)
        fname = filename + f'.{i + 2}'
        success, output = download_with_wget(url, fname)

        if require_all and not success:
            log.error(
                f"{host} failed to provide file ({url}), but {primary_host} did?\n"
                f"wget output:\n{indent(output)}")
            return ReturnCode.FILE_MISSING_FROM_ONE_HOST
        elif not success:
            log.warning(
                f"{host} failed to provide file ({url}). "
                f"Continuing based solely upon {primary_host}.")
        else:
            log.info(f"got file {url} as {fname}")
            got_files.append(fname)

    for i, got_file in enumerate(got_files):
        if got_file == got_files[-1]:
            break  # break on last file, nothing after it to compare to

        compare_to = got_files[i + 1]
        if not files_are_equal(got_file, compare_to):
            log.error(f"files not equal: {got_file} and {compare_to}")
            return ReturnCode.FILES_NOT_EQUAL

    return ReturnCode.SUCCESS


def check_multisig(sums_file: str, sigfilename: str, args: argparse.Namespace) -> tuple[int, str, list[SigData], list[SigData], list[SigData]]:
    # check signature
    #
    # We don't write output to a file because this command will almost certainly
    # fail with GPG exit code '2' (and so not writing to --output) because of the
    # likely presence of multiple untrusted signatures.
    retval, output = verify_with_gpg(sums_file, sigfilename)

    if args.verbose:
        log.info(f"gpg output:\n{indent(output)}")

    good, unknown, bad = parse_gpg_result(output.splitlines())

    if unknown and args.import_keys:
        # Retrieve unknown keys and then try GPG again.
        for unsig in unknown:
            if prompt_yn(f" ? Retrieve key {unsig.key} ({unsig.name})? (y/N) "):
                ran = subprocess.run(
                    ["gpg", "--keyserver", args.keyserver, "--recv-keys", unsig.key])

                if ran.returncode != 0:
                    log.warning(f"failed to retrieve key {unsig.key}")

        # Reparse the GPG output now that we have more keys
        retval, output = verify_with_gpg(sums_file, sigfilename)
        good, unknown, bad = parse_gpg_result(output.splitlines())

    return retval, output, good, unknown, bad


def prompt_yn(prompt) -> bool:
    """Return true if the user inputs 'y'."""
    got = ''
    while got not in ['y', 'n']:
        got = input(prompt).lower()
    return got == 'y'

def verify_shasums_signature(
    signature_file_path: str, sums_file_path: str, args: argparse.Namespace
) -> tuple[
   ReturnCode, list[SigData], list[SigData], list[SigData], list[SigData]
]:
    min_good_sigs = args.min_good_sigs
    gpg_allowed_codes = [0, 2]  # 2 is returned when untrusted signatures are present.

    gpg_retval, gpg_output, good, unknown, bad = check_multisig(sums_file_path, signature_file_path, args)

    if gpg_retval not in gpg_allowed_codes:
        if gpg_retval == 1:
            log.critical(f"Bad signature (code: {gpg_retval}).")
        else:
            log.critical(f"unexpected GPG exit code ({gpg_retval})")

        log.error(f"gpg output:\n{indent(gpg_output)}")
        return (ReturnCode.INTEGRITY_FAILURE, [], [], [], [])

    # Decide which keys we trust, though not "trust" in the GPG sense, but rather
    # which pubkeys convince us that this sums file is legitimate. In other words,
    # which pubkeys within the Bitcoin community do we trust for the purposes of
    # binary verification?
    trusted_keys = set()
    if args.trusted_keys:
        trusted_keys |= set(args.trusted_keys.split(','))

    # Tally signatures and make sure we have enough goods to fulfill
    # our threshold.
    good_trusted = [sig for sig in good if sig.trusted or sig.key in trusted_keys]
    good_untrusted = [sig for sig in good if sig not in good_trusted]
    num_trusted = len(good_trusted) + len(good_untrusted)
    log.info(f"got {num_trusted} good signatures")

    if num_trusted < min_good_sigs:
        log.info("Maybe you need to import "
                  f"(`gpg --keyserver {args.keyserver} --recv-keys <key-id>`) "
                  "some of the following keys: ")
        log.info('')
        for sig in unknown:
            log.info(f"    {sig.key} ({sig.name})")
        log.info('')
        log.error(
            "not enough trusted sigs to meet threshold "
            f"({num_trusted} vs. {min_good_sigs})")

        return (ReturnCode.NOT_ENOUGH_GOOD_SIGS, [], [], [], [])

    for sig in good_trusted:
        log.info(f"GOOD SIGNATURE: {sig}")

    for sig in good_untrusted:
        log.info(f"GOOD SIGNATURE (untrusted): {sig}")

    for sig in [sig for sig in good if sig.status == 'expired']:
        log.warning(f"key {sig.key} for {sig.name} is expired")

    for sig in bad:
        log.warning(f"BAD SIGNATURE: {sig}")

    for sig in unknown:
        log.warning(f"UNKNOWN SIGNATURE: {sig}")

    return (ReturnCode.SUCCESS, good_trusted, good_untrusted, unknown, bad)


def parse_sums_file(sums_file_path: str, filename_filter: list[str]) -> list[list[str]]:
    # extract hashes/filenames of binaries to verify from hash file;
    # each line has the following format: "<hash> <binary_filename>"
    with open(sums_file_path, 'r') as hash_file:
        return [line.split()[:2] for line in hash_file if len(filename_filter) == 0 or any(f in line for f in filename_filter)]


def verify_binary_hashes(hashes_to_verify: list[list[str]]) -> tuple[ReturnCode, dict[str, str]]:
    offending_files = []
    files_to_hashes = {}

    for hash_expected, binary_filename in hashes_to_verify:
        with open(binary_filename, 'rb') as binary_file:
            hash_calculated = sha256(binary_file.read()).hexdigest()
        if hash_calculated != hash_expected:
            offending_files.append(binary_filename)
        else:
            files_to_hashes[binary_filename] = hash_calculated

    if offending_files:
        joined_files = '\n'.join(offending_files)
        log.critical(
            "Hashes don't match.\n"
            f"Offending files:\n{joined_files}")
        return (ReturnCode.INTEGRITY_FAILURE, files_to_hashes)

    return (ReturnCode.SUCCESS, files_to_hashes)


def verify_published_handler(args: argparse.Namespace) -> ReturnCode:
    WORKINGDIR = Path(tempfile.gettempdir()) / f"bitcoin_verify_binaries.{args.version}"

    def cleanup():
        log.info("cleaning up files")
        os.chdir(Path.home())
        shutil.rmtree(WORKINGDIR)

    # determine remote dir dependent on provided version string
    try:
        version_base, version_rc, os_filter = parse_version_string(args.version)
        version_tuple = [int(i) for i in version_base.split('.')]
    except Exception as e:
        log.debug(e)
        log.error(f"unable to parse version; expected format is {VERSION_FORMAT}")
        log.error(f"  e.g. {VERSION_EXAMPLE}")
        return ReturnCode.BAD_VERSION

    remote_dir = f"/bin/{VERSIONPREFIX}{version_base}/"
    if version_rc:
        remote_dir += f"test.{version_rc}/"
    remote_sigs_path = remote_dir + SIGNATUREFILENAME
    remote_sums_path = remote_dir + SUMS_FILENAME

    # create working directory
    os.makedirs(WORKINGDIR, exist_ok=True)
    os.chdir(WORKINGDIR)

    hosts = [HOST1, HOST2]

    got_sig_status = get_files_from_hosts_and_compare(
        hosts, remote_sigs_path, SIGNATUREFILENAME, args.require_all_hosts)
    if got_sig_status != ReturnCode.SUCCESS:
        return got_sig_status

    # Multi-sig verification is available after 22.0.
    if version_tuple[0] < 22:
        log.error("Version too old - single sig not supported. Use a previous "
                  "version of this script from the repo.")
        return ReturnCode.BAD_VERSION

    got_sums_status = get_files_from_hosts_and_compare(
        hosts, remote_sums_path, SUMS_FILENAME, args.require_all_hosts)
    if got_sums_status != ReturnCode.SUCCESS:
        return got_sums_status

    # Verify the signature on the SHA256SUMS file
    sigs_status, good_trusted, good_untrusted, unknown, bad = verify_shasums_signature(SIGNATUREFILENAME, SUMS_FILENAME, args)
    if sigs_status != ReturnCode.SUCCESS:
        if sigs_status == ReturnCode.INTEGRITY_FAILURE:
            cleanup()
        return sigs_status

    # Extract hashes and filenames
    hashes_to_verify = parse_sums_file(SUMS_FILENAME, [os_filter])
    if not hashes_to_verify:
        available_versions = ["-".join(line[1].split("-")[2:]) for line in parse_sums_file(SUMS_FILENAME, [])]
        closest_match = difflib.get_close_matches(os_filter, available_versions, cutoff=0, n=1)[0]
        log.error(f"No files matched the platform specified. Did you mean: {closest_match}")
        return ReturnCode.NO_BINARIES_MATCH

    # remove binaries that are known not to be hosted by bitcoincore.org
    fragments_to_remove = ['-unsigned', '-debug', '-codesignatures']
    for fragment in fragments_to_remove:
        nobinaries = [i for i in hashes_to_verify if fragment in i[1]]
        if nobinaries:
            remove_str = ', '.join(i[1] for i in nobinaries)
            log.info(
                f"removing *{fragment} binaries ({remove_str}) from verification "
                f"since {HOST1} does not host *{fragment} binaries")
            hashes_to_verify = [i for i in hashes_to_verify if fragment not in i[1]]

    # download binaries
    for _, binary_filename in hashes_to_verify:
        log.info(f"downloading {binary_filename} to {WORKINGDIR}")
        success, output = download_with_wget(
            HOST1 + remote_dir + binary_filename, binary_filename)

        if not success:
            log.error(
                f"failed to download {binary_filename}\n"
                f"wget output:\n{indent(output)}")
            return ReturnCode.BINARY_DOWNLOAD_FAILED

    # verify hashes
    hashes_status, files_to_hashes = verify_binary_hashes(hashes_to_verify)
    if hashes_status != ReturnCode.SUCCESS:
        return hashes_status


    if args.cleanup:
        cleanup()
    else:
        log.info(f"did not clean up {WORKINGDIR}")

    if args.json:
        output = {
            'good_trusted_sigs': [str(s) for s in good_trusted],
            'good_untrusted_sigs': [str(s) for s in good_untrusted],
            'unknown_sigs': [str(s) for s in unknown],
            'bad_sigs': [str(s) for s in bad],
            'verified_binaries': files_to_hashes,
        }
        print(json.dumps(output, indent=2))
    else:
        for filename in files_to_hashes:
            print(f"VERIFIED: {filename}")

    return ReturnCode.SUCCESS


def verify_binaries_handler(args: argparse.Namespace) -> ReturnCode:
    binary_to_basename = {}
    for file in args.binary:
        binary_to_basename[PurePath(file).name] = file

    sums_sig_path = None
    if args.sums_sig_file:
        sums_sig_path = Path(args.sums_sig_file)
    else:
        log.info(f"No signature file specified, assuming it is {args.sums_file}.asc")
        sums_sig_path = Path(args.sums_file).with_suffix(".asc")

    # Verify the signature on the SHA256SUMS file
    sigs_status, good_trusted, good_untrusted, unknown, bad = verify_shasums_signature(str(sums_sig_path), args.sums_file, args)
    if sigs_status != ReturnCode.SUCCESS:
        return sigs_status

    # Extract hashes and filenames
    hashes_to_verify = parse_sums_file(args.sums_file, [k for k, n in binary_to_basename.items()])
    if not hashes_to_verify:
        log.error(f"No files in {args.sums_file} match the specified binaries")
        return ReturnCode.NO_BINARIES_MATCH

    # Make sure all files are accounted for
    sums_file_path = Path(args.sums_file)
    missing_files = []
    files_to_hash = []
    if len(binary_to_basename) > 0:
        for file_hash, file in hashes_to_verify:
            files_to_hash.append([file_hash, binary_to_basename[file]])
            del binary_to_basename[file]
        if len(binary_to_basename) > 0:
            log.error(f"Not all specified binaries are in {args.sums_file}")
            return ReturnCode.NO_BINARIES_MATCH
    else:
        log.info(f"No binaries specified, assuming all files specified in {args.sums_file} are located relatively")
        for file_hash, file in hashes_to_verify:
            file_path = Path(sums_file_path.parent.joinpath(file))
            if file_path.exists():
                files_to_hash.append([file_hash, str(file_path)])
            else:
                missing_files.append(file)

    # verify hashes
    hashes_status, files_to_hashes = verify_binary_hashes(files_to_hash)
    if hashes_status != ReturnCode.SUCCESS:
        return hashes_status

    if args.json:
        output = {
            'good_trusted_sigs': [str(s) for s in good_trusted],
            'good_untrusted_sigs': [str(s) for s in good_untrusted],
            'unknown_sigs': [str(s) for s in unknown],
            'bad_sigs': [str(s) for s in bad],
            'verified_binaries': files_to_hashes,
            "missing_binaries": missing_files,
        }
        print(json.dumps(output, indent=2))
    else:
        for filename in files_to_hashes:
            print(f"VERIFIED: {filename}")
        for filename in missing_files:
            print(f"MISSING: {filename}")

    return ReturnCode.SUCCESS


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        '-v', '--verbose', action='store_true',
        default=bool_from_env('BINVERIFY_VERBOSE'),
    )
    parser.add_argument(
        '-q', '--quiet', action='store_true',
        default=bool_from_env('BINVERIFY_QUIET'),
    )
    parser.add_argument(
        '--import-keys', action='store_true',
        default=bool_from_env('BINVERIFY_IMPORTKEYS'),
        help='if specified, ask to import each unknown builder key'
    )
    parser.add_argument(
        '--min-good-sigs', type=int, action='store', nargs='?',
        default=int(os.environ.get('BINVERIFY_MIN_GOOD_SIGS', 3)),
        help=(
            'The minimum number of good signatures to require successful termination.'),
    )
    parser.add_argument(
        '--keyserver', action='store', nargs='?',
        default=os.environ.get('BINVERIFY_KEYSERVER', 'hkps://keys.openpgp.org'),
        help='which keyserver to use',
    )
    parser.add_argument(
        '--trusted-keys', action='store', nargs='?',
        default=os.environ.get('BINVERIFY_TRUSTED_KEYS', ''),
        help='A list of trusted signer GPG keys, separated by commas. Not "trusted keys" in the GPG sense.',
    )
    parser.add_argument(
        '--json', action='store_true',
        default=bool_from_env('BINVERIFY_JSON'),
        help='If set, output the result as JSON',
    )

    subparsers = parser.add_subparsers(title="Commands", required=True, dest="command")

    pub_parser = subparsers.add_parser("pub", help="Verify a published release.")
    pub_parser.set_defaults(func=verify_published_handler)
    pub_parser.add_argument(
        'version', type=str, help=(
            f'version of the bitcoin release to download; of the format '
            f'{VERSION_FORMAT}. Example: {VERSION_EXAMPLE}')
    )
    pub_parser.add_argument(
        '--cleanup', action='store_true',
        default=bool_from_env('BINVERIFY_CLEANUP'),
        help='if specified, clean up files afterwards'
    )
    pub_parser.add_argument(
        '--require-all-hosts', action='store_true',
        default=bool_from_env('BINVERIFY_REQUIRE_ALL_HOSTS'),
        help=(
            f'If set, require all hosts ({HOST1}, {HOST2}) to provide signatures. '
            '(Sometimes bitcoin.org lags behind bitcoincore.org.)')
    )

    bin_parser = subparsers.add_parser("bin", help="Verify local binaries.")
    bin_parser.set_defaults(func=verify_binaries_handler)
    bin_parser.add_argument("--sums-sig-file", "-s", help="Path to the SHA256SUMS.asc file to verify")
    bin_parser.add_argument("sums_file", help="Path to the SHA256SUMS file to verify")
    bin_parser.add_argument(
        "binary", nargs="*",
        help="Path to a binary distribution file to verify. Can be specified multiple times for multiple files to verify."
    )

    args = parser.parse_args()
    if args.quiet:
        log.setLevel(logging.WARNING)

    return args.func(args)


if __name__ == '__main__':
    sys.exit(main())

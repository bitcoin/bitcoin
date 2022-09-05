#!/usr/bin/env python3
# Copyright (c) 2020-2021 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Script for verifying Syscoin Core release binaries

This script attempts to download the signature file SHA256SUMS.asc from
syscoincore.org and syscoin.org and compares them.
It first checks if the signature passes, and then downloads the files
specified in the file, and checks if the hashes of these files match those
that are specified in the signature file.
The script returns 0 if everything passes the checks. It returns 1 if either
the signature check or the hash check doesn't pass. If an error occurs the
return value is >= 2.
"""
from hashlib import sha256
import os
import subprocess
import sys
from textwrap import indent

WORKINGDIR = "/tmp/syscoin_verify_binaries"
HASHFILE = "hashes.tmp"
HOST1 = "https://syscoincore.org"
HOST2 = "https://syscoin.org"
VERSIONPREFIX = "syscoin-core-"
SIGNATUREFILENAME = "SHA256SUMS.asc"


def parse_version_string(version_str):
    if version_str.startswith(VERSIONPREFIX):  # remove version prefix
        version_str = version_str[len(VERSIONPREFIX):]

    parts = version_str.split('-')
    version_base = parts[0]
    version_rc = ""
    version_os = ""
    if len(parts) == 2:  # "<version>-rcN" or "version-platform"
        if "rc" in parts[1]:
            version_rc = parts[1]
        else:
            version_os = parts[1]
    elif len(parts) == 3:  # "<version>-rcN-platform"
        version_rc = parts[1]
        version_os = parts[2]

    return version_base, version_rc, version_os


def download_with_wget(remote_file, local_file=None):
    if local_file:
        wget_args = ['wget', '-O', local_file, remote_file]
    else:
        # use timestamping mechanism if local filename is not explicitly set
        wget_args = ['wget', '-N', remote_file]

    result = subprocess.run(wget_args,
                            stderr=subprocess.STDOUT, stdout=subprocess.PIPE)
    return result.returncode == 0, result.stdout.decode().rstrip()


def files_are_equal(filename1, filename2):
    with open(filename1, 'rb') as file1:
        contents1 = file1.read()
    with open(filename2, 'rb') as file2:
        contents2 = file2.read()
    return contents1 == contents2


def verify_with_gpg(signature_filename, output_filename):
    result = subprocess.run(['gpg', '--yes', '--decrypt', '--output',
                             output_filename, signature_filename],
                             stderr=subprocess.STDOUT, stdout=subprocess.PIPE)
    return result.returncode, result.stdout.decode().rstrip()


def remove_files(filenames):
    for filename in filenames:
        os.remove(filename)


def main(args):
    # sanity check
    if len(args) < 1:
        print("Error: need to specify a version on the command line")
        return 3

    # determine remote dir dependent on provided version string
    version_base, version_rc, os_filter = parse_version_string(args[0])
    remote_dir = f"/bin/{VERSIONPREFIX}{version_base}/"
    if version_rc:
        remote_dir += f"test.{version_rc}/"
    remote_sigfile = remote_dir + SIGNATUREFILENAME

    # create working directory
    os.makedirs(WORKINGDIR, exist_ok=True)
    os.chdir(WORKINGDIR)

    # fetch first signature file
    sigfile1 = SIGNATUREFILENAME
    success, output = download_with_wget(HOST1 + remote_sigfile, sigfile1)
    if not success:
        print("Error: couldn't fetch signature file. "
              "Have you specified the version number in the following format?")
        print(f"[{VERSIONPREFIX}]<version>[-rc[0-9]][-platform] "
              f"(example: {VERSIONPREFIX}0.21.0-rc3-osx)")
        print("wget output:")
        print(indent(output, '\t'))
        return 4

    # fetch second signature file
    sigfile2 = SIGNATUREFILENAME + ".2"
    success, output = download_with_wget(HOST2 + remote_sigfile, sigfile2)
    if not success:
        print("syscoin.org failed to provide signature file, "
              "but syscoincore.org did?")
        print("wget output:")
        print(indent(output, '\t'))
        remove_files([sigfile1])
        return 5

    # ensure that both signature files are equal
    if not files_are_equal(sigfile1, sigfile2):
        print("syscoin.org and syscoincore.org signature files were not equal?")
        print(f"See files {WORKINGDIR}/{sigfile1} and {WORKINGDIR}/{sigfile2}")
        return 6

    # check signature and extract data into file
    retval, output = verify_with_gpg(sigfile1, HASHFILE)
    if retval != 0:
        if retval == 1:
            print("Bad signature.")
        elif retval == 2:
            print("gpg error. Do you have the Syscoin Core binary release "
                  "signing key installed?")
        print("gpg output:")
        print(indent(output, '\t'))
        remove_files([sigfile1, sigfile2, HASHFILE])
        return 1

    # extract hashes/filenames of binaries to verify from hash file;
    # each line has the following format: "<hash> <binary_filename>"
    with open(HASHFILE, 'r', encoding='utf8') as hash_file:
        hashes_to_verify = [
            line.split()[:2] for line in hash_file if os_filter in line]
    remove_files([HASHFILE])
    if not hashes_to_verify:
        print("error: no files matched the platform specified")
        return 7

    # download binaries
    for _, binary_filename in hashes_to_verify:
        print(f"Downloading {binary_filename}")
        download_with_wget(HOST1 + remote_dir + binary_filename)

    # verify hashes
    offending_files = []
    for hash_expected, binary_filename in hashes_to_verify:
        with open(binary_filename, 'rb') as binary_file:
            hash_calculated = sha256(binary_file.read()).hexdigest()
        if hash_calculated != hash_expected:
            offending_files.append(binary_filename)
    if offending_files:
        print("Hashes don't match.")
        print("Offending files:")
        print('\n'.join(offending_files))
        return 1
    verified_binaries = [entry[1] for entry in hashes_to_verify]

    # clean up files if desired
    if len(args) >= 2:
        print("Clean up the binaries")
        remove_files([sigfile1, sigfile2] + verified_binaries)
    else:
        print(f"Keep the binaries in {WORKINGDIR}")

    print("Verified hashes of")
    print('\n'.join(verified_binaries))
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))

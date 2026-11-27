#!/usr/bin/env python3
# Copyright (c) The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit.

import json
import os
import sys
import subprocess
from pathlib import Path
from unittest import mock

import verify


def main():
    """Tests ordered roughly from faster to slower."""
    test_inactive_signature_quorum()
    expect_code(run_verify("", "pub", '0.32'), 4, "Nonexistent version should fail")
    expect_code(run_verify("", "pub", '0.32.awefa.12f9h'), 11, "Malformed version should fail")
    expect_code(run_verify('--min-good-sigs 20', "pub", "22.0"), 9, "--min-good-sigs 20 should fail")

    # With the default threshold, trust three 22.0 signers in GPG or list their verified fingerprints in BINVERIFY_TRUSTED_KEYS.
    print("- testing verification (22.0-x86_64-linux-gnu.tar.gz)", flush=True)
    _220_x86_64_linux_gnu = run_verify("--json", "pub", "22.0-x86_64-linux-gnu.tar.gz")
    try:
        result = json.loads(_220_x86_64_linux_gnu.stdout.decode())
    except Exception:
        print("failed on 22.0-x86_64-linux-gnu.tar.gz --json:")
        print_process_failure(_220_x86_64_linux_gnu)
        raise

    expect_code(_220_x86_64_linux_gnu, 0, "22.0-x86_64-linux-gnu.tar.gz should succeed")
    v = result['verified_binaries']
    assert result['good_trusted_sigs']
    assert isinstance(result['expired_sigs'], list)
    assert len(v) == 1
    assert v['bitcoin-22.0-x86_64-linux-gnu.tar.gz'] == '59ebd25dd82a51638b7a6bb914586201e67db67b919b2a1ff08925a7936d1b16'

    print("- testing verification (22.0)", flush=True)
    _220 = run_verify("--json", "pub", "22.0")
    try:
        result = json.loads(_220.stdout.decode())
    except Exception:
        print("failed on 22.0 --json:")
        print_process_failure(_220)
        raise

    expect_code(_220, 0, "22.0 should succeed")
    v = result['verified_binaries']
    assert result['good_trusted_sigs']
    assert v['bitcoin-22.0-aarch64-linux-gnu.tar.gz'] == 'ac718fed08570a81b3587587872ad85a25173afa5f9fbbd0c03ba4d1714cfa3e'
    assert v['bitcoin-22.0-osx64.tar.gz'] == '2744d199c3343b2d94faffdfb2c94d75a630ba27301a70e47b0ad30a7e0155e9'
    assert v['bitcoin-22.0-x86_64-linux-gnu.tar.gz'] == '59ebd25dd82a51638b7a6bb914586201e67db67b919b2a1ff08925a7936d1b16'


@mock.patch.object(verify.log, 'disabled', new=True)
def test_inactive_signature_quorum():
    active = 'GOODSIG AAAABBBBCCCCDDDD Active Builder'
    expired = 'EXPKEYSIG 1111222233334444 Expired Builder'
    revoked = 'REVKEYSIG 5555666677778888 Revoked Builder'
    revoked_expired = 'EXPKEYSIG 9999AAAABBBBCCCC Revoked Expired Builder\n[GNUPG:] KEYREVOKED'

    def gpg_output(*sigs: str) -> str:
        return '\n'.join(f'[GNUPG:] NEWSIG\n[GNUPG:] {sig}' for sig in sigs)

    # Revocation takes precedence regardless of the order GnuPG reports the statuses in
    for sig in (revoked_expired, f'KEYREVOKED\n[GNUPG:] {expired}'):
        assert verify.parse_gpg_result(gpg_output(sig).splitlines())[0][0].status == 'revoked'

    def verify_fake_gpg(parser, options: list[str], *sigs: str, min_good_sigs: int = 1):
        args = parser.parse_args([
            *options,
            '--min-good-sigs', str(min_good_sigs),
            'bin', verify.SUMS_FILENAME,
        ])
        with mock.patch.object(verify, 'verify_with_gpg', return_value=(0, gpg_output(*sigs))):
            return verify.verify_shasums_signature(verify.SIGNATUREFILENAME, verify.SUMS_FILENAME, args)

    # Every key is explicitly trusted, so only its inactive status can exclude it
    trusted_keys = ','.join(sig.split()[1] for sig in (active, expired, revoked, revoked_expired))
    for env, options, allow_expired in (
        ({}, [], False),
        ({}, ['--allow-expired'], True),
        ({'BINVERIFY_ALLOW_EXPIRED': '0'}, [], False),
        ({'BINVERIFY_ALLOW_EXPIRED': '0'}, ['--allow-expired'], True),
        ({'BINVERIFY_ALLOW_EXPIRED': '1'}, [], True),
        ({'BINVERIFY_ALLOW_EXPIRED': '1'}, ['--allow-expired'], True),
    ):
        with mock.patch.dict(os.environ, env, clear=True):
            parser = verify.build_parser()

        options = [*options, '--trusted-keys', trusted_keys]
        result, good_trusted, good_untrusted, _, _, expired_sigs = verify_fake_gpg(parser, options, expired)
        assert result == (verify.ReturnCode.SUCCESS if allow_expired else verify.ReturnCode.NOT_ENOUGH_GOOD_SIGS)
        assert good_trusted == []
        assert good_untrusted == []
        assert [sig.key for sig in expired_sigs] == ['1111222233334444']

        result, *_, expired_sigs = verify_fake_gpg(parser, options, revoked)
        assert result == verify.ReturnCode.NOT_ENOUGH_GOOD_SIGS
        assert expired_sigs == []

        result, *_, expired_sigs = verify_fake_gpg(parser, options, revoked_expired)
        assert result == verify.ReturnCode.NOT_ENOUGH_GOOD_SIGS
        assert expired_sigs == []

        result, good_trusted, good_untrusted, _, _, expired_sigs = verify_fake_gpg(parser, options, active, expired, revoked)
        assert result == verify.ReturnCode.SUCCESS
        assert [sig.key for sig in good_trusted] == ['AAAABBBBCCCCDDDD']
        assert good_untrusted == []
        assert [str(sig) for sig in expired_sigs] == [
            "SigData('1111222233334444', 'Expired Builder', trusted=False, status='expired')"]

        result, *_ = verify_fake_gpg(parser, options, active, expired, revoked, min_good_sigs=2)
        assert result == (verify.ReturnCode.SUCCESS if allow_expired else verify.ReturnCode.NOT_ENOUGH_GOOD_SIGS)

        result, *_ = verify_fake_gpg(parser, options, active, expired, revoked, min_good_sigs=3)
        assert result == verify.ReturnCode.NOT_ENOUGH_GOOD_SIGS

    primary = 'A' * 40
    subkey = 'B' * 24 + active.split()[1]
    other_subkey = 'C' * 24 + expired.split()[1]

    def validsig(fingerprint, primary_key=''):
        return f'\n[GNUPG:] VALIDSIG {fingerprint} 2026-01-01 1 0 4 0 1 8 00 {primary_key}'

    active += validsig(subkey, primary)
    expired += validsig(other_subkey, primary)
    with mock.patch.dict(os.environ, {'BINVERIFY_ALLOW_EXPIRED': '1'}, clear=True):
        parser = verify.build_parser()

    # Two copies of the active signature and the expired signing subkey share one primary.
    undefined = active + '\n[GNUPG:] TRUST_UNDEFINED 0 pgp'
    sigs = (undefined, undefined, expired)
    for options, threshold, expected in (
        ([], 1, verify.ReturnCode.NOT_ENOUGH_GOOD_SIGS),
        (['--trusted-keys'], 1, verify.ReturnCode.NOT_ENOUGH_GOOD_SIGS),
        (['--trusted-keys', ','], 1, verify.ReturnCode.NOT_ENOUGH_GOOD_SIGS),
        (['--trusted-keys', 'D' * 40], 1, verify.ReturnCode.NOT_ENOUGH_GOOD_SIGS),
        (['--trusted-keys', f'{primary[:20]}\t{primary[20:]}'.lower()], 1, verify.ReturnCode.SUCCESS),
        (['--trusted-keys', f'{subkey},'], 1, verify.ReturnCode.SUCCESS),
        (['--trusted-keys', f'{subkey}, {other_subkey}'], 2, verify.ReturnCode.NOT_ENOUGH_GOOD_SIGS),
    ):
        result, _, good_untrusted, *_ = verify_fake_gpg(parser, options, *sigs, min_good_sigs=threshold)
        assert result == expected
        assert not any(sig.status for sig in good_untrusted)

    # Trust just the expired subkey. Its active sibling stays untrusted.
    active += '\n[GNUPG:] TRUST_NEVER 0 pgp'
    local_key = '1' * 40
    gpg_trusted = f'GOODSIG {local_key[-16:]} Other Builder' + validsig(local_key) + '\n[GNUPG:] TRUST_FULLY 0 pgp'
    result, good_trusted, good_untrusted, _, _, expired_sigs = verify_fake_gpg(
        parser, ['--trusted-keys', other_subkey], active, expired, gpg_trusted, min_good_sigs=2)
    assert result == verify.ReturnCode.SUCCESS
    assert [sig.primary_fingerprint for sig in good_trusted] == [local_key]
    assert [sig.key for sig in good_untrusted] == [subkey[-16:]]
    assert [sig.key for sig in expired_sigs] == [other_subkey[-16:]]

    print("✓ 'Inactive signature quorum' passed")


def run_verify(global_args: str, command: str, command_args: str) -> subprocess.CompletedProcess:
    maybe_here = Path.cwd() / 'verify.py'
    path = maybe_here if maybe_here.exists() else Path.cwd() / 'contrib' / 'verify-binaries' / 'verify.py'

    if command == "pub":
        command += " --cleanup"

    return subprocess.run(
        f"{path} {global_args} {command} {command_args}",
        stdout=subprocess.PIPE, stderr=subprocess.PIPE, shell=True)


def expect_code(completed: subprocess.CompletedProcess, expected_code: int, msg: str):
    if completed.returncode != expected_code:
        print(f"{msg!r} failed: got code {completed.returncode}, expected {expected_code}")
        print_process_failure(completed)
        sys.exit(1)
    else:
        print(f"✓ {msg!r} passed")


def print_process_failure(completed: subprocess.CompletedProcess):
    print(f"stdout:\n{completed.stdout.decode()}")
    print(f"stderr:\n{completed.stderr.decode()}")


if __name__ == '__main__':
    main()

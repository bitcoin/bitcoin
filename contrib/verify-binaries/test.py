#!/usr/bin/env python3

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

    def verify_fake_gpg(parser, options: list[str], *sigs: str, min_good_sigs: int = 1):
        # Every key is explicitly trusted, so only its inactive status can exclude it
        args, _ = parser.parse_known_args([
            *options,
            '--min-good-sigs', str(min_good_sigs),
            '--trusted-keys', ','.join(sig.split()[1] for sig in (active, expired, revoked)),
            'bin', verify.SUMS_FILENAME,
        ])
        output = '\n'.join(f'[GNUPG:] NEWSIG\n[GNUPG:] {sig}' for sig in sigs)
        with mock.patch.object(verify, 'verify_with_gpg', return_value=(0, output)):
            return verify.verify_shasums_signature(verify.SIGNATUREFILENAME, verify.SUMS_FILENAME, args)

    # Check option recognition and environment defaults for both commands
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
        for command in (['pub', '22.0'], ['bin', verify.SUMS_FILENAME]):
            args, unknown_options = parser.parse_known_args([*options, *command])
            assert unknown_options == []
            assert getattr(args, 'allow_expired', False) == allow_expired

        result, good_trusted, good_untrusted, _, _, expired_sigs = verify_fake_gpg(parser, options, expired)
        assert result == (verify.ReturnCode.SUCCESS if allow_expired else verify.ReturnCode.NOT_ENOUGH_GOOD_SIGS)
        assert [sig.key for sig in good_trusted] == []
        assert good_untrusted == []
        assert [sig.key for sig in expired_sigs] == ['1111222233334444']

        result, *_, expired_sigs = verify_fake_gpg(parser, options, revoked)
        assert result == verify.ReturnCode.NOT_ENOUGH_GOOD_SIGS
        assert expired_sigs == []

        result, good_trusted, good_untrusted, _, _, expired_sigs = verify_fake_gpg(parser, options, active, expired, revoked)
        assert result == verify.ReturnCode.SUCCESS
        assert [sig.key for sig in good_trusted] == ['AAAABBBBCCCCDDDD']
        assert good_untrusted == []
        assert [sig.key for sig in expired_sigs] == ['1111222233334444']

        result, *_ = verify_fake_gpg(parser, options, active, expired, revoked, min_good_sigs=2)
        assert result == (verify.ReturnCode.SUCCESS if allow_expired else verify.ReturnCode.NOT_ENOUGH_GOOD_SIGS)

        result, *_ = verify_fake_gpg(parser, options, active, expired, revoked, min_good_sigs=3)
        assert result == verify.ReturnCode.NOT_ENOUGH_GOOD_SIGS

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

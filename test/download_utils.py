#!/usr/bin/env python3
#
# Copyright (c) The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit/.

import argparse
import hashlib
import json
import os
import re
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import time
import urllib.request


QA_MANIFEST = "qa-assets.json"
QA_LICENSE = "QA-ASSETS-LICENSE"


def file_sha256(path):
    digest = hashlib.sha256()
    with path.open('rb') as data:
        for chunk in iter(lambda: data.read(1024 * 1024), b''):
            digest.update(chunk)
    return digest.hexdigest()


def script_assets_valid(path):
    if path.is_symlink() or not path.is_file():
        raise RuntimeError(f"Missing or incorrect script test assets: {path}")
    try:
        with path.open(encoding='utf-8') as data:
            valid = isinstance(json.load(data), list)
    except (OSError, ValueError) as error:
        raise RuntimeError(f"Invalid script test assets: {path}") from error
    if not valid:
        raise RuntimeError(f"Invalid script test assets: {path}")


def qa_files(directory):
    if directory.is_symlink() or not directory.is_dir():
        raise RuntimeError(f"Missing or unsafe test asset directory: {directory}")
    files = {}
    for path in sorted(directory.rglob('*')):
        if path.is_symlink():
            raise RuntimeError(f"Unexpected symlink in test assets: {path}")
        if path.is_file() and path != directory / QA_MANIFEST:
            files[path.relative_to(directory).as_posix()] = file_sha256(path)
    if not files or set(files) == {QA_LICENSE}:
        raise RuntimeError(f"Empty test assets: {directory}")
    return files


def prepare_qa_assets(root, kind):
    revision = subprocess.check_output(['git', '-C', str(root), 'rev-parse', 'HEAD'], text=True).strip()
    if not re.fullmatch(r'[0-9a-f]{40}', revision):
        raise RuntimeError(f"Unexpected qa-assets revision: {revision}")
    directory = root / kind
    if kind == 'unit_test_data':
        script_assets_valid(directory / 'script_assets_test.json')
    files = qa_files(directory)
    shutil.copyfile(root / 'LICENSE', directory / QA_LICENSE)
    files[QA_LICENSE] = file_sha256(directory / QA_LICENSE)
    manifest = {'revision': revision, 'files': files}
    (directory / QA_MANIFEST).write_text(json.dumps(manifest, sort_keys=True) + '\n', encoding='utf-8')
    print(f"Prepared {kind}: {len(manifest['files'])} files from {revision}")


def verify_qa_assets(root, kind):
    directory = root / kind
    manifest_path = directory / QA_MANIFEST
    if manifest_path.is_symlink():
        raise RuntimeError(f"Unexpected test asset manifest symlink: {manifest_path}")
    manifest = json.loads(manifest_path.read_text(encoding='utf-8'))
    revision = manifest.get('revision', '')
    expected = os.environ.get('QA_ASSETS_REVISION')
    if not re.fullmatch(r'[0-9a-f]{40}', revision) or (expected and revision != expected) or manifest.get('files') != qa_files(directory):
        raise RuntimeError(f"Incomplete or corrupt prepared test assets: {directory}")
    if kind == 'unit_test_data':
        script_assets_valid(directory / 'script_assets_test.json')
    print(f"Verified {kind} from {revision}")
    return manifest['files']


def download_from_url(url, archive):
    print(f"Fetching: {url}")
    last_print_time = time.time()

    def progress_hook(progress_bytes, total_size):
        nonlocal last_print_time
        now = time.time()
        percent = min(100, (progress_bytes * 100) / total_size)
        bar_length = 40
        filled_length = int(bar_length * percent / 100)
        bar = '#' * filled_length + '-' * (bar_length - filled_length)
        if now - last_print_time >= 1 or percent >= 100:
            print(f'\rDownloading: [{bar}] {percent:.1f}%', flush=True, end="")
            last_print_time = now

    with urllib.request.urlopen(url) as response:
        if response.status != 200:
            raise RuntimeError(f"HTTP request failed with status code: {response.status}")

        sock_info = response.fp.raw._sock.getpeername()
        print(f"Connected to {sock_info[0]}")

        total_size = int(response.getheader("Content-Length"))
        progress_bytes = 0

        with open(archive, 'wb') as file:
            while True:
                chunk = response.read(8192)
                if not chunk:
                    break
                file.write(chunk)
                progress_bytes += len(chunk)
                progress_hook(progress_bytes, total_size)

        if progress_bytes < total_size:
            raise RuntimeError(f"Download incomplete: expected {total_size} bytes, got {progress_bytes} bytes")

    print('\n', flush=True, end="") # Flush to avoid error output on the same line.


def download_script_assets(script_assets_dir):
    prepared = os.environ.get('QA_ASSETS_DIR')
    if prepared:
        verify_qa_assets(Path(prepared), 'unit_test_data')
        source = Path(prepared) / 'unit_test_data' / 'script_assets_test.json'
        destination = script_assets_dir / source.name
        if source.resolve() != destination.resolve():
            script_assets_dir.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(source, destination)
        return
    script_assets_dir.mkdir(parents=True, exist_ok=True)
    script_assets = script_assets_dir / "script_assets_test.json"
    if script_assets.exists():
        script_assets_valid(script_assets)
        return
    url = "https://raw.githubusercontent.com/bitcoin-core/qa-assets/main/unit_test_data/script_assets_test.json"
    # Keep a failed transfer out of the path consumed by unit tests.
    with tempfile.TemporaryDirectory(dir=script_assets_dir) as staging:
        temporary_path = Path(staging) / script_assets.name
        try:
            download_from_url(url, temporary_path)
        except Exception as error:
            print(f"\nDownload failed: {error}\nRetrying download after failure ...", file=sys.stderr)
            time.sleep(12)
            download_from_url(url, temporary_path)
        script_assets_valid(temporary_path)
        temporary_path.replace(script_assets)


def download_fuzz_assets(root):
    prepared = os.environ.get('QA_ASSETS_DIR')
    if prepared:
        source = Path(prepared) / 'fuzz_corpora'
        files = verify_qa_assets(Path(prepared), 'fuzz_corpora')
        destination = root / 'fuzz_corpora'
        if destination.resolve() == source.resolve():
            raise RuntimeError('Fuzz working corpus must differ from shared inputs')
        if destination.exists() or destination.is_symlink():
            if qa_files(destination) == files:
                return
            raise RuntimeError(f'Working corpus differs from prepared inputs; preserve it and choose a fresh directory: {destination}')
        root.mkdir(parents=True, exist_ok=True)
        with tempfile.TemporaryDirectory(dir=root, prefix='.fuzz-corpora-') as staging:
            working = Path(staging) / 'fuzz_corpora'
            working.mkdir()
            for name in files:
                target = working / name
                target.parent.mkdir(parents=True, exist_ok=True)
                # Do not copy read-only modes: fuzzers create target dirs and seeds.
                shutil.copyfile(source / name, target)
            if qa_files(working) != files:
                raise RuntimeError('Working corpus copy failed verification')
            working.rename(destination)
        return
    if not (root / 'fuzz_corpora').is_dir():
        subprocess.run(['git', 'clone', '--depth=1', 'https://github.com/bitcoin-core/qa-assets', str(root)], check=True)
    subprocess.run(['git', '-C', str(root), 'log', '-1'], check=True)


def main():
    parser = argparse.ArgumentParser(description='Prepare, verify or reuse shared qa-assets inputs.')
    parser.add_argument('command', choices=['prepare', 'verify', 'unit', 'fuzz'])
    parser.add_argument('directory', nargs='?', type=Path)
    parser.add_argument('kind', nargs='?', choices=['unit_test_data', 'fuzz_corpora'])
    args = parser.parse_args()
    if args.directory is None or (args.command in ('prepare', 'verify') and args.kind is None):
        parser.error('directory and, for prepare/verify, kind are required')
    if args.command == 'prepare':
        prepare_qa_assets(args.directory, args.kind)
    elif args.command == 'verify':
        verify_qa_assets(args.directory, args.kind)
    elif args.command == 'unit':
        download_script_assets(args.directory)
    else:
        download_fuzz_assets(args.directory)


if __name__ == '__main__':
    main()

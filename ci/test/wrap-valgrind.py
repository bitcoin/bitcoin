#!/usr/bin/env python3
# Copyright (c) The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit/.

import os
import shlex
from pathlib import Path


def main():
    base_root = Path(os.environ["BASE_ROOT_DIR"])
    base_out = Path(os.environ["BASE_OUTDIR"])
    suppressions_file = base_root / "test" / "sanitizer_suppressions" / "valgrind.supp"
    target_names = {b.name for b in (base_out / "bin").iterdir()}

    for exe in base_root.rglob("*"):
        if exe.name in target_names and exe.is_file() and os.access(exe, os.X_OK):
            print(f"Wrap {exe} ...")
            original_path = exe.with_name(f"{exe.name}_orig")
            exe.rename(original_path)
            exe.write_text(
                "#!/usr/bin/env bash\n"
                "exec valgrind --gen-suppressions=all --quiet --error-exitcode=1 "
                f"--suppressions={shlex.quote(str(suppressions_file))} "
                f'{shlex.quote(str(original_path))} "$@"\n'
            )
            exe.chmod(exe.stat().st_mode | 0o111)


if __name__ == "__main__":
    main()

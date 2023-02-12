#!/usr/bin/env bash
#
# Copyright (c) 2020-2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C.UTF-8

for b_name in {"${BASE_OUTDIR}/bin"/*,src/secp256k1/*tests,src/minisketch/test{,-verify},src/univalue/{test_json,unitester,object}}.exe; do
    # shellcheck disable=SC2044
    for b in $(find "${BASE_ROOT_DIR}" -executable -type f -name "$(basename "$b_name")"); do
      if (file "$b" | grep "Windows"); then
        echo "Wrap $b ..."
        mv "$b" "${b}_orig"
        echo '#!/usr/bin/env bash' > "$b"
        echo "( wine \"${b}_orig\" \"\$@\" ) || ( sleep 1 && wine \"${b}_orig\" \"\$@\" )" >> "$b"
        chmod +x "$b"
      fi
    done
done

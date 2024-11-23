#!/usr/bin/env bash
#
# Copyright (c) 2018-2019 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C.UTF-8

for b_name in "${BASE_OUTDIR}/bin"/*; do
    # shellcheck disable=SC2044
    for b in $(find "${BASE_ROOT_DIR}" -executable -type f -name "$(basename "$b_name")"); do
      echo "Wrap $b ..."
      mv "$b" "${b}_orig"
      echo '#!/usr/bin/env bash' > "$b"
      echo "valgrind --gen-suppressions=all --quiet --error-exitcode=1 --suppressions=${BASE_ROOT_DIR}/contrib/valgrind.supp \"${b}_orig\" \"\$@\"" >> "$b"
      chmod +x "$b"
    done
done

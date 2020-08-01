#!/bin/bash
# Shebang must use python3 (not python or python2)

export LC_ALL=C
EXIT_CODE=0
for PYTHON_FILE in $(git ls-files -- "*.py"); do
    if [[ $(head -c 2 "${PYTHON_FILE}") == "#!" &&
          $(head -n 1 "${PYTHON_FILE}") != "#!/usr/bin/env python3" ]]; then
        echo "Missing shebang \"#!/usr/bin/env python3\" in ${PYTHON_FILE} (do not use python or python2)"
        EXIT_CODE=1
    fi
done
exit ${EXIT_CODE}

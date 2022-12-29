#!/bin/sh

YARN=yarn
if which yarn >/dev/null ; then
    YARN=yarn
else
    if which npm >/dev/null ; then
        YARN=npm
    else
        echo "No yarn or npm installed."
        exit 1
    fi
fi

cd js_build/js-bindings/tests && ${YARN} install && exec node ./test.js

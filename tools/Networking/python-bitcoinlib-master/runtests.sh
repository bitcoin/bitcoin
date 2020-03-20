#!/usr/bin/env sh
#-*-mode: sh; encoding: utf-8-*-

_MY_DIR="$( cd "$( dirname "${0}" )" && pwd )"
set -ex
[ -d "${_MY_DIR}" ]
[ "${_MY_DIR}/runtests.sh" -ef "${0}" ]
cd "${_MY_DIR}"
exec tox ${1+"${@}"}

#!/usr/bin/env bash
#
# Copyright (c) 2018-2019 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C.UTF-8

FOLD_COMMAND=""
if [[ ${TRAVIS} == "true" ]]; then
  FOLD_COMMAND="travis_fold:"
fi

BEGIN_FOLD () {
  echo ""
  CURRENT_FOLD_NAME=$1
  echo "${FOLD_COMMAND}start:${CURRENT_FOLD_NAME}"
}

END_FOLD () {
  RET=$?
  echo "${FOLD_COMMAND}end:${CURRENT_FOLD_NAME}"
  if [ $RET != 0 ]; then
    echo "${CURRENT_FOLD_NAME} failed with status code ${RET}"
  fi
}


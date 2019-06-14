#!/usr/bin/env bash
#
# Copyright (c) 2018-2019 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C

if [ "$TRAVIS_OS_NAME" == "osx" ]; then
  # update first to install required ruby dependency
  travis_retry brew update
  travis_retry brew reinstall git -- --with-pcre2 # for  --perl-regexp
  travis_retry brew install grep # gnu grep for --perl-regexp support
  PATH="$(brew --prefix grep)/libexec/gnubin:$PATH"
  travis_retry brew install shellcheck
  travis_retry brew upgrade python
  PATH="$(brew --prefix python)/bin:$PATH"
  export PATH
else
  SHELLCHECK_VERSION=v0.7.1
  curl -sL "https://github.com/koalaman/shellcheck/releases/download/${SHELLCHECK_VERSION}/shellcheck-${SHELLCHECK_VERSION}.linux.x86_64.tar.xz" | tar --xz -xf - --directory /tmp/
  PATH="/tmp/shellcheck-${SHELLCHECK_VERSION}:${PATH}"
  export PATH
fi

travis_retry pip3 install codespell==1.17.1
travis_retry pip3 install flake8==3.8.3
travis_retry pip3 install vulture==2.3
travis_retry pip3 install yq

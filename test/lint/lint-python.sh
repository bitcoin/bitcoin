#!/usr/bin/env bash
#
# Copyright (c) 2017-2021 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
#
# Check for specified flake8 warnings in python files.

export LC_ALL=C
export MYPY_CACHE_DIR="${BASE_ROOT_DIR}/test/.mypy_cache"

enabled=(
    E101 # indentation contains mixed spaces and tabs
    E112 # expected an indented block
    E113 # unexpected indentation
    E115 # expected an indented block (comment)
    E116 # unexpected indentation (comment)
    E125 # continuation line with same indent as next logical line
    E129 # visually indented line with same indent as next logical line
    E131 # continuation line unaligned for hanging indent
    E133 # closing bracket is missing indentation
    E223 # tab before operator
    E224 # tab after operator
    E242 # tab after ','
    E266 # too many leading '#' for block comment
    E271 # multiple spaces after keyword
    E272 # multiple spaces before keyword
    E273 # tab after keyword
    E274 # tab before keyword
    E275 # missing whitespace after keyword
    E304 # blank lines found after function decorator
    E306 # expected 1 blank line before a nested definition
    E401 # multiple imports on one line
    E402 # module level import not at top of file
    E502 # the backslash is redundant between brackets
    E701 # multiple statements on one line (colon)
    E702 # multiple statements on one line (semicolon)
    E703 # statement ends with a semicolon
    E711 # comparison to None should be 'if cond is None:'
    E714 # test for object identity should be "is not"
    E721 # do not compare types, use "isinstance()"
    E742 # do not define classes named "l", "O", or "I"
    E743 # do not define functions named "l", "O", or "I"
    E901 # SyntaxError: invalid syntax
    E902 # TokenError: EOF in multi-line string
    F401 # module imported but unused
    F402 # import module from line N shadowed by loop variable
    F403 # 'from foo_module import *' used; unable to detect undefined names
    F404 # future import(s) name after other statements
    F405 # foo_function may be undefined, or defined from star imports: bar_module
    F406 # "from module import *" only allowed at module level
    F407 # an undefined __future__ feature name was imported
    F601 # dictionary key name repeated with different values
    F602 # dictionary key variable name repeated with different values
    F621 # too many expressions in an assignment with star-unpacking
    F622 # two or more starred expressions in an assignment (a, *b, *c = d)
    F631 # assertion test is a tuple, which are always True
    F632 # use ==/!= to compare str, bytes, and int literals
    F701 # a break statement outside of a while or for loop
    F702 # a continue statement outside of a while or for loop
    F703 # a continue statement in a finally block in a loop
    F704 # a yield or yield from statement outside of a function
    F705 # a return statement with arguments inside a generator
    F706 # a return statement outside of a function/method
    F707 # an except: block as not the last exception handler
    F811 # redefinition of unused name from line N
    F812 # list comprehension redefines 'foo' from line N
    F821 # undefined name 'Foo'
    F822 # undefined name name in __all__
    F823 # local variable name … referenced before assignment
    F831 # duplicate argument name in function definition
    F841 # local variable 'foo' is assigned to but never used
    W191 # indentation contains tabs
    W291 # trailing whitespace
    W292 # no newline at end of file
    W293 # blank line contains whitespace
    W601 # .has_key() is deprecated, use "in"
    W602 # deprecated form of raising exception
    W603 # "<>" is deprecated, use "!="
    W604 # backticks are deprecated, use "repr()"
    W605 # invalid escape sequence "x"
    W606 # 'async' and 'await' are reserved keywords starting with Python 3.7
)

if ! command -v flake8 > /dev/null; then
    echo "Skipping Python linting since flake8 is not installed."
    exit 0
elif PYTHONWARNINGS="ignore" flake8 --version | grep -q "Python 2"; then
    echo "Skipping Python linting since flake8 is running under Python 2. Install the Python 3 version of flake8."
    exit 0
fi

EXIT_CODE=0

# shellcheck disable=SC2046
if ! PYTHONWARNINGS="ignore" flake8 --ignore=B,C,E,F,I,N,W --select=$(IFS=","; echo "${enabled[*]}") $(
    if [[ $# == 0 ]]; then
        git ls-files "*.py"
    else
        echo "$@"
    fi
); then
    EXIT_CODE=1
fi

mapfile -t FILES < <(git ls-files "test/functional/*.py" "contrib/devtools/*.py")
if ! mypy --show-error-codes "${FILES[@]}"; then
    EXIT_CODE=1
fi

exit $EXIT_CODE

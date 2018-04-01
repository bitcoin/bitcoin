#!/bin/sh
#
# Copyright (c) 2017 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
#
# Check for specified flake8 warnings in python files.

export LC_ALL=C

# E101 indentation contains mixed spaces and tabs
# E112 expected an indented block
# E113 unexpected indentation
# E115 expected an indented block (comment)
# E116 unexpected indentation (comment)
# E125 continuation line with same indent as next logical line
# E129 visually indented line with same indent as next logical line
# E131 continuation line unaligned for hanging indent
# E133 closing bracket is missing indentation
# E223 tab before operator
# E224 tab after operator
# E242 tab after ','
# E266 too many leading '#' for block comment
# E271 multiple spaces after keyword
# E272 multiple spaces before keyword
# E273 tab after keyword
# E274 tab before keyword
# E275 missing whitespace after keyword
# E304 blank lines found after function decorator
# E306 expected 1 blank line before a nested definition
# E401 multiple imports on one line
# E402 module level import not at top of file
# E502 the backslash is redundant between brackets
# E701 multiple statements on one line (colon)
# E702 multiple statements on one line (semicolon)
# E703 statement ends with a semicolon
# E714 test for object identity should be "is not"
# E721 do not compare types, use "isinstance()"
# E741 do not use variables named "l", "O", or "I" # disabled
# E742 do not define classes named "l", "O", or "I"
# E743 do not define functions named "l", "O", or "I"
# E901 SyntaxError: invalid syntax
# E902 TokenError: EOF in multi-line string
# F401 module imported but unused
# F402 import module from line N shadowed by loop variable
# F404 future import(s) name after other statements
# F406 "from module import *" only allowed at module level
# F407 an undefined __future__ feature name was imported
# F601 dictionary key name repeated with different values
# F602 dictionary key variable name repeated with different values
# F621 too many expressions in an assignment with star-unpacking
# F622 two or more starred expressions in an assignment (a, *b, *c = d)
# F631 assertion test is a tuple, which are always True
# F701 a break statement outside of a while or for loop
# F702 a continue statement outside of a while or for loop
# F703 a continue statement in a finally block in a loop
# F704 a yield or yield from statement outside of a function
# F705 a return statement with arguments inside a generator
# F706 a return statement outside of a function/method
# F707 an except: block as not the last exception handler
# F811 redefinition of unused name from line N
# F812 list comprehension redefines 'foo' from line N
# F821 undefined name 'Foo'
# F822 undefined name name in __all__
# F823 local variable name … referenced before assignment
# F831 duplicate argument name in function definition
# F841 local variable 'foo' is assigned to but never used
# W191 indentation contains tabs
# W291 trailing whitespace
# W292 no newline at end of file
# W293 blank line contains whitespace
# W504 line break after binary operator # disabled
# W601 .has_key() is deprecated, use "in"
# W602 deprecated form of raising exception
# W603 "<>" is deprecated, use "!="
# W604 backticks are deprecated, use "repr()"
# W605 invalid escape sequence "x" # disabled
# W606 'async' and 'await' are reserved keywords starting with Python 3.7

if ! command -v flake8 > /dev/null; then
    echo "Skipping Python linting since flake8 is not installed. Install by running \"pip3 install flake8\""
    exit 0
elif PYTHONWARNINGS="ignore" flake8 --version | grep -q "Python 2"; then
    echo "Skipping Python linting since flake8 is running under Python 2. Install the Python 3 version of flake8 by running \"pip3 install flake8\""
    exit 0
fi

PYTHONWARNINGS="ignore" git ls-files "*.py" | xargs flake8 --ignore=B,C,E,F,I,N,W --select=E101,E112,E113,E115,E116,E125,E129,E131,E133,E223,E224,E242,E266,E271,E272,E273,E274,E275,E304,E306,E401,E402,E502,E701,E702,E703,E714,E721,E742,E743,F401,E901,E902,F402,F404,F406,F407,F601,F602,F621,F622,F631,F701,F702,F703,F704,F705,F706,F707,F811,F812,F821,F822,F823,F831,F841,W191,W291,W292,W601,W602,W603,W604,W606 #,E741,W504,W605

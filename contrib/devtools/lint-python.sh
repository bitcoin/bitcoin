#!/bin/sh
#
# Copyright (c) 2017 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
#
# Check for specified flake8 warnings in python files.

# E112 expected an indented block
# E113 unexpected indentation
# E115 expected an indented block (comment)
# E116 unexpected indentation (comment)
# E125 continuation line with same indent as next logical line
# E131 continuation line unaligned for hanging indent
# E133 closing bracket is missing indentation
# E223 tab before operator
# E224 tab after operator
# E271 multiple spaces after keyword
# E272 multiple spaces before keyword
# E273 tab after keyword
# E274 tab before keyword
# E275 missing whitespace after keyword
# E304 blank lines found after function decorator
# E306 expected 1 blank line before a nested definition
# E502 the backslash is redundant between brackets
# E702 multiple statements on one line (semicolon)
# E703 statement ends with a semicolon
# E714 test for object identity should be "is not"
# E721 do not compare types, use "isinstance()"
# E741 do not use variables named "l", "O", or "I"
# E742 do not define classes named "l", "O", or "I"
# E743 do not define functions named "l", "O", or "I"
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
# F822 undefined name name in __all__
# F823 local variable name … referenced before assignment
# F831 duplicate argument name in function definition
# W292 no newline at end of file
# W504 line break after binary operator
# W601 .has_key() is deprecated, use "in"
# W602 deprecated form of raising exception
# W603 "<>" is deprecated, use "!="
# W604 backticks are deprecated, use "repr()"
# W605 invalid escape sequence "x"

flake8 --ignore=B,C,E,F,I,N,W --select=E112,E113,E115,E116,E125,E131,E133,E223,E224,E271,E272,E273,E274,E275,E304,E306,E502,E702,E703,E714,E721,E741,E742,E743,F401,F402,F404,F406,F407,F601,F602,F621,F622,F631,F701,F702,F703,F704,F705,F706,F707,F811,F812,F822,F823,F831,W292,W504,W601,W602,W603,W604,W605 .

#!/bin/sh
#
# Copyright (c) 2017 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
#
# Check for flake8 warnings in python files.

# Ignored warnings:
# E101 indentation contains mixed spaces and tabs
# E111 indentation is not a multiple of four
# E114 indentation is not a multiple of four (comment)
# E121 continuation line under-indented for hanging indent
# E122 continuation line missing indentation or outdented
# E123 closing bracket does not match indentation of opening bracket's line
# E124 closing bracket does not match visual indentation
# E126 continuation line over-indented for hanging indent
# E127 continuation line over-indented for visual indent
# E128 continuation line under-indented for visual indent
# E129 visually indented line with same indent as next logical line
# E201 whitespace after '[' or '{'
# E202 whitespace before ')', ']' or '}'
# E203 whitespace before ':'
# E211 whitespace before '('
# E221 multiple spaces before operator
# E222 multiple spaces after operator
# E225 missing whitespace around operator
# E226 missing whitespace around arithmetic operator
# E227 missing whitespace around bitwise or shift operator
# E228 missing whitespace around modulo operator
# E231 missing whitespace after ',' or ':'
# E241 multiple spaces after ','
# E251 unexpected spaces around keyword / parameter equals
# E261 at least two spaces before inline comment
# E262 inline comment should start with '# '
# E265 block comment should start with '# '
# E266 too many leading '#' for block comment
# E301 expected 1 blank line, found N
# E302 expected 2 blank lines, found N
# E303 too many blank lines (N)
# E305 expected 2 blank lines after class or function definition, found N
# E401 multiple imports on one line
# E402 module level import not at top of file
# E501 line too long (N > 79 characters)
# E701 multiple statements on one line (colon)
# E704 multiple statements on one line (def)
# E711 comparison to None should be 'if cond is None:'
# E712 comparison to False should be 'if cond is False:' or 'if not cond:'
# E713 test for membership should be 'not in'
# E722 do not use bare except
# E731 do not assign a lambda expression, use a def
# E999 SyntaxError: invalid syntax
# F403 'from ... import *' used; unable to detect undefined names
# F405 '...' may be undefined, or defined from star imports: ...
# F821 undefined name '...'
# F841 local variable '...' is assigned to but never used
# W191 indentation contains tabs
# W291 trailing whitespace
# W293 blank line contains whitespace
# W391 blank line at end of file
# W503 line break before binary operator

flake8 --ignore=E101,E111,E114,E121,E122,E123,E124,E126,E127,E128,E129,E201,E202,E203,E211,E221,E222,E225,E226,E227,E228,E231,E241,E251,E261,E262,E265,E266,E301,E302,E303,E305,E401,E402,E501,E701,E704,E711,E712,E713,E722,E731,E999,F403,F405,F821,F841,W191,W291,W293,W391,W503 .

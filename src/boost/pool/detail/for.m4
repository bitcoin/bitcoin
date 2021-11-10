m4_dnl 
m4_dnl Copyright (C) 2000 Stephen Cleary
m4_dnl 
m4_dnl Distributed under the Boost Software License, Version 1.0. (See accompany-
m4_dnl ing file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
m4_dnl
m4_dnl See http://www.boost.org for updates, documentation, and revision history.
m4_dnl 
m4_dnl
m4_dnl 
m4_dnl BOOST_M4_FOR: repeat a given text for a range of values
m4_dnl   $1 - variable to hold the current value.
m4_dnl   $2 - the starting value.
m4_dnl   $3 - the ending value (text is _not_ repeated for this value).
m4_dnl   $4 - the text to repeat.
m4_dnl   $5 - the delimeter text (optional).
m4_dnl
m4_dnl If the starting value is < ending value:
m4_dnl   Will repeat $4, binding $1 to the values in the range [$2, $3).
m4_dnl Else (that is, starting value >= ending value):
m4_dnl   Will do nothing
m4_dnl Repeats $5 in-between each occurrence of $4
m4_dnl
m4_dnl Logic:
m4_dnl   Set $1 to $2 and call BOOST_M4_FOR_LIST_HELPER:
m4_dnl     If $1 >= $3, do nothing
m4_dnl     Else
m4_dnl       output $4,
m4_dnl       set $1 to itself incremented,
m4_dnl       If $1 != $3, output $5,
m4_dnl       and use recursion
m4_dnl
m4_define(`BOOST_M4_FOR',
          `m4_ifelse(m4_eval($# < 4 || $# > 5), 1,
                     `m4_errprint(m4___file__:m4___line__: `Boost m4 script: BOOST_M4_FOR: Wrong number of arguments ($#)')',
                     `m4_pushdef(`$1', `$2')BOOST_M4_FOR_HELPER($@)m4_popdef(`$1')')')m4_dnl
m4_define(`BOOST_M4_FOR_HELPER',
          `m4_ifelse(m4_eval($1 >= $3), 1, ,
                     `$4`'m4_define(`$1', m4_incr($1))m4_ifelse(m4_eval($1 != $3), 1, `$5')`'BOOST_M4_FOR_HELPER($@)')')m4_dnl
m4_dnl 
m4_dnl Testing/Examples:
m4_dnl 
m4_dnl The following line will output:
m4_dnl   "repeat.m4:42: Boost m4 script: BOOST_M4_FOR: Wrong number of arguments (3)"
m4_dnl BOOST_M4_FOR(i, 1, 3)
m4_dnl
m4_dnl The following line will output:
m4_dnl   "repeat.m4:46: Boost m4 script: BOOST_M4_FOR: Wrong number of arguments (6)"
m4_dnl BOOST_M4_FOR(i, 1, 3, i, ` ', 13)
m4_dnl
m4_dnl The following line will output (nothing):
m4_dnl   ""
m4_dnl BOOST_M4_FOR(i, 7, 0, i )
m4_dnl
m4_dnl The following line will output (nothing):
m4_dnl   ""
m4_dnl BOOST_M4_FOR(i, 0, 0, i )
m4_dnl
m4_dnl The following line will output:
m4_dnl   "0 1 2 3 4 5 6 "
m4_dnl BOOST_M4_FOR(i, 0, 7, i )
m4_dnl
m4_dnl The following line will output:
m4_dnl   "-13 -12 -11 "
m4_dnl BOOST_M4_FOR(i, -13, -10, i )
m4_dnl
m4_dnl The following two lines will output:
m4_dnl   "(0, 0) (0, 1) (0, 2) (0, 3) "
m4_dnl   "(1, 0) (1, 1) (1, 2) (1, 3) "
m4_dnl   "(2, 0) (2, 1) (2, 2) (2, 3) "
m4_dnl   "(3, 0) (3, 1) (3, 2) (3, 3) "
m4_dnl   "(4, 0) (4, 1) (4, 2) (4, 3) "
m4_dnl   "(5, 0) (5, 1) (5, 2) (5, 3) "
m4_dnl   "(6, 0) (6, 1) (6, 2) (6, 3) "
m4_dnl   "(7, 0) (7, 1) (7, 2) (7, 3) "
m4_dnl   ""
m4_dnl BOOST_M4_FOR(i, 0, 8, BOOST_M4_FOR(j, 0, 4, (i, j) )
m4_dnl )
m4_dnl
m4_dnl The following line will output (nothing):
m4_dnl   ""
m4_dnl BOOST_M4_FOR(i, 7, 0, i, |)
m4_dnl
m4_dnl The following line will output (nothing):
m4_dnl   ""
m4_dnl BOOST_M4_FOR(i, 0, 0, i, |)
m4_dnl
m4_dnl The following line will output:
m4_dnl   "0|1|2|3|4|5|6"
m4_dnl BOOST_M4_FOR(i, 0, 7, i, |)
m4_dnl
m4_dnl The following line will output:
m4_dnl   "-13, -12, -11"
m4_dnl BOOST_M4_FOR(i, -13, -10, i, `, ')
m4_dnl
m4_dnl The following two lines will output:
m4_dnl   "[(0, 0), (0, 1), (0, 2), (0, 3)],"
m4_dnl   "[(1, 0), (1, 1), (1, 2), (1, 3)],"
m4_dnl   "[(2, 0), (2, 1), (2, 2), (2, 3)],"
m4_dnl   "[(3, 0), (3, 1), (3, 2), (3, 3)],"
m4_dnl   "[(4, 0), (4, 1), (4, 2), (4, 3)],"
m4_dnl   "[(5, 0), (5, 1), (5, 2), (5, 3)],"
m4_dnl   "[(6, 0), (6, 1), (6, 2), (6, 3)],"
m4_dnl   "[(7, 0), (7, 1), (7, 2), (7, 3)]"
m4_dnl BOOST_M4_FOR(i, 0, 8, `[BOOST_M4_FOR(j, 0, 4, (i, j), `, ')]', `,
m4_dnl ')
m4_dnl
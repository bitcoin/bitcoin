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
m4_dnl Avoid the use of any m4_* identifiers in this header file,
m4_dnl  as that may cause incompatibility problems with future
m4_dnl  versions of m4.
m4_dnl
m4_dnl This is a normal header file, except that lines starting
m4_dnl  with `m4_dnl' will be stripped, TBA_FOR
m4_dnl  macros will be replaced with repeated text, and text in
m4_dnl  single quotes (`...') will have their single quotes
m4_dnl  stripped.
m4_dnl
m4_dnl
m4_dnl Check to make sure NumberOfArguments was defined.  If it's not defined,
m4_dnl  default to 3
m4_dnl
m4_ifdef(`NumberOfArguments', , `m4_errprint(m4___file__:m4___line__`: NumberOfArguments is not defined; defaulting to 3
')m4_define(`NumberOfArguments', 3)')m4_dnl
m4_ifelse(NumberOfArguments, , `m4_errprint(m4___file__:m4___line__`: NumberOfArguments is defined to be empty; defaulting to 3
')m4_define(`NumberOfArguments', 3)')m4_dnl
m4_dnl
m4_dnl Check to make sure NumberOfArguments >= 1.  If it's not, then fatal error.
m4_dnl
m4_ifelse(m4_eval(NumberOfArguments < 1), 1, `m4_errprint(m4___file__:m4___line__`: NumberOfArguments ('NumberOfArguments`) is less than 1
')m4_m4exit(1)')m4_dnl
m4_dnl
m4_dnl Include the BOOST_M4_FOR macro definition
m4_dnl
m4_include(`for.m4')`'m4_dnl
m4_dnl
m4_dnl Begin the generated file.
m4_dnl
// Copyright (C) 2000 Stephen Cleary
//
// Distributed under the Boost Software License, Version 1.0. (See accompany-
// ing file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org for updates, documentation, and revision history.

m4_dnl These warnings apply to the file generated from this file.
m4_dnl Of course, you may freely edit this file.
// This file was AUTOMATICALLY GENERATED from "m4___file__"
//  Do NOT include directly!
//  Do NOT edit!

m4_dnl
m4_dnl First we define a simple 'cv_qual' macro which takes a number, either
m4_dnl   0, 1, 2, or 3, and determines cv-qualification.
m4_dnl
m4_define(`cv_qual',
`m4_ifelse($1, 0, `',
`m4_ifelse($1, 1, `const ',
`m4_ifelse($1, 2, `volatile ',
`m4_ifelse($1, 3, `const volatile ',
`m4_errprint(m4___file__:m4___line__: `Boost m4 script: cv-determiner: Not 0, 1, 2, or 3 (was '$1`)')'
)')')')')m4_dnl
m4_dnl
m4_dnl Next we go through the actual loop.  For each number of arguments from
m4_dnl   1 to NumberOfArguments, we create a template function that takes that
m4_dnl   many template arguments, and also generate all cv-qualified permutations
m4_dnl   of that function.
m4_dnl
BOOST_M4_FOR(N, 1, NumberOfArguments + 1,
`BOOST_M4_FOR(cv, 0, m4_eval(4 ** N),
  `template <BOOST_M4_FOR(i, 0, N, `typename T`'i', `, ')>
element_type * construct(BOOST_M4_FOR(i, 0, N,
    `cv_qual(m4_eval((cv >> (i * 2)) % 4))T`'i & a`'i', `, '))
{
  element_type * const ret = (malloc)();
  if (ret == 0)
    return ret;
  try { new (ret) element_type(BOOST_M4_FOR(i, 0, N, `a`'i', `, ')); }
  catch (...) { (free)(ret); throw; }
  return ret;
}
')')

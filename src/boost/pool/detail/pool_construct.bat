@echo off
rem
rem Copyright (C) 2000, 2001 Stephen Cleary
rem
rem Distributed under the Boost Software License, Version 1.0. (See accompany-
rem ing file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)



rem Check for Windows NT
if %OS%==Windows_NT goto NT

rem Not NT - run m4 as normal, then exit
m4 -P -E -DNumberOfArguments=%1 pool_construct.m4 > pool_construct.ipp
goto end

rem DJGPP programs (including m4) running on Windows/NT do NOT support long
rem  file names (see the DJGPP v2 FAQ, question 8.1)
rem Note that the output doesn't have to be a short name because it's an
rem  argument to the command shell, not m4.
:NT
m4 -P -E -DNumberOfArguments=%1 < pool_construct.m4 > pool_construct.ipp

:end

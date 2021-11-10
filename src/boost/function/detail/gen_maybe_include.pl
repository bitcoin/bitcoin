#!/usr/bin/perl -w
#
# Boost.Function library
#
# Copyright (C) 2001-2003 Douglas Gregor (gregod@cs.rpi.edu)
#
# Permission to copy, use, sell and distribute this software is granted
# provided this copyright notice appears in all copies.
# Permission to modify the code and to distribute modified code is granted
# provided this copyright notice appears in all copies, and a notice
# that the code was modified is included with the copyright notice.
#
# This software is provided "as is" without express or implied warranty,
# and with no claim as to its suitability for any purpose.
#
# For more information, see http://www.boost.org
use English;

$max_args = $ARGV[0];

open (OUT, ">maybe_include.hpp") or die("Cannot write to maybe_include.hpp");
for($on_arg = 0; $on_arg <= $max_args; ++$on_arg) {
    if ($on_arg == 0) {
	print OUT "#if";
    }
    else {
	print OUT "#elif";
    }
    print OUT " BOOST_FUNCTION_NUM_ARGS == $on_arg\n";
    print OUT "#  undef BOOST_FUNCTION_MAX_ARGS_DEFINED\n";
    print OUT "#  define BOOST_FUNCTION_MAX_ARGS_DEFINED $on_arg\n";
    print OUT "#  ifndef BOOST_FUNCTION_$on_arg\n";
    print OUT "#    define BOOST_FUNCTION_$on_arg\n";
    print OUT "#    include <boost/function/function_template.hpp>\n";
    print OUT "#  endif\n";
}
print OUT "#else\n";
print OUT "#  error Cannot handle Boost.Function objects that accept more than $max_args arguments!\n";
print OUT "#endif\n";

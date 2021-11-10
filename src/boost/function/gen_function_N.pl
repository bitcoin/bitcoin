#!/usr/bin/perl -w
#
# Boost.Function library
#
# Copyright Douglas Gregor 2001-2003. Use, modification and
# distribution is subject to the Boost Software License, Version
# 1.0. (See accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)
#
# For more information, see http://www.boost.org
use English;

if ($#ARGV < 0) {
  print "Usage: perl gen_function_N <number of arguments>\n";
  exit;
}


$totalNumArgs = $ARGV[0];
for ($numArgs = 0; $numArgs <= $totalNumArgs; ++$numArgs) {
  open OUT, ">function$numArgs.hpp";
  print OUT "#define BOOST_FUNCTION_NUM_ARGS $numArgs\n";
  print OUT "#include <boost/function/detail/maybe_include.hpp>\n";
  print OUT "#undef BOOST_FUNCTION_NUM_ARGS\n";
  close OUT;
}

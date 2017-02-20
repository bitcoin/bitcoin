# See the file LICENSE for redistribution information.
#
# Copyright (c) 1996-2009 Oracle.  All rights reserved.
#
# $Id$
#
# Byte Order Test
# Use existing tests and run with both byte orders.
proc byteorder { method {nentries 1000} } {
	source ./include.tcl
	puts "Byteorder: $method $nentries"

	eval {test001 $method $nentries 0 0 "001" -lorder 1234}
	eval {verify_dir $testdir}
	eval {test001 $method $nentries 0 0 "001" -lorder 4321}
	eval {verify_dir $testdir}
	eval {test003 $method -lorder 1234}
	eval {verify_dir $testdir}
	eval {test003 $method -lorder 4321}
	eval {verify_dir $testdir}
	eval {test010 $method $nentries 5 "010" -lorder 1234}
	eval {verify_dir $testdir}
	eval {test010 $method $nentries 5 "010" -lorder 4321}
	eval {verify_dir $testdir}
	eval {test011 $method $nentries 5 "011" -lorder 1234}
	eval {verify_dir $testdir}
	eval {test011 $method $nentries 5 "011" -lorder 4321}
	eval {verify_dir $testdir}
	eval {test018 $method $nentries -lorder 1234}
	eval {verify_dir $testdir}
	eval {test018 $method $nentries -lorder 4321}
	eval {verify_dir $testdir}
}

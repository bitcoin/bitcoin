# See the file LICENSE for redistribution information.
#
# Copyright (c) 1998-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	test034
# TEST	test032 with off-page or overflow case with non-duplicates
# TEST	and duplicates.
# TEST	
# TEST	DB_GET_BOTH, DB_GET_BOTH_RANGE functionality with off-page 
# TEST	or overflow case within non-duplicates and duplicates.
proc test034 { method {nentries 10000} args} {
	set pgindex [lsearch -exact $args "-pagesize"]
	if { $pgindex != -1 } {
		puts "Test034: Skipping for specific pagesizes"
		return
	}

	# Test without duplicate and without overflow.
	eval {test032 $method $nentries 1 "034" 0} $args

	# Test without duplicate but with overflows.
	eval {test032 $method [expr $nentries / 100] 1 "034" 1}	$args

	# Test with off-page duplicates
	eval {test032 $method $nentries 20 "034" 0 -pagesize 512} $args

	# Test with multiple pages of off-page duplicates
	eval {test032 $method [expr $nentries / 10] 100 "034" 0 -pagesize 512} \
	    $args

	# Test with overflow duplicate. 
	eval {test032 $method [expr $nentries / 100] 5 "034" 1} $args 
}

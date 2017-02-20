# See the file LICENSE for redistribution information.
#
# Copyright (c) 1998-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	test040
# TEST	Test038 with off-page duplicates
# TEST	DB_GET_BOTH functionality with off-page duplicates.
proc test040 { method {nentries 10000} args} {
	set pgindex [lsearch -exact $args "-pagesize"]
	if { $pgindex != -1 } {
		puts "Test040: skipping for specific pagesizes"
		return
	}
	# Test with off-page duplicates
	eval {test038 $method $nentries 20 "040" -pagesize 512} $args

	# Test with multiple pages of off-page duplicates
	eval {test038 $method [expr $nentries / 10] 100 "040" -pagesize 512} \
	    $args
}

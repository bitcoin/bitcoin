# See the file LICENSE for redistribution information.
#
# Copyright (c) 2007-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	repmgr004
# TEST	Basic repmgr test with in-memory logs.
# TEST
# TEST	Create an appointed master and two clients, process some records and 
# TEST	verify resulting databases.
# TEST
# TEST	Run for btree only because access method shouldn't matter.
# TEST
proc repmgr004 { method { niter 100 } { tnum "004" } args } {

	source ./include.tcl

	if { $is_freebsd_test == 1 } {
		puts "Skipping replication manager test on FreeBSD platform."
		return
	}

	if { $is_windows9x_test == 1 } {
		puts "Skipping replication test on Win9x platform."
		return
	}

	# Skip for all methods except btree.
	if { $checking_valid_methods } {
		return btree
	}
	if { [is_btree $method] == 0 } {
		puts "Repmgr$tnum: skipping for non-btree method $method."
		return
	}

	set args [convert_args $method $args]

	puts "Repmgr$tnum ($method): Basic repmgr test in-memory logs."
	basic_repmgr_test $method $niter $tnum 0 1 0 0 0 $args
}


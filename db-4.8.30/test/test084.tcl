# See the file LICENSE for redistribution information.
#
# Copyright (c) 2000-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	test084
# TEST	Basic sanity test (test001) with large (64K) pages.
proc test084 { method {nentries 10000} {tnum "084"} {pagesize 65536} args} {
	source ./include.tcl

	set txnenv 0
	set eindex [lsearch -exact $args "-env"]
	#
	# If we are using an env, then testfile should just be the db name.
	# Otherwise it is the test directory and the name.
	if { $eindex == -1 } {
		set testfile $testdir/test$tnum-empty.db
		set env NULL
	} else {
		set testfile test$tnum-empty.db
		incr eindex
		set env [lindex $args $eindex]
		set txnenv [is_txnenv $env]
		if { $txnenv == 1 } {
			append args " -auto_commit "
		}
		set testdir [get_home $env]
	}

	set pgindex [lsearch -exact $args "-pagesize"]
	if { $pgindex != -1 } {
		puts "Test084: skipping for specific pagesizes"
		return
	}

	cleanup $testdir $env

	set args "-pagesize $pagesize $args"

	eval {test001 $method $nentries 0 0 $tnum} $args

	set omethod [convert_method $method]
	set args [convert_args $method $args]

	# For good measure, create a second database that's empty
	# with the large page size.  (There was a verifier bug that
	# choked on empty 64K pages. [#2408])
	set db [eval {berkdb_open -create -mode 0644} $args $omethod $testfile]
	error_check_good empty_db [is_valid_db $db] TRUE
	error_check_good empty_db_close [$db close] 0
}

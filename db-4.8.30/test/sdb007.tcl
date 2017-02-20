# See the file LICENSE for redistribution information.
#
# Copyright (c) 1999-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	sdb007
# TEST	Tests page size difference errors between subdbs.
# TEST	If the physical file already exists, we ignore pagesize specifications
# TEST	on any subsequent -creates.
# TEST
# TEST 	1.  Create/open a subdb with system default page size.
# TEST	    Create/open a second subdb specifying a different page size.
# TEST	    The create should succeed, but the pagesize of the new db
# TEST	    will be the system default page size.
# TEST 	2.  Create/open a subdb with a specified, non-default page size.
# TEST	    Create/open a second subdb specifying a different page size.
# TEST	    The create should succeed, but the pagesize of the new db
# TEST	    will be the specified page size from the first create.

proc sdb007 { method args } {
	source ./include.tcl

	set db2args [convert_args -btree $args]
	set args [convert_args $method $args]
	set omethod [convert_method $method]

	if { [is_queue $method] == 1 } {
		puts "Subdb007: skipping for method $method"
		return
	}
	set pgindex [lsearch -exact $args "-pagesize"]
	if { $pgindex != -1 } {
		puts "Subdb007: skipping for specific page sizes"
		return
	}

	puts "Subdb007: $method ($args) subdb tests with different page sizes"

	set txnenv 0
	set envargs ""
	set eindex [lsearch -exact $args "-env"]
	#
	# If we are using an env, then testfile should just be the db name.
	# Otherwise it is the test directory and the name.
	if { $eindex == -1 } {
		set testfile $testdir/subdb007.db
		set env NULL
	} else {
		set testfile subdb007.db
		incr eindex
		set env [lindex $args $eindex]
		set envargs " -env $env "
		set txnenv [is_txnenv $env]
		if { $txnenv == 1 } {
			append args " -auto_commit "
			append envargs " -auto_commit "
			append db2args " -auto_commit "
		}
		set testdir [get_home $env]
	}
	set sub1 "sub1"
	set sub2 "sub2"
	cleanup $testdir $env
	set txn ""

	puts "\tSubdb007.a.0: create subdb with default page size"
	set db [eval {berkdb_open -create -mode 0644} \
	    $args $envargs {$omethod $testfile $sub1}]
	error_check_good subdb [is_valid_db $db] TRUE

	# Figure out what the default page size is so that we can send
	# a different value to the next -create call.
	set default_psize [stat_field $db stat "Page size"]
	error_check_good dbclose [$db close] 0

	if { $default_psize == 512 } {
		set psize 2048
	} else {
		set psize 512
	}

	puts "\tSubdb007.a.1: Create 2nd subdb with different specified page size"
	set db2 [eval {berkdb_open -create -btree} \
	    $db2args $envargs {-pagesize $psize $testfile $sub2}]
	error_check_good db2_create [is_valid_db $db2] TRUE

	set actual_psize [stat_field $db2 stat "Page size"]
	error_check_good check_pagesize [expr $actual_psize == $default_psize] 1
	error_check_good db2close [$db2 close] 0

	set ret [eval {berkdb dbremove} $envargs {$testfile}]

	puts "\tSubdb007.b.0: Create subdb with specified page size"
	set db [eval {berkdb_open -create -mode 0644} \
	    $args $envargs {-pagesize $psize $omethod $testfile $sub1}]
	error_check_good subdb [is_valid_db $db] TRUE
	error_check_good dbclose [$db close] 0

	puts "\tSubdb007.b.1: Create 2nd subdb with different specified page size"
	set newpsize [expr $psize * 2]
	set db2 [eval {berkdb_open -create -mode 0644} $args \
	    $envargs {-pagesize $newpsize $omethod $testfile $sub2}]
	error_check_good subdb [is_valid_db $db2] TRUE
	set actual_psize [stat_field $db2 stat "Page size"]
	error_check_good check_pagesize [expr $actual_psize == $psize] 1
	error_check_good db2close [$db2 close] 0
}

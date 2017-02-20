# See the file LICENSE for redistribution information.
#
# Copyright (c) 1999-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	sdb008
# TEST	Tests explicit setting of lorders for subdatabases -- the
# TEST	lorder should be ignored.
proc sdb008 { method args } {
	source ./include.tcl

	set args [convert_args $method $args]
	set omethod [convert_method $method]

	if { [is_queue $method] == 1 } {
		puts "Subdb008: skipping for method $method"
		return
	}
	set eindex [lsearch -exact $args "-env"]

	# If we are using an env, then testfile should just be the db name.
	# Otherwise it is the test directory and the name.
	if { $eindex == -1 } {
		set testfile1 $testdir/subdb008a.db
		set testfile2 $testdir/subdb008b.db
		set env NULL
	} else {
		set testfile1 subdb008a.db
		set testfile2 subdb008b.db
		incr eindex
		set env [lindex $args $eindex]
		set txnenv [is_txnenv $env]
		if { $txnenv == 1 } {
			append args " -auto_commit "
		}
		set testdir [get_home $env]
	}
	cleanup $testdir $env

	puts "Subdb008: $method ($args) subdb tests with different lorders"

	puts "\tSubdb008.a.0: create subdb with system default lorder"
	set lorder "1234"
	if { [big_endian] } {
		set lorder "4321"
	}
	set db [eval {berkdb_open -create -mode 0644} \
	    $args {$omethod $testfile1 "sub1"}]
	error_check_good subdb [is_valid_db $db] TRUE
	error_check_good dbclose [$db close] 0

	# Explicitly try to create subdb's of each byte order.  In both
	# cases the subdb should be forced to the byte order of the
	# parent database.
	puts "\tSubdb008.a.1: Try to create subdb with -1234 lorder"
	set db [eval {berkdb_open -create -mode 0644} \
	    $args {-lorder 1234 $omethod $testfile1 "sub2"}]
	error_check_good lorder_1234 [eval $db get_lorder] $lorder
	error_check_good subdb [is_valid_db $db] TRUE
	error_check_good dbclose [$db close] 0

	puts "\tSubdb008.a.2: Try to create subdb with -4321 lorder"
	set db [eval {berkdb_open -create -mode 0644} \
	    $args {-lorder 4321 $omethod $testfile1 "sub3"}]
	error_check_good lorder_4321 [eval $db get_lorder] $lorder
	error_check_good subdb [is_valid_db $db] TRUE
	error_check_good dbclose [$db close] 0

	puts "\tSubdb008.b.0: create subdb with non-default lorder"
	set reverse_lorder "4321"
	if { [big_endian] } {
		set reverse_lorder "1234"
	}
	set db [eval {berkdb_open -create -mode 0644} \
	    {-lorder $reverse_lorder} $args {$omethod $testfile2 "sub1"}]
	error_check_good subdb [is_valid_db $db] TRUE
	error_check_good dbclose [$db close] 0

	puts "\tSubdb008.b.1: Try to create subdb with -1234 lorder"
	set db [eval {berkdb_open -create -mode 0644} \
	    $args {-lorder 1234 $omethod $testfile2 "sub2"}]
	error_check_good lorder_1234 [eval $db get_lorder] $reverse_lorder
	error_check_good subdb [is_valid_db $db] TRUE
	error_check_good dbclose [$db close] 0

	puts "\tSubdb008.b.2: Try to create subdb with -4321 lorder"
	set db [eval {berkdb_open -create -mode 0644} \
	    $args {-lorder 4321 $omethod $testfile2 "sub3"}]
	error_check_good lorder_4321 [eval $db get_lorder] $reverse_lorder
	error_check_good subdb [is_valid_db $db] TRUE
	error_check_good dbclose [$db close] 0
}

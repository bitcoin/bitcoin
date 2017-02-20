# See the file LICENSE for redistribution information.
#
# Copyright (c) 1999-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	test053
# TEST	Test of the DB_REVSPLITOFF flag in the Btree and Btree-w-recnum
# TEST	methods.
proc test053 { method args } {
	global alphabet
	global errorCode
	global is_je_test
	source ./include.tcl

	set args [convert_args $method $args]
	set omethod [convert_method $method]

	puts "\tTest053: Test of cursor stability across btree splits."
	if { [is_btree $method] != 1 && [is_rbtree $method] != 1 } {
		puts "Test053: skipping for method $method."
		return
	}
	
	if { [is_partition_callback $args] == 1 } {
		puts "Test053: skipping for method $method with partition callback."
		return
	}

	set pgindex [lsearch -exact $args "-pagesize"]
	if { $pgindex != -1 } {
		puts "Test053: skipping for specific pagesizes"
		return
	}

	set txn ""
	set flags ""

	puts "\tTest053.a: Create $omethod $args database."
	set txnenv 0
	set eindex [lsearch -exact $args "-env"]
	#
	# If we are using an env, then testfile should just be the db name.
	# Otherwise it is the test directory and the name.
	if { $eindex == -1 } {
		set testfile $testdir/test053.db
		set env NULL
	} else {
		set testfile test053.db
		incr eindex
		set env [lindex $args $eindex]
		set txnenv [is_txnenv $env]
		if { $txnenv == 1 } {
			append args " -auto_commit "
		}
		set testdir [get_home $env]
	}
	set t1 $testdir/t1
	cleanup $testdir $env

	set oflags \
	    "-create -revsplitoff -pagesize 1024 $args $omethod"
	set db [eval {berkdb_open} $oflags $testfile]
	error_check_good dbopen [is_valid_db $db] TRUE

	set nkeys 8
	set npages 15

	# We want to create a db with npages leaf pages, and have each page
	# be near full with keys that we can predict. We set pagesize above
	# to 1024 bytes, it should breakdown as follows (per page):
	#
	#	~20 bytes overhead
	#	key: ~4 bytes overhead, XXX0N where X is a letter, N is 0-9
	#	data: ~4 bytes overhead, + 100 bytes
	#
	# then, with 8 keys/page we should be just under 1024 bytes
	puts "\tTest053.b: Create $npages pages with $nkeys pairs on each."
	set keystring [string range $alphabet 0 [expr $npages -1]]
	set data [repeat DATA 22]
	for { set i 0 } { $i < $npages } {incr i } {
		set key ""
		set keyroot \
		    [repeat [string toupper [string range $keystring $i $i]] 3]
		set key_set($i) $keyroot
		for {set j 0} { $j < $nkeys} {incr j} {
			if { $j < 10 } {
				set key [set keyroot]0$j
			} else {
				set key $keyroot$j
			}
			if { $txnenv == 1 } {
				set t [$env txn]
				error_check_good txn [is_valid_txn $t $env] TRUE
				set txn "-txn $t"
			}
			set ret [eval {$db put} $txn {$key $data}]
			error_check_good dbput $ret 0
			if { $txnenv == 1 } {
				error_check_good txn [$t commit] 0
			}
		}
	}

# We really should not skip this test for partitioned dbs we need to
# calculate how many pages there should be which is tricky if we
# don't know where the keys are going to fall.  If they are all
# in one partition then we can subtract the extra leaf pages
# in the extra partitions.  The test further on should at least
# check that the number of pages is the same as what is found here.
	if { !$is_je_test && ![is_substr $args "-partition"] } {
		puts "\tTest053.c: Check page count."
		error_check_good page_count:check \
		    [is_substr [$db stat] "{Leaf pages} $npages"] 1
	}

	puts "\tTest053.d: Delete all but one key per page."
	for {set i 0} { $i < $npages } {incr i } {
		for {set j 1} { $j < $nkeys } {incr j } {
			if { $txnenv == 1 } {
				set t [$env txn]
				error_check_good txn [is_valid_txn $t $env] TRUE
				set txn "-txn $t"
			}
			set ret [eval {$db del} $txn {$key_set($i)0$j}]
			error_check_good dbdel $ret 0
			if { $txnenv == 1 } {
				error_check_good txn [$t commit] 0
			}
		}
	}

	if { !$is_je_test && ![is_substr $args "-partition"] } {
		puts "\tTest053.e: Check to make sure all pages are still there."
		error_check_good page_count:check \
		    [is_substr [$db stat] "{Leaf pages} $npages"] 1
	}

	if { $txnenv == 1 } {
		set t [$env txn]
		error_check_good txn [is_valid_txn $t $env] TRUE
		set txn "-txn $t"
	}
	set dbc [eval {$db cursor} $txn]
	error_check_good db:cursor [is_valid_cursor $dbc $db] TRUE

	# walk cursor through tree forward, backward.
	# delete one key, repeat
	for {set i 0} { $i < $npages} {incr i} {
		puts -nonewline \
		    "\tTest053.f.$i: Walk curs through tree: forward..."
		for { set j $i; set curr [$dbc get -first]} { $j < $npages} { \
		    incr j; set curr [$dbc get -next]} {
			error_check_bad dbc:get:next [llength $curr] 0
			error_check_good dbc:get:keys \
			    [lindex [lindex $curr 0] 0] $key_set($j)00
		}
		puts -nonewline "backward..."
		for { set j [expr $npages - 1]; set curr [$dbc get -last]} { \
		    $j >= $i } { \
		    set j [incr j -1]; set curr [$dbc get -prev]} {
			error_check_bad dbc:get:prev [llength $curr] 0
			error_check_good dbc:get:keys \
			    [lindex [lindex $curr 0] 0] $key_set($j)00
		}
		puts "complete."

		if { [is_rbtree $method] == 1} {
			puts "\t\tTest053.f.$i:\
			    Walk through tree with record numbers."
			for {set j 1} {$j <= [expr $npages - $i]} {incr j} {
				set curr [eval {$db get} $txn {-recno $j}]
				error_check_bad \
				    db_get:recno:$j [llength $curr] 0
				error_check_good db_get:recno:keys:$j \
				    [lindex [lindex $curr 0] 0] \
				    $key_set([expr $j + $i - 1])00
			}
		}
		puts "\tTest053.g.$i:\
		    Delete single key ([expr $npages - $i] keys left)."
		set ret [eval {$db del} $txn {$key_set($i)00}]
		error_check_good dbdel $ret 0
		error_check_good del:check \
		    [llength [eval {$db get} $txn {$key_set($i)00}]] 0
	}

	# end for loop, verify db_notfound
	set ret [$dbc get -first]
	error_check_good dbc:get:verify [llength $ret] 0

	# loop: until single key restored on each page
	for {set i 0} { $i < $npages} {incr i} {
		puts "\tTest053.i.$i:\
		    Restore single key ([expr $i + 1] keys in tree)."
		set ret [eval {$db put} $txn {$key_set($i)00 $data}]
		error_check_good dbput $ret 0

		puts -nonewline \
		    "\tTest053.j: Walk cursor through tree: forward..."
		for { set j 0; set curr [$dbc get -first]} { $j <= $i} {\
				  incr j; set curr [$dbc get -next]} {
			error_check_bad dbc:get:next [llength $curr] 0
			error_check_good dbc:get:keys \
			    [lindex [lindex $curr 0] 0] $key_set($j)00
		}
		error_check_good dbc:get:next [llength $curr] 0

		puts -nonewline "backward..."
		for { set j $i; set curr [$dbc get -last]} { \
		    $j >= 0 } { \
		    set j [incr j -1]; set curr [$dbc get -prev]} {
			error_check_bad dbc:get:prev [llength $curr] 0
			error_check_good dbc:get:keys \
			    [lindex [lindex $curr 0] 0] $key_set($j)00
		}
		puts "complete."
		error_check_good dbc:get:prev [llength $curr] 0

		if { [is_rbtree $method] == 1} {
			puts "\t\tTest053.k.$i:\
			    Walk through tree with record numbers."
			for {set j 1} {$j <= [expr $i + 1]} {incr j} {
				set curr [eval {$db get} $txn {-recno $j}]
				error_check_bad \
				    db_get:recno:$j [llength $curr] 0
				error_check_good db_get:recno:keys:$j \
				    [lindex [lindex $curr 0] 0] \
				    $key_set([expr $j - 1])00
			}
		}
	}

	error_check_good dbc_close [$dbc close] 0
	if { $txnenv == 1 } {
		error_check_good txn [$t commit] 0
	}
	error_check_good db_close [$db close] 0

	puts "Test053 complete."
}

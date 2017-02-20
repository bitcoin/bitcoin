# See the file LICENSE for redistribution information.
#
# Copyright (c) 2003-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	test103
# TEST	Test bulk get when record numbers wrap around.
# TEST
# TEST	Load database with items starting before and ending after
# TEST	the record number wrap around point.  Run bulk gets (-multi_key)
# TEST	with various buffer sizes and verify the contents returned match
# TEST	the results from a regular cursor get.
# TEST
# TEST	Then delete items to create a sparse database and make sure it
# TEST	still works.  Test both -multi and -multi_key since they behave
# TEST	differently.
proc test103 { method {nentries 100} {start 4294967250} {tnum "103"} args} {
	source ./include.tcl

	set args [convert_args $method $args]
	set omethod [convert_method $method]
	puts "Test$tnum: $method ($args) Test of bulk get with wraparound."

	if { [is_queueext $method] == 0 } {
		puts "\tSkipping Test$tnum for method $method."
		return
	}

	# Create the database and open the dictionary
	set txnenv 0
	set eindex [lsearch -exact $args "-env"]
	#
	# If we are using an env, then testfile should just be the db name.
	# Otherwise it is the test directory and the name.
	if { $eindex == -1 } {
		set testfile $testdir/test$tnum.db
		set env NULL
	} else {
		set testfile test$tnum.db
		incr eindex
		set env [lindex $args $eindex]
		set txnenv [is_txnenv $env]
		if { $txnenv == 1 } {
			append args " -auto_commit "
			#
			# If we are using txns and running with the
			# default, set the default down a bit.
			#
			if { $nentries == 10000 } {
				set nentries 100
			}
		}
		set testdir [get_home $env]
	}

	cleanup $testdir $env

	set db [eval {berkdb_open_noerr \
	     -create -mode 0644} $args $omethod $testfile]
	error_check_good dbopen [is_valid_db $db] TRUE

	# Find the pagesize so we can use it to size the buffer.
	set stat [$db stat]
	set pagesize [get_pagesize $stat]

	set did [open $dict]

	puts "\tTest$tnum.a: put/get loop"
	set txn ""

	# Here is the loop where we put each key/data pair
	set count 0
	set k [expr $start + 1]
	while { [gets $did str] != -1 && $count < $nentries } {
		#
		# We cannot use 'incr' because it gets unhappy since
		# expr above is using 64-bits.
		set k [expr $k + 1]
		#
		# Detect if we're more than 32 bits now.  If so, wrap
		# our key back to 1.
		#
		if { [expr $k > 0xffffffff] } {
			set k 1
		}
		if { $txnenv == 1 } {
			set t [$env txn]
			error_check_good txn [is_valid_txn $t $env] TRUE
			set txn "-txn $t"
		}
		set ret [eval {$db put} $txn {$k [chop_data $method $str]}]
		error_check_good db_put $ret 0
		if { $txnenv == 1 } {
			error_check_good txn [$t commit] 0
		}
		incr count
	}
	close $did

	# Run tests in verbose mode for debugging.
	set verbose 0

	puts "\tTest$tnum.b: Bulk get with large buffer (retrieves all data)."
	# Buffer is large enough that everything fits in a single get.
	check_multi_recno $db [expr $pagesize * $nentries] multi_key $verbose

	puts "\tTest$tnum.c: Bulk get with buffer = (2 x pagesize)."
	# Buffer gets several items at a get, but not all.
	check_multi_recno $db [expr $pagesize * 2] multi_key $verbose

	# Skip tests if buffer would be smaller than allowed.
	if { $pagesize >= 1024  } {
		puts "\tTest$tnum.d: Bulk get with buffer = pagesize."
		check_multi_recno $db $pagesize multi_key $verbose
	}

	if { $pagesize >= 2048 } {
		puts "\tTest$tnum.e: Bulk get with buffer < pagesize\
		    (returns EINVAL)."
		catch {
			check_multi_recno $db [expr $pagesize / 2] \
			    multi_key $verbose
		} res
		error_check_good \
		    bufsize_less_than_pagesize [is_substr $res "invalid"] 1
	}

	# For a sparsely populated database, test with both -multi_key and
	# -multi.  In any sort of record numbered database, -multi does not
	# return keys, so it returns all items.  -multi_key returns both keys
	# and data so it skips deleted items.
	puts "\tTest$tnum.f: Delete every 10th item to create sparse database."
	if { $txnenv == 1 } {
		set t [$env txn]
		error_check_good txn [is_valid_txn $t $env] TRUE
		set txn "-txn $t"
	}
	set curs [ eval {$db cursor} $txn]
	error_check_good cursor [is_valid_cursor $curs $db] TRUE

	set count 0
	for { set kd [$curs get -first] } { $count < $nentries } \
	    { set kd [$curs get -next] } {
		if { [expr $count % 10 == 0] } {
			error_check_good cdelete [$curs del] 0
		}
		incr count
	}
	error_check_good curs_close [$curs close] 0
	if { $txnenv == 1 } {
		error_check_good txn [$t commit] 0
	}

	puts "\tTest$tnum.g: Sparse database, large buffer, multi_key."
	check_multi_recno $db [expr $pagesize * $nentries] multi_key $verbose
	puts "\tTest$tnum.h: Sparse database, large buffer, multi."
	check_multi_recno $db [expr $pagesize * $nentries] multi $verbose

	puts "\tTest$tnum.i: \
	    Sparse database, buffer = (2 x pagesize), multi_key."
	check_multi_recno $db [expr $pagesize * 2] multi_key $verbose
	puts "\tTest$tnum.j: Sparse database, buffer = (2 x pagesize), multi."
	check_multi_recno $db [expr $pagesize * 2] multi $verbose

	if { $pagesize >= 1024  } {
		puts "\tTest$tnum.k: \
		    Sparse database, buffer = pagesize, multi_key."
		check_multi_recno $db $pagesize multi_key $verbose
		puts "\tTest$tnum.k: Sparse database, buffer = pagesize, multi."
		check_multi_recno $db $pagesize multi $verbose
	}

	error_check_good db_close [$db close] 0
}

# The proc check_multi_recno is a modification of the utility routine
# check_multi_key specifically for recno methods.  We use this instead
# check_multi, even with the -multi flag, because the check_multi utility
# assumes that dups are being used which can't happen with record-based
# methods.
proc check_multi_recno { db size flag {verbose 0}} {
	source ./include.tcl
	set c [eval { $db cursor} ]
	set m [eval { $db cursor} ]

	set j 1

	# Walk the database with -multi_key or -multi bulk get.
	for {set d [$m get -first -$flag $size] } { [llength $d] != 0 } {
	    set d [$m get -next -$flag $size] } {
		if {$verbose == 1 } {
			puts "FETCH $j"
			incr j
		}
		# For each bulk get return, compare the results to what we
		# get by walking the db with an ordinary cursor get.
		for {set i 0} { $i < [llength $d] } { incr i } {
			set kd [lindex $d $i]
			set k [lindex $kd 0]
			set data [lindex $kd 1]
			set len [string length $data]

			if {$verbose == 1 } {
				puts ">> $k << >> $len << "
			}
			# If we hit a deleted item in -multi, skip over it.
			if { $flag == "multi" && $len == 0 } {
				continue
			}

			set check [$c get -next]
			set cd [lindex $check 0]
			set ck [lindex $cd 0]
			set cdata [lindex $cd 1]

			error_check_good key $k $ck
			error_check_good data_len $len [string length $cdata]
			error_check_good data $data $cdata
		}
	}
}

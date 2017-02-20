# See the file LICENSE for redistribution information.
#
# Copyright (c) 2000-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	test102
# TEST	Bulk get test for record-based methods. [#2934]
proc test102 { method {nsets 1000} {tnum "102"} args } {
	source ./include.tcl
	set args [convert_args $method $args]
	set omethod [convert_method $method]

	if { [is_rbtree $method] == 1 || [is_record_based $method] == 0} {
		puts "Test$tnum skipping for method $method"
		return
	}

	set txnenv 0
	set eindex [lsearch -exact $args "-env"]
	#
	# If we are using an env, then testfile should just be the db name.
	# Otherwise it is the test directory and the name.
	if { $eindex == -1 } {
		set basename $testdir/test$tnum
		set env NULL
		# If we've our own env, no reason to swap--this isn't
		# an mpool test.
		set carg { -cachesize {0 25000000 0} }
	} else {
		set basename test$tnum
		incr eindex
		set env [lindex $args $eindex]
		set txnenv [is_txnenv $env]
		if { $txnenv == 1 } {
			puts "Skipping for environment with txns"
			return
		}
		set testdir [get_home $env]
		set carg {}
	}
	cleanup $testdir $env

	puts "Test$tnum: $method ($args) Bulk get test"

	# Open and populate the database.
	puts "\tTest$tnum.a: Creating $method database\
	    with $nsets entries."
	set dargs "$carg $args"
	set testfile $basename.db
	set db [eval {berkdb_open_noerr -create} $omethod $dargs $testfile]
	error_check_good db_open [is_valid_db $db] TRUE
	t102_populate $db $method $nsets $txnenv 0

	# Determine the pagesize so we can use it to size the buffer.
	set stat [$db stat]
	set pagesize [get_pagesize $stat]

	# Run get tests.  The gettest should succeed as long as
	# the buffer is at least as large as the page size.  Test for
	# failure of a small buffer unless the page size is so small
	# we can't define a smaller buffer (buffers must be multiples
	# of 1024).  A "big buffer" should succeed in all cases because
	# we define it to be larger than 65536, the largest page
	# currently allowed.
	set maxpage [expr 1024 * 64]
	set bigbuf [expr $maxpage + 1024]
	set smallbuf 1024

	# Run regular db->get tests.
	if { $pagesize > 1024 } {
		t102_gettest $db $tnum b $smallbuf 1
	} else {
		puts "Skipping Test$tnum.b for small pagesize."
	}
	t102_gettest $db $tnum c $bigbuf 0

	# Run cursor get tests.
	if { $pagesize > 1024 } {
		t102_gettest $db $tnum d $smallbuf 1
	} else {
		puts "Skipping Test$tnum.b for small pagesize."
	}
	t102_cgettest $db $tnum e $bigbuf 0

	if { [is_fixed_length $method] == 1 } {
		puts "Skipping overflow tests for fixed-length method $omethod."
	} else {

		# Set up for overflow tests
		puts "\tTest$tnum.f: Growing database with overflow sets"
		t102_populate $db $method [expr $nsets / 100] $txnenv 10000

		# Run overflow get tests.  Test should fail for overflow pages
		# with our standard big buffer but succeed at twice that size.
		t102_gettest $db $tnum g $bigbuf 1
		t102_gettest $db $tnum h [expr $bigbuf * 2] 0

		# Run overflow cursor get tests.  Test will fail for overflow
		# pages with 8K buffer but succeed with a large buffer.
		t102_cgettest $db $tnum i 8192 1
		t102_cgettest $db $tnum j $bigbuf 0
	}
	error_check_good db_close [$db close] 0
}

proc t102_gettest { db tnum letter bufsize expectfail } {
	t102_gettest_body $db $tnum $letter $bufsize $expectfail 0
}
proc t102_cgettest { db tnum letter bufsize expectfail } {
	t102_gettest_body $db $tnum $letter $bufsize $expectfail 1
}

# Basic get test
proc t102_gettest_body { db tnum letter bufsize expectfail usecursor } {
	global errorCode

	foreach flag { multi multi_key } {
		if { $usecursor == 0 } {
			if { $flag == "multi_key" } {
				# db->get does not allow multi_key
				continue
			} else {
				set action "db get -$flag"
			}
		} else {
			set action "dbc get -$flag -set/-next"
		}
		puts "\tTest$tnum.$letter: $action with bufsize $bufsize"

		set allpassed TRUE
		set saved_err ""

		# Cursor for $usecursor.
		if { $usecursor != 0 } {
			set getcurs [$db cursor]
			error_check_good \
			    getcurs [is_valid_cursor $getcurs $db] TRUE
		}

		# Traverse DB with cursor;  do get/c_get($flag) on each item.
		set dbc [$db cursor]
		error_check_good is_valid_dbc [is_valid_cursor $dbc $db] TRUE
		for { set dbt [$dbc get -first] } { [llength $dbt] != 0 } \
		    { set dbt [$dbc get -next] } {
			set key [lindex [lindex $dbt 0] 0]
			set datum [lindex [lindex $dbt 0] 1]

			if { $usecursor == 0 } {
				set ret [catch \
				    {eval $db get -$flag $bufsize $key} res]
			} else {
				set res {}
				for { set ret [catch {eval $getcurs get\
				    -$flag $bufsize -set $key} tres] } \
				    { $ret == 0 && [llength $tres] != 0 } \
				    { set ret [catch {eval $getcurs get\
				    -$flag $bufsize -next} tres]} {
					eval lappend res $tres
				}
			}

			# If we expect a failure, be more tolerant if the above
			# fails; just make sure it's a DB_BUFFER_SMALL or an
			# EINVAL (if the buffer is smaller than the pagesize,
			# it's EINVAL), mark it, and move along.
			if { $expectfail != 0 && $ret != 0 } {
				if { [is_substr $errorCode DB_BUFFER_SMALL] != 1 && \
				    [is_substr $errorCode EINVAL] != 1 } {
					error_check_good \
					    "$flag failure errcode" \
					    $errorCode "DB_BUFFER_SMALL or EINVAL"
				}
				set allpassed FALSE
				continue
			}
			error_check_good "get_$flag ($key)" $ret 0
		}

		if { $expectfail == 1 } {
			error_check_good allpassed $allpassed FALSE
			puts "\t\tTest$tnum.$letter:\
			    returned at least one DB_BUFFER_SMALL (as expected)"
		} else {
			error_check_good allpassed $allpassed TRUE
			puts "\t\tTest$tnum.$letter: succeeded (as expected)"
		}

		error_check_good dbc_close [$dbc close] 0
		if { $usecursor != 0 } {
			error_check_good getcurs_close [$getcurs close] 0
		}
	}
}

proc t102_populate { db method nentries txnenv pad_bytes } {
	source ./include.tcl

	set did [open $dict]
	set count 0
	set txn ""
	set pflags ""
	set gflags " -recno "

	while { [gets $did str] != -1 && $count < $nentries } {
		set key [expr $count + 1]
		set datastr $str
		# Create overflow pages only if method is not fixed-length.
		if { [is_fixed_length $method] == 0 } {
			append datastr [repeat "a" $pad_bytes]
		}
		if { $txnenv == 1 } {
			set t [$env txn]
			error_check_good txn [is_valid_txn $t $env] TRUE
			set txn "-txn $t"
		}
		set ret [eval {$db put} \
		    $txn $pflags {$key [chop_data $method $datastr]}]
		error_check_good put $ret 0
		if { $txnenv == 1 } {
			error_check_good txn [$t commit] 0
		}

		set ret [eval {$db get} $gflags {$key}]
		error_check_good $key:dbget [llength $ret] 1
		incr count
	}
	close $did

	# This will make debugging easier, and since the database is
	# read-only from here out, it's cheap.
	error_check_good db_sync [$db sync] 0
}


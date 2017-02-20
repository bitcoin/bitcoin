# See the file LICENSE for redistribution information.
#
# Copyright (c) 2000-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	test095
# TEST	Bulk get test for methods supporting dups. [#2934]
proc test095 { method {tnum "095"} args } {
	source ./include.tcl
	global is_je_test
	global is_qnx_test

	set args [convert_args $method $args]
	set omethod [convert_method $method]

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

	# Tcl leaves a lot of memory allocated after this test
	# is run in the tclsh.  This ends up being a problem on
	# QNX runs as later tests then run out of memory.
	if { $is_qnx_test } {
		puts "Test$tnum skipping for QNX"
		return
	}
	if { [is_record_based $method] == 1 || [is_rbtree $method] == 1 } {
		puts "Test$tnum skipping for method $method"
		return
	}

	# The test's success is dependent on the relationship between
	# the amount of data loaded and the buffer sizes we pick, so
	# these parameters don't belong on the command line.
	set nsets 300
	set noverflows 25

	# We run the meat of the test twice: once with unsorted dups,
	# once with sorted dups.
	foreach { dflag sort } { -dup unsorted {-dup -dupsort} sorted } {
		if { $is_je_test || [is_compressed $args] } {
			if { $sort == "unsorted" } {
				continue
			}
		}

		set testfile $basename-$sort.db
		set did [open $dict]

		# Open and populate the database with $nsets sets of dups.
		# Each set contains as many dups as its number
		puts "\tTest$tnum.a:\
		    Creating database with $nsets sets of $sort dups."
		set dargs "$dflag $carg $args"
		set db [eval {berkdb_open_noerr -create} \
		    $omethod $dargs $testfile]
		error_check_good db_open [is_valid_db $db] TRUE
		t95_populate $db $did $nsets 0

		# Determine the pagesize so we can use it to size the buffer.
		set stat [$db stat]
		set pagesize [get_pagesize $stat]

		# Run basic get tests.
		#
		# A small buffer will fail if it is smaller than the pagesize.
		# Skip small buffer tests if the page size is so small that
		# we can't define a buffer smaller than the page size.
		# (Buffers must be 1024 or multiples of 1024.)
		#
		# A big buffer of 66560 (64K + 1K) should always be large
		# enough to contain the data, so the test should succeed
		# on all platforms.  We picked this number because it
		# is larger than the largest allowed pagesize, so the test
		# always fills more than a page at some point.

		set maxpage [expr 1024 * 64]
		set bigbuf [expr $maxpage + 1024]
		set smallbuf 1024

		if { $pagesize > 1024 } {
			t95_gettest $db $tnum b $smallbuf 1
		} else {
			puts "Skipping small buffer test Test$tnum.b"
		}
		t95_gettest $db $tnum c $bigbuf 0

		# Run cursor get tests.
		if { $pagesize > 1024 } {
			t95_cgettest $db $tnum b $smallbuf 1
		} else {
			puts "Skipping small buffer test Test$tnum.d"
		}
		t95_cgettest $db $tnum e $bigbuf 0

		# Run invalid flag combination tests
		# Sync and reopen test file so errors won't be sent to stderr
		error_check_good db_sync [$db sync] 0
		set noerrdb [eval berkdb_open_noerr $dargs $testfile]
		t95_flagtest $noerrdb $tnum f [expr 8192]
		t95_cflagtest $noerrdb $tnum g [expr 100]
		error_check_good noerrdb_close [$noerrdb close] 0

		# Set up for overflow tests
		set max [expr 4096 * $noverflows]
		puts "\tTest$tnum.h: Add $noverflows overflow sets\
		    to database (max item size $max)"
		t95_populate $db $did $noverflows 4096

		# Run overflow get tests.  The overflow test fails with
		# our standard big buffer doubled, but succeeds with a
		# buffer sized to handle $noverflows pairs of data of
		# size $max.
		t95_gettest $db $tnum i $bigbuf 1
		t95_gettest $db $tnum j [expr $bigbuf * 2] 1
		t95_gettest $db $tnum k [expr $max * $noverflows * 2] 0

		# Run overflow cursor get tests.
		t95_cgettest $db $tnum l $bigbuf 1
		# Expand buffer to accommodate basekey as well as the padding.
		t95_cgettest $db $tnum m [expr ($max + 512) * 2] 0

		error_check_good db_close [$db close] 0
		close $did
	}
}

proc t95_gettest { db tnum letter bufsize expectfail } {
	t95_gettest_body $db $tnum $letter $bufsize $expectfail 0
}
proc t95_cgettest { db tnum letter bufsize expectfail } {
	t95_gettest_body $db $tnum $letter $bufsize $expectfail 1
}
proc t95_flagtest { db tnum letter bufsize } {
	t95_flagtest_body $db $tnum $letter $bufsize 0
}
proc t95_cflagtest { db tnum letter bufsize } {
	t95_flagtest_body $db $tnum $letter $bufsize 1
}

# Basic get test
proc t95_gettest_body { db tnum letter bufsize expectfail usecursor } {
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
			error_check_good getcurs [is_valid_cursor $getcurs $db] TRUE
		}

		# Traverse DB with cursor;  do get/c_get($flag) on each item.
		set dbc [$db cursor]
		error_check_good is_valid_dbc [is_valid_cursor $dbc $db] TRUE
		for { set dbt [$dbc get -first] } { [llength $dbt] != 0 } \
		    { set dbt [$dbc get -nextnodup] } {
			set key [lindex [lindex $dbt 0] 0]
			set datum [lindex [lindex $dbt 0] 1]

			if { $usecursor == 0 } {
				set ret [catch {eval $db get -$flag $bufsize $key} res]
			} else {
				set res {}
				for { set ret [catch {eval $getcurs get -$flag $bufsize\
				    -set $key} tres] } \
				    { $ret == 0 && [llength $tres] != 0 } \
				    { set ret [catch {eval $getcurs get -$flag $bufsize\
				    -nextdup} tres]} {
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
			if { $flag == "multi_key" } {
				t95_verify $res TRUE
			} else {
				t95_verify $res FALSE
			}
		}
		set ret [catch {eval $db get -$flag $bufsize} res]

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

# Test of invalid flag combinations
proc t95_flagtest_body { db tnum letter bufsize usecursor } {
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
			set action "dbc get -$flag"
		}
		puts "\tTest$tnum.$letter: $action with invalid flag combinations"

		# Cursor for $usecursor.
		if { $usecursor != 0 } {
			set getcurs [$db cursor]
			error_check_good getcurs [is_valid_cursor $getcurs $db] TRUE
		}

		if { $usecursor == 0 } {
			# Disallowed flags for db->get
			set badflags [list consume consume_wait {rmw some_key}]

			foreach badflag $badflags {
				catch {eval $db get -$flag $bufsize -$badflag} ret
				error_check_good \
				    db:get:$flag:$badflag [is_substr $errorCode EINVAL] 1
			}
		} else {
			# Disallowed flags for db->cget
			set cbadflags [list last get_recno join_item \
			    {multi_key 1000} prev prevnodup]

			set dbc [$db cursor]
			$dbc get -first
			foreach badflag $cbadflags {
				catch {eval $dbc get -$flag $bufsize -$badflag} ret
				error_check_good dbc:get:$flag:$badflag \
					[is_substr $errorCode EINVAL] 1
			}
			error_check_good dbc_close [$dbc close] 0
		}
		if { $usecursor != 0 } {
			error_check_good getcurs_close [$getcurs close] 0
		}
	}
	puts "\t\tTest$tnum.$letter completed"
}

# Verify that a passed-in list of key/data pairs all match the predicted
# structure (e.g. {{thing1 thing1.0}}, {{key2 key2.0} {key2 key2.1}}).
proc t95_verify { res multiple_keys } {
	global alphabet

	set i 0
	set orig_key [lindex [lindex $res 0] 0]
	set nkeys [string trim $orig_key $alphabet']
	set base_key [string trim $orig_key 0123456789]
	set datum_count 0

	while { 1 } {
		set key [lindex [lindex $res $i] 0]
		set datum [lindex [lindex $res $i] 1]

		if { $datum_count >= $nkeys } {
			if { [llength $key] != 0 } {
				# If there are keys beyond $nkeys, we'd
				# better have multiple_keys set.
				error_check_bad "keys beyond number $i allowed"\
				    $multiple_keys FALSE

				# If multiple_keys is set, accept the new key.
				set orig_key $key
				set nkeys [eval string trim \
				    $orig_key {$alphabet'}]
				set base_key [eval string trim \
				    $orig_key 0123456789]
				set datum_count 0
			} else {
				# datum_count has hit nkeys.  We're done.
				return
			}
		}

		error_check_good returned_key($i) $key $orig_key
		error_check_good returned_datum($i) \
		    $datum $base_key.[format %4u $datum_count]
		incr datum_count
		incr i
	}
}

# Add nsets dup sets, each consisting of {word$ndups word$n} pairs,
# with "word" having (i * pad_bytes)  bytes extra padding.
proc t95_populate { db did nsets pad_bytes } {
	set txn ""
	for { set i 1 } { $i <= $nsets } { incr i } {
		# basekey is a padded dictionary word
		gets $did basekey

		append basekey [repeat "a" [expr $pad_bytes * $i]]

		# key is basekey with the number of dups stuck on.
		set key $basekey$i

		for { set j 0 } { $j < $i } { incr j } {
			set data $basekey.[format %4u $j]
			error_check_good db_put($key,$data) \
			    [eval {$db put} $txn {$key $data}] 0
		}
	}

	# This will make debugging easier, and since the database is
	# read-only from here out, it's cheap.
	error_check_good db_sync [$db sync] 0
}

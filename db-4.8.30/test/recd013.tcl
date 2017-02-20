# See the file LICENSE for redistribution information.
#
# Copyright (c) 2000-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	recd013
# TEST	Test of cursor adjustment on child transaction aborts. [#2373]
#
# XXX
# Other tests that cover more specific variants of the same issue
# are in the access method tests for now.  This is probably wrong;  we
# put this one here because they're closely based on and intertwined
# with other, non-transactional cursor stability tests that are among
# the access method tests, and because we need at least one test to
# fit under recd and keep logtrack from complaining.  We'll sort out the mess
# later;  the important thing, for now, is that everything that needs to gets
# tested.  (This really shouldn't be under recd at all, since it doesn't
# run recovery!)
proc recd013 { method { nitems 100 } args } {
	source ./include.tcl
	global alphabet log_log_record_types

	set args [convert_args $method $args]
	set omethod [convert_method $method]
	set tnum "013"
	set pgsz 512

	if  { [is_partition_callback $args] == 1 } {
		set nodump 1
	} else {
		set nodump 0
	}

	puts "Recd$tnum $method ($args): Test of aborted cursor adjustments."
	set pgindex [lsearch -exact $args "-pagesize"]
	if { $pgindex != -1 } {
		puts "Recd013: skipping for specific pagesizes"
		return
	}

	set testfile recd$tnum.db
	env_cleanup $testdir

	set i 0
	if { [is_record_based $method] == 1 } {
		set keybase ""
	} else {
		set keybase "key"
	}

	puts "\tRecd$tnum.a:\
	    Create environment, database, and parent transaction."
	set flags "-create -txn -home $testdir"

	set env_cmd "berkdb_env $flags"
	set env [eval $env_cmd]
	error_check_good dbenv [is_valid_env $env] TRUE

	set oflags \
    "-auto_commit -env $env -create -mode 0644 -pagesize $pgsz $args $omethod"
	set db [eval {berkdb_open} $oflags $testfile]
	error_check_good dbopen [is_valid_db $db] TRUE

	# Create a database containing $nitems items, numbered with odds.
	# We'll then put the even numbers during the body of the test.
	set txn [$env txn]
	error_check_good init_txn [is_valid_txn $txn $env] TRUE
	for { set i 1 } { $i <= 2 * $nitems } { incr i 2 } {
		set key $keybase$i
		set data [chop_data $method $i$alphabet]

		# First, try to put the item in a child transaction,
		# then abort and verify all the cursors we've done up until
		# now.
		set ctxn [$env txn -parent $txn]
		error_check_good child_txn($i) [is_valid_txn $ctxn $env] TRUE
		error_check_good fake_put($i) [$db put -txn $ctxn $key $data] 0
		error_check_good ctxn_abort($i) [$ctxn abort] 0
		for { set j 1 } { $j < $i } { incr j 2 } {
			error_check_good dbc_get($j):1 [$dbc($j) get -current] \
			    [list [list $keybase$j \
			    [pad_data $method $j$alphabet]]]
		}

		# Then put for real.
		error_check_good init_put($i) [$db put -txn $txn $key $data] 0

		# Set a cursor of the parent txn to each item.
		set dbc($i) [$db cursor -txn $txn]
		error_check_good dbc_getset($i) \
		    [$dbc($i) get -set $key] \
		    [list [list $keybase$i [pad_data $method $i$alphabet]]]

		# And verify all the cursors, including the one we just
		# created.
		for { set j 1 } { $j <= $i } { incr j 2 } {
			error_check_good dbc_get($j):2 [$dbc($j) get -current] \
			    [list [list $keybase$j \
			    [pad_data $method $j$alphabet]]]
		}
	}

	puts "\t\tRecd$tnum.a.1: Verify cursor stability after init."
	for { set i 1 } { $i <= 2 * $nitems } { incr i 2 } {
		error_check_good dbc_get($i):3 [$dbc($i) get -current] \
		    [list [list $keybase$i [pad_data $method $i$alphabet]]]
	}

	puts "\tRecd$tnum.b: Put test."
	puts "\t\tRecd$tnum.b.1: Put items."
	set ctxn [$env txn -parent $txn]
	error_check_good txn [is_valid_txn $ctxn $env] TRUE
	for { set i 2 } { $i <= 2 * $nitems } { incr i 2 } {
		set key $keybase$i
		set data [chop_data $method $i$alphabet]
		error_check_good child_put($i) [$db put -txn $ctxn $key $data] 0

		# If we're a renumbering recno, this is uninteresting.
		# Stir things up by putting a few additional records at
		# the beginning.
		if { [is_rrecno $method] == 1 } {
			set curs [$db cursor -txn $ctxn]
			error_check_bad llength_get_first \
			    [llength [$curs get -first]] 0
			error_check_good cursor [is_valid_cursor $curs $db] TRUE
			# expect a recno!
			error_check_good rrecno_put($i) \
			    [$curs put -before ADDITIONAL.$i] 1
			error_check_good curs_close [$curs close] 0
		}
	}

	puts "\t\tRecd$tnum.b.2: Verify cursor stability after abort."
	error_check_good ctxn_abort [$ctxn abort] 0

	for { set i 1 } { $i <= 2 * $nitems } { incr i 2 } {
		error_check_good dbc_get($i):4 [$dbc($i) get -current] \
		    [list [list $keybase$i [pad_data $method $i$alphabet]]]
	}

	# Clean up cursors.
	for { set i 1 } { $i <= 2 * $nitems } { incr i 2 } {
		error_check_good dbc($i)_close [$dbc($i) close] 0
	}

	# Sync and verify.
	error_check_good txn_commit [$txn commit] 0
	set txn [$env txn]
	error_check_good txn [is_valid_txn $txn $env] TRUE

	error_check_good db_sync [$db sync] 0
	error_check_good db_verify \
	    [verify_dir $testdir "\t\tRecd$tnum.b.3: " 0 0 $nodump] 0

	# Now put back all the even records, this time in the parent.
	# Commit and re-begin the transaction so we can abort and
	# get back to a nice full database.
	for { set i 2 } { $i <= 2 * $nitems } { incr i 2 } {
		set key $keybase$i
		set data [chop_data $method $i$alphabet]
		error_check_good child_put($i) [$db put -txn $txn $key $data] 0
	}
	error_check_good txn_commit [$txn commit] 0
	set txn [$env txn]
	error_check_good txn [is_valid_txn $txn $env] TRUE

	# Delete test.  Set a cursor to each record.  Delete the even ones
	# in the parent and check cursor stability.  Then open a child
	# transaction, and delete the odd ones.  Verify that the database
	# is empty.
	puts "\tRecd$tnum.c: Delete test."
	unset dbc

	# Create cursors pointing at each item.
	for { set i 1 } { $i <= 2 * $nitems } { incr i } {
		set dbc($i) [$db cursor -txn $txn]
		error_check_good dbc($i)_create [is_valid_cursor $dbc($i) $db] \
		    TRUE
		error_check_good dbc_getset($i) [$dbc($i) get -set $keybase$i] \
		    [list [list $keybase$i [pad_data $method $i$alphabet]]]
	}

	puts "\t\tRecd$tnum.c.1: Delete even items in child txn and abort."

	if { [is_rrecno $method] != 1 } {
		set init 2
		set bound [expr 2 * $nitems]
		set step 2
	} else {
		# In rrecno, deletes will renumber the items, so we have
		# to take that into account when we delete by recno.
		set init 2
		set bound [expr $nitems + 1]
		set step 1
	}

	set ctxn [$env txn -parent $txn]
	for { set i $init } { $i <= $bound } { incr i $step } {
		error_check_good del($i) [$db del -txn $ctxn $keybase$i] 0
	}
	error_check_good ctxn_abort [$ctxn abort] 0

	# Verify that no items are deleted.
	for { set i 1 } { $i <= 2 * $nitems } { incr i } {
		error_check_good dbc_get($i):5 [$dbc($i) get -current] \
		    [list [list $keybase$i [pad_data $method $i$alphabet]]]
	}

	puts "\t\tRecd$tnum.c.2: Delete even items in child txn and commit."
	set ctxn [$env txn -parent $txn]
	for { set i $init } { $i <= $bound } { incr i $step } {
		error_check_good del($i) [$db del -txn $ctxn $keybase$i] 0
	}
	error_check_good ctxn_commit [$ctxn commit] 0

	# Verify that even items are deleted and odd items are not.
	for { set i 1 } { $i <= 2 * $nitems } { incr i 2 } {
		if { [is_rrecno $method] != 1 } {
			set j $i
		} else {
			set j [expr ($i - 1) / 2 + 1]
		}
		error_check_good dbc_get($i):6 [$dbc($i) get -current] \
		    [list [list $keybase$j [pad_data $method $i$alphabet]]]
	}
	for { set i 2 } { $i <= 2 * $nitems } { incr i 2 } {
		error_check_good dbc_get($i):7 [$dbc($i) get -current] ""
	}

	puts "\t\tRecd$tnum.c.3: Delete odd items in child txn."

	set ctxn [$env txn -parent $txn]

	for { set i 1 } { $i <= 2 * $nitems } { incr i 2 } {
		if { [is_rrecno $method] != 1 } {
			set j $i
		} else {
			# If this is an rrecno, just delete the first
			# item repeatedly--the renumbering will make
			# that delete everything.
			set j 1
		}
		error_check_good del($i) [$db del -txn $ctxn $keybase$j] 0
	}

	# Verify that everyone's deleted.
	for { set i 1 } { $i <= 2 * $nitems } { incr i } {
		error_check_good get_deleted($i) \
		    [llength [$db get -txn $ctxn $keybase$i]] 0
	}

	puts "\t\tRecd$tnum.c.4: Verify cursor stability after abort."
	error_check_good ctxn_abort [$ctxn abort] 0

	# Verify that even items are deleted and odd items are not.
	for { set i 1 } { $i <= 2 * $nitems } { incr i 2 } {
		if { [is_rrecno $method] != 1 } {
			set j $i
		} else {
			set j [expr ($i - 1) / 2 + 1]
		}
		error_check_good dbc_get($i):8 [$dbc($i) get -current] \
		    [list [list $keybase$j [pad_data $method $i$alphabet]]]
	}
	for { set i 2 } { $i <= 2 * $nitems } { incr i 2 } {
		error_check_good dbc_get($i):9 [$dbc($i) get -current] ""
	}

	# Clean up cursors.
	for { set i 1 } { $i <= 2 * $nitems } { incr i } {
		error_check_good dbc($i)_close [$dbc($i) close] 0
	}

	# Sync and verify.
	error_check_good db_sync [$db sync] 0
	error_check_good db_verify \
	    [verify_dir $testdir "\t\tRecd$tnum.c.5: " 0 0 $nodump] 0

	puts "\tRecd$tnum.d: Clean up."
	error_check_good txn_commit [$txn commit] 0
	error_check_good db_close [$db close] 0
	error_check_good log_flush [$env log_flush] 0
	error_check_good env_close [$env close] 0
	error_check_good verify_dir \
	    [verify_dir $testdir "\t\tRecd$tnum.d.1: " 0 0 $nodump] 0

	if { $log_log_record_types == 1 } {
		logtrack_read $testdir
	}
}

# See the file LICENSE for redistribution information.
#
# Copyright (c) 1999-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	test067
# TEST	Test of DB_CURRENT partial puts onto almost empty duplicate
# TEST	pages, with and without DB_DUP_SORT.
# TEST
# TEST	Test of DB_CURRENT partial puts on almost-empty duplicate pages.
# TEST	This test was written to address the following issue, #2 in the
# TEST	list of issues relating to bug #0820:
# TEST
# TEST	2. DBcursor->put, DB_CURRENT flag, off-page duplicates, hash and btree:
# TEST	In Btree, the DB_CURRENT overwrite of off-page duplicate records
# TEST	first deletes the record and then puts the new one -- this could
# TEST	be a problem if the removal of the record causes a reverse split.
# TEST	Suggested solution is to acquire a cursor to lock down the current
# TEST	record, put a new record after that record, and then delete using
# TEST	the held cursor.
# TEST
# TEST	It also tests the following, #5 in the same list of issues:
# TEST	5. DBcursor->put, DB_AFTER/DB_BEFORE/DB_CURRENT flags, DB_DBT_PARTIAL
# TEST	set, duplicate comparison routine specified.
# TEST	The partial change does not change how data items sort, but the
# TEST	record to be put isn't built yet, and that record supplied is the
# TEST	one that's checked for ordering compatibility.
proc test067 { method {ndups 1000} {tnum "067"} args } {
	source ./include.tcl
	global alphabet
	global errorCode
	global is_je_test

	set args [convert_args $method $args]
	set omethod [convert_method $method]

	set txn ""
	set txnenv 0
	set eindex [lsearch -exact $args "-env"]

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
			if { $ndups == 1000 } {
				set ndups 100
			}
		}
		set testdir [get_home $env]
	}

	cleanup $testdir $env
	if { [is_record_based $method] == 1 || [is_rbtree $method] == 1 } {
	    puts "\tTest$tnum: skipping for method $method."
	    return
	}

	puts "Test$tnum:\
	    $method ($args) Partial puts on near-empty duplicate pages."

	foreach dupopt { "-dup" "-dup -dupsort" } {
		if { $is_je_test || [is_compressed $args] } {
			if { $dupopt == "-dup" } {
				continue
			}
		}

		#
		# Testdir might get reset from the env's home dir back
		# to the default if this calls something that sources
		# include.tcl, since testdir is a global.  Set it correctly
		# here each time through the loop.
		#
		if { $env != "NULL" } {
			set testdir [get_home $env]
		}
		cleanup $testdir $env
		set db [eval {berkdb_open -create -mode 0644 \
		    $omethod} $args $dupopt {$testfile}]
		error_check_good db_open [is_valid_db $db] TRUE

		puts "\tTest$tnum.a ($dupopt): Put $ndups duplicates."

		set key "key_test$tnum"

		for { set ndx 0 } { $ndx < $ndups } { incr ndx } {
			set data $alphabet$ndx

			if { $txnenv == 1 } {
				set t [$env txn]
				error_check_good txn [is_valid_txn $t $env] TRUE
				set txn "-txn $t"
			}
			# No need for pad_data since we're skipping recno.
			set ret [eval {$db put} $txn {$key $data}]
			error_check_good put($key,$data) $ret 0
			if { $txnenv == 1 } {
				error_check_good txn [$t commit] 0
			}
		}

		# Sync so we can inspect database if the next section bombs.
		error_check_good db_sync [$db sync] 0
		puts "\tTest$tnum.b ($dupopt):\
		    Deleting dups (last first), overwriting each."

		if { $txnenv == 1 } {
			set t [$env txn]
			error_check_good txn [is_valid_txn $t $env] TRUE
			set txn "-txn $t"
		}
		set dbc [eval {$db cursor} $txn]
		error_check_good cursor_create [is_valid_cursor $dbc $db] TRUE

		set count 0
		while { $count < $ndups - 1 } {
			# set cursor to last item in db
			set ret [$dbc get -last]
			error_check_good \
			    verify_key [lindex [lindex $ret 0] 0] $key

			# for error reporting
			set currdatum [lindex [lindex $ret 0] 1]

			# partial-overwrite it
			# (overwrite offsets 1-4 with "bcde"--which they
			# already are)

			# Even though we expect success, we catch this
			# since it might return EINVAL, and we want that
			# to FAIL.
			set errorCode NONE
			set ret [catch {eval $dbc put -current \
				{-partial [list 1 4]} "bcde"} \
				res]
			error_check_good \
				partial_put_valid($currdatum) $errorCode NONE
			error_check_good partial_put($currdatum) $res 0

			# delete it
			error_check_good dbc_del [$dbc del] 0

			#puts $currdatum

			incr count
		}

		error_check_good dbc_close [$dbc close] 0
		if { $txnenv == 1 } {
			error_check_good txn [$t commit] 0
		}
		error_check_good db_close [$db close] 0
	}
}

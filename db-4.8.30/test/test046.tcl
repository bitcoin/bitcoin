# See the file LICENSE for redistribution information.
#
# Copyright (c) 1999-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	test046
# TEST	Overwrite test of small/big key/data with cursor checks.
proc test046 { method args } {
	global alphabet
	global errorInfo
	global errorCode
	source ./include.tcl

	set args [convert_args $method $args]
	set omethod [convert_method $method]

	puts "\tTest046: Overwrite test with cursor and small/big key/data."
	puts "\tTest046:\t$method $args"

	if { [is_rrecno $method] == 1} {
		puts "\tTest046: skipping for method $method."
		return
	}

	set key "key"
	set data "data"
	set txn ""
	set flags ""

	if { [is_record_based $method] == 1} {
		set key ""
	}

	puts "\tTest046: Create $method database."
	set txnenv 0
	set eindex [lsearch -exact $args "-env"]
	#
	# If we are using an env, then testfile should just be the db name.
	# Otherwise it is the test directory and the name.
	if { $eindex == -1 } {
		set testfile $testdir/test046
		set env NULL
	} else {
		set testfile test046
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

	set oflags "-create -mode 0644 $args $omethod"
	set db [eval {berkdb_open} $oflags $testfile.a.db]
	error_check_good dbopen [is_valid_db $db] TRUE

	# keep nkeys even
	set nkeys 20

	# Fill page w/ small key/data pairs
	puts "\tTest046: Fill page with $nkeys small key/data pairs."
	for { set i 1 } { $i <= $nkeys } { incr i } {
		if { $txnenv == 1 } {
			set t [$env txn]
			error_check_good txn [is_valid_txn $t $env] TRUE
			set txn "-txn $t"
		}
		if { [is_record_based $method] == 1} {
			set ret [eval {$db put} $txn {$i $data$i}]
		} elseif { $i < 10 } {
			set ret [eval {$db put} $txn [set key]00$i \
			    [set data]00$i]
		} elseif { $i < 100 } {
			set ret [eval {$db put} $txn [set key]0$i \
			    [set data]0$i]
		} else {
			set ret [eval {$db put} $txn {$key$i $data$i}]
		}
		error_check_good dbput $ret 0
		if { $txnenv == 1 } {
			error_check_good txn [$t commit] 0
		}
	}

	# open curs to db
	if { $txnenv == 1 } {
		set t [$env txn]
		error_check_good txn [is_valid_txn $t $env] TRUE
		set txn "-txn $t"
	}
	set dbc [eval {$db cursor} $txn]
	error_check_good db_cursor [is_substr $dbc $db] 1

	# get db order of keys
	for {set i 1; set ret [$dbc get -first]} { [llength $ret] != 0} { \
	    set ret [$dbc get -next]} {
		set key_set($i) [lindex [lindex $ret 0] 0]
		set data_set($i) [lindex [lindex $ret 0] 1]
		incr i
	}

	puts "\tTest046.a: Deletes by key."
	puts "\t\tTest046.a.1: Get data with SET, then delete before cursor."
	# get key in middle of page, call this the nth set curr to it
	set i [expr $nkeys/2]
	set ret [$dbc get -set $key_set($i)]
	error_check_bad dbc_get:set [llength $ret] 0
	set curr $ret

	# delete before cursor(n-1), make sure it is gone
	set i [expr $i - 1]
	error_check_good db_del [eval {$db del} $txn {$key_set($i)}] 0

	# use set_range to get first key starting at n-1, should
	# give us nth--but only works for btree
	if { [is_btree $method] == 1 } {
		set ret [$dbc get -set_range $key_set($i)]
	} else {
		if { [is_record_based $method] == 1 } {
			set ret [$dbc get -set $key_set($i)]
			error_check_good \
			    dbc_get:deleted(recno) [llength [lindex $ret 1]] 0
			#error_check_good \
			#   catch:get [catch {$dbc get -set $key_set($i)} ret] 1
			#error_check_good \
			#   dbc_get:deleted(recno) [is_substr $ret "KEYEMPTY"] 1
		} else {
			set ret [$dbc get -set $key_set($i)]
			error_check_good dbc_get:deleted [llength $ret] 0
		}
		set ret [$dbc get -set $key_set([incr i])]
		incr i -1
	}
	error_check_bad dbc_get:set(R)(post-delete) [llength $ret] 0
	error_check_good dbc_get(match):set $ret $curr

	puts "\t\tTest046.a.2: Delete cursor item by key."
	# nth key, which cursor should be on now
	set i [incr i]
	set ret [eval {$db del} $txn {$key_set($i)}]
	error_check_good db_del $ret 0

	# this should return n+1 key/data, curr has nth key/data
	if { [string compare $omethod "-btree"] == 0 } {
		set ret [$dbc get -set_range $key_set($i)]
	} else {
		if { [is_record_based $method] == 1 } {
			set ret [$dbc get -set $key_set($i)]
			error_check_good \
			    dbc_get:deleted(recno) [llength [lindex $ret 1]] 0
			#error_check_good \
			#   catch:get [catch {$dbc get -set $key_set($i)} ret] 1
			#error_check_good \
			#   dbc_get:deleted(recno) [is_substr $ret "KEYEMPTY"] 1
		} else {
			set ret [$dbc get -set $key_set($i)]
			error_check_good dbc_get:deleted [llength $ret] 0
		}
		set ret [$dbc get -set $key_set([expr $i+1])]
	}
	error_check_bad dbc_get(post-delete):set_range [llength $ret] 0
	error_check_bad dbc_get(no-match):set_range $ret $curr

	puts "\t\tTest046.a.3: Delete item after cursor."
	# we'll delete n+2, since we have deleted n-1 and n
	# i still equal to nth, cursor on n+1
	set i [incr i]
	set ret [$dbc get -set $key_set($i)]
	error_check_bad dbc_get:set [llength $ret] 0
	set curr [$dbc get -next]
	error_check_bad dbc_get:next [llength $curr] 0
	set ret [$dbc get -prev]
	error_check_bad dbc_get:prev [llength $curr] 0
	# delete *after* cursor pos.
	error_check_good db:del [eval {$db del} $txn {$key_set([incr i])}] 0

	# make sure item is gone, try to get it
	if { [string compare $omethod "-btree"] == 0} {
		set ret [$dbc get -set_range $key_set($i)]
	} else {
		if { [is_record_based $method] == 1 } {
			set ret [$dbc get -set $key_set($i)]
			error_check_good \
			    dbc_get:deleted(recno) [llength [lindex $ret 1]] 0
			#error_check_good \
			#   catch:get [catch {$dbc get -set $key_set($i)} ret] 1
			#error_check_good \
			#   dbc_get:deleted(recno) [is_substr $ret "KEYEMPTY"] 1
		} else {
			set ret [$dbc get -set $key_set($i)]
			error_check_good dbc_get:deleted [llength $ret] 0
		}
		set ret [$dbc get -set $key_set([expr $i +1])]
	}
	error_check_bad dbc_get:set(_range) [llength $ret] 0
	error_check_bad dbc_get:set(_range) $ret $curr
	error_check_good dbc_get:set [lindex [lindex $ret 0] 0] \
											$key_set([expr $i+1])

	puts "\tTest046.b: Deletes by cursor."
	puts "\t\tTest046.b.1: Delete, do DB_NEXT."
	error_check_good dbc:del [$dbc del] 0
	set ret [$dbc get -next]
	error_check_bad dbc_get:next [llength $ret] 0
	set i [expr $i+2]
	# i = n+4
	error_check_good dbc_get:next(match) \
		[lindex [lindex $ret 0] 0] $key_set($i)

	puts "\t\tTest046.b.2: Delete, do DB_PREV."
	error_check_good dbc:del [$dbc del] 0
	set ret [$dbc get -prev]
	error_check_bad dbc_get:prev [llength $ret] 0
	set i [expr $i-3]
	# i = n+1 (deleted all in between)
	error_check_good dbc_get:prev(match) \
		[lindex [lindex $ret 0] 0] $key_set($i)

	puts "\t\tTest046.b.3: Delete, do DB_CURRENT."
	error_check_good dbc:del [$dbc del] 0
	# we just deleted, so current item should be KEYEMPTY, throws err
	set ret [$dbc get -current]
	error_check_good dbc_get:curr:deleted [llength [lindex $ret 1]] 0
	#error_check_good catch:get:current [catch {$dbc get -current} ret] 1
	#error_check_good dbc_get:curr:deleted [is_substr $ret "DB_KEYEMPTY"] 1

	puts "\tTest046.c: Inserts (before/after), by key then cursor."
	puts "\t\tTest046.c.1: Insert by key before the cursor."
	# i is at curs pos, i=n+1, we want to go BEFORE
	set i [incr i -1]
	set ret [eval {$db put} $txn {$key_set($i) $data_set($i)}]
	error_check_good db_put:before $ret 0

	puts "\t\tTest046.c.2: Insert by key after the cursor."
	set i [incr i +2]
	set ret [eval {$db put} $txn {$key_set($i) $data_set($i)}]
	error_check_good db_put:after $ret 0

	puts "\t\tTest046.c.3: Insert by curs with deleted curs (should fail)."
	# cursor is on n+1, we'll change i to match
	set i [incr i -1]

	error_check_good dbc:close [$dbc close] 0
	if { $txnenv == 1 } {
		error_check_good txn [$t commit] 0
	}
	error_check_good db:close [$db close] 0
	if { [is_record_based $method] == 1} {
		puts "\t\tSkipping the rest of test for method $method."
		puts "\tTest046 ($method) complete."
		return
	} else {
		# Reopen without printing __db_errs.
		set db [eval {berkdb_open_noerr} $oflags $testfile.a.db]
		error_check_good dbopen [is_valid_db $db] TRUE
		if { $txnenv == 1 } {
			set t [$env txn]
			error_check_good txn [is_valid_txn $t $env] TRUE
			set txn "-txn $t"
		}
		set dbc [eval {$db cursor} $txn]
		error_check_good cursor [is_valid_cursor $dbc $db] TRUE

		# should fail with EINVAL (deleted cursor)
		set errorCode NONE
		error_check_good catch:put:before 1 \
			[catch {$dbc put -before $data_set($i)} ret]
		error_check_good dbc_put:deleted:before \
			[is_substr $errorCode "EINVAL"] 1

		# should fail with EINVAL
		set errorCode NONE
		error_check_good catch:put:after 1 \
			[catch {$dbc put -after $data_set($i)} ret]
		error_check_good dbc_put:deleted:after \
			[is_substr $errorCode "EINVAL"] 1

		puts "\t\tTest046.c.4:\
		    Insert by cursor before/after existent cursor."
		# can't use before after w/o dup except renumber in recno
		# first, restore an item so they don't fail
		#set ret [eval {$db put} $txn {$key_set($i) $data_set($i)}]
		#error_check_good db_put $ret 0

		#set ret [$dbc get -set $key_set($i)]
		#error_check_bad dbc_get:set [llength $ret] 0
		#set i [incr i -2]
		# i = n - 1
		#set ret [$dbc get -prev]
		#set ret [$dbc put -before $key_set($i) $data_set($i)]
		#error_check_good dbc_put:before $ret 0
		# cursor pos is adjusted to match prev, recently inserted
		#incr i
		# i = n
		#set ret [$dbc put -after $key_set($i) $data_set($i)]
		#error_check_good dbc_put:after $ret 0
	}

	# For the next part of the test, we need a db with no dups to test
	# overwrites
	puts "\tTest046.d.0: Cleanup, close db, open new db with no dups."
	error_check_good dbc:close [$dbc close] 0
	if { $txnenv == 1 } {
		error_check_good txn [$t commit] 0
	}
	error_check_good db:close [$db close] 0

	set db [eval {berkdb_open} $oflags $testfile.d.db]
	error_check_good dbopen [is_valid_db $db] TRUE
	# Fill page w/ small key/data pairs
	puts "\tTest046.d.0: Fill page with $nkeys small key/data pairs."
	for { set i 1 } { $i < $nkeys } { incr i } {
		if { $txnenv == 1 } {
			set t [$env txn]
			error_check_good txn [is_valid_txn $t $env] TRUE
			set txn "-txn $t"
		}
		set ret [eval {$db put} $txn {$key$i $data$i}]
		error_check_good dbput $ret 0
		if { $txnenv == 1 } {
			error_check_good txn [$t commit] 0
		}
	}

	if { $txnenv == 1 } {
		set t [$env txn]
		error_check_good txn [is_valid_txn $t $env] TRUE
		set txn "-txn $t"
	}
	set dbc [eval {$db cursor} $txn]
	error_check_good db_cursor [is_valid_cursor $dbc $db] TRUE
	set nkeys 20

	# Prepare cursor on item
	set ret [$dbc get -first]
	error_check_bad dbc_get:first [llength $ret] 0

	# Prepare unique big/small values for an initial
	# and an overwrite set of key/data
	foreach ptype {init over} {
		foreach size {big small} {
			if { [string compare $size big] == 0 } {
				set key_$ptype$size \
				    KEY_$size[repeat alphabet 250]
				set data_$ptype$size \
				    DATA_$size[repeat alphabet 250]
			} else {
				set key_$ptype$size \
				    KEY_$size[repeat alphabet 10]
				set data_$ptype$size \
				    DATA_$size[repeat alphabet 10]
			}
		}
	}

	set i 0
	# Do all overwrites for key and cursor
	foreach type {key_over curs_over} {
		# Overwrite (i=initial) four different kinds of pairs
		incr i
		puts "\tTest046.d: Overwrites $type."
		foreach i_pair {\
		    {small small} {big small} {small big} {big big} } {
			# Overwrite (w=write) with four different kinds of data
		   foreach w_pair {\
		       {small small} {big small} {small big} {big big} } {

				# we can only overwrite if key size matches
				if { [string compare [lindex \
				    $i_pair 0] [lindex $w_pair 0]] != 0} {
					continue
				}

				# first write the initial key/data
				set ret [$dbc put -keyfirst \
				    key_init[lindex $i_pair 0] \
				    data_init[lindex $i_pair 1]]
				error_check_good \
				    dbc_put:curr:init:$i_pair $ret 0
				set ret [$dbc get -current]
				error_check_bad dbc_get:curr [llength $ret] 0
				error_check_good dbc_get:curr:data \
				    [lindex [lindex $ret 0] 1] \
				    data_init[lindex $i_pair 1]

				# Now, try to overwrite: dups not supported in
				# this db
				if { [string compare $type key_over] == 0 } {
					puts "\t\tTest046.d.$i: Key\
					    Overwrite:($i_pair) by ($w_pair)."
					set ret [eval {$db put} $txn \
					    $"key_init[lindex $i_pair 0]" \
					    $"data_over[lindex $w_pair 1]"]
					error_check_good \
				dbput:over:i($i_pair):o($w_pair) $ret 0
					# check value
					set ret [eval {$db get} $txn \
					    $"key_init[lindex $i_pair 0]"]
					error_check_bad \
					    db:get:check [llength $ret] 0
					error_check_good db:get:compare_data \
					    [lindex [lindex $ret 0] 1] \
					    $"data_over[lindex $w_pair 1]"
				} else {
					# This is a cursor overwrite
					puts \
		"\t\tTest046.d.$i:Curs Overwrite:($i_pair) by ($w_pair)."
					set ret [$dbc put -current \
					    $"data_over[lindex $w_pair 1]"]
					error_check_good \
				    dbcput:over:i($i_pair):o($w_pair) $ret 0
					# check value
					set ret [$dbc get -current]
					error_check_bad \
					    dbc_get:curr [llength $ret] 0
					error_check_good dbc_get:curr:data \
					    [lindex [lindex $ret 0] 1] \
					    $"data_over[lindex $w_pair 1]"
				}
			} ;# foreach write pair
		} ;# foreach initial pair
	} ;# foreach type big/small

	puts "\tTest046.d.3: Cleanup for next part of test."
	error_check_good dbc_close [$dbc close] 0
	if { $txnenv == 1 } {
		error_check_good txn [$t commit] 0
	}
	error_check_good db_close [$db close] 0

	if { [is_rbtree $method] == 1} {
		puts "\tSkipping the rest of Test046 for method $method."
		puts "\tTest046 complete."
		return
	}

	puts "\tTest046.e.1: Open db with sorted dups."
	set db [eval {berkdb_open_noerr} $oflags -dup -dupsort $testfile.e.db]
	error_check_good dbopen [is_valid_db $db] TRUE

	# keep nkeys even
	set nkeys 20
	set ndups 20

	# Fill page w/ small key/data pairs
	puts "\tTest046.e.2:\
	    Put $nkeys small key/data pairs and $ndups sorted dups."
	for { set i 0 } { $i < $nkeys } { incr i } {
		if { $txnenv == 1 } {
			set t [$env txn]
			error_check_good txn [is_valid_txn $t $env] TRUE
			set txn "-txn $t"
		}
		if { $i < 10 } {
			set ret [eval {$db put} $txn [set key]0$i [set data]0$i]
		} else {
			set ret [eval {$db put} $txn {$key$i $data$i}]
		}
		error_check_good dbput $ret 0
		if { $txnenv == 1 } {
			error_check_good txn [$t commit] 0
		}
	}

	if { $txnenv == 1 } {
		set t [$env txn]
		error_check_good txn [is_valid_txn $t $env] TRUE
		set txn "-txn $t"
	}
	# open curs to db
	set dbc [eval {$db cursor} $txn]
	error_check_good db_cursor [is_substr $dbc $db] 1

	# get db order of keys
	for {set i 0; set ret [$dbc get -first]} { [llength $ret] != 0} { \
			set ret [$dbc get -next]} {
		set key_set($i) [lindex [lindex $ret 0] 0]
		set data_set($i) [lindex [lindex $ret 0] 1]
		incr i
	}

	# put 20 sorted duplicates on key in middle of page
	set i [expr $nkeys/2]
	set ret [$dbc get -set $key_set($i)]
	error_check_bad dbc_get:set [llength $ret] 0

	set keym $key_set($i)

	for { set i 0 } { $i < $ndups } { incr i } {
		if { $i < 10 } {
			set ret [eval {$db put} $txn {$keym DUPLICATE_0$i}]
		} else {
			set ret [eval {$db put} $txn {$keym DUPLICATE_$i}]
		}
		error_check_good db_put:DUP($i) $ret 0
	}

	puts "\tTest046.e.3: Check duplicate duplicates"
	set ret [eval {$db put} $txn {$keym DUPLICATE_00}]
	error_check_good dbput:dupdup [is_substr $ret "DB_KEYEXIST"] 1

	# get dup ordering
	for {set i 0; set ret [$dbc get -set $keym]} { [llength $ret] != 0} {\
			set ret [$dbc get -nextdup] } {
		set dup_set($i) [lindex [lindex $ret 0] 1]
		incr i
	}

	# put cursor on item in middle of dups
	set i [expr $ndups/2]
	set ret [$dbc get -get_both $keym $dup_set($i)]
	error_check_bad dbc_get:get_both [llength $ret] 0

	puts "\tTest046.f: Deletes by cursor."
	puts "\t\tTest046.f.1: Delete by cursor, do a DB_NEXT, check cursor."
	set ret [$dbc get -current]
	error_check_bad dbc_get:current [llength $ret] 0
	error_check_good dbc:del [$dbc del] 0
	set ret [$dbc get -next]
	error_check_bad dbc_get:next [llength $ret] 0
	error_check_good \
	    dbc_get:nextdup [lindex [lindex $ret 0] 1] $dup_set([incr i])

	puts "\t\tTest046.f.2: Delete by cursor, do DB_PREV, check cursor."
	error_check_good dbc:del [$dbc del] 0
	set ret [$dbc get -prev]
	error_check_bad dbc_get:prev [llength $ret] 0
	set i [incr i -2]
	error_check_good dbc_get:prev [lindex [lindex $ret 0] 1] $dup_set($i)

	puts "\t\tTest046.f.3: Delete by cursor, do DB_CURRENT, check cursor."
	error_check_good dbc:del [$dbc del] 0
	set ret [$dbc get -current]
	error_check_good dbc_get:current:deleted [llength [lindex $ret 1]] 0
	#error_check_good catch:dbc_get:curr [catch {$dbc get -current} ret] 1
	#error_check_good \
	#   dbc_get:current:deleted [is_substr $ret "DB_KEYEMPTY"] 1
	error_check_good dbc_close [$dbc close] 0
	if { $txnenv == 1 } {
		error_check_good txn [$t commit] 0
	}

	if { $txnenv == 1 } {
		set t [$env txn]
		error_check_good txn [is_valid_txn $t $env] TRUE
		set txn "-txn $t"
	}
	# restore deleted keys
	error_check_good db_put:1 [eval {$db put} $txn {$keym $dup_set($i)}] 0
	error_check_good db_put:2 [eval {$db put} $txn \
	    {$keym $dup_set([incr i])}] 0
	error_check_good db_put:3 [eval {$db put} $txn \
	    {$keym $dup_set([incr i])}] 0
	if { $txnenv == 1 } {
		error_check_good txn [$t commit] 0
	}

	# tested above

	# Reopen database without __db_err, reset cursor
	error_check_good dbclose [$db close] 0
	set db [eval {berkdb_open_noerr} $oflags -dup -dupsort $testfile.e.db]
	error_check_good dbopen [is_valid_db $db] TRUE
	if { $txnenv == 1 } {
		set t [$env txn]
		error_check_good txn [is_valid_txn $t $env] TRUE
		set txn "-txn $t"
	}
	set dbc [eval {$db cursor} $txn]
	error_check_good db_cursor [is_valid_cursor $dbc $db] TRUE

	set ret [$dbc get -set $keym]
	error_check_bad dbc_get:set [llength $ret] 0
	set ret2 [$dbc get -current]
	error_check_bad dbc_get:current [llength $ret2] 0
	# match
	error_check_good dbc_get:current/set(match) $ret $ret2
	# right one?
	error_check_good \
	    dbc_get:curr/set(matchdup) [lindex [lindex $ret 0] 1] $dup_set(0)

	# cursor is on first dup
	set ret [$dbc get -next]
	error_check_bad dbc_get:next [llength $ret] 0
	# now on second dup
	error_check_good dbc_get:next [lindex [lindex $ret 0] 1] $dup_set(1)
	# check cursor
	set ret [$dbc get -current]
	error_check_bad dbc_get:curr [llength $ret] 0
	error_check_good \
	    dbcget:curr(compare) [lindex [lindex $ret 0] 1] $dup_set(1)

	puts "\tTest046.g: Inserts."
	puts "\t\tTest046.g.1: Insert by key before cursor."
	set i 0

	# use "spam" to prevent a duplicate duplicate.
	set ret [eval {$db put} $txn {$keym $dup_set($i)spam}]
	error_check_good db_put:before $ret 0
	# make sure cursor was maintained
	set ret [$dbc get -current]
	error_check_bad dbc_get:curr [llength $ret] 0
	error_check_good \
	    dbc_get:current(post-put) [lindex [lindex $ret 0] 1] $dup_set(1)

	puts "\t\tTest046.g.2: Insert by key after cursor."
	set i [expr $i + 2]
	# use "eggs" to prevent a duplicate duplicate
	set ret [eval {$db put} $txn {$keym $dup_set($i)eggs}]
	error_check_good db_put:after $ret 0
	# make sure cursor was maintained
	set ret [$dbc get -current]
	error_check_bad dbc_get:curr [llength $ret] 0
	error_check_good \
	    dbc_get:curr(post-put,after) [lindex [lindex $ret 0] 1] $dup_set(1)

	puts "\t\tTest046.g.3: Insert by curs before/after curs (should fail)."
	# should return EINVAL (dupsort specified)
	error_check_good dbc_put:before:catch \
	    [catch {$dbc put -before $dup_set([expr $i -1])} ret] 1
	error_check_good \
	    dbc_put:before:deleted [is_substr $errorCode "EINVAL"] 1
	error_check_good dbc_put:after:catch \
	    [catch {$dbc put -after $dup_set([expr $i +2])} ret] 1
	error_check_good \
	    dbc_put:after:deleted [is_substr $errorCode "EINVAL"] 1

	puts "\tTest046.h: Cursor overwrites."
	puts "\t\tTest046.h.1: Test that dupsort disallows current overwrite."
	set ret [$dbc get -set $keym]
	error_check_bad dbc_get:set [llength $ret] 0
	error_check_good \
	    catch:dbc_put:curr [catch {$dbc put -current DATA_OVERWRITE} ret] 1
	error_check_good dbc_put:curr:dupsort [is_substr $errorCode EINVAL] 1

	puts "\t\tTest046.h.2: New db (no dupsort)."
	error_check_good dbc_close [$dbc close] 0
	if { $txnenv == 1 } {
		error_check_good txn [$t commit] 0
	}
	error_check_good db_close [$db close] 0

	# Skip the rest of the test for compressed btree, now that
	# we're no longer running with -dupsort. 
	if { [is_compressed $args] == 1 } {
		puts "Skipping remainder of test046\
		    for btree with compression."
		return
	}

	set db [eval {berkdb_open} \
	    $oflags -dup $testfile.h.db]
	error_check_good db_open [is_valid_db $db] TRUE
	if { $txnenv == 1 } {
		set t [$env txn]
		error_check_good txn [is_valid_txn $t $env] TRUE
		set txn "-txn $t"
	}
	set dbc [eval {$db cursor} $txn]
	error_check_good db_cursor [is_valid_cursor $dbc $db] TRUE

	for {set i 0} {$i < $nkeys} {incr i} {
		if { $i < 10 } {
			set ret [eval {$db put} $txn {key0$i datum0$i}]
			error_check_good db_put $ret 0
		} else {
			set ret [eval {$db put} $txn {key$i datum$i}]
			error_check_good db_put $ret 0
		}
		if { $i == 0 } {
			for {set j 0} {$j < $ndups} {incr j} {
				if { $i < 10 } {
					set keyput key0$i
				} else {
					set keyput key$i
				}
				if { $j < 10 } {
					set ret [eval {$db put} $txn \
					    {$keyput DUP_datum0$j}]
				} else {
					set ret [eval {$db put} $txn \
					    {$keyput DUP_datum$j}]
				}
				error_check_good dbput:dup $ret 0
			}
		}
	}

	for {set i 0; set ret [$dbc get -first]} { [llength $ret] != 0} { \
	    set ret [$dbc get -next]} {
		set key_set($i) [lindex [lindex $ret 0] 0]
		set data_set($i) [lindex [lindex $ret 0] 1]
		incr i
	}

	for {set i 0; set ret [$dbc get -set key00]} {\
	    [llength $ret] != 0} {set ret [$dbc get -nextdup]} {
		set dup_set($i) [lindex [lindex $ret 0] 1]
		incr i
	}
	set i 0
	set keym key0$i
	set ret [$dbc get -set $keym]
	error_check_bad dbc_get:set [llength $ret] 0
	error_check_good \
	    dbc_get:set(match) [lindex [lindex $ret 0] 1] $dup_set($i)

	set ret [$dbc get -nextdup]
	error_check_bad dbc_get:nextdup [llength $ret] 0
	error_check_good dbc_get:nextdup(match) \
	    [lindex [lindex $ret 0] 1] $dup_set([expr $i + 1])

	puts "\t\tTest046.h.3: Insert by cursor before cursor (DB_BEFORE)."
	set ret [$dbc put -before BEFOREPUT]
	error_check_good dbc_put:before $ret 0
	set ret [$dbc get -current]
	error_check_bad dbc_get:curr [llength $ret] 0
	error_check_good \
	    dbc_get:curr:match [lindex [lindex $ret 0] 1] BEFOREPUT
	# make sure that this is actually a dup w/ dup before
	set ret [$dbc get -prev]
	error_check_bad dbc_get:prev [llength $ret] 0
	error_check_good dbc_get:prev:match \
		[lindex [lindex $ret 0] 1] $dup_set($i)
	set ret [$dbc get -prev]
	# should not be a dup
	error_check_bad dbc_get:prev(no_dup) \
		[lindex [lindex $ret 0] 0] $keym

	puts "\t\tTest046.h.4: Insert by cursor after cursor (DB_AFTER)."
	set ret [$dbc get -set $keym]

	# delete next 3 when fix
	#puts "[$dbc get -current]\
	#   [$dbc get -next] [$dbc get -next] [$dbc get -next] [$dbc get -next]"
	#set ret [$dbc get -set $keym]

	error_check_bad dbc_get:set [llength $ret] 0
	set ret [$dbc put -after AFTERPUT]
	error_check_good dbc_put:after $ret 0
	#puts [$dbc get -current]

	# delete next 3 when fix
	#set ret [$dbc get -set $keym]
	#puts "[$dbc get -current] next: [$dbc get -next] [$dbc get -next]"
	#set ret [$dbc get -set AFTERPUT]
	#set ret [$dbc get -set $keym]
	#set ret [$dbc get -next]
	#puts $ret

	set ret [$dbc get -current]
	error_check_bad dbc_get:curr [llength $ret] 0
	error_check_good dbc_get:curr:match [lindex [lindex $ret 0] 1] AFTERPUT
	set ret [$dbc get -prev]
	# now should be on first item (non-dup) of keym
	error_check_bad dbc_get:prev1 [llength $ret] 0
	error_check_good \
	    dbc_get:match [lindex [lindex $ret 0] 1] $dup_set($i)
	set ret [$dbc get -next]
	error_check_bad dbc_get:next [llength $ret] 0
	error_check_good \
	    dbc_get:match2 [lindex [lindex $ret 0] 1] AFTERPUT
	set ret [$dbc get -next]
	error_check_bad dbc_get:next [llength $ret] 0
	# this is the dup we added previously
	error_check_good \
	    dbc_get:match3 [lindex [lindex $ret 0] 1] BEFOREPUT

	# now get rid of the dups we added
	error_check_good dbc_del [$dbc del] 0
	set ret [$dbc get -prev]
	error_check_bad dbc_get:prev2 [llength $ret] 0
	error_check_good dbc_del2 [$dbc del] 0
	# put cursor on first dup item for the rest of test
	set ret [$dbc get -set $keym]
	error_check_bad dbc_get:first [llength $ret] 0
	error_check_good \
	    dbc_get:first:check [lindex [lindex $ret 0] 1] $dup_set($i)

	puts "\t\tTest046.h.5: Overwrite small by small."
	set ret [$dbc put -current DATA_OVERWRITE]
	error_check_good dbc_put:current:overwrite $ret 0
	set ret [$dbc get -current]
	error_check_good dbc_get:current(put,small/small) \
	    [lindex [lindex $ret 0] 1] DATA_OVERWRITE

	puts "\t\tTest046.h.6: Overwrite small with big."
	set ret [$dbc put -current DATA_BIG_OVERWRITE[repeat $alphabet 200]]
	error_check_good dbc_put:current:overwrite:big $ret 0
	set ret [$dbc get -current]
	error_check_good dbc_get:current(put,small/big) \
	    [is_substr [lindex [lindex $ret 0] 1] DATA_BIG_OVERWRITE] 1

	puts "\t\tTest046.h.7: Overwrite big with big."
	set ret [$dbc put -current DATA_BIG_OVERWRITE2[repeat $alphabet 200]]
	error_check_good dbc_put:current:overwrite(2):big $ret 0
	set ret [$dbc get -current]
	error_check_good dbc_get:current(put,big/big) \
	    [is_substr [lindex [lindex $ret 0] 1] DATA_BIG_OVERWRITE2] 1

	puts "\t\tTest046.h.8: Overwrite big with small."
	set ret [$dbc put -current DATA_OVERWRITE2]
	error_check_good dbc_put:current:overwrite:small $ret 0
	set ret [$dbc get -current]
	error_check_good dbc_get:current(put,big/small) \
	    [is_substr [lindex [lindex $ret 0] 1] DATA_OVERWRITE2] 1

	puts "\tTest046.i: Cleaning up from test."
	error_check_good dbc_close [$dbc close] 0
	if { $txnenv == 1 } {
		error_check_good txn [$t commit] 0
	}
	error_check_good db_close [$db close] 0

	puts "\tTest046 complete."
}

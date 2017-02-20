# See the file LICENSE for redistribution information.
#
# Copyright (c) 1999-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	test052
# TEST	Renumbering record Recno test.
proc test052 { method args } {
	global alphabet
	global errorInfo
	global errorCode
	source ./include.tcl

	set args [convert_args $method $args]
	set omethod [convert_method $method]

	puts "Test052: Test of renumbering recno."
	if { [is_rrecno $method] != 1} {
		puts "Test052: skipping for method $method."
		return
	}

	set data "data"
	set txn ""
	set flags ""

	puts "\tTest052: Create $method database."
	set txnenv 0
	set eindex [lsearch -exact $args "-env"]
	#
	# If we are using an env, then testfile should just be the db name.
	# Otherwise it is the test directory and the name.
	if { $eindex == -1 } {
		set testfile $testdir/test052.db
		set env NULL
	} else {
		set testfile test052.db
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
	set db [eval {berkdb_open} $oflags $testfile]
	error_check_good dbopen [is_valid_db $db] TRUE

	# keep nkeys even
	set nkeys 20

	# Fill page w/ small key/data pairs
	puts "\tTest052: Fill page with $nkeys small key/data pairs."
	for { set i 1 } { $i <= $nkeys } { incr i } {
		if { $txnenv == 1 } {
			set t [$env txn]
			error_check_good txn [is_valid_txn $t $env] TRUE
			set txn "-txn $t"
		}
		set ret [eval {$db put} $txn {$i $data$i}]
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
	error_check_good db_cursor [is_valid_cursor $dbc $db] TRUE

	# get db order of keys
	for {set i 1; set ret [$dbc get -first]} { [llength $ret] != 0} { \
	    set ret [$dbc get -next]} {
		set keys($i) [lindex [lindex $ret 0] 0]
		set darray($i) [lindex [lindex $ret 0] 1]
		incr i
	}

	puts "\tTest052: Deletes by key."
	puts "\tTest052.a: Get data with SET, then delete before cursor."
	# get key in middle of page, call this the nth set curr to it
	set i [expr $nkeys/2]
	set k $keys($i)
	set ret [$dbc get -set $k]
	error_check_bad dbc_get:set [llength $ret] 0
	error_check_good dbc_get:set [lindex [lindex $ret 0] 1] $darray($i)

	# delete by key before current
	set i [incr i -1]
	error_check_good db_del:before [eval {$db del} $txn {$keys($i)}] 0
	# with renumber, current's data should be constant, but key==--key
	set i [incr i +1]
	error_check_good dbc:data \
	    [lindex [lindex [$dbc get -current] 0] 1] $darray($i)
	error_check_good dbc:keys \
	    [lindex [lindex [$dbc get -current] 0] 0] $keys([expr $nkeys/2 - 1])

	puts "\tTest052.b: Delete cursor item by key."
	set i [expr $nkeys/2 ]

	set ret [$dbc get -set $keys($i)]
	error_check_bad dbc:get [llength $ret] 0
	error_check_good dbc:get:curs [lindex [lindex $ret 0] 1] \
	    $darray([expr $i + 1])
	error_check_good db_del:curr [eval {$db del} $txn {$keys($i)}] 0
	set ret [$dbc get -current]

	# After a delete, cursor should return DB_NOTFOUND.
	error_check_good dbc:get:key [llength [lindex [lindex $ret 0] 0]] 0
	error_check_good dbc:get:data [llength [lindex [lindex $ret 0] 1]] 0

	# And the item after the cursor should now be
	# key: $nkeys/2, data: $nkeys/2 + 2
	set ret [$dbc get -next]
	error_check_bad dbc:getnext [llength $ret] 0
	error_check_good dbc:getnext:data \
	    [lindex [lindex $ret 0] 1] $darray([expr $i + 2])
	error_check_good dbc:getnext:keys \
	    [lindex [lindex $ret 0] 0] $keys($i)

	puts "\tTest052.c: Delete item after cursor."
	# should be { keys($nkeys/2), darray($nkeys/2 + 2) }
	set i [expr $nkeys/2]
	# deleting data for key after current (key $nkeys/2 + 1)
	error_check_good db_del [eval {$db del} $txn {$keys([expr $i + 1])}] 0

	# current should be constant
	set ret [$dbc get -current]
	error_check_bad dbc:get:current [llength $ret] 0
	error_check_good dbc:get:keys [lindex [lindex $ret 0] 0] \
	    $keys($i)
	error_check_good dbc:get:data [lindex [lindex $ret 0] 1] \
	    $darray([expr $i + 2])

	puts "\tTest052: Deletes by cursor."
	puts "\tTest052.d: Delete, do DB_NEXT."
	set i 1
	set ret [$dbc get -first]
	error_check_bad dbc_get:first [llength $ret] 0
	error_check_good dbc_get:first [lindex [lindex $ret 0] 1] $darray($i)
	error_check_good dbc_del [$dbc del] 0
	set ret [$dbc get -current]
	error_check_good dbc_get:current [llength $ret] 0

	set ret [$dbc get -next]
	error_check_bad dbc_get:next [llength $ret] 0
	error_check_good dbc:get:curs \
	    [lindex [lindex $ret 0] 1] $darray([expr $i + 1])
	error_check_good dbc:get:keys \
	    [lindex [lindex $ret 0] 0] $keys($i)

	# Move one more forward, so we're not on the first item.
	error_check_bad dbc:getnext [llength [$dbc get -next]] 0

	puts "\tTest052.e: Delete, do DB_PREV."
	error_check_good dbc:del [$dbc del] 0
	set ret [$dbc get -current]
	error_check_good dbc:get:curr [llength $ret] 0

	# next should now reference the record that was previously after
	# old current
	set ret [$dbc get -next]
	error_check_bad get:next [llength $ret] 0
	error_check_good dbc:get:next:data \
	    [lindex [lindex $ret 0] 1] $darray([expr $i + 3])
	error_check_good dbc:get:next:keys \
	    [lindex [lindex $ret 0] 0] $keys([expr $i + 1])


	set ret [$dbc get -prev]
	error_check_bad dbc:get:curr [llength $ret] 0
	error_check_good dbc:get:curr:compare \
	    [lindex [lindex $ret 0] 1] $darray([expr $i + 1])
	error_check_good dbc:get:curr:keys \
	    [lindex [lindex $ret 0] 0] $keys($i)

	# The rest of the test was written with the old rrecno semantics,
	# which required a separate c_del(CURRENT) test;  to leave
	# the database in the expected state, we now delete the first item.
	set ret [$dbc get -first]
	error_check_bad getfirst [llength $ret] 0
	error_check_good delfirst [$dbc del] 0

	puts "\tTest052: Inserts."
	puts "\tTest052.g: Insert before (DB_BEFORE)."
	set i 1
	set ret [$dbc get -first]
	error_check_bad dbc:get:first [llength $ret] 0
	error_check_good dbc_get:first \
	    [lindex [lindex $ret 0] 0] $keys($i)
	error_check_good dbc_get:first:data \
	    [lindex [lindex $ret 0] 1] $darray([expr $i + 3])

	set ret [$dbc put -before $darray($i)]
	# should return new key, which should be $keys($i)
	error_check_good dbc_put:before $ret $keys($i)
	# cursor should adjust to point to new item
	set ret [$dbc get -current]
	error_check_bad dbc_get:curr [llength $ret] 0
	error_check_good dbc_put:before:keys \
	    [lindex [lindex $ret 0] 0] $keys($i)
	error_check_good dbc_put:before:data \
	    [lindex [lindex $ret 0] 1] $darray($i)

	set ret [$dbc get -next]
	error_check_bad dbc_get:next [llength $ret] 0
	error_check_good dbc_get:next:compare \
	   $ret [list [list $keys([expr $i + 1]) $darray([expr $i + 3])]]
	set ret [$dbc get -prev]
	error_check_bad dbc_get:prev [llength $ret] 0

	puts "\tTest052.h: Insert by cursor after (DB_AFTER)."
	set i [incr i]
	set ret [$dbc put -after $darray($i)]
	# should return new key, which should be $keys($i)
	error_check_good dbcput:after $ret $keys($i)
	# cursor should reference new item
	set ret [$dbc get -current]
	error_check_good dbc:get:current:keys \
	    [lindex [lindex $ret 0] 0] $keys($i)
	error_check_good dbc:get:current:data \
	    [lindex [lindex $ret 0] 1] $darray($i)

	# items after curs should be adjusted
	set ret [$dbc get -next]
	error_check_bad dbc:get:next [llength $ret] 0
	error_check_good dbc:get:next:compare \
	    $ret [list [list $keys([expr $i + 1]) $darray([expr $i + 2])]]

	puts "\tTest052.i: Insert (overwrite) current item (DB_CURRENT)."
	set i 1
	set ret [$dbc get -first]
	error_check_bad dbc_get:first [llength $ret] 0
	# choose a datum that is not currently in db
	set ret [$dbc put -current $darray([expr $i + 2])]
	error_check_good dbc_put:curr $ret 0
	# curs should be on new item
	set ret [$dbc get -current]
	error_check_bad dbc_get:current [llength $ret] 0
	error_check_good dbc_get:curr:keys \
	    [lindex [lindex $ret 0] 0] $keys($i)
	error_check_good dbc_get:curr:data \
	    [lindex [lindex $ret 0] 1] $darray([expr $i + 2])

	set ret [$dbc get -next]
	error_check_bad dbc_get:next [llength $ret] 0
	set i [incr i]
	error_check_good dbc_get:next \
	    $ret [list [list $keys($i) $darray($i)]]

	error_check_good dbc_close [$dbc close] 0
	if { $txnenv == 1 } {
		error_check_good txn [$t commit] 0
	}
	error_check_good db_close [$db close] 0

	puts "\tTest052 complete."
}

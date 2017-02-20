# See the file LICENSE for redistribution information.
#
# Copyright (c) 1999-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	test072
# TEST	Test of cursor stability when duplicates are moved off-page.
proc test072 { method {pagesize 512} {ndups 20} {tnum "072"} args } {
	source ./include.tcl
	global alphabet
	global is_je_test

	set omethod [convert_method $method]
	set args [convert_args $method $args]

	set txnenv 0
	set eindex [lsearch -exact $args "-env"]
	#
	# If we are using an env, then testfile name should just be
	# the db name.  Otherwise it is the test directory and the name.
	if { $eindex == -1 } {
		set basename $testdir/test$tnum
		set env NULL
	} else {
		set basename test$tnum
		incr eindex
		set env [lindex $args $eindex]
		set txnenv [is_txnenv $env]
		if { $txnenv == 1 } {
			append args " -auto_commit "
		}
		set testdir [get_home $env]
	}
	cleanup $testdir $env

	# Keys must sort $prekey < $key < $postkey.
	set prekey "a key"
	set key "the key"
	set postkey "z key"

	# Make these distinguishable from each other and from the
	# alphabets used for the $key's data.
	set predatum "1234567890"
	set postdatum "0987654321"

	puts -nonewline "Test$tnum $omethod ($args): "
	if { [is_record_based $method] || [is_rbtree $method] } {
		puts "Skipping for method $method."
		return
	} else {
		puts "\nTest$tnum: Test of cursor stability when\
		    duplicates are moved off-page."
	}
	set pgindex [lsearch -exact $args "-pagesize"]
	if { $pgindex != -1 } {
		puts "Test$tnum: skipping for specific pagesizes"
		return
	}

	append args " -pagesize $pagesize "
	set txn ""

	set dlist [list "-dup" "-dup -dupsort"]
	set testid 0
	foreach dupopt $dlist {
		if { $is_je_test || [is_compressed $args] } {
			if { $dupopt == "-dup" } {
				continue
			}
		}

		incr testid
		set duptestfile $basename$testid.db
		set db [eval {berkdb_open -create -mode 0644} \
		    $omethod $args $dupopt {$duptestfile}]
		error_check_good "db open" [is_valid_db $db] TRUE

		puts \
"\tTest$tnum.a: ($dupopt) Set up surrounding keys and cursors."
		if { $txnenv == 1 } {
			set t [$env txn]
			error_check_good txn [is_valid_txn $t $env] TRUE
			set txn "-txn $t"
		}
		set ret [eval {$db put} $txn {$prekey $predatum}]
		error_check_good pre_put $ret 0
		set ret [eval {$db put} $txn {$postkey $postdatum}]
		error_check_good post_put $ret 0

		set precursor [eval {$db cursor} $txn]
		error_check_good precursor [is_valid_cursor $precursor \
		    $db] TRUE
		set postcursor [eval {$db cursor} $txn]
		error_check_good postcursor [is_valid_cursor $postcursor \
		    $db] TRUE
		error_check_good preset [$precursor get -set $prekey] \
			[list [list $prekey $predatum]]
		error_check_good postset [$postcursor get -set $postkey] \
			[list [list $postkey $postdatum]]

		puts "\tTest$tnum.b: Put/create cursor/verify all cursor loop."

		for { set i 0 } { $i < $ndups } { incr i } {
			set datum [format "%4d$alphabet" [expr $i + 1000]]
			set data($i) $datum

			# Uncomment these lines to see intermediate steps.
			# error_check_good db_sync($i) [$db sync] 0
			# error_check_good db_dump($i) \
			#     [catch {exec $util_path/db_dump \
			#	-da $duptestfile > $testdir/out.$i}] 0

			set ret [eval {$db put} $txn {$key $datum}]
			error_check_good "db put ($i)" $ret 0

			set dbc($i) [eval {$db cursor} $txn]
			error_check_good "db cursor ($i)"\
			    [is_valid_cursor $dbc($i) $db] TRUE

			error_check_good "dbc get -get_both ($i)"\
			    [$dbc($i) get -get_both $key $datum]\
			    [list [list $key $datum]]

			for { set j 0 } { $j < $i } { incr j } {
				set dbt [$dbc($j) get -current]
				set k [lindex [lindex $dbt 0] 0]
				set d [lindex [lindex $dbt 0] 1]

				#puts "cursor $j after $i: $d"

				eval {$db sync}

				error_check_good\
				    "cursor $j key correctness after $i puts" \
				    $k $key
				error_check_good\
				    "cursor $j data correctness after $i puts" \
				    $d $data($j)
			}

			# Check correctness of pre- and post- cursors.  Do an
			# error_check_good on the lengths first so that we don't
			# spew garbage as the "got" field and screw up our
			# terminal.  (It's happened here.)
			set pre_dbt [$precursor get -current]
			set post_dbt [$postcursor get -current]
			error_check_good \
			    "key earlier cursor correctness after $i puts" \
			    [string length [lindex [lindex $pre_dbt 0] 0]] \
			    [string length $prekey]
			error_check_good \
			    "data earlier cursor correctness after $i puts" \
			    [string length [lindex [lindex $pre_dbt 0] 1]] \
			    [string length $predatum]
			error_check_good \
			    "key later cursor correctness after $i puts" \
			    [string length [lindex [lindex $post_dbt 0] 0]] \
			    [string length $postkey]
			error_check_good \
			    "data later cursor correctness after $i puts" \
			    [string length [lindex [lindex $post_dbt 0] 1]]\
			    [string length $postdatum]

			error_check_good \
			    "earlier cursor correctness after $i puts" \
			    $pre_dbt [list [list $prekey $predatum]]
			error_check_good \
			    "later cursor correctness after $i puts" \
			    $post_dbt [list [list $postkey $postdatum]]
		}

		puts "\tTest$tnum.c: Reverse Put/create cursor/verify all cursor loop."
		set end [expr $ndups * 2 - 1]
		for { set i $end } { $i >= $ndups } { set i [expr $i - 1] } {
			set datum [format "%4d$alphabet" [expr $i + 1000]]
			set data($i) $datum

			# Uncomment these lines to see intermediate steps.
			# error_check_good db_sync($i) [$db sync] 0
			# error_check_good db_dump($i) \
			#     [catch {exec $util_path/db_dump \
			# 	-da $duptestfile > $testdir/out.$i}] 0

			set ret [eval {$db put} $txn {$key $datum}]
			error_check_good "db put ($i)" $ret 0

			error_check_bad dbc($i)_stomped [info exists dbc($i)] 1
			set dbc($i) [eval {$db cursor} $txn]
			error_check_good "db cursor ($i)"\
			    [is_valid_cursor $dbc($i) $db] TRUE

			error_check_good "dbc get -get_both ($i)"\
			    [$dbc($i) get -get_both $key $datum]\
			    [list [list $key $datum]]

			for { set j $i } { $j < $end } { incr j } {
				set dbt [$dbc($j) get -current]
				set k [lindex [lindex $dbt 0] 0]
				set d [lindex [lindex $dbt 0] 1]

				#puts "cursor $j after $i: $d"

				eval {$db sync}

				error_check_good\
				    "cursor $j key correctness after $i puts" \
				    $k $key
				error_check_good\
				    "cursor $j data correctness after $i puts" \
				    $d $data($j)
			}

			# Check correctness of pre- and post- cursors.  Do an
			# error_check_good on the lengths first so that we don't
			# spew garbage as the "got" field and screw up our
			# terminal.  (It's happened here.)
			set pre_dbt [$precursor get -current]
			set post_dbt [$postcursor get -current]
			error_check_good \
			    "key earlier cursor correctness after $i puts" \
			    [string length [lindex [lindex $pre_dbt 0] 0]] \
			    [string length $prekey]
			error_check_good \
			    "data earlier cursor correctness after $i puts" \
			    [string length [lindex [lindex $pre_dbt 0] 1]] \
			    [string length $predatum]
			error_check_good \
			    "key later cursor correctness after $i puts" \
			    [string length [lindex [lindex $post_dbt 0] 0]] \
			    [string length $postkey]
			error_check_good \
			    "data later cursor correctness after $i puts" \
			    [string length [lindex [lindex $post_dbt 0] 1]]\
			    [string length $postdatum]

			error_check_good \
			    "earlier cursor correctness after $i puts" \
			    $pre_dbt [list [list $prekey $predatum]]
			error_check_good \
			    "later cursor correctness after $i puts" \
			    $post_dbt [list [list $postkey $postdatum]]
		}

		# Close cursors.
		puts "\tTest$tnum.d: Closing cursors."
		for { set i 0 } { $i <= $end } { incr i } {
			error_check_good "dbc close ($i)" [$dbc($i) close] 0
		}
		unset dbc
		error_check_good precursor_close [$precursor close] 0
		error_check_good postcursor_close [$postcursor close] 0
		if { $txnenv == 1 } {
			error_check_good txn [$t commit] 0
		}
		error_check_good "db close" [$db close] 0
	}
}

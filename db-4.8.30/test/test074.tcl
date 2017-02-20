# See the file LICENSE for redistribution information.
#
# Copyright (c) 1999-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	test074
# TEST	Test of DB_NEXT_NODUP.
proc test074 { method {dir -nextnodup} {nitems 100} {tnum "074"} args } {
	source ./include.tcl
	global alphabet
	global is_je_test
	global rand_init

	set omethod [convert_method $method]
	set args [convert_args $method $args]

	berkdb srand $rand_init

	# Data prefix--big enough that we get a mix of on-page, off-page,
	# and multi-off-page dups with the default nitems
	if { [is_fixed_length $method] == 1 } {
		set globaldata "somedata"
	} else {
		set globaldata [repeat $alphabet 4]
	}

	puts "Test$tnum $omethod ($args): Test of $dir"

	# First, test non-dup (and not-very-interesting) case with
	# all db types.

	puts "\tTest$tnum.a: No duplicates."

	set txnenv 0
	set eindex [lsearch -exact $args "-env"]
	#
	# If we are using an env, then testfile should just be the db name.
	# Otherwise it is the test directory and the name.
	if { $eindex == -1 } {
		set testfile $testdir/test$tnum-nodup.db
		set env NULL
	} else {
		set testfile test$tnum-nodup.db
		incr eindex
		set env [lindex $args $eindex]
		set txnenv [is_txnenv $env]
		if { $txnenv == 1 } {
			append args " -auto_commit "
		}
		set testdir [get_home $env]
	}
	cleanup $testdir $env
	set db [eval {berkdb_open -create -mode 0644} $omethod\
	    $args {$testfile}]
	error_check_good db_open [is_valid_db $db] TRUE
	set txn ""

	# Insert nitems items.
	puts "\t\tTest$tnum.a.1: Put loop."
	for {set i 1} {$i <= $nitems} {incr i} {
		#
		# If record based, set key to $i * 2 to leave
		# holes/unused entries for further testing.
		#
		if {[is_record_based $method] == 1} {
			set key [expr $i * 2]
		} else {
			set key "key$i"
		}
		set data "$globaldata$i"
		if { $txnenv == 1 } {
			set t [$env txn]
			error_check_good txn [is_valid_txn $t $env] TRUE
			set txn "-txn $t"
		}
		set ret [eval {$db put} $txn {$key \
		    [chop_data $method $data]}]
		error_check_good put($i) $ret 0
		if { $txnenv == 1 } {
			error_check_good txn [$t commit] 0
		}
	}

	puts "\t\tTest$tnum.a.2: Get($dir)"

	# foundarray($i) is set when key number i is found in the database
	if { $txnenv == 1 } {
		set t [$env txn]
		error_check_good txn [is_valid_txn $t $env] TRUE
		set txn "-txn $t"
	}
	set dbc [eval {$db cursor} $txn]
	error_check_good db_cursor [is_valid_cursor $dbc $db] TRUE

	# Initialize foundarray($i) to zero for all $i
	for {set i 1} {$i < $nitems} {incr i} {
		set foundarray($i) 0
	}

	# Walk database using $dir and record each key gotten.
	for {set i 1} {$i <= $nitems} {incr i} {
		set dbt [$dbc get $dir]
		set key [lindex [lindex $dbt 0] 0]
		if {[is_record_based $method] == 1} {
			set num [expr $key / 2]
			set desired_key $key
			error_check_good $method:num $key [expr $num * 2]
		} else {
			set num [string range $key 3 end]
			set desired_key key$num
		}

		error_check_good dbt_correct($i) $dbt\
		    [list [list $desired_key\
		    [pad_data $method $globaldata$num]]]

		set foundarray($num) 1
	}

	puts "\t\tTest$tnum.a.3: Final key."
	error_check_good last_db_get [$dbc get $dir] [list]

	puts "\t\tTest$tnum.a.4: Verify loop."
	for { set i 1 } { $i <= $nitems } { incr i } {
		error_check_good found_key($i) $foundarray($i) 1
	}

	error_check_good dbc_close(nodup) [$dbc close] 0
	if { $txnenv == 1 } {
		error_check_good txn [$t commit] 0
	}

	# If we are a method that doesn't allow dups, verify that
	# we get an empty list if we try to use DB_NEXT_DUP
	if { [is_record_based $method] == 1 || [is_rbtree $method] == 1 } {
		if { $txnenv == 1 } {
			set t [$env txn]
			error_check_good txn [is_valid_txn $t $env] TRUE
			set txn "-txn $t"
		}
		puts "\t\tTest$tnum.a.5: Check DB_NEXT_DUP for $method."
		set dbc [eval {$db cursor} $txn]
		error_check_good db_cursor [is_valid_cursor $dbc $db] TRUE

		set dbt [$dbc get $dir]
		error_check_good $method:nextdup [$dbc get -nextdup] [list]
		error_check_good dbc_close(nextdup) [$dbc close] 0
		if { $txnenv == 1 } {
			error_check_good txn [$t commit] 0
		}
	}
	error_check_good db_close(nodup) [$db close] 0

	# Quit here if we're a method that won't allow dups.
	if { [is_record_based $method] == 1 || [is_rbtree $method] == 1 } {
		puts "\tTest$tnum: Skipping remainder for method $method."
		return
	}

	foreach opt { "-dup" "-dupsort" } {
		if { $is_je_test || [is_compressed $args] } {
			if { $opt == "-dup" } {
				continue
			}
		}

		#
		# If we are using an env, then testfile should just be the
		# db name.  Otherwise it is the test directory and the name.
		if { $eindex == -1 } {
			set testfile $testdir/test$tnum$opt.db
		} else {
			set testfile test$tnum$opt.db
		}

		if { [string compare $opt "-dupsort"] == 0 } {
			set opt "-dup -dupsort"
		}

		puts "\tTest$tnum.b: Duplicates ($opt)."

		puts "\t\tTest$tnum.b.1 ($opt): Put loop."
		set db [eval {berkdb_open -create -mode 0644}\
		    $opt $omethod $args {$testfile}]
		error_check_good db_open [is_valid_db $db] TRUE

		# Insert nitems different keys such that key i has i dups.
		for {set i 1} {$i <= $nitems} {incr i} {
			set key key$i

			for {set j 1} {$j <= $i} {incr j} {
				if { $j < 10 } {
					set data "${globaldata}00$j"
				} elseif { $j < 100 } {
					set data "${globaldata}0$j"
				} else {
					set data "$globaldata$j"
				}

				if { $txnenv == 1 } {
					set t [$env txn]
					error_check_good txn \
					    [is_valid_txn $t $env] TRUE
					set txn "-txn $t"
				}
				set ret [eval {$db put} $txn {$key $data}]
				error_check_good put($i,$j) $ret 0
				if { $txnenv == 1 } {
					error_check_good txn [$t commit] 0
				}
			}
		}

		# Initialize foundarray($i) to 0 for all i.
		unset foundarray
		for { set i 1 } { $i <= $nitems } { incr i } {
			set foundarray($i) 0
		}

		# Get loop--after each get, move forward a random increment
		# within the duplicate set.
		puts "\t\tTest$tnum.b.2 ($opt): Get loop."
		set one "001"
		if { $txnenv == 1 } {
			set t [$env txn]
			error_check_good txn [is_valid_txn $t $env] TRUE
			set txn "-txn $t"
		}
		set dbc [eval {$db cursor} $txn]
		error_check_good dbc($opt) [is_valid_cursor $dbc $db] TRUE
		for { set i 1 } { $i <= $nitems } { incr i } {
			set dbt [$dbc get $dir]
			set key [lindex [lindex $dbt 0] 0]
			set num [string range $key 3 end]

			set desired_key key$num
			if { [string compare $dir "-prevnodup"] == 0 } {
				if { $num < 10 } {
					set one "00$num"
				} elseif { $num < 100 } {
					set one "0$num"
				} else {
					set one $num
				}
			}

			error_check_good dbt_correct($i) $dbt\
				[list [list $desired_key\
				    "$globaldata$one"]]

			set foundarray($num) 1

			# Go forward by some number w/i dup set.
			set inc [berkdb random_int 0 [expr $num - 1]]
			for { set j 0 } { $j < $inc } { incr j } {
				eval {$dbc get -nextdup}
			}
		}

		puts "\t\tTest$tnum.b.3 ($opt): Final key."
		error_check_good last_db_get($opt) [$dbc get $dir] [list]

		# Verify
		puts "\t\tTest$tnum.b.4 ($opt): Verify loop."
		for { set i 1 } { $i <= $nitems } { incr i } {
			error_check_good found_key($i) $foundarray($i) 1
		}

		error_check_good dbc_close [$dbc close] 0
		if { $txnenv == 1 } {
			error_check_good txn [$t commit] 0
		}
		error_check_good db_close [$db close] 0
	}
}

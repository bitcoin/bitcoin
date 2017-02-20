# See the file LICENSE for redistribution information.
#
# Copyright (c) 1996-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	test011
# TEST	Duplicate test
# TEST		Small key/data pairs.
# TEST		Test DB_KEYFIRST, DB_KEYLAST, DB_BEFORE and DB_AFTER.
# TEST		To test off-page duplicates, run with small pagesize.
# TEST
# TEST	Use the first 10,000 entries from the dictionary.
# TEST	Insert each with self as key and data; add duplicate records for each.
# TEST	Then do some key_first/key_last add_before, add_after operations.
# TEST	This does not work for recno
# TEST
# TEST	To test if dups work when they fall off the main page, run this with
# TEST	a very tiny page size.
proc test011 { method {nentries 10000} {ndups 5} {tnum "011"} args } {
	global dlist
	global rand_init
	source ./include.tcl

	set dlist ""

	# Btree with compression does not support unsorted duplicates.
	if { [is_compressed $args] == 1 } {
		puts "Test$tnum skipping for btree with compression."
		return
	}

	if { [is_rbtree $method] == 1 } {
		puts "Test$tnum skipping for method $method"
		return
	}
	if { [is_record_based $method] == 1 } {
		test011_recno $method $nentries $tnum $args
		return
	}
	if {$ndups < 5} {
		set ndups 5
	}

	set args [convert_args $method $args]
	set omethod [convert_method $method]

	berkdb srand $rand_init

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
			reduce_dups nentries ndups
		}
		set testdir [get_home $env]
	}

	puts -nonewline "Test$tnum: $method $nentries small $ndups dup "
	puts "key/data pairs, cursor ops"

	set t1 $testdir/t1
	set t2 $testdir/t2
	set t3 $testdir/t3
	cleanup $testdir $env

	set db [eval {berkdb_open -create \
	    -mode 0644} [concat $args "-dup"] {$omethod $testfile}]
	error_check_good dbopen [is_valid_db $db] TRUE

	set did [open $dict]

	set pflags ""
	set gflags ""
	set txn ""
	set count 0

	# Here is the loop where we put and get each key/data pair
	# We will add dups with values 1, 3, ... $ndups.  Then we'll add
	# 0 and $ndups+1 using keyfirst/keylast.  We'll add 2 and 4 using
	# add before and add after.
	puts "\tTest$tnum.a: put and get duplicate keys."
	set i ""
	for { set i 1 } { $i <= $ndups } { incr i 2 } {
		lappend dlist $i
	}
	set maxodd $i
	while { [gets $did str] != -1 && $count < $nentries } {
		for { set i 1 } { $i <= $ndups } { incr i 2 } {
			set datastr $i:$str
			if { $txnenv == 1 } {
				set t [$env txn]
				error_check_good txn [is_valid_txn $t $env] TRUE
				set txn "-txn $t"
			}
			set ret [eval {$db put} $txn $pflags {$str $datastr}]
			error_check_good put $ret 0
			if { $txnenv == 1 } {
				error_check_good txn [$t commit] 0
			}
		}

		# Now retrieve all the keys matching this key
		set x 1
		if { $txnenv == 1 } {
			set t [$env txn]
			error_check_good txn [is_valid_txn $t $env] TRUE
			set txn "-txn $t"
		}
		set dbc [eval {$db cursor} $txn]
		for {set ret [$dbc get "-set" $str ]} \
		    {[llength $ret] != 0} \
		    {set ret [$dbc get "-next"] } {
			if {[llength $ret] == 0} {
				break
			}
			set k [lindex [lindex $ret 0] 0]
			if { [string compare $k $str] != 0 } {
				break
			}
			set datastr [lindex [lindex $ret 0] 1]
			set d [data_of $datastr]

			error_check_good Test$tnum:put $d $str
			set id [ id_of $datastr ]
			error_check_good Test$tnum:dup# $id $x
			incr x 2
		}
		error_check_good Test$tnum:numdups $x $maxodd
		error_check_good curs_close [$dbc close] 0
		if { $txnenv == 1 } {
			error_check_good txn [$t commit] 0
		}
		incr count
	}
	close $did

	# Now we will get each key from the DB and compare the results
	# to the original.
	puts "\tTest$tnum.b: \
	    traverse entire file checking duplicates before close."
	if { $txnenv == 1 } {
		set t [$env txn]
		error_check_good txn [is_valid_txn $t $env] TRUE
		set txn "-txn $t"
	}
	dup_check $db $txn $t1 $dlist
	if { $txnenv == 1 } {
		error_check_good txn [$t commit] 0
	}

	# Now compare the keys to see if they match the dictionary entries
	set q q
	filehead $nentries $dict $t3
	filesort $t3 $t2
	filesort $t1 $t3

	error_check_good Test$tnum:diff($t3,$t2) \
	    [filecmp $t3 $t2] 0

	error_check_good db_close [$db close] 0

	set db [eval {berkdb_open} $args $testfile]
	error_check_good dbopen [is_valid_db $db] TRUE

	puts "\tTest$tnum.c: \
	    traverse entire file checking duplicates after close."
	if { $txnenv == 1 } {
		set t [$env txn]
		error_check_good txn [is_valid_txn $t $env] TRUE
		set txn "-txn $t"
	}
	dup_check $db $txn $t1 $dlist
	if { $txnenv == 1 } {
		error_check_good txn [$t commit] 0
	}

	# Now compare the keys to see if they match the dictionary entries
	filesort $t1 $t3
	error_check_good Test$tnum:diff($t3,$t2) \
	    [filecmp $t3 $t2] 0

	puts "\tTest$tnum.d: Testing key_first functionality"
	if { $txnenv == 1 } {
		set t [$env txn]
		error_check_good txn [is_valid_txn $t $env] TRUE
		set txn "-txn $t"
	}
	add_dup $db $txn $nentries "-keyfirst" 0 0
	set dlist [linsert $dlist 0 0]
	dup_check $db $txn $t1 $dlist
	if { $txnenv == 1 } {
		error_check_good txn [$t commit] 0
	}

	puts "\tTest$tnum.e: Testing key_last functionality"
	if { $txnenv == 1 } {
		set t [$env txn]
		error_check_good txn [is_valid_txn $t $env] TRUE
		set txn "-txn $t"
	}
	add_dup $db $txn $nentries "-keylast" [expr $maxodd - 1] 0
	lappend dlist [expr $maxodd - 1]
	dup_check $db $txn $t1 $dlist
	if { $txnenv == 1 } {
		error_check_good txn [$t commit] 0
	}

	puts "\tTest$tnum.f: Testing add_before functionality"
	if { $txnenv == 1 } {
		set t [$env txn]
		error_check_good txn [is_valid_txn $t $env] TRUE
		set txn "-txn $t"
	}
	add_dup $db $txn $nentries "-before" 2 3
	set dlist [linsert $dlist 2 2]
	dup_check $db $txn $t1 $dlist
	if { $txnenv == 1 } {
		error_check_good txn [$t commit] 0
	}

	puts "\tTest$tnum.g: Testing add_after functionality"
	if { $txnenv == 1 } {
		set t [$env txn]
		error_check_good txn [is_valid_txn $t $env] TRUE
		set txn "-txn $t"
	}
	add_dup $db $txn $nentries "-after" 4 4
	set dlist [linsert $dlist 4 4]
	dup_check $db $txn $t1 $dlist
	if { $txnenv == 1 } {
		error_check_good txn [$t commit] 0
	}

	error_check_good db_close [$db close] 0
}

proc add_dup {db txn nentries flag dataval iter} {
	source ./include.tcl

	set dbc [eval {$db cursor} $txn]
	set did [open $dict]
	set count 0
	while { [gets $did str] != -1 && $count < $nentries } {
		set datastr $dataval:$str
		set ret [$dbc get "-set" $str]
		error_check_bad "cget(SET)" [is_substr $ret Error] 1
		for { set i 1 } { $i < $iter } { incr i } {
			set ret [$dbc get "-next"]
			error_check_bad "cget(NEXT)" [is_substr $ret Error] 1
		}

		if { [string compare $flag "-before"] == 0 ||
		    [string compare $flag "-after"] == 0 } {
			set ret [$dbc put $flag $datastr]
		} else {
			set ret [$dbc put $flag $str $datastr]
		}
		error_check_good "$dbc put $flag" $ret 0
		incr count
	}
	close $did
	$dbc close
}

proc test011_recno { method {nentries 10000} {tnum "011"} largs } {
	global dlist
	source ./include.tcl

	set largs [convert_args $method $largs]
	set omethod [convert_method $method]
	set renum [is_rrecno $method]

	puts "Test$tnum: \
	    $method ($largs) $nentries test cursor insert functionality"

	# Create the database and open the dictionary
	set eindex [lsearch -exact $largs "-env"]
	#
	# If we are using an env, then testfile should just be the db name.
	# Otherwise it is the test directory and the name.
	set txnenv 0
	if { $eindex == -1 } {
		set testfile $testdir/test$tnum.db
		set env NULL
	} else {
		set testfile test$tnum.db
		incr eindex
		set env [lindex $largs $eindex]
		set txnenv [is_txnenv $env]
		if { $txnenv == 1 } {
			append largs " -auto_commit "
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
	set t1 $testdir/t1
	set t2 $testdir/t2
	set t3 $testdir/t3
	cleanup $testdir $env

	if {$renum == 1} {
		append largs " -renumber"
	}
	set db [eval {berkdb_open \
	     -create -mode 0644} $largs {$omethod $testfile}]
	error_check_good dbopen [is_valid_db $db] TRUE

	set did [open $dict]

	set pflags ""
	set gflags ""
	set txn ""
	set count 0

	# The basic structure of the test is that we pick a random key
	# in the database and then add items before, after, ?? it.  The
	# trickiness is that with RECNO, these are not duplicates, they
	# are creating new keys.  Therefore, every time we do this, the
	# keys assigned to other values change.  For this reason, we'll
	# keep the database in tcl as a list and insert properly into
	# it to verify that the right thing is happening.  If we do not
	# have renumber set, then the BEFORE and AFTER calls should fail.

	# Seed the database with an initial record
	gets $did str
	if { $txnenv == 1 } {
		set t [$env txn]
		error_check_good txn [is_valid_txn $t $env] TRUE
		set txn "-txn $t"
	}
	set ret [eval {$db put} $txn {1 [chop_data $method $str]}]
	if { $txnenv == 1 } {
		error_check_good txn [$t commit] 0
	}
	error_check_good put $ret 0
	set count 1

	set dlist "NULL $str"

	# Open a cursor
	if { $txnenv == 1 } {
		set t [$env txn]
		error_check_good txn [is_valid_txn $t $env] TRUE
		set txn "-txn $t"
	}
	set dbc [eval {$db cursor} $txn]
	puts "\tTest$tnum.a: put and get entries"
	while { [gets $did str] != -1 && $count < $nentries } {
		# Pick a random key
		set key [berkdb random_int 1 $count]
		set ret [$dbc get -set $key]
		set k [lindex [lindex $ret 0] 0]
		set d [lindex [lindex $ret 0] 1]
		error_check_good cget:SET:key $k $key
		error_check_good \
		    cget:SET $d [pad_data $method [lindex $dlist $key]]

		# Current
		set ret [$dbc put -current [chop_data $method $str]]
		error_check_good cput:$key $ret 0
		set dlist [lreplace $dlist $key $key [pad_data $method $str]]

		# Before
		if { [gets $did str] == -1 } {
			continue;
		}

		if { $renum == 1 } {
			set ret [$dbc put \
			    -before [chop_data $method $str]]
			error_check_good cput:$key:BEFORE $ret $key
			set dlist [linsert $dlist $key $str]
			incr count

			# After
			if { [gets $did str] == -1 } {
				continue;
			}
			set ret [$dbc put \
			    -after [chop_data $method $str]]
			error_check_good cput:$key:AFTER $ret [expr $key + 1]
			set dlist [linsert $dlist [expr $key + 1] $str]
			incr count
		}

		# Now verify that the keys are in the right place
		set i 0
		for {set ret [$dbc get "-set" $key]} \
		    {[string length $ret] != 0 && $i < 3} \
		    {set ret [$dbc get "-next"] } {
			set check_key [expr $key + $i]

			set k [lindex [lindex $ret 0] 0]
			error_check_good cget:$key:loop $k $check_key

			set d [lindex [lindex $ret 0] 1]
			error_check_good cget:data $d \
			    [pad_data $method [lindex $dlist $check_key]]
			incr i
		}
	}
	close $did
	error_check_good cclose [$dbc close] 0
	if { $txnenv == 1 } {
		error_check_good txn [$t commit] 0
	}

	# Create  check key file.
	set oid [open $t2 w]
	for {set i 1} {$i <= $count} {incr i} {
		puts $oid $i
	}
	close $oid

	puts "\tTest$tnum.b: dump file"
	if { $txnenv == 1 } {
		set t [$env txn]
		error_check_good txn [is_valid_txn $t $env] TRUE
		set txn "-txn $t"
	}
	dump_file $db $txn $t1 test011_check
	if { $txnenv == 1 } {
		error_check_good txn [$t commit] 0
	}
	error_check_good Test$tnum:diff($t2,$t1) \
	    [filecmp $t2 $t1] 0

	error_check_good db_close [$db close] 0

	puts "\tTest$tnum.c: close, open, and dump file"
	eval open_and_dump_file $testfile $env $t1 test011_check \
	    dump_file_direction "-first" "-next" $largs
	error_check_good Test$tnum:diff($t2,$t1) \
	    [filecmp $t2 $t1] 0

	puts "\tTest$tnum.d: close, open, and dump file in reverse direction"
	eval open_and_dump_file $testfile $env $t1 test011_check \
	    dump_file_direction "-last" "-prev" $largs

	filesort $t1 $t3 -n
	error_check_good Test$tnum:diff($t2,$t3) \
	    [filecmp $t2 $t3] 0
}

proc test011_check { key data } {
	global dlist

	error_check_good "get key $key" $data [lindex $dlist $key]
}

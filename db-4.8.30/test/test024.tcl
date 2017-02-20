# See the file LICENSE for redistribution information.
#
# Copyright (c) 1996-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	test024
# TEST	Record number retrieval test.
# TEST	Test the Btree and Record number get-by-number functionality.
proc test024 { method {nentries 10000} args} {
	source ./include.tcl
	global rand_init

	set do_renumber [is_rrecno $method]
	set args [convert_args $method $args]
	set omethod [convert_method $method]

	puts "Test024: $method ($args)"

	# Btree with compression does not support -recnum.
	if { [is_compressed $args] == 1 } {
		puts "Test024 skipping for compressed btree with -recnum."
		return
	}
	if { [string compare $omethod "-hash"] == 0 } {
		puts "Test024 skipping for method HASH"
		return
	}
	if { [is_partitioned $args] } {
		puts "Test024 skipping for partitioned $omethod"
		return
	}

	berkdb srand $rand_init

	# Create the database and open the dictionary
	set txnenv 0
	set eindex [lsearch -exact $args "-env"]
	#
	# If we are using an env, then testfile should just be the db name.
	# Otherwise it is the test directory and the name.
	if { $eindex == -1 } {
		set testfile $testdir/test024.db
		set env NULL
	} else {
		set testfile test024.db
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
	set t1 $testdir/t1
	set t2 $testdir/t2
	set t3 $testdir/t3

	cleanup $testdir $env

	# Read the first nentries dictionary elements and reverse them.
	# Keep a list of these (these will be the keys).
	puts "\tTest024.a: initialization"
	set keys ""
	set did [open $dict]
	set count 0
	while { [gets $did str] != -1 && $count < $nentries } {
		lappend keys [reverse $str]
		incr count
	}
	close $did

	# Generate sorted order for the keys
	set sorted_keys [lsort $keys]
	# Create the database
	if { [string compare $omethod "-btree"] == 0 } {
		set db [eval {berkdb_open -create \
			-mode 0644 -recnum} $args {$omethod $testfile}]
		error_check_good dbopen [is_valid_db $db] TRUE
	} else  {
		set db [eval {berkdb_open -create \
			-mode 0644} $args {$omethod $testfile}]
		error_check_good dbopen [is_valid_db $db] TRUE
	}

	set pflags ""
	set gflags ""
	set txn ""

	if { [is_record_based $method] == 1 } {
		set gflags " -recno"
	}

	puts "\tTest024.b: put/get loop"
	foreach k $keys {
		if { [is_record_based $method] == 1 } {
			set key [lsearch $sorted_keys $k]
			incr key
		} else {
			set key $k
		}
		if { $txnenv == 1 } {
			set t [$env txn]
			error_check_good txn [is_valid_txn $t $env] TRUE
			set txn "-txn $t"
		}
		set ret [eval {$db put} \
		    $txn $pflags {$key [chop_data $method $k]}]
		error_check_good put $ret 0
		set ret [eval {$db get} $txn $gflags {$key}]
		error_check_good \
		    get $ret [list [list $key [pad_data $method $k]]]
		if { $txnenv == 1 } {
			error_check_good txn [$t commit] 0
		}
	}

	# Now we will get each key from the DB and compare the results
	# to the original.
	puts "\tTest024.c: dump file"

	# Put sorted keys in file
	set oid [open $t1 w]
	foreach k $sorted_keys {
		puts $oid [pad_data $method $k]
	}
	close $oid

	# Instead of using dump_file; get all the keys by keynum
	set oid [open $t2 w]
	if { [string compare $omethod "-btree"] == 0 } {
		set do_renumber 1
	}

	set gflags " -recno"

	if { $txnenv == 1 } {
		set t [$env txn]
		error_check_good txn [is_valid_txn $t $env] TRUE
		set txn "-txn $t"
	}
	for { set k 1 } { $k <= $count } { incr k } {
		set ret [eval {$db get} $txn $gflags {$k}]
		puts $oid [lindex [lindex $ret 0] 1]
		error_check_good recnum_get [lindex [lindex $ret 0] 1] \
		    [pad_data $method [lindex $sorted_keys [expr $k - 1]]]
	}
	close $oid
	if { $txnenv == 1 } {
		error_check_good txn [$t commit] 0
	}
	error_check_good db_close [$db close] 0

	error_check_good Test024.c:diff($t1,$t2) \
	    [filecmp $t1 $t2] 0

	# Now, reopen the file and run the last test again.
	puts "\tTest024.d: close, open, and dump file"
	set db [eval {berkdb_open -rdonly} $args $testfile]
	error_check_good dbopen [is_valid_db $db] TRUE
	set oid [open $t2 w]
	if { $txnenv == 1 } {
		set t [$env txn]
		error_check_good txn [is_valid_txn $t $env] TRUE
		set txn "-txn $t"
	}
	for { set k 1 } { $k <= $count } { incr k } {
		set ret [eval {$db get} $txn $gflags {$k}]
		puts $oid [lindex [lindex $ret 0] 1]
		error_check_good recnum_get [lindex [lindex $ret 0] 1] \
		    [pad_data $method [lindex $sorted_keys [expr $k - 1]]]
	}
	if { $txnenv == 1 } {
		error_check_good txn [$t commit] 0
	}
	close $oid
	error_check_good db_close [$db close] 0
	error_check_good Test024.d:diff($t1,$t2) \
	    [filecmp $t1 $t2] 0

	# Now, reopen the file and run the last test again in reverse direction.
	puts "\tTest024.e: close, open, and dump file in reverse direction"
	set db [eval {berkdb_open -rdonly} $args $testfile]
	error_check_good dbopen [is_valid_db $db] TRUE
	# Put sorted keys in file
	set rsorted ""
	foreach k $sorted_keys {
		set rsorted [linsert $rsorted 0 $k]
	}
	set oid [open $t1 w]
	foreach k $rsorted {
		puts $oid [pad_data $method $k]
	}
	close $oid

	set oid [open $t2 w]
	if { $txnenv == 1 } {
		set t [$env txn]
		error_check_good txn [is_valid_txn $t $env] TRUE
		set txn "-txn $t"
	}
	for { set k $count } { $k > 0 } { incr k -1 } {
		set ret [eval {$db get} $txn $gflags {$k}]
		puts $oid [lindex [lindex $ret 0] 1]
		error_check_good recnum_get [lindex [lindex $ret 0] 1] \
		    [pad_data $method [lindex $sorted_keys [expr $k - 1]]]
	}
	if { $txnenv == 1 } {
		error_check_good txn [$t commit] 0
	}
	close $oid
	error_check_good db_close [$db close] 0
	error_check_good Test024.e:diff($t1,$t2) \
	    [filecmp $t1 $t2] 0

	# Now try deleting elements and making sure they work
	puts "\tTest024.f: delete test"
	set db [eval {berkdb_open} $args $testfile]
	error_check_good dbopen [is_valid_db $db] TRUE
	while { $count > 0 } {
		set kndx [berkdb random_int 1 $count]
		set kval [lindex $keys [expr $kndx - 1]]
		set recno [expr [lsearch $sorted_keys $kval] + 1]

		if { $txnenv == 1 } {
			set t [$env txn]
			error_check_good txn [is_valid_txn $t $env] TRUE
			set txn "-txn $t"
		}
		if { [is_record_based $method] == 1 } {
			set ret [eval {$db del} $txn {$recno}]
		} else {
			set ret [eval {$db del} $txn {$kval}]
		}
		error_check_good delete $ret 0
		if { $txnenv == 1 } {
			error_check_good txn [$t commit] 0
		}

		# Remove the key from the key list
		set ndx [expr $kndx - 1]
		set keys [lreplace $keys $ndx $ndx]

		if { $do_renumber == 1 } {
			set r [expr $recno - 1]
			set sorted_keys [lreplace $sorted_keys $r $r]
		}

		# Check that the keys after it have been renumbered
		if { $txnenv == 1 } {
			set t [$env txn]
			error_check_good txn [is_valid_txn $t $env] TRUE
			set txn "-txn $t"
		}
		if { $do_renumber == 1 && $recno != $count } {
			set r [expr $recno - 1]
			set ret [eval {$db get} $txn $gflags {$recno}]
			error_check_good get_after_del \
			    [lindex [lindex $ret 0] 1] [lindex $sorted_keys $r]
		}
		if { $txnenv == 1 } {
			error_check_good txn [$t commit] 0
		}

		# Decrement count
		incr count -1
	}
	error_check_good db_close [$db close] 0
}

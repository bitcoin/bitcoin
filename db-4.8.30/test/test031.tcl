# See the file LICENSE for redistribution information.
#
# Copyright (c) 1996-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	test031
# TEST	Duplicate sorting functionality
# TEST	Make sure DB_NODUPDATA works.
# TEST
# TEST	Use the first 10,000 entries from the dictionary.
# TEST	Insert each with self as key and "ndups" duplicates
# TEST	For the data field, prepend random five-char strings (see test032)
# TEST	that we force the duplicate sorting code to do something.
# TEST	Along the way, test that we cannot insert duplicate duplicates
# TEST	using DB_NODUPDATA.
# TEST
# TEST	By setting ndups large, we can make this an off-page test
# TEST	After all are entered, retrieve all; verify output.
# TEST	Close file, reopen, do retrieve and re-verify.
# TEST	This does not work for recno
proc test031 { method {nentries 10000} {ndups 5} {tnum "031"} args } {
	global alphabet
	global rand_init
	source ./include.tcl

	berkdb srand $rand_init

	set args [convert_args $method $args]
	set checkargs [split_partition_args $args]

	# The checkdb is of type hash so it can't use compression.
	set checkargs [strip_compression_args $checkargs]
	set omethod [convert_method $method]

	# Create the database and open the dictionary
	set txnenv 0
	set eindex [lsearch -exact $args "-env"]
	#
	# If we are using an env, then testfile should just be the db name.
	# Otherwise it is the test directory and the name.
	if { $eindex == -1 } {
		set testfile $testdir/test$tnum.db
		set checkdb $testdir/checkdb.db
		set env NULL
	} else {
		set testfile test$tnum.db
		set checkdb checkdb.db
		incr eindex
		set env [lindex $args $eindex]
		set txnenv [is_txnenv $env]
		if { $txnenv == 1 } {
			append args " -auto_commit "
			append checkargs " -auto_commit "
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
	set t1 $testdir/t1
	set t2 $testdir/t2
	set t3 $testdir/t3
	cleanup $testdir $env

	puts "Test$tnum: \
	    $method ($args) $nentries small $ndups sorted dup key/data pairs"
	if { [is_record_based $method] == 1 || \
	    [is_rbtree $method] == 1 } {
		puts "Test$tnum skipping for method $omethod"
		return
	}
	set db [eval {berkdb_open -create \
		-mode 0644} $args {$omethod -dup -dupsort $testfile}]
	error_check_good dbopen [is_valid_db $db] TRUE
	set did [open $dict]

	set check_db [eval {berkdb_open \
	     -create -mode 0644} $checkargs {-hash $checkdb}]
	error_check_good dbopen:check_db [is_valid_db $check_db] TRUE

	set pflags ""
	set gflags ""
	set txn ""
	set count 0

	# Here is the loop where we put and get each key/data pair
	puts "\tTest$tnum.a: Put/get loop, check nodupdata"
	if { $txnenv == 1 } {
		set t [$env txn]
		error_check_good txn [is_valid_txn $t $env] TRUE
		set txn "-txn $t"
	}
	set dbc [eval {$db cursor} $txn]
	error_check_good cursor_open [is_valid_cursor $dbc $db] TRUE
	while { [gets $did str] != -1 && $count < $nentries } {
		# Re-initialize random string generator
		randstring_init $ndups

		set dups ""
		for { set i 1 } { $i <= $ndups } { incr i } {
			set pref [randstring]
			set dups $dups$pref
			set datastr $pref:$str
			if { $i == 2 } {
				set nodupstr $datastr
			}
			set ret [eval {$db put} \
			    $txn $pflags {$str [chop_data $method $datastr]}]
			error_check_good put $ret 0
		}

		# Test DB_NODUPDATA using the DB handle
		set ret [eval {$db put -nodupdata} \
		    $txn $pflags {$str [chop_data $method $nodupstr]}]
		error_check_good db_nodupdata [is_substr $ret "DB_KEYEXIST"] 1

		set ret [eval {$check_db put} \
		    $txn $pflags {$str [chop_data $method $dups]}]
		error_check_good checkdb_put $ret 0

		# Now retrieve all the keys matching this key
		set x 0
		set lastdup ""
		# Test DB_NODUPDATA using cursor handle
		set ret [$dbc get -set $str]
		error_check_bad dbc_get [llength $ret] 0
		set datastr [lindex [lindex $ret 0] 1]
		error_check_bad dbc_data [string length $datastr] 0
		set ret [eval {$dbc put -nodupdata} \
		    {$str [chop_data $method $datastr]}]
		error_check_good dbc_nodupdata [is_substr $ret "DB_KEYEXIST"] 1

		for {set ret [$dbc get -set $str]} \
		    {[llength $ret] != 0} \
		    {set ret [$dbc get -nextdup] } {
			set k [lindex [lindex $ret 0] 0]
			if { [string compare $k $str] != 0 } {
				break
			}
			set datastr [lindex [lindex $ret 0] 1]
			if {[string length $datastr] == 0} {
				break
			}
			if {[string compare \
			    $lastdup [pad_data $method $datastr]] > 0} {
				error_check_good \
				    sorted_dups($lastdup,$datastr) 0 1
			}
			incr x
			set lastdup $datastr
		}
		error_check_good "Test$tnum:ndups:$str" $x $ndups
		incr count
	}
	error_check_good cursor_close [$dbc close] 0
	if { $txnenv == 1 } {
		error_check_good txn [$t commit] 0
	}
	close $did

	# Now we will get each key from the DB and compare the results
	# to the original.
	puts "\tTest$tnum.b: Checking file for correct duplicates"
	if { $txnenv == 1 } {
		set t [$env txn]
		error_check_good txn [is_valid_txn $t $env] TRUE
		set txn "-txn $t"
	}
	set dbc [eval {$db cursor} $txn]
	error_check_good cursor_open(2) [is_valid_cursor $dbc $db] TRUE

	set lastkey "THIS WILL NEVER BE A KEY VALUE"
	# no need to delete $lastkey
	set firsttimethru 1
	for {set ret [$dbc get -first]} \
	    {[llength $ret] != 0} \
	    {set ret [$dbc get -next] } {
		set k [lindex [lindex $ret 0] 0]
		set d [lindex [lindex $ret 0] 1]
		error_check_bad data_check:$d [string length $d] 0

		if { [string compare $k $lastkey] != 0 } {
			# Remove last key from the checkdb
			if { $firsttimethru != 1 } {
				error_check_good check_db:del:$lastkey \
				    [eval {$check_db del} $txn {$lastkey}] 0
			}
			set firsttimethru 0
			set lastdup ""
			set lastkey $k
			set dups [lindex [lindex [eval {$check_db get} \
				$txn {$k}] 0] 1]
			error_check_good check_db:get:$k \
			    [string length $dups] [expr $ndups * 4]
		}

		if { [string compare $lastdup $d] > 0 } {
			error_check_good dup_check:$k:$d 0 1
		}
		set lastdup $d

		set pref [string range $d 0 3]
		set ndx [string first $pref $dups]
		error_check_good valid_duplicate [expr $ndx >= 0] 1
		set a [string range $dups 0 [expr $ndx - 1]]
		set b [string range $dups [expr $ndx + 4] end]
		set dups $a$b
	}
	# Remove last key from the checkdb
	if { [string length $lastkey] != 0 } {
		error_check_good check_db:del:$lastkey \
		[eval {$check_db del} $txn {$lastkey}] 0
	}

	# Make sure there is nothing left in check_db

	set check_c [eval {$check_db cursor} $txn]
	set ret [$check_c get -first]
	error_check_good check_c:get:$ret [llength $ret] 0
	error_check_good check_c:close [$check_c close] 0

	error_check_good dbc_close [$dbc close] 0
	if { $txnenv == 1 } {
		error_check_good txn [$t commit] 0
	}
	error_check_good check_db:close [$check_db close] 0
	error_check_good db_close [$db close] 0
}

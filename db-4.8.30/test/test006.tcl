# See the file LICENSE for redistribution information.
#
# Copyright (c) 1996-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	test006
# TEST	Small keys/medium data
# TEST		Put/get per key
# TEST		Keyed delete and verify
# TEST
# TEST	Keyed delete test.
# TEST	Create database.
# TEST	Go through database, deleting all entries by key.
# TEST	Then do the same for unsorted and sorted dups.
proc test006 { method {nentries 10000} {reopen 0} {tnum "006"} \
    {ndups 5} args } {

	test006_body $method $nentries $reopen $tnum 1 "" "" $args

	# For methods supporting dups, run the test with sorted and
	# with unsorted dups.
	if { [is_btree $method] == 1 || [is_hash $method] == 1 } {
		foreach {sort flags} {unsorted -dup sorted "-dup -dupsort"} {
			test006_body $method $nentries $reopen \
			    $tnum $ndups $sort $flags $args
		}
	}
}

proc test006_body { method {nentries 10000} {reopen 0} {tnum "006"} \
    {ndups 5} sort flags {largs ""} } {
	global is_je_test
	source ./include.tcl

	if { [is_compressed $largs] && $sort == "unsorted" } {
		puts "Test$tnum skipping $sort duplicates for compression"
		return
	}

	set do_renumber [is_rrecno $method]
        set largs [convert_args $method $largs]
        set omethod [convert_method $method]

	set tname Test$tnum
	set dbname test$tnum

	# Create the database and open the dictionary
	set txnenv 0
	set eindex [lsearch -exact $largs "-env"]
	#
	# If we are using an env, then testfile should just be the db name.
	# Otherwise it is the test directory and the name.
	if { $eindex == -1 } {
		set basename $testdir/$dbname
		set env NULL
	} else {
		set basename $dbname
		incr eindex
		set env [lindex $largs $eindex]
		if { $is_je_test && $sort == "unsorted" } {
			puts "Test$tnum skipping $sort duplicates for JE"
			return
		}
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
	puts -nonewline "$tname: $method ($flags $largs) "
	puts -nonewline "$nentries equal small key; medium data pairs"
	if {$reopen == 1} {
		puts " (with close)"
	} else {
		puts ""
	}

	set pflags ""
	set gflags ""
	set txn ""
	if { [is_record_based $method] == 1 } {
		append gflags " -recno"
	}

	cleanup $testdir $env

	# Here is the loop where we put and get each key/data pair.

	set count 0
	set testfile $basename$sort.db
	set db [eval {berkdb_open -create \
	    -mode 0644} $largs $flags {$omethod $testfile}]
	error_check_good dbopen [is_valid_db $db] TRUE

	puts "\t$tname.a: put/get loop"
	set did [open $dict]
	while { [gets $did str] != -1 && $count < $nentries } {
                if { [is_record_based $method] == 1 } {
                        set key [expr $count + 1 ]
                } else {
                        set key $str
                }

		set str [make_data_str $str]
		for { set j 1 } { $j <= $ndups } {incr j} {
			set datastr $j$str
			if { $txnenv == 1 } {
	 			set t [$env txn]
				error_check_good txn \
				    [is_valid_txn $t $env] TRUE
				set txn "-txn $t"
			}
			set ret [eval {$db put} $txn $pflags \
			    {$key [chop_data $method $datastr]}]
			error_check_good put $ret 0
			if { $txnenv == 1 } {
				error_check_good txn \
				    [$t commit] 0
			}
		}
		incr count
	}
	close $did

	# Close and reopen database, if testing reopen.

	if { $reopen == 1 } {
		error_check_good db_close [$db close] 0

		set db [eval {berkdb_open} $largs $flags {$testfile}]
		error_check_good dbopen [is_valid_db $db] TRUE
	}

	# Now we will get each key from the DB and compare the results
	# to the original, then delete it.

	puts "\t$tname.b: get/delete loop"
	if { $txnenv == 1 } {
		set t [$env txn]
		error_check_good txn [is_valid_txn $t $env] TRUE
		set txn "-txn $t"
	}
	set dbc [eval {$db cursor} $txn]
	error_check_good db_cursor [is_substr $dbc $db] 1

	set i 1
	for { set ret [$dbc get -first] } \
	    { [string length $ret] != 0 } \
	    { set ret [$dbc get -next] } {
		set key [lindex [lindex $ret 0] 0]
		set data [lindex [lindex $ret 0] 1]
		if { $i == 1 } {
			set curkey $key
		}
		error_check_good seq_get:key:$i $key $curkey

		if { $i == $ndups } {
			set i 1
		} else {
			incr i
		}

		# Now delete the key
		set ret [$dbc del]
		error_check_good db_del:$key $ret 0
	}
	error_check_good dbc_close [$dbc close] 0
	if { $txnenv == 1 } {
		error_check_good txn [$t commit] 0
	}
	error_check_good db_close [$db close] 0

	puts "\t$tname.c: verify empty file"
	# Double check that file is now empty
	set db [eval {berkdb_open} $largs $flags $testfile]
	error_check_good dbopen [is_valid_db $db] TRUE
	if { $txnenv == 1 } {
		set t [$env txn]
		error_check_good txn [is_valid_txn $t $env] TRUE
		set txn "-txn $t"
	}
	set dbc [eval {$db cursor} $txn]
	error_check_good db_cursor [is_substr $dbc $db] 1
	set ret [$dbc get -first]
	error_check_good get_on_empty [string length $ret] 0
	error_check_good dbc_close [$dbc close] 0
	if { $txnenv == 1 } {
	error_check_good txn [$t commit] 0
	}
error_check_good db_close [$db close] 0
}

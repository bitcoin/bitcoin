# See the file LICENSE for redistribution information.
#
# Copyright (c) 1996-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	test093
# TEST	Test set_bt_compare (btree key comparison function) and
# TEST	set_h_compare (hash key comparison function).
# TEST
# TEST	Open a database with a comparison function specified,
# TEST	populate, and close, saving a list with that key order as
# TEST	we do so.  Reopen and read in the keys, saving in another
# TEST	list; the keys should be in the order specified by the
# TEST	comparison function.  Sort the original saved list of keys
# TEST	using the comparison function, and verify that it matches
# TEST	the keys as read out of the database.

proc test093 { method {nentries 10000} {tnum "093"} args} {
	source ./include.tcl

	set dbargs [convert_args $method $args]
	set omethod [convert_method $method]

	if { [is_btree $method] == 1 } {
		set compflag -btcompare
	} elseif { [is_hash $method] == 1 } {
		set compflag -hashcompare
	} else {
		puts "Test$tnum: skipping for method $method."
		return
	}

	set txnenv 0
	set eindex [lsearch -exact $dbargs "-env"]
	if { $eindex != -1 } {
		incr eindex
		set env [lindex $dbargs $eindex]
		set envflags [$env get_open_flags]

		# We can't run this test for the -thread option because
		# the comparison function requires the ability to allocate
		# memory at the DBT level and our Tcl interface does not
		# offer that.
		if { [lsearch -exact $envflags "-thread"] != -1 } {
			puts "Skipping Test$tnum for threaded env"
			return
		}
		set rpcenv [is_rpcenv $env]
		if { $rpcenv == 1 } {
			puts "Test$tnum: skipping for RPC"
			return
		}
		set txnenv [is_txnenv $env]
		if { $txnenv == 1 } {
			append dbargs " -auto_commit "
			if { $nentries == 10000 } {
				set nentries 100
			}
		}
		set testdir [get_home $env]
		cleanup $testdir $env
	} else {
		set env NULL
	}

	puts "Test$tnum: $method ($args) $nentries entries using $compflag"

	test093_run $omethod $dbargs $nentries $tnum \
	    $compflag test093_cmp1 test093_sort1
	test093_runbig $omethod $dbargs $nentries $tnum \
	    $compflag test093_cmp1 test093_sort1
	test093_run $omethod $dbargs $nentries $tnum \
	    $compflag test093_cmp2 test093_sort2

	# Don't bother running the second, really slow, comparison
	# function on test093_runbig (file contents).

	# Clean up so general verification (without the custom comparison
	# function) doesn't fail.
	if { $env != "NULL" } {
		set testdir [get_home $env]
	}
	cleanup $testdir $env
}

proc test093_run { method dbargs nentries tnum compflag cmpfunc sortfunc } {
	source ./include.tcl
	global btvals
	global btvalsck

	# We'll need any encryption args separated from the db args
	# so we can pass them to dbverify.
	set encargs ""
	set dbargs [split_encargs $dbargs encargs]

	# If we are using an env, then testfile should just be the db name.
	# Otherwise it is the test directory and the name.
	set eindex [lsearch -exact $dbargs "-env"]
	set txnenv 0
	if { $eindex == -1 } {
		set testfile $testdir/test$tnum.db
		set env NULL
		set envargs ""
	} else {
		set testfile test$tnum.db
		incr eindex
		set env [lindex $dbargs $eindex]
		set envargs " -env $env "
		set txnenv [is_txnenv $env]
		set testdir [get_home $env]
	}
	cleanup $testdir $env

	set db [eval {berkdb_open $compflag $cmpfunc \
	     -create -mode 0644} $method $encargs $dbargs $testfile]
	error_check_good dbopen [is_valid_db $db] TRUE
	set did [open $dict]

	set t1 $testdir/t1
	set t2 $testdir/t2
	set t3 $testdir/t3
	set txn ""

	# Use btvals to save the order of the keys as they are
	# written to the database.  The btvalsck variable will contain
	# the values as sorted by the comparison function.
	set btvals {}
	set btvalsck {}

	puts "\tTest$tnum.a: put/get loop"
	# Here is the loop where we put and get each key/data pair
	set count 0
	while { [gets $did str] != -1 && $count < $nentries } {
		set key $str
		set str [reverse $str]
		if { $txnenv == 1 } {
			set t [$env txn]
			error_check_good txn [is_valid_txn $t $env] TRUE
			set txn "-txn $t"
		}
		set ret [eval \
		    {$db put} $txn {$key [chop_data $method $str]}]
		error_check_good put $ret 0
		if { $txnenv == 1 } {
			error_check_good txn [$t commit] 0
		}

		lappend btvals $key

		set ret [eval {$db get $key}]
		error_check_good \
		    get $ret [list [list $key [pad_data $method $str]]]

		incr count
	}
	close $did

	# Now we will get each key from the DB and compare the results
	# to the original.
	puts "\tTest$tnum.b: dump file"
	if { $txnenv == 1 } {
		set t [$env txn]
		error_check_good txn [is_valid_txn $t $env] TRUE
		set txn "-txn $t"
	}
	dump_file $db $txn $t1 test093_check
	if { $txnenv == 1 } {
		error_check_good txn [$t commit] 0
	}
	error_check_good db_close [$db close] 0

	# Run verify to check the internal structure and order.
	if { [catch {eval {berkdb dbverify} $compflag $cmpfunc\
	    $envargs $encargs {$testfile}} res] } {
		error "FAIL: Verification failed with $res"
	}

	# Now compare the keys to see if they match the dictionary (or ints)
	filehead $nentries $dict $t2
	filesort $t2 $t3
	file rename -force $t3 $t2
	filesort $t1 $t3

	error_check_good Test$tnum:diff($t3,$t2) \
	    [filecmp $t3 $t2] 0

	puts "\tTest$tnum.c: dump file in order"
	# Now, reopen the file and run the last test again.
	# We open it here, ourselves, because all uses of the db
	# need to have the correct comparison func set.  Then
	# call dump_file_direction directly.
	set btvalsck {}
	set db [eval {berkdb_open $compflag $cmpfunc -rdonly} \
	     $dbargs $encargs $method $testfile]
	error_check_good dbopen [is_valid_db $db] TRUE
	if { $txnenv == 1 } {
		set t [$env txn]
		error_check_good txn [is_valid_txn $t $env] TRUE
		set txn "-txn $t"
	}
	dump_file_direction $db $txn $t1 test093_check "-first" "-next"
	if { $txnenv == 1 } {
		error_check_good txn [$t commit] 0
	}
	error_check_good db_close [$db close] 0

	if { [is_hash $method] == 1 || [is_partition_callback $dbargs] == 1 } {
		return
	}

	# We need to sort btvals according to the comparison function.
	# Once that is done, btvalsck and btvals should be the same.
	puts "\tTest$tnum.d: check file order"

	$sortfunc

	error_check_good btvals:len [llength $btvals] [llength $btvalsck]
	for {set i 0} {$i < $nentries} {incr i} {
		error_check_good vals:$i [lindex $btvals $i] \
		    [lindex $btvalsck $i]
	}
}

proc test093_runbig { method dbargs nentries tnum compflag cmpfunc sortfunc } {
	source ./include.tcl
	global btvals
	global btvalsck

	# We'll need any encryption args separated from the db args
	# so we can pass them to dbverify.
	set encargs ""
	set dbargs [split_encargs $dbargs encargs]

	# Create the database and open the dictionary
	set eindex [lsearch -exact $dbargs "-env"]
	#
	# If we are using an env, then testfile should just be the db name.
	# Otherwise it is the test directory and the name.
	set txnenv 0
	if { $eindex == -1 } {
		set testfile $testdir/test$tnum.db
		set env NULL
		set envargs ""
	} else {
		set testfile test$tnum.db
		incr eindex
		set env [lindex $dbargs $eindex]
		set envargs " -env $env "
		set txnenv [is_txnenv $env]
		set testdir [get_home $env]
	}
	cleanup $testdir $env

	set db [eval {berkdb_open $compflag $cmpfunc \
	     -create -mode 0644} $method $encargs $dbargs $testfile]
	error_check_good dbopen [is_valid_db $db] TRUE

	set t1 $testdir/t1
	set t2 $testdir/t2
	set t3 $testdir/t3
	set t4 $testdir/t4
	set t5 $testdir/t5
	set txn ""
	set btvals {}
	set btvalsck {}
	puts "\tTest$tnum.e:\
	    big key put/get loop key=filecontents data=filename"

	# Here is the loop where we put and get each key/data pair
	set file_list [get_file_list 1]

	set count 0
	foreach f $file_list {
		set fid [open $f r]
		fconfigure $fid -translation binary
		set key [read $fid]
		close $fid

		set key $f$key

		set fcopy [open $t5 w]
		fconfigure $fcopy -translation binary
		puts -nonewline $fcopy $key
		close $fcopy

		if { $txnenv == 1 } {
			set t [$env txn]
			error_check_good txn [is_valid_txn $t $env] TRUE
			set txn "-txn $t"
		}
		set ret [eval {$db put} $txn {$key \
		    [chop_data $method $f]}]
		error_check_good put_file $ret 0
		if { $txnenv == 1 } {
			error_check_good txn [$t commit] 0
		}

		lappend btvals $key

		# Should really catch errors
		set fid [open $t4 w]
		fconfigure $fid -translation binary
		if [catch {eval {$db get} {$key}} data] {
			puts -nonewline $fid $data
		} else {
			# Data looks like {{key data}}
			set key [lindex [lindex $data 0] 0]
			puts -nonewline $fid $key
		}
		close $fid
		error_check_good \
		    Test093:diff($t5,$t4) [filecmp $t5 $t4] 0

		incr count
	}

	# Now we will get each key from the DB and compare the results
	# to the original.
	puts "\tTest$tnum.f: big dump file"
	if { $txnenv == 1 } {
		set t [$env txn]
		error_check_good txn [is_valid_txn $t $env] TRUE
		set txn "-txn $t"
	}
	dump_file $db $txn $t1 test093_checkbig
	if { $txnenv == 1 } {
		error_check_good txn [$t commit] 0
	}
	error_check_good db_close [$db close] 0

	# Run verify to check the internal structure and order.
	if { [catch {eval {berkdb dbverify} $compflag $cmpfunc\
	    $envargs $encargs {$testfile}} res] } {
		error "FAIL: Verification failed with $res"
	}

	puts "\tTest$tnum.g: dump file in order"
	# Now, reopen the file and run the last test again.
	# We open it here, ourselves, because all uses of the db
	# need to have the correct comparison func set.  Then
	# call dump_file_direction directly.

	set btvalsck {}
	set db [eval {berkdb_open $compflag $cmpfunc -rdonly} \
	     $encargs $dbargs $method $testfile]
	error_check_good dbopen [is_valid_db $db] TRUE
	if { $txnenv == 1 } {
		set t [$env txn]
		error_check_good txn [is_valid_txn $t $env] TRUE
		set txn "-txn $t"
	}
	dump_file_direction $db $txn $t1 test093_checkbig "-first" "-next"
	if { $txnenv == 1 } {
		error_check_good txn [$t commit] 0
	}
	error_check_good db_close [$db close] 0

	if { [is_hash $method] == 1 || [is_partition_callback $dbargs] == 1 } {
		return
	}

	# We need to sort btvals according to the comparison function.
	# Once that is done, btvalsck and btvals should be the same.
	puts "\tTest$tnum.h: check file order"

	$sortfunc
	error_check_good btvals:len [llength $btvals] [llength $btvalsck]

	set end [llength $btvals]
	for {set i 0} {$i < $end} {incr i} {
		error_check_good vals:$i [lindex $btvals $i] \
		    [lindex $btvalsck $i]
	}
}

# Simple bt comparison.
proc test093_cmp1 { a b } {
	return [string compare $b $a]
}

# Simple bt sorting.
proc test093_sort1 {} {
	global btvals
	#
	# This one is easy, just sort in reverse.
	#
	set btvals [lsort -decreasing $btvals]
}

proc test093_cmp2 { a b } {
	set arev [reverse $a]
	set brev [reverse $b]
	return [string compare $arev $brev]
}

proc test093_sort2 {} {
	global btvals

	# We have to reverse them, then sorts them.
	# Then reverse them back to real words.
	set rbtvals {}
	foreach i $btvals {
		lappend rbtvals [reverse $i]
	}
	set rbtvals [lsort -increasing $rbtvals]
	set newbtvals {}
	foreach i $rbtvals {
		lappend newbtvals [reverse $i]
	}
	set btvals $newbtvals
}

# Check function for test093; keys and data are identical
proc test093_check { key data } {
	global btvalsck

	error_check_good "key/data mismatch" $data [reverse $key]
	lappend btvalsck $key
}

# Check function for test093 big keys;
proc test093_checkbig { key data } {
	source ./include.tcl
	global btvalsck

	set fid [open $data r]
	fconfigure $fid -translation binary
	set cont [read $fid]
	close $fid
	error_check_good "key/data mismatch" $key $data$cont
	lappend btvalsck $key
}


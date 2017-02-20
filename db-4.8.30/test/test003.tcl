# See the file LICENSE for redistribution information.
#
# Copyright (c) 1996-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	test003
# TEST	Small keys/large data
# TEST		Put/get per key
# TEST		Dump file
# TEST		Close, reopen
# TEST		Dump file
# TEST
# TEST	Take the source files and dbtest executable and enter their names
# TEST	as the key with their contents as data.  After all are entered,
# TEST	retrieve all; compare output to original. Close file, reopen, do
# TEST	retrieve and re-verify.
proc test003 { method args} {
	global names
	source ./include.tcl

	set args [convert_args $method $args]
	set omethod [convert_method $method]

	if {[is_fixed_length $method] == 1} {
		puts "Test003 skipping for method $method"
		return
	}
	puts "Test003: $method ($args) filename=key filecontents=data pairs"

	# Create the database and open the dictionary
	set limit 0
	set txnenv 0
	set eindex [lsearch -exact $args "-env"]
	#
	# If we are using an env, then testfile should just be the db name.
	# Otherwise it is the test directory and the name.
	if { $eindex == -1 } {
		set testfile $testdir/test003.db
		set env NULL
	} else {
		set testfile test003.db
		incr eindex
		set env [lindex $args $eindex]
		set txnenv [is_txnenv $env]
		if { $txnenv == 1 } {
			append args " -auto_commit "
			set limit 100
		}
		set testdir [get_home $env]
	}
	set t1 $testdir/t1
	set t2 $testdir/t2
	set t3 $testdir/t3
	set t4 $testdir/t4

	cleanup $testdir $env
	set db [eval {berkdb_open \
	     -create -mode 0644} $args $omethod $testfile]
	error_check_good dbopen [is_valid_db $db] TRUE
	set pflags ""
	set gflags ""
	set txn ""
	if { [is_record_based $method] == 1 } {
		set checkfunc test003_recno.check
		append gflags "-recno"
	} else {
		set checkfunc test003.check
	}

	# Here is the loop where we put and get each key/data pair
	set file_list [get_file_list]
	set len [llength $file_list]
	puts "\tTest003.a: put/get loop $len entries"
	set count 0
	foreach f $file_list {
		if { [string compare [file type $f] "file"] != 0 } {
			continue
		}

		if { [is_record_based $method] == 1 } {
			set key [expr $count + 1]
			set names([expr $count + 1]) $f
		} else {
			set key $f
		}

		# Should really catch errors
		set fid [open $f r]
		fconfigure $fid -translation binary
		set data [read $fid]
		close $fid
		if { $txnenv == 1 } {
			set t [$env txn]
			error_check_good txn [is_valid_txn $t $env] TRUE
			set txn "-txn $t"
		}
		set ret [eval {$db put} \
		    $txn $pflags {$key [chop_data $method $data]}]
		error_check_good put $ret 0
		if { $txnenv == 1 } {
			error_check_good txn [$t commit] 0
		}

		# Should really catch errors
		set fid [open $t4 w]
		fconfigure $fid -translation binary
		if [catch {eval {$db get} $gflags {$key}} data] {
			puts -nonewline $fid $data
		} else {
			# Data looks like {{key data}}
			set key [lindex [lindex $data 0] 0]
			set data [lindex [lindex $data 0] 1]
			puts -nonewline $fid [pad_data $method $data]
		}
		close $fid

		error_check_good \
		    Test003:diff($f,$t4) [filecmp $f $t4] 0

		incr count
	}

	# Now we will get each key from the DB and compare the results
	# to the original.
	puts "\tTest003.b: dump file"
	if { $txnenv == 1 } {
		set t [$env txn]
		error_check_good txn [is_valid_txn $t $env] TRUE
		set txn "-txn $t"
	}
	dump_bin_file $db $txn $t1 $checkfunc
	if { $txnenv == 1 } {
		error_check_good txn [$t commit] 0
	}
	error_check_good db_close [$db close] 0

	# Now compare the keys to see if they match the entries in the
	# current directory
	if { [is_record_based $method] == 1 } {
		set oid [open $t2 w]
		for {set i 1} {$i <= $count} {set i [incr i]} {
			puts $oid $i
		}
		close $oid
		file rename -force $t1 $t3
	} else {
		set oid [open $t2.tmp w]
		foreach f $file_list {
			if { [string compare [file type $f] "file"] != 0 } {
				continue
			}
			puts $oid $f
		}
		close $oid
		filesort $t2.tmp $t2
		fileremove $t2.tmp
		filesort $t1 $t3
	}

	error_check_good \
	    Test003:diff($t3,$t2) [filecmp $t3 $t2] 0

	# Now, reopen the file and run the last test again.
	puts "\tTest003.c: close, open, and dump file"
	eval open_and_dump_file $testfile $env $t1 $checkfunc \
	    dump_bin_file_direction "-first" "-next" $args

	if { [is_record_based $method] == 1 } {
		filesort $t1 $t3 -n
	}

	error_check_good \
	    Test003:diff($t3,$t2) [filecmp $t3 $t2] 0

	# Now, reopen the file and run the last test again in reverse direction.
	puts "\tTest003.d: close, open, and dump file in reverse direction"
	eval open_and_dump_file $testfile $env $t1 $checkfunc \
	    dump_bin_file_direction "-last" "-prev" $args

	if { [is_record_based $method] == 1 } {
		filesort $t1 $t3 -n
	}

	error_check_good \
	    Test003:diff($t3,$t2) [filecmp $t3 $t2] 0
}

# Check function for test003; key should be file name; data should be contents
proc test003.check { binfile tmpfile } {
	source ./include.tcl

	error_check_good Test003:datamismatch($binfile,$tmpfile) \
	    [filecmp $binfile $tmpfile] 0
}
proc test003_recno.check { binfile tmpfile } {
	global names
	source ./include.tcl

	set fname $names($binfile)
	error_check_good key"$binfile"_exists [info exists names($binfile)] 1
	error_check_good Test003:datamismatch($fname,$tmpfile) \
	    [filecmp $fname $tmpfile] 0
}

# See the file LICENSE for redistribution information.
#
# Copyright (c) 1999-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	sdb004
# TEST	Tests large subdb names
# TEST		subdb name = filecontents,
# TEST		key = filename, data = filecontents
# TEST			Put/get per key
# TEST			Dump file
# TEST			Dump subdbs, verify data and subdb name match
# TEST
# TEST	Create 1 db with many large subdbs.  Use the contents as subdb names.
# TEST	Take the source files and dbtest executable and enter their names as
# TEST	the key with their contents as data.  After all are entered, retrieve
# TEST	all; compare output to original. Close file, reopen, do retrieve and
# TEST	re-verify.
proc sdb004 { method args} {
	global names
	source ./include.tcl

	set args [convert_args $method $args]
	set omethod [convert_method $method]

	if { [is_queue $method] == 1 || [is_fixed_length $method] == 1 } {
		puts "Subdb004: skipping for method $method"
		return
	}

	puts "Subdb004: $method ($args) \
	    filecontents=subdbname filename=key filecontents=data pairs"

	set txnenv 0
	set envargs ""
	set eindex [lsearch -exact $args "-env"]
	#
	# If we are using an env, then testfile should just be the db name.
	# Otherwise it is the test directory and the name.
	if { $eindex == -1 } {
		set testfile $testdir/subdb004.db
		set env NULL
	} else {
		set testfile subdb004.db
		incr eindex
		set env [lindex $args $eindex]
		set envargs " -env $env "
		set txnenv [is_txnenv $env]
		if { $txnenv == 1 } {
			append args " -auto_commit "
			append envargs " -auto_commit "
		}
		set testdir [get_home $env]
	}
	# Create the database and open the dictionary
	set t1 $testdir/t1
	set t2 $testdir/t2
	set t3 $testdir/t3
	set t4 $testdir/t4

	cleanup $testdir $env
	set pflags ""
	set gflags ""
	set txn ""
	if { [is_record_based $method] == 1 } {
		set checkfunc subdb004_recno.check
		append gflags "-recno"
	} else {
		set checkfunc subdb004.check
	}

	# Here is the loop where we put and get each key/data pair
	# Note that the subdatabase name is passed in as a char *, not
	# in a DBT, so it may not contain nulls;  use only source files.
	set file_list [glob $src_root/*/*.c]
	set fcount [llength $file_list]
	if { $txnenv == 1 && $fcount > 100 } {
		set file_list [lrange $file_list 0 99]
		set fcount 100
	}

	set count 0
	if { [is_record_based $method] == 1 } {
		set oid [open $t2 w]
		for {set i 1} {$i <= $fcount} {set i [incr i]} {
			puts $oid $i
		}
		close $oid
	} else {
		set oid [open $t2.tmp w]
		foreach f $file_list {
			puts $oid $f
		}
		close $oid
		filesort $t2.tmp $t2
	}
	puts "\tSubdb004.a: Set/Check each subdb"
	foreach f $file_list {
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
		set subdb $data
		close $fid
		set db [eval {berkdb_open -create -mode 0644} \
		    $args {$omethod $testfile $subdb}]
		error_check_good dbopen [is_valid_db $db] TRUE
		if { $txnenv == 1 } {
			set t [$env txn]
			error_check_good txn [is_valid_txn $t $env] TRUE
			set txn "-txn $t"
		}
		set ret [eval \
		    {$db put} $txn $pflags {$key [chop_data $method $data]}]
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
			puts -nonewline $fid $data
		}
		close $fid

		error_check_good Subdb004:diff($f,$t4) \
		    [filecmp $f $t4] 0

		incr count

		# Now we will get each key from the DB and compare the results
		# to the original.
		# puts "\tSubdb004.b: dump file"
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

	}

	#
	# Now for each file, check that the subdb name is the same
	# as the data in that subdb and that the filename is the key.
	#
	puts "\tSubdb004.b: Compare subdb names with key/data"
	set db [eval {berkdb_open -rdonly} $envargs {$testfile}]
	error_check_good dbopen [is_valid_db $db] TRUE
	if { $txnenv == 1 } {
		set t [$env txn]
		error_check_good txn [is_valid_txn $t $env] TRUE
		set txn "-txn $t"
	}
	set c [eval {$db cursor} $txn]
	error_check_good db_cursor [is_valid_cursor $c $db] TRUE

	for {set d [$c get -first] } { [llength $d] != 0 } \
	    {set d [$c get -next] } {
		set subdbname [lindex [lindex $d 0] 0]
		set subdb [eval {berkdb_open} $args {$testfile $subdbname}]
		error_check_good dbopen [is_valid_db $db] TRUE

		# Output the subdb name
		set ofid [open $t3 w]
		fconfigure $ofid -translation binary
		if { [string compare "\0" \
		    [string range $subdbname end end]] == 0 } {
			set slen [expr [string length $subdbname] - 2]
			set subdbname [string range $subdbname 1 $slen]
		}
		puts -nonewline $ofid $subdbname
		close $ofid

		# Output the data
		set subc [eval {$subdb cursor} $txn]
		error_check_good db_cursor [is_valid_cursor $subc $subdb] TRUE
		set d [$subc get -first]
		error_check_good dbc_get [expr [llength $d] != 0] 1
		set key [lindex [lindex $d 0] 0]
		set data [lindex [lindex $d 0] 1]

		set ofid [open $t1 w]
		fconfigure $ofid -translation binary
		puts -nonewline $ofid $data
		close $ofid

		$checkfunc $key $t1
		$checkfunc $key $t3

		error_check_good Subdb004:diff($t3,$t1) \
		    [filecmp $t3 $t1] 0
		error_check_good curs_close [$subc close] 0
		error_check_good db_close [$subdb close] 0
	}
	error_check_good curs_close [$c close] 0
	if { $txnenv == 1 } {
		error_check_good txn [$t commit] 0
	}
	error_check_good db_close [$db close] 0

	if { [is_record_based $method] != 1 } {
		fileremove $t2.tmp
	}
}

# Check function for subdb004; key should be file name; data should be contents
proc subdb004.check { binfile tmpfile } {
	source ./include.tcl

	error_check_good Subdb004:datamismatch($binfile,$tmpfile) \
	    [filecmp $binfile $tmpfile] 0
}
proc subdb004_recno.check { binfile tmpfile } {
	global names
	source ./include.tcl

	set fname $names($binfile)
	error_check_good key"$binfile"_exists [info exists names($binfile)] 1
	error_check_good Subdb004:datamismatch($fname,$tmpfile) \
	    [filecmp $fname $tmpfile] 0
}

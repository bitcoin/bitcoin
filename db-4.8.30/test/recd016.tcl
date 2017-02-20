# See the file LICENSE for redistribution information.
#
# Copyright (c) 1996-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	recd016
# TEST	Test recovery after checksum error.
proc recd016 { method args} {
	global fixed_len
	global log_log_record_types
	global datastr
	source ./include.tcl

	set pgindex [lsearch -exact $args "-pagesize"]
	if { $pgindex != -1 } {
		puts "Recd016: skipping for specific pagesizes"
		return
	}
	if { [is_queueext $method] == 1 || [is_partitioned $args]} {
		puts "Recd016: skipping for method $method"
		return
	}

	puts "Recd016: $method recovery after checksum error"

	# Create the database and environment.
	env_cleanup $testdir

	set testfile recd016.db
	set flags "-create -txn -home $testdir"

	puts "\tRecd016.a: creating environment"
	set env_cmd "berkdb_env $flags"
	set dbenv [eval $env_cmd]
	error_check_good dbenv [is_valid_env $dbenv] TRUE

	set pgsize 512
	set orig_fixed_len $fixed_len
	set fixed_len [expr $pgsize / 4]
	set opts [convert_args $method $args]
	set omethod [convert_method $method]
	set oflags "-create $omethod -mode 0644 \
	    -auto_commit -chksum -pagesize $pgsize $opts $testfile"
	set db [eval {berkdb_open} -env $dbenv $oflags]

	#
	# Put some data.
	#
	set nument 50
	puts "\tRecd016.b: Put some data"
	for { set i 1 } { $i <= $nument } { incr i } {
		# Use 'i' as key so method doesn't matter
		set key $i
		set data $i$datastr

		# Put, in a txn.
		set txn [$dbenv txn]
		error_check_good txn_begin [is_valid_txn $txn $dbenv] TRUE
		error_check_good db_put \
		    [$db put -txn $txn $key [chop_data $method $data]] 0
		error_check_good txn_commit [$txn commit] 0
	}
	error_check_good db_close [$db close] 0
	error_check_good log_flush [$dbenv log_flush] 0
	error_check_good env_close [$dbenv close] 0
	#
	# We need to remove the env so that we don't get cached
	# pages.
	#
	error_check_good env_remove [berkdb envremove -home $testdir] 0

	puts "\tRecd016.c: Overwrite part of database"
	#
	# First just touch some bits in the file.  We want to go
	# through the paging system, so touch some data pages,
	# like the middle of page 2.
	# We should get a checksum error for the checksummed file.
	#
	set pg 2
	set fid [open $testdir/$testfile r+]
	fconfigure $fid -translation binary
	set seeklen [expr $pgsize * $pg + 200]
	seek $fid $seeklen start
	set byte [read $fid 1]
	binary scan $byte c val
	set newval [expr ~$val]
	set newbyte [binary format c $newval]
	seek $fid $seeklen start
	puts -nonewline $fid $newbyte
	close $fid

	#
	# Verify we get the checksum error.  When we get it, it should
	# log the error as well, so when we run recovery we'll need to
	# do catastrophic recovery.  We do this in a sub-process so that
	# the files are closed after the panic.
	#
	set f1 [open |$tclsh_path r+]
	puts $f1 "source $test_path/test.tcl"

	set env_cmd "berkdb_env_noerr $flags"
	set dbenv [send_cmd $f1 $env_cmd]
	error_check_good dbenv [is_valid_env $dbenv] TRUE

	set db [send_cmd $f1  "{berkdb_open_noerr} -env $dbenv $oflags"]
	error_check_good db [is_valid_db $db] TRUE

	# We need to set non-blocking mode so that after each command
	# we can read all the remaining output from that command and
	# we can know what the output from one command is.
	fconfigure $f1 -blocking 0
	set ret [read $f1]
	set got_err 0
	for { set i 1 } { $i <= $nument } { incr i } {
		set stat [send_cmd $f1  "catch {$db get $i} r"]
		set getret [send_cmd $f1  "puts \$r"]
		set ret [read $f1]
		if { $stat == 1 } {
			error_check_good dbget:fail [is_substr $getret \
			    "checksum error: page $pg"] 1
			set got_err 1
			break
		} else {
			set key [lindex [lindex $getret 0] 0]
			set data [lindex [lindex $getret 0] 1]
			error_check_good keychk $key $i
			error_check_good datachk $data \
			    [pad_data $method $i$datastr]
		}
	}
	error_check_good got_chksum $got_err 1
	set ret [send_cmd $f1  "$db close"]
	set extra [read $f1]
	error_check_good db:fail [is_substr $ret "run recovery"] 1

	set ret [send_cmd $f1  "$dbenv close"]
	error_check_good env_close:fail [is_substr $ret "handles still open"] 1
	close $f1

        # Keep track of the log types we've seen
	if { $log_log_record_types == 1} {
		logtrack_read $testdir
	}

	puts "\tRecd016.d: Run normal recovery"
	set ret [catch {exec $util_path/db_recover -h $testdir} r]
	error_check_good db_recover $ret 1
	error_check_good dbrec:fail \
	    [is_substr $r "checksum error"] 1

	catch {fileremove $testdir/$testfile} ret
	puts "\tRecd016.e: Run catastrophic recovery"
	set ret [catch {exec $util_path/db_recover -c -h $testdir} r]
	error_check_good db_recover $ret 0

	#
	# Now verify the data was reconstructed correctly.
	#
	set env_cmd "berkdb_env_noerr $flags"
	set dbenv [eval $env_cmd]
	error_check_good dbenv [is_valid_env $dbenv] TRUE

	set db [eval {berkdb_open} -env $dbenv $oflags]
	error_check_good db [is_valid_db $db] TRUE

	for { set i 1 } { $i <= $nument } { incr i } {
		set stat [catch {$db get $i} ret]
		error_check_good stat $stat 0
		set key [lindex [lindex $ret 0] 0]
		set data [lindex [lindex $ret 0] 1]
		error_check_good keychk $key $i
		error_check_good datachk $data [pad_data $method $i$datastr]
	}
	error_check_good db_close [$db close] 0
	error_check_good log_flush [$dbenv log_flush] 0
	error_check_good env_close [$dbenv close] 0
	set fixed_len $orig_fixed_len
	return
}

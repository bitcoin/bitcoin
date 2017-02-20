# See the file LICENSE for redistribution information.
#
# Copyright (c) 1996-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	test037
# TEST	Test DB_RMW
proc test037 { method {nentries 100} args } {
	global encrypt

	source ./include.tcl
	set eindex [lsearch -exact $args "-env"]
	#
	# If we are using an env, then skip this test.  It needs its own.
	if { $eindex != -1 } {
		incr eindex
		set env [lindex $args $eindex]
		puts "Test037 skipping for env $env"
		return
	}

	puts "Test037: RMW $method"

	set args [convert_args $method $args]
	set encargs ""
	set args [split_encargs $args encargs]
	set omethod [convert_method $method]
	set pageargs ""
	split_pageargs $args pageargs

	# Create the database
	env_cleanup $testdir
	set testfile test037.db

	set local_env \
	    [eval {berkdb_env -create -mode 0644 -txn} \
	         $encargs $pageargs -home $testdir]
	error_check_good dbenv [is_valid_env $local_env] TRUE

	set db [eval {berkdb_open -env $local_env  \
	     -create -mode 0644 $omethod} $args {$testfile}]
	error_check_good dbopen [is_valid_db $db] TRUE

	set did [open $dict]
	set count 0

	set pflags ""
	set gflags ""
	set txn ""

	if { [is_record_based $method] == 1 } {
		append gflags " -recno"
	}

	puts "\tTest037.a: Creating database"
	# Here is the loop where we put and get each key/data pair
	while { [gets $did str] != -1 && $count < $nentries } {
		if { [is_record_based $method] == 1 } {
			global kvals

			set key [expr $count + 1]
			set kvals($key) [pad_data $method $str]
		} else {
			set key $str
		}
		set ret [eval {$db put} \
		    $txn $pflags {$key [chop_data $method $str]}]
		error_check_good put $ret 0

		set ret [eval {$db get} $txn $gflags {$key}]
		error_check_good get \
		    [lindex [lindex $ret 0] 1] [pad_data $method $str]
		incr count
	}
	close $did
	error_check_good dbclose [$db close] 0
	error_check_good envclode [$local_env close] 0

	puts "\tTest037.b: Setting up environments"

	# Open local environment
	set env_cmd \
	    [concat berkdb_env -create -txn $encargs $pageargs -home $testdir]
	set local_env [eval $env_cmd]
	error_check_good dbenv [is_valid_env $local_env] TRUE

	# Open local transaction
	set local_txn [$local_env txn]
	error_check_good txn_open [is_valid_txn $local_txn $local_env] TRUE

	# Open remote environment
	set f1 [open |$tclsh_path r+]
	puts $f1 "source $test_path/test.tcl"

	set remote_env [send_cmd $f1 $env_cmd]
	error_check_good remote:env_open [is_valid_env $remote_env] TRUE

	# Open remote transaction
	set remote_txn [send_cmd $f1 "$remote_env txn"]
	error_check_good \
	    remote:txn_open [is_valid_txn $remote_txn $remote_env] TRUE

	# Now try put test without RMW.  Gets on one site should not
	# lock out gets on another.

	# Open databases and dictionary
	puts "\tTest037.c: Opening databases"
	set did [open $dict]
	set rkey 0

	set db [eval {berkdb_open -auto_commit -env $local_env } $args {$testfile}]
	error_check_good dbopen [is_valid_db $db] TRUE
	set rdb [send_cmd $f1 \
	    "berkdb_open -auto_commit -env $remote_env $args -mode 0644  $testfile"]
	error_check_good remote:dbopen [is_valid_db $rdb] TRUE

	puts "\tTest037.d: Testing without RMW"

	# Now, get a key and try to "get" it from both DBs.
	error_check_bad "gets on new open" [gets $did str] -1
	incr rkey
	if { [is_record_based $method] == 1 } {
		set key $rkey
	} else {
		set key $str
	}

	set rec [eval {$db get -txn $local_txn} $gflags {$key}]
	error_check_good local_get [lindex [lindex $rec 0] 1] \
	    [pad_data $method $str]

	set r [send_timed_cmd $f1 0 "$rdb get -txn $remote_txn $gflags $key"]
	error_check_good remote_send $r 0

	# Now sleep before releasing local record lock
	tclsleep 5
	error_check_good local_commit [$local_txn commit] 0

	# Now get the remote result
	set remote_time [rcv_result $f1]
	error_check_good no_rmw_get:remote_time [expr $remote_time <= 1] 1

	# Commit the remote
	set r [send_cmd $f1 "$remote_txn commit"]
	error_check_good remote_commit $r 0

	puts "\tTest037.e: Testing with RMW"

	# Open local transaction
	set local_txn [$local_env txn]
	error_check_good \
	    txn_open [is_valid_txn $local_txn $local_env] TRUE

	# Open remote transaction
	set remote_txn [send_cmd $f1 "$remote_env txn"]
	error_check_good remote:txn_open \
	    [is_valid_txn $remote_txn $remote_env] TRUE

	# Now, get a key and try to "get" it from both DBs.
	error_check_bad "gets on new open" [gets $did str] -1
	incr rkey
	if { [is_record_based $method] == 1 } {
		set key $rkey
	} else {
		set key $str
	}

	set rec [eval {$db get -txn $local_txn -rmw} $gflags {$key}]
	error_check_good \
	    local_get [lindex [lindex $rec 0] 1] [pad_data $method $str]

	set r [send_timed_cmd $f1 0 "$rdb get -txn $remote_txn $gflags $key"]
	error_check_good remote_send $r 0

	# Now sleep before releasing local record lock
	tclsleep 5
	error_check_good local_commit [$local_txn commit] 0

	# Now get the remote result
	set remote_time [rcv_result $f1]
	error_check_good rmw_get:remote_time [expr $remote_time > 4] 1

	# Commit the remote
	set r [send_cmd $f1 "$remote_txn commit"]
	error_check_good remote_commit $r 0

	# Close everything up: remote first
	set r [send_cmd $f1 "$rdb close"]
	error_check_good remote_db_close $r 0

	set r [send_cmd $f1 "$remote_env close"]

	# Close locally
	error_check_good db_close [$db close] 0
	$local_env close
	close $did
	close $f1
}

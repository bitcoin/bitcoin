# See the file LICENSE for redistribution information.
#
# Copyright (c) 2000-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	recd011
# TEST	Verify that recovery to a specific timestamp works.
proc recd011 { method {niter 200} {ckpt_freq 15} {sleep_time 1} args } {
	source ./include.tcl
	global rand_init
	berkdb srand $rand_init

	set args [convert_args $method $args]
	set omethod [convert_method $method]
	set tnum "011"

	puts "Recd$tnum ($method $args): Test recovery to a specific timestamp."

	set testfile recd$tnum.db
	env_cleanup $testdir

	set i 0
	if { [is_record_based $method] == 1 } {
		set key 1
		set bigkey 1001
	} else {
		set key KEY
		set bigkey BIGKEY
	}

	puts "\tRecd$tnum.a: Create environment and database."
	set bufsize [expr 8 * 1024]
	set maxsize [expr 8 * $bufsize]
	set flags "-create -txn -home $testdir -log_buffer $bufsize \
	    -log_max $maxsize"

	set env_cmd "berkdb_env $flags"
	set dbenv [eval $env_cmd]
	error_check_good dbenv [is_valid_env $dbenv] TRUE

	set oflags "-auto_commit -env $dbenv -create -mode 0644 $args $omethod"
	set db [eval {berkdb_open} $oflags $testfile]
	error_check_good dbopen [is_valid_db $db] TRUE

	# Main loop:  every second or so, increment the db in a txn.
	puts "\t\tInitial Checkpoint"
	error_check_good "Initial Checkpoint" [$dbenv txn_checkpoint] 0

	puts "\tRecd$tnum.b ($niter iterations):\
	    Transaction-protected increment loop."
	for { set i 0 } { $i <= $niter } { incr i } {
		set str [random_data 4096 0 NOTHING]
		set data $i
		set bigdata $i$str

		# Put, in a txn.
		set txn [$dbenv txn]
		error_check_good txn_begin [is_valid_txn $txn $dbenv] TRUE
		error_check_good db_put \
		    [$db put -txn $txn $key [chop_data $method $data]] 0
		error_check_good db_put \
		    [$db put -txn $txn $bigkey [chop_data $method $bigdata]] 0
		error_check_good txn_commit [$txn commit] 0

		# We need to sleep before taking the timestamp to guarantee
		# that the timestamp is *after* this transaction commits.
		# Since the resolution of the system call used by Berkeley DB
		# is less than a second, rounding to the nearest second can
		# otherwise cause off-by-one errors in the test.
		tclsleep $sleep_time

		set timeof($i) [timestamp -r]

		# If an appropriate period has elapsed, checkpoint.
		if { $i % $ckpt_freq == $ckpt_freq - 1 } {
			puts "\t\tIteration $i: Checkpointing."
			error_check_good ckpt($i) [$dbenv txn_checkpoint] 0
		}

		# Sleep again to ensure that the next operation definitely
		# occurs after the timestamp.
		tclsleep $sleep_time
	}
	error_check_good db_close [$db close] 0
	error_check_good env_close [$dbenv close] 0

	# Now, loop through and recover to each timestamp, verifying the
	# expected increment.
	puts "\tRecd$tnum.c: Recover to each timestamp and check."
	for { set i $niter } { $i >= 0 } { incr i -1 } {

		# Run db_recover.
		set t [clock format $timeof($i) -format "%y%m%d%H%M.%S"]
		# puts $t
		berkdb debug_check
		set ret [catch {exec $util_path/db_recover -h $testdir -t $t} r]
		error_check_good db_recover($i,$t,$r) $ret 0

		# Now open the db and check the timestamp.
		set db [eval {berkdb_open} $args $testdir/$testfile]
		error_check_good db_open($i) [is_valid_db $db] TRUE

		set dbt [$db get $key]
		set datum [lindex [lindex $dbt 0] 1]
		error_check_good timestamp_recover $datum [pad_data $method $i]

		error_check_good db_close [$db close] 0
	}

	# Finally, recover to a time well before the first timestamp
	# and well after the last timestamp.  The latter should
	# be just like the timestamp of the last test performed;
	# the former should fail.
	puts "\tRecd$tnum.d: Recover to before the first timestamp."
	set t [clock format [expr $timeof(0) - 1000] -format "%y%m%d%H%M.%S"]
	set ret [catch {exec $util_path/db_recover -h $testdir -t $t} r]
	error_check_bad db_recover(before,$t) $ret 0

	puts "\tRecd$tnum.e: Recover to after the last timestamp."
	set t [clock format \
	    [expr $timeof($niter) + 1000] -format "%y%m%d%H%M.%S"]
	set ret [catch {exec $util_path/db_recover -h $testdir -t $t} r]
	error_check_good db_recover(after,$t) $ret 0

	# Now open the db and check the timestamp.
	set db [eval {berkdb_open} $args $testdir/$testfile]
	error_check_good db_open(after) [is_valid_db $db] TRUE

	set dbt [$db get $key]
	set datum2 [lindex [lindex $dbt 0] 1]

	error_check_good timestamp_recover $datum2 $datum
	error_check_good db_close [$db close] 0
}

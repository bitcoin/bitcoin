# See the file LICENSE for redistribution information.
#
# Copyright (c) 1999-2009 Oracle.  All rights reserved.
#
# $Id$
#

proc t106_initial { nitems nprod id tnum dbenv order args } {
	source ./include.tcl

	set pid [pid]
	puts "\tTest$tnum: Producer $pid initializing DBs"

	# Each producer initially loads a small number of items to
	# each btree database, then enters a RMW loop where it randomly
	# selects and executes a cursor operations which either:
	# 1.  Read-modify-write an item in db2; or
	# 2.  Read-modify-write an item in both db2 and db3, randomly
	# selecting between db2 and db3 on which to open first, which to
	# read first, which to write first, which to close first.  This
	# may create deadlocks so keep trying until it's successful.

	# Open queue database
	set dbq [eval {berkdb_open -create -queue -env $dbenv\
	    -auto_commit -len 32 queue.db} ]
	error_check_good dbq_open [is_valid_db $dbq] TRUE

	# Open four btree databases
	set db1 [berkdb_open \
	    -create -btree -env $dbenv -auto_commit testfile1.db]
	error_check_good db1_open [is_valid_db $db1] TRUE
	set db2 [berkdb_open \
	    -create -btree -env $dbenv -auto_commit testfile2.db]
	error_check_good db2_open [is_valid_db $db2] TRUE
	set db3 [berkdb_open \
	    -create -btree -env $dbenv -auto_commit testfile3.db]
	error_check_good db3_open [is_valid_db $db3] TRUE
	set db4 [berkdb_open \
	    -create -btree -env $dbenv -auto_commit testfile4.db]
	error_check_good db4_open [is_valid_db $db4] TRUE

	# Initialize databases with $nitems items from each producer.
	set did [open $dict]
	for { set i 1 } { $i <= $nitems } { incr i } {
		set db2data [read $did [berkdb random_int 300 700]]
		set db3data [read $did [berkdb random_int 500 1000]]
		set qdata [read $did 32]
		set suffix _0_$i
		set db23key "testclient$id$suffix"
		set suffix _$i
		set db4key key$id$suffix

		set t [$dbenv txn]
		set txn "-txn $t"
		error_check_good db2_put [eval {$db2 put} $txn\
		    {$db23key $db2data}] 0
		error_check_good db3_put [eval {$db3 put} $txn\
		    {$db23key $db3data}] 0
		error_check_good db4_put [eval {$db4 put} $txn\
		    {$db4key $db23key}] 0

		set c [$dbenv txn -parent $t]
		set ctxn "-txn $c"
		set qrecno [eval {$dbq put -append} $ctxn {$qdata}]
		error_check_good db1_put [eval {$db1 put} $ctxn\
		    {$qrecno $db2data}] 0
		error_check_good commit_child [$c commit] 0
		error_check_good commit_parent [$t commit] 0
	}
	close $did

	set ret [catch {$dbq close} res]
	error_check_good dbq_close:$pid $ret 0
	set ret [catch {$db1 close} res]
	error_check_good db1_close:$pid $ret 0
	set ret [catch {$db2 close} res]
	error_check_good db2_close:$pid $ret 0
	set ret [catch {$db3 close} res]
	error_check_good db3_close:$pid $ret 0
	set ret [catch {$db4 close} res]
	error_check_good db4_close:$pid $ret 0

	puts "\t\tTest$tnum: Initializer $pid finished."
}

proc t106_produce { nitems nprod id tnum dbenv order niter args } {
	source ./include.tcl

	set pid [pid]
	set did [open $dict]
	puts "\tTest$tnum: Producer $pid initializing DBs"

	# Open queue database
	set dbq [eval {berkdb_open -create -queue -env $dbenv\
	    -auto_commit -len 32 queue.db} ]
	error_check_good dbq_open [is_valid_db $dbq] TRUE

	# Open four btree databases
	set db1 [berkdb_open \
	    -create -btree -env $dbenv -auto_commit testfile1.db]
	error_check_good db1_open [is_valid_db $db1] TRUE
	set db2 [berkdb_open \
	    -create -btree -env $dbenv -auto_commit testfile2.db]
	error_check_good db2_open [is_valid_db $db2] TRUE
	set db3 [berkdb_open \
	    -create -btree -env $dbenv -auto_commit testfile3.db]
	error_check_good db3_open [is_valid_db $db3] TRUE
	set db4 [berkdb_open \
	    -create -btree -env $dbenv -auto_commit testfile4.db]
	error_check_good db4_open [is_valid_db $db4] TRUE

	# Now go into RMW phase.
	for { set i 1 } { $i <= $niter } { incr i } {

		set op [berkdb random_int 1 2]
		set newdb2data [read $did [berkdb random_int 300 700]]
		set qdata [read $did 32]

		if { $order == "ordered" } {
			set n [expr $i % $nitems]
			if { $n == 0 } {
				set n $nitems
			}
			set suffix _0_$n
		} else {
			# Retrieve a random key from the list
			set suffix _0_[berkdb random_int 1 $nitems]
		}
		set key "testclient$id$suffix"

		set t [$dbenv txn]
		set txn "-txn $t"

		# Now execute op1 or op2
		if { $op == 1 } {
			op1 $db2 $key $newdb2data $txn
		} elseif { $op == 2 } {
			set newdb3data [read $did [berkdb random_int 500 1000]]
			op2 $db2 $db3 $key $newdb2data $newdb3data $txn $dbenv
		} else {
			puts "FAIL: unrecogized op $op"
		}
		set c [$dbenv txn -parent $t]
		set ctxn "-txn $c"
		set qrecno [eval {$dbq put -append} $ctxn {$qdata}]
		error_check_good db1_put [eval {$db1 put} $ctxn\
		    {$qrecno $newdb2data}] 0
		error_check_good child_commit [$c commit] 0
		error_check_good parent_commit [$t commit] 0
	}
	close $did

	set ret [catch {$dbq close} res]
	error_check_good dbq_close:$pid $ret 0
	set ret [catch {$db1 close} res]
	error_check_good db1_close:$pid $ret 0
	set ret [catch {$db2 close} res]
	error_check_good db2_close:$pid $ret 0
	set ret [catch {$db3 close} res]
	error_check_good db3_close:$pid $ret 0
	set ret [catch {$db4 close} res]
	error_check_good db4_close:$pid $ret 0

	puts "\t\tTest$tnum: Producer $pid finished."
}

proc t106_consume { nitems tnum outputfile mode dbenv niter args } {
	source ./include.tcl
	set pid [pid]
	puts "\tTest$tnum: Consumer $pid starting ($niter iterations)."

	# Open queue database and btree database 1.
	set dbq [eval {berkdb_open \
	    -create -queue -env $dbenv -auto_commit -len 32 queue.db} ]
	error_check_good dbq_open:$pid [is_valid_db $dbq] TRUE

	set db1 [eval {berkdb_open \
	    -create -btree -env $dbenv -auto_commit testfile1.db} ]
	error_check_good db1_open:$pid [is_valid_db $db1] TRUE

	set oid [open $outputfile a]

	for { set i 1 } { $i <= $nitems } {incr i } {
		set t [$dbenv txn]
		set txn "-txn $t"
		set ret [eval {$dbq get $mode} $txn]
		set qrecno [lindex [lindex $ret 0] 0]
		set db1curs [eval {$db1 cursor} $txn]
		if {[catch {eval $db1curs get -set -rmw $qrecno} res]} {
			puts "FAIL: $db1curs get: $res"
		}
		error_check_good db1curs_del [$db1curs del] 0
		error_check_good db1curs_close [$db1curs close] 0
		error_check_good txn_commit [$t commit] 0
	}

	error_check_good output_close:$pid [close $oid] ""

	set ret [catch {$dbq close} res]
	error_check_good dbq_close:$pid $ret 0
	set ret [catch {$db1 close} res]
	error_check_good db1_close:$pid $ret 0
	puts "\t\tTest$tnum: Consumer $pid finished."
}

# op1 overwrites one data item in db2.
proc op1 { db2 key newdata txn } {

	set db2c [eval {$db2 cursor} $txn]
puts "in op1, key is $key"
	set ret [eval {$db2c get -set -rmw $key}]
	# Make sure we retrieved something
	error_check_good db2c_get [llength $ret] 1
	error_check_good db2c_put [eval {$db2c put} -current {$newdata}] 0
	error_check_good db2c_close [$db2c close] 0
}

# op 2
proc op2 { db2 db3 key newdata2 newdata3 txn dbenv } {

	# Randomly choose whether to work on db2 or db3 first for
	# each operation: open cursor, get, put, close.
	set open1 [berkdb random_int 0 1]
	set get1 [berkdb random_int 0 1]
	set put1 [berkdb random_int 0 1]
	set close1 [berkdb random_int 0 1]
puts "open [expr $open1 + 2] first, get [expr $get1 + 2] first,\
    put [expr $put1 + 2] first, close [expr $close1 + 2] first"
puts "in op2, key is $key"

	# Open cursor
	if { $open1 == 0 } {
		set db2c [eval {$db2 cursor} $txn]
		set db3c [eval {$db3 cursor} $txn]
	} else {
		set db3c [eval {$db3 cursor} $txn]
		set db2c [eval {$db2 cursor} $txn]
	}
	error_check_good db2_cursor [is_valid_cursor $db2c $db2] TRUE
	error_check_good db3_cursor [is_valid_cursor $db3c $db3] TRUE

	# Do the following until we succeed and don't get DB_DEADLOCK:
	if { $get1 == 0 } {
		get_set_rmw $db2c $key $dbenv
		get_set_rmw $db3c $key $dbenv
	} else {
		get_set_rmw $db3c $key $dbenv
		get_set_rmw $db2c $key $dbenv
	}

	# Put new data.
	if { $put1 == 0 } {
		error_check_good db2c_put [eval {$db2c put} \
		    -current {$newdata2}] 0
		error_check_good db3c_put [eval {$db3c put} \
		    -current {$newdata3}] 0
	} else {
		error_check_good db3c_put [eval {$db3c put} \
		    -current {$newdata3}] 0
		error_check_good db2c_put [eval {$db2c put} \
		    -current {$newdata2}] 0
	}
	if { $close1 == 0 } {
		error_check_good db2c_close [$db2c close] 0
		error_check_good db3c_close [$db3c close] 0
	} else {
		error_check_good db3c_close [$db3c close] 0
		error_check_good db2c_close [$db2c close] 0
	}
}

proc get_set_rmw { dbcursor key dbenv } {

	while { 1 } {
		if {[catch {set ret [eval {$dbcursor get -set -rmw} $key]}\
		    res ]} {
			# If the get failed, break if it failed for any
			# reason other than deadlock.  If we have deadlock,
			# the deadlock detector should break the deadlock
			# as we keep trying.
			if { [is_substr $res DB_LOCK_DEADLOCK] != 1 } {
				puts "FAIL: get_set_rmw: $res"
				break
			}
		} else {
			# We succeeded.  Go back to the body of the test.
			break
		}
	}
}

source ./include.tcl
source $test_path/test.tcl

# Verify usage
set usage "t106script.tcl dir runtype nitems nprod outputfile id tnum order"
if { $argc < 10 } {
	puts stderr "FAIL:[timestamp] Usage: $usage"
	exit
}

# Initialize arguments
set dir [lindex $argv 0]
set runtype [lindex $argv 1]
set nitems [lindex $argv 2]
set nprod [lindex $argv 3]
set outputfile [lindex $argv 4]
set id [lindex $argv 5]
set tnum [lindex $argv 6]
set order [lindex $argv 7]
set niter [lindex $argv 8]
# args is the string "{ -len 20 -pad 0}", so we need to extract the
# " -len 20 -pad 0" part.
set args [lindex [lrange $argv 9 end] 0]

# Open env
set dbenv [berkdb_env -home $dir -txn]
error_check_good dbenv_open [is_valid_env $dbenv] TRUE

# Invoke initial, produce or consume based on $runtype
if { $runtype == "INITIAL" } {
	t106_initial $nitems $nprod $id $tnum $dbenv $order $args
} elseif { $runtype == "PRODUCE" } {
	t106_produce $nitems $nprod $id $tnum $dbenv $order $niter $args
} elseif { $runtype == "WAIT" } {
	t106_consume $nitems $tnum $outputfile -consume_wait $dbenv $args
} else {
	error_check_good bad_args $runtype "either PRODUCE, or WAIT"
}
error_check_good env_close [$dbenv close] 0
exit

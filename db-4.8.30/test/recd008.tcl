# See the file LICENSE for redistribution information.
#
# Copyright (c) 1996-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	recd008
# TEST	Test deeply nested transactions and many-child transactions.
proc recd008 { method {breadth 4} {depth 4} args} {
	global kvals
	source ./include.tcl

	set args [convert_args $method $args]
	set omethod [convert_method $method]

	puts "Recd008: $method $breadth X $depth deeply nested transactions"

	# Create the database and environment.
	env_cleanup $testdir

	set dbfile recd008.db

	puts "\tRecd008.a: create database"
	set db [eval {berkdb_open -create} $args $omethod $testdir/$dbfile]
	error_check_good dbopen [is_valid_db $db] TRUE

	# Make sure that we have enough entries to span a couple of
	# different pages.
	set did [open $dict]
	set count 0
	while { [gets $did str] != -1 && $count < 1000 } {
		if { [is_record_based $method] == 1 } {
			set key [expr $count + 1]
		} else {
			set key $str
		}
		if { $count == 500} {
			set p1 $key
			set kvals($p1) $str
		}
		set ret [$db put $key [chop_data $method $str]]
		error_check_good put $ret 0

		incr count
	}
	close $did
	error_check_good db_close [$db close] 0

	set txn_max [expr int([expr pow($breadth,$depth)])]
	if { $txn_max < 20 } {
		set txn_max 20
	}

	puts "\tRecd008.b: create environment for $txn_max transactions"

	set max_locks 2500
	set eflags "-mode 0644 -create -lock_max_locks $max_locks \
	    -lock_max_objects $max_locks -txn_max $txn_max -txn -home $testdir"
	set env_cmd "berkdb_env $eflags"
	set dbenv [eval $env_cmd]
	error_check_good env_open [is_valid_env $dbenv] TRUE

	reset_env $dbenv

	set rlist {
	{ {recd008_parent abort ENV DB $method $p1 TXNID 1 1 $breadth $depth}
		"Recd008.c: child abort parent" }
	{ {recd008_parent commit ENV DB $method $p1 TXNID 1 1 $breadth $depth}
		"Recd008.d: child commit parent" }
	}
	foreach pair $rlist {
		set cmd [subst [lindex $pair 0]]
		set msg [lindex $pair 1]
		op_recover abort $testdir $env_cmd $dbfile $cmd $msg $args
		eval recd008_setkval $dbfile $p1 $args
		op_recover commit $testdir $env_cmd $dbfile $cmd $msg $args
		eval recd008_setkval $dbfile $p1 $args
	}

	puts "\tRecd008.e: Verify db_printlog can read logfile"
	set tmpfile $testdir/printlog.out
	set stat [catch {exec $util_path/db_printlog -h $testdir \
	    > $tmpfile} ret]
	error_check_good db_printlog $stat 0
	fileremove $tmpfile
}

proc recd008_setkval { dbfile p1 args} {
	global kvals
	source ./include.tcl

	set db [eval {berkdb_open} $args $testdir/$dbfile]
	error_check_good dbopen [is_valid_db $db] TRUE
	set ret [$db get $p1]
	error_check_good dbclose [$db close] 0
	set kvals($p1) [lindex [lindex $ret 0] 1]
}

# This is a lot like the op_recover procedure.  We cannot use that
# because it was not meant to be called recursively.  This proc
# knows about depth/breadth and file naming so that recursive calls
# don't overwrite various initial and afterop files, etc.
#
# The basic flow of this is:
#	(Initial file)
#	Parent begin transaction (in op_recover)
#	Parent starts children
#		Recursively call recd008_recover
#		(children modify p1)
#	Parent modifies p1
#	(Afterop file)
#	Parent commit/abort (in op_recover)
#	(Final file)
#	Recovery test (in op_recover)
proc recd008_parent { op env db method p1key parent b0 d0 breadth depth } {
	global kvals
	source ./include.tcl

	#
	# Save copy of original data
	# Acquire lock on data
	#
	set olddata [pad_data $method $kvals($p1key)]
	set ret [$db get -rmw -txn $parent $p1key]
	set Dret [lindex [lindex $ret 0] 1]
	error_check_good get_parent_RMW $Dret $olddata

	#
	# Parent spawns off children
	#
	set ret [recd008_txn $op $env $db $method $p1key $parent \
	    $b0 $d0 $breadth $depth]

	puts "Child runs complete.  Parent modifies data."

	#
	# Parent modifies p1
	#
	set newdata $olddata.parent
	set ret [$db put -txn $parent $p1key [chop_data $method $newdata]]
	error_check_good db_put $ret 0

	#
	# Save value in kvals for later comparison
	#
	switch $op {
		"commit" {
			set kvals($p1key) $newdata
		}
		"abort" {
			set kvals($p1key) $olddata
		}
	}
	return 0
}

proc recd008_txn { op env db method p1key parent b0 d0 breadth depth } {
	global log_log_record_types
	global kvals
	source ./include.tcl

	for {set d 1} {$d < $d0} {incr d} {
		puts -nonewline "\t"
	}
	puts "Recd008_txn: $op parent:$parent $breadth $depth ($b0 $d0)"

	# Save the initial file and open the environment and the file
	for {set b $b0} {$b <= $breadth} {incr b} {
		#
		# Begin child transaction
		#
		set t [$env txn -parent $parent]
		error_check_bad txn_begin $t NULL
		error_check_good txn_begin [is_valid_txn $t $env] TRUE
		set startd [expr $d0 + 1]
		set child $b:$startd:$t
		set olddata [pad_data $method $kvals($p1key)]
		set newdata $olddata.$child
		set ret [$db get -rmw -txn $t $p1key]
		set Dret [lindex [lindex $ret 0] 1]
		error_check_good get_parent_RMW $Dret $olddata

		#
		# Recursively call to set up nested transactions/children
		#
		for {set d $startd} {$d <= $depth} {incr d} {
			set ret [recd008_txn commit $env $db $method $p1key $t \
			    $b $d $breadth $depth]
			set ret [recd008_txn abort $env $db $method $p1key $t \
			    $b $d $breadth $depth]
		}
		#
		# Modifies p1.
		#
		set ret [$db put -txn $t $p1key [chop_data $method $newdata]]
		error_check_good db_put $ret 0

		#
		# Commit or abort
		#
		for {set d 1} {$d < $startd} {incr d} {
			puts -nonewline "\t"
		}
		puts "Executing txn_$op:$t"
		error_check_good txn_$op:$t [$t $op] 0
		for {set d 1} {$d < $startd} {incr d} {
			puts -nonewline "\t"
		}
		set ret [$db get -rmw -txn $parent $p1key]
		set Dret [lindex [lindex $ret 0] 1]
		set newdata [pad_data $method $newdata]
		switch $op {
			"commit" {
				puts "Command executed and committed."
				error_check_good get_parent_RMW $Dret $newdata
				set kvals($p1key) $newdata
			}
			"abort" {
				puts "Command executed and aborted."
				error_check_good get_parent_RMW $Dret $olddata
				set kvals($p1key) $olddata
			}
		}
	}
	return 0
}

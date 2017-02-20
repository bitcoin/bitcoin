# See the file LICENSE for redistribution information.
#
# Copyright (c) 1996-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	recd003
# TEST	Duplicate recovery tests.  For every known duplicate log message,
# TEST	makes sure that we exercise redo, undo, and do-nothing condition.
# TEST
# TEST	Test all the duplicate log messages and recovery operations.  We make
# TEST	sure that we exercise all possible recovery actions: redo, undo, undo
# TEST	but no fix necessary and redo but no fix necessary.
proc recd003 { method {select 0} args } {
	source ./include.tcl
	global rand_init

	set envargs ""
	set zero_idx [lsearch -exact $args "-zero_log"]
	if { $zero_idx != -1 } {
		set args [lreplace $args $zero_idx $zero_idx]
		set envargs "-zero_log"
	}

	set largs [convert_args $method $args]
	set omethod [convert_method $method]

	if { [is_record_based $method] == 1 || [is_rbtree $method] == 1 } {
		puts "Recd003 skipping for method $method"
		return
	}
	puts "Recd003: $method duplicate recovery tests ($envargs)"

	berkdb srand $rand_init

	env_cleanup $testdir
	# See comment in recd001.tcl for why there are two database files...
	set testfile recd003.db
	set testfile2 recd003-2.db
	set eflags "-create -txn -home $testdir $envargs"

	puts "\tRecd003.a: creating environment"
	set env_cmd "berkdb_env $eflags"
	set dbenv [eval $env_cmd]
	error_check_bad dbenv $dbenv NULL

	# Create the databases.
	set oflags \
	    "-create $largs -mode 0644 $omethod -dup -env $dbenv $testfile"
	set db [eval {berkdb_open} $oflags]
	error_check_bad db_open $db NULL
	error_check_good db_open [is_substr $db db] 1
	error_check_good db_close [$db close] 0
	set oflags \
	    "-create $largs -mode 0644 $omethod -dup -env $dbenv $testfile2"
	set db [eval {berkdb_open} $oflags]
	error_check_bad db_open $db NULL
	error_check_good db_open [is_substr $db db] 1
	error_check_good db_close [$db close] 0
	reset_env $dbenv

	# These are all the data values that we're going to need to read
	# through the operation table and run the recovery tests.
	set n 10
	set dupn 2000
	set bign 500

	# List of recovery tests: {CMD MSG} pairs
	set dlist {
	{ {populate DB $omethod TXNID $n 1 0}
	    "Recd003.b: add dups"}
	{ {DB del -txn TXNID duplicate_key}
	    "Recd003.c: remove dups all at once"}
	{ {populate DB $omethod TXNID $n 1 0}
	    "Recd003.d: add dups (change state)"}
	{ {unpopulate DB TXNID 0}
	    "Recd003.e: remove dups 1 at a time"}
	{ {populate DB $omethod TXNID $dupn 1 0}
	    "Recd003.f: dup split"}
	{ {DB del -txn TXNID duplicate_key}
	    "Recd003.g: remove dups (change state)"}
	{ {populate DB $omethod TXNID $n 1 1}
	    "Recd003.h: add big dup"}
	{ {DB del -txn TXNID duplicate_key}
	    "Recd003.i: remove big dup all at once"}
	{ {populate DB $omethod TXNID $n 1 1}
	    "Recd003.j: add big dup (change state)"}
	{ {unpopulate DB TXNID 0}
	    "Recd003.k: remove big dup 1 at a time"}
	{ {populate DB $omethod TXNID $bign 1 1}
	    "Recd003.l: split big dup"}
	}

	foreach pair $dlist {
		set cmd [subst [lindex $pair 0]]
		set msg [lindex $pair 1]
		if { $select != 0 } {
			set tag [lindex $msg 0]
			set tail [expr [string length $tag] - 2]
			set tag [string range $tag $tail $tail]
			if { [lsearch $select $tag] == -1 } {
				continue
			}
		}
		op_recover abort $testdir $env_cmd $testfile $cmd $msg $largs
		op_recover commit $testdir $env_cmd $testfile $cmd $msg $largs
		#
		# Note that since prepare-discard ultimately aborts
		# the txn, it must come before prepare-commit.
		#
		op_recover prepare-abort $testdir $env_cmd $testfile2 \
			$cmd $msg $largs
		op_recover prepare-discard $testdir $env_cmd $testfile2 \
			$cmd $msg $largs
		op_recover prepare-commit $testdir $env_cmd $testfile2 \
			$cmd $msg $largs
	}

	puts "\tRecd003.m: Verify db_printlog can read logfile"
	set tmpfile $testdir/printlog.out
	set stat [catch {exec $util_path/db_printlog -h $testdir \
	    > $tmpfile} ret]
	error_check_good db_printlog $stat 0
	fileremove $tmpfile
}

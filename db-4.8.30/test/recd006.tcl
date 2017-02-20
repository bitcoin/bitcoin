# See the file LICENSE for redistribution information.
#
# Copyright (c) 1996-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	recd006
# TEST	Nested transactions.
proc recd006 { method {select 0} args } {
	global kvals
	source ./include.tcl

	set envargs ""
	set zero_idx [lsearch -exact $args "-zero_log"]
	if { $zero_idx != -1 } {
		set args [lreplace $args $zero_idx $zero_idx]
		set envargs "-zero_log"
	}

	set args [convert_args $method $args]
	set omethod [convert_method $method]

	if { [is_record_based $method] == 1 || [is_rbtree $method] == 1 } {
		puts "Recd006 skipping for method $method"
		return
	}
	puts "Recd006: $method nested transactions ($envargs)"

	# Create the database and environment.
	env_cleanup $testdir

	set dbfile recd006.db
	set testfile $testdir/$dbfile

	puts "\tRecd006.a: create database"
	set oflags "-create $args $omethod $testfile"
	set db [eval {berkdb_open} $oflags]
	error_check_good dbopen [is_valid_db $db] TRUE

	# Make sure that we have enough entries to span a couple of
	# different pages.
	set did [open $dict]
	set count 0
	while { [gets $did str] != -1 && $count < 1000 } {
		if { [string compare $omethod "-recno"] == 0 } {
			set key [expr $count + 1]
		} else {
			set key $str
		}

		set ret [$db put -nooverwrite $key $str]
		error_check_good put $ret 0

		incr count
	}
	close $did

	# Variables used below:
	# p1: a pair of keys that are likely to be on the same page.
	# p2: a pair of keys that are likely to be on the same page,
	# but on a page different than those in p1.
	set dbc [$db cursor]
	error_check_good dbc [is_substr $dbc $db] 1

	set ret [$dbc get -first]
	error_check_bad dbc_get:DB_FIRST [llength $ret] 0
	set p1 [lindex [lindex $ret 0] 0]
	set kvals($p1) [lindex [lindex $ret 0] 1]

	set ret [$dbc get -next]
	error_check_bad dbc_get:DB_NEXT [llength $ret] 0
	lappend p1 [lindex [lindex $ret 0] 0]
	set kvals([lindex [lindex $ret 0] 0]) [lindex [lindex $ret 0] 1]

	set ret [$dbc get -last]
	error_check_bad dbc_get:DB_LAST [llength $ret] 0
	set p2 [lindex [lindex $ret 0] 0]
	set kvals($p2) [lindex [lindex $ret 0] 1]

	set ret [$dbc get -prev]
	error_check_bad dbc_get:DB_PREV [llength $ret] 0
	lappend p2 [lindex [lindex $ret 0] 0]
	set kvals([lindex [lindex $ret 0] 0]) [lindex [lindex $ret 0] 1]

	error_check_good dbc_close [$dbc close] 0
	error_check_good db_close [$db close] 0

	# Now create the full transaction environment.
	set eflags "-create -txn -home $testdir"

	puts "\tRecd006.b: creating environment"
	set env_cmd "berkdb_env $eflags"
	set dbenv [eval $env_cmd]
	error_check_bad dbenv $dbenv NULL

	# Reset the environment.
	reset_env $dbenv

	set p1 [list $p1]
	set p2 [list $p2]

	# List of recovery tests: {CMD MSG} pairs
	set rlist {
	{ {nesttest DB TXNID ENV 1 $p1 $p2 commit commit}
		"Recd006.c: children (commit commit)"}
	{ {nesttest DB TXNID ENV 0 $p1 $p2 commit commit}
		"Recd006.d: children (commit commit)"}
	{ {nesttest DB TXNID ENV 1 $p1 $p2 commit abort}
		"Recd006.e: children (commit abort)"}
	{ {nesttest DB TXNID ENV 0 $p1 $p2 commit abort}
		"Recd006.f: children (commit abort)"}
	{ {nesttest DB TXNID ENV 1 $p1 $p2 abort abort}
		"Recd006.g: children (abort abort)"}
	{ {nesttest DB TXNID ENV 0 $p1 $p2 abort abort}
		"Recd006.h: children (abort abort)"}
	{ {nesttest DB TXNID ENV 1 $p1 $p2 abort commit}
		"Recd006.i: children (abort commit)"}
	{ {nesttest DB TXNID ENV 0 $p1 $p2 abort commit}
		"Recd006.j: children (abort commit)"}
	}

	foreach pair $rlist {
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
		op_recover abort $testdir $env_cmd $dbfile $cmd $msg $args
		op_recover commit $testdir $env_cmd $dbfile $cmd $msg $args
	}

	puts "\tRecd006.k: Verify db_printlog can read logfile"
	set tmpfile $testdir/printlog.out
	set stat [catch {exec $util_path/db_printlog -h $testdir \
	    > $tmpfile} ret]
	error_check_good db_printlog $stat 0
	fileremove $tmpfile
}

# Do the nested transaction test.
# We want to make sure that children inherit properly from their
# parents and that locks are properly handed back to parents
# and that the right thing happens on commit/abort.
# In particular:
#	Write lock on parent, properly acquired by child.
#	Committed operation on child gives lock to parent so that
#		other child can also get the lock.
#	Aborted op by child releases lock so other child can get it.
#	Correct database state if child commits
#	Correct database state if child aborts
proc nesttest { db parent env do p1 p2 child1 child2} {
	global kvals
	source ./include.tcl

	if { $do == 1 } {
		set func toupper
	} else {
		set func tolower
	}

	# Do an RMW on the parent to get a write lock.
	set p10 [lindex $p1 0]
	set p11 [lindex $p1 1]
	set p20 [lindex $p2 0]
	set p21 [lindex $p2 1]

	set ret [$db get -rmw -txn $parent $p10]
	set res $ret
	set Dret [lindex [lindex $ret 0] 1]
	if { [string compare $Dret $kvals($p10)] == 0 ||
	    [string compare $Dret [string toupper $kvals($p10)]] == 0 } {
		set val 0
	} else {
		set val $Dret
	}
	error_check_good get_parent_RMW $val 0

	# OK, do child 1
	set kid1 [$env txn -parent $parent]
	error_check_good kid1 [is_valid_txn $kid1 $env] TRUE

	# Reading write-locked parent object should be OK
	#puts "\tRead write-locked parent object for kid1."
	set ret [$db get -txn $kid1 $p10]
	error_check_good kid1_get10 $ret $res

	# Now update this child
	set data [lindex [lindex [string $func $ret] 0] 1]
	set ret [$db put -txn $kid1 $p10 $data]
	error_check_good kid1_put10 $ret 0

	#puts "\tKid1 successful put."

	# Now start child2
	#puts "\tBegin txn for kid2."
	set kid2 [$env txn -parent $parent]
	error_check_good kid2 [is_valid_txn $kid2 $env] TRUE

	# Getting anything in the p1 set should deadlock, so let's
	# work on the p2 set.
	set data [string $func $kvals($p20)]
	#puts "\tPut data for kid2."
	set ret [$db put -txn $kid2 $p20 $data]
	error_check_good kid2_put20 $ret 0

	#puts "\tKid2 data put successful."

	# Now let's do the right thing to kid1
	puts -nonewline "\tKid1 $child1..."
	if { [string compare $child1 "commit"] == 0 } {
		error_check_good kid1_commit [$kid1 commit] 0
	} else {
		error_check_good kid1_abort [$kid1 abort] 0
	}
	puts "complete"

	# In either case, child2 should now be able to get the
	# lock, either because it is inherited by the parent
	# (commit) or because it was released (abort).
	set data [string $func $kvals($p11)]
	set ret [$db put -txn $kid2 $p11 $data]
	error_check_good kid2_put11 $ret 0

	# Now let's do the right thing to kid2
	puts -nonewline "\tKid2 $child2..."
	if { [string compare $child2 "commit"] == 0 } {
		error_check_good kid2_commit [$kid2 commit] 0
	} else {
		error_check_good kid2_abort [$kid2 abort] 0
	}
	puts "complete"

	# Now, let parent check that the right things happened.
	# First get all four values
	set p10_check [lindex [lindex [$db get -txn $parent $p10] 0] 0]
	set p11_check [lindex [lindex [$db get -txn $parent $p11] 0] 0]
	set p20_check [lindex [lindex [$db get -txn $parent $p20] 0] 0]
	set p21_check [lindex [lindex [$db get -txn $parent $p21] 0] 0]

	if { [string compare $child1 "commit"] == 0 } {
		error_check_good parent_kid1 $p10_check \
		    [string tolower [string $func $kvals($p10)]]
	} else {
		error_check_good \
		    parent_kid1 $p10_check [string tolower $kvals($p10)]
	}
	if { [string compare $child2 "commit"] == 0 } {
		error_check_good parent_kid2 $p11_check \
		    [string tolower [string $func $kvals($p11)]]
		error_check_good parent_kid2 $p20_check \
		    [string tolower [string $func $kvals($p20)]]
	} else {
		error_check_good parent_kid2 $p11_check $kvals($p11)
		error_check_good parent_kid2 $p20_check $kvals($p20)
	}

	# Now do a write on the parent for 21 whose lock it should
	# either have or should be available.
	set ret [$db put -txn $parent $p21 [string $func $kvals($p21)]]
	error_check_good parent_put21 $ret 0

	return 0
}

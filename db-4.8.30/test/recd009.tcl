# See the file LICENSE for redistribution information.
#
# Copyright (c) 1999-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	recd009
# TEST	Verify record numbering across split/reverse splits and recovery.
proc recd009 { method {select 0} args} {
	global fixed_len
	source ./include.tcl

	if { [is_rbtree $method] != 1 && [is_rrecno $method] != 1} {
		puts "Recd009 skipping for method $method."
		return
	}

	set opts [convert_args $method $args]
	set method [convert_method $method]

	puts "\tRecd009: Test record numbers across splits and recovery"

	set testfile recd009.db
	env_cleanup $testdir
	set mkeys 1000
	set nkeys 5
	set data "data"

	puts "\tRecd009.a: Create $method environment and database."
	set flags "-create -txn -home $testdir"

	set env_cmd "berkdb_env $flags"
	set dbenv [eval $env_cmd]
	error_check_good dbenv [is_valid_env $dbenv] TRUE

	set oflags "-env $dbenv -pagesize 8192 -create -mode 0644 $opts $method"
	set db [eval {berkdb_open} $oflags $testfile]
	error_check_good dbopen [is_valid_db $db] TRUE

	# Fill page with small key/data pairs.  Keep at leaf.
	puts "\tRecd009.b: Fill page with $nkeys small key/data pairs."
	for { set i 1 } { $i <= $nkeys } { incr i } {
		if { [is_recno $method] == 1 } {
			set key $i
		} else {
			set key key000$i
		}
		set ret [$db put $key $data$i]
		error_check_good dbput $ret 0
	}
	error_check_good db_close [$db close] 0
	error_check_good env_close [$dbenv close] 0

	set newnkeys [expr $nkeys + 1]
	# List of recovery tests: {CMD MSG} pairs.
	set rlist {
	{ {recd009_split DB TXNID 1 $method $newnkeys $mkeys}
	    "Recd009.c: split"}
	{ {recd009_split DB TXNID 0 $method $newnkeys $mkeys}
	    "Recd009.d: reverse split"}
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
		set reverse [string first "reverse" $msg]
		if { $reverse == -1 } {
			set abortkeys $nkeys
			set commitkeys $mkeys
			set abortpg 0
			set commitpg 1
		} else {
			set abortkeys $mkeys
			set commitkeys $nkeys
			set abortpg 1
			set commitpg 0
		}
		op_recover abort $testdir $env_cmd $testfile $cmd $msg $args
		recd009_recnocheck $testdir $testfile $opts $abortkeys $abortpg
		op_recover commit $testdir $env_cmd $testfile $cmd $msg $args
		recd009_recnocheck $testdir $testfile $opts \
		    $commitkeys $commitpg
	}
	puts "\tRecd009.e: Verify db_printlog can read logfile"
	set tmpfile $testdir/printlog.out
	set stat [catch {exec $util_path/db_printlog -h $testdir \
	    > $tmpfile} ret]
	error_check_good db_printlog $stat 0
	fileremove $tmpfile
}

#
# This procedure verifies that the database has only numkeys number
# of keys and that they are in order.
#
proc recd009_recnocheck { tdir testfile opts numkeys numpg} {
	source ./include.tcl

	set db [eval {berkdb_open} $opts $tdir/$testfile]
	error_check_good dbopen [is_valid_db $db] TRUE

	puts "\tRecd009_recnocheck: Verify page count of $numpg on split."
	set stat [$db stat]
	error_check_bad stat:check-split [is_substr $stat \
		"{{Internal pages} 0}"] $numpg

	set type [$db get_type]
	set dbc [$db cursor]
	error_check_good dbcursor [is_valid_cursor $dbc $db] TRUE
	set i 1
	puts "\tRecd009_recnocheck: Checking $numkeys record numbers."
	for {set d [$dbc get -first]} { [llength $d] != 0 } {
	    set d [$dbc get -next]} {
		if { [is_btree $type] } {
			set thisi [$dbc get -get_recno]
		} else {
			set thisi [lindex [lindex $d 0] 0]
		}
		error_check_good recno_check $i $thisi
		error_check_good record_count [expr $i <= $numkeys] 1
		incr i
	}
	error_check_good curs_close [$dbc close] 0
	error_check_good db_close [$db close] 0
}

proc recd009_split { db txn split method nkeys mkeys } {
	global errorCode
	source ./include.tcl

	set data "data"

	set isrecno [is_recno $method]
	# if mkeys is above 1000, need to adjust below for lexical order
	if { $split == 1 } {
		puts "\tRecd009_split: Add $mkeys pairs to force split."
		for {set i $nkeys} { $i <= $mkeys } { incr i } {
			if { $isrecno == 1 } {
				set key $i
			} else {
				if { $i >= 100 } {
					set key key0$i
				} elseif { $i >= 10 } {
					set key key00$i
				} else {
					set key key000$i
				}
			}
			set ret [$db put -txn $txn $key $data$i]
			error_check_good dbput:more $ret 0
		}
	} else {
		puts "\tRecd009_split: Delete added keys to force reverse split."
		# Since rrecno renumbers, we delete downward.
		for {set i $mkeys} { $i >= $nkeys } { set i [expr $i - 1] } {
			if { $isrecno == 1 } {
				set key $i
			} else {
				if { $i >= 100 } {
					set key key0$i
				} elseif { $i >= 10 } {
					set key key00$i
				} else {
					set key key000$i
				}
			}
			error_check_good db_del:$i [$db del -txn $txn $key] 0
		}
	}
	return 0
}

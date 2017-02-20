# See the file LICENSE for redistribution information.
#
# Copyright (c) 1999-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	recd010
# TEST	Test stability of btree duplicates across btree off-page dup splits
# TEST	and reverse splits and across recovery.
proc recd010 { method {select 0} args} {
	if { [is_btree $method] != 1 } {
		puts "Recd010 skipping for method $method."
		return
	}

	set pgindex [lsearch -exact $args "-pagesize"]
	if { $pgindex != -1 } {
		puts "Recd010: skipping for specific pagesizes"
		return
	}
	set largs $args
	append largs " -dup "
	recd010_main $method $select $largs
	append largs " -dupsort "
	recd010_main $method $select $largs
}

proc recd010_main { method select largs } {
	global fixed_len
	global kvals
	global kvals_dups
	source ./include.tcl


	set opts [convert_args $method $largs]
	set method [convert_method $method]

	puts "Recd010 ($opts): Test duplicates across splits and recovery"

	set testfile recd010.db
	env_cleanup $testdir
	#
	# Set pagesize small to generate lots of off-page dups
	#
	set page 512
	set mkeys 1000
	set firstkeys 5
	set data "data"
	set key "recd010_key"

	puts "\tRecd010.a: Create environment and database."
	set flags "-create -txn -home $testdir"

	set env_cmd "berkdb_env $flags"
	set dbenv [eval $env_cmd]
	error_check_good dbenv [is_valid_env $dbenv] TRUE

	set oflags "-env $dbenv -create -mode 0644 $opts $method"
	set db [eval {berkdb_open} -pagesize $page $oflags $testfile]
	error_check_good dbopen [is_valid_db $db] TRUE

	# Fill page with small key/data pairs.  Keep at leaf.
	puts "\tRecd010.b: Fill page with $firstkeys small dups."
	for { set i 1 } { $i <= $firstkeys } { incr i } {
		set ret [$db put $key $data$i]
		error_check_good dbput $ret 0
	}
	set kvals 1
	set kvals_dups $firstkeys
	error_check_good db_close [$db close] 0
	error_check_good env_close [$dbenv close] 0

	# List of recovery tests: {CMD MSG} pairs.
	if { $mkeys < 100 } {
		puts "Recd010 mkeys of $mkeys too small"
		return
	}
	set rlist {
	{ {recd010_split DB TXNID 1 2 $mkeys}
	    "Recd010.c: btree split 2 large dups"}
	{ {recd010_split DB TXNID 0 2 $mkeys}
	    "Recd010.d: btree reverse split 2 large dups"}
	{ {recd010_split DB TXNID 1 10 $mkeys}
	    "Recd010.e: btree split 10 dups"}
	{ {recd010_split DB TXNID 0 10 $mkeys}
	    "Recd010.f: btree reverse split 10 dups"}
	{ {recd010_split DB TXNID 1 100 $mkeys}
	    "Recd010.g: btree split 100 dups"}
	{ {recd010_split DB TXNID 0 100 $mkeys}
	    "Recd010.h: btree reverse split 100 dups"}
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
		op_recover abort $testdir $env_cmd $testfile $cmd $msg $largs
		recd010_check $testdir $testfile $opts abort $reverse $firstkeys
		op_recover commit $testdir $env_cmd $testfile $cmd $msg $largs
		recd010_check $testdir $testfile $opts commit $reverse $firstkeys
	}
	puts "\tRecd010.i: Verify db_printlog can read logfile"
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
proc recd010_check { tdir testfile opts op reverse origdups } {
	global kvals
	global kvals_dups
	source ./include.tcl

	set db [eval {berkdb_open} $opts $tdir/$testfile]
	error_check_good dbopen [is_valid_db $db] TRUE

	set data "data"

	if { $reverse == -1 } {
		puts "\tRecd010_check: Verify split after $op"
	} else {
		puts "\tRecd010_check: Verify reverse split after $op"
	}

	set stat [$db stat]
	if { [expr ([string compare $op "abort"] == 0 && $reverse == -1) || \
		   ([string compare $op "commit"] == 0 && $reverse != -1)]} {
		set numkeys 0
		set allkeys [expr $numkeys + 1]
		set numdups $origdups
		#
		# If we abort the adding of dups, or commit
		# the removal of dups, either way check that
		# we are back at the beginning.  Check that:
		# - We have 0 internal pages.
		# - We have only 1 key (the original we primed the db
		# with at the beginning of the test).
		# - We have only the original number of dups we primed
		# the db with at the beginning of the test.
		#
		error_check_good stat:orig0 [is_substr $stat \
			"{{Internal pages} 0}"] 1
		error_check_good stat:orig1 [is_substr $stat \
			"{{Number of keys} 1}"] 1
		error_check_good stat:orig2 [is_substr $stat \
			"{{Number of records} $origdups}"] 1
	} else {
		set numkeys $kvals
		set allkeys [expr $numkeys + 1]
		set numdups $kvals_dups
		#
		# If we abort the removal of dups, or commit the
		# addition of dups, check that:
		# - We have > 0 internal pages.
		# - We have the number of keys.
		#
		error_check_bad stat:new0 [is_substr $stat \
			"{{Internal pages} 0}"] 1
		error_check_good stat:new1 [is_substr $stat \
			"{{Number of keys} $allkeys}"] 1
	}

	set dbc [$db cursor]
	error_check_good dbcursor [is_valid_cursor $dbc $db] TRUE
	puts "\tRecd010_check: Checking key and duplicate values"
	set key "recd010_key"
	#
	# Check dups are there as they should be.
	#
	for {set ki 0} {$ki < $numkeys} {incr ki} {
		set datacnt 0
		for {set d [$dbc get -set $key$ki]} { [llength $d] != 0 } {
		    set d [$dbc get -nextdup]} {
			set thisdata [lindex [lindex $d 0] 1]
			if { $datacnt < 10 } {
				set pdata $data.$ki.00$datacnt
			} elseif { $datacnt < 100 } {
				set pdata $data.$ki.0$datacnt
			} else {
				set pdata $data.$ki.$datacnt
			}
			error_check_good dup_check $thisdata $pdata
			incr datacnt
		}
		error_check_good dup_count $datacnt $numdups
	}
	#
	# Check that the number of expected keys (allkeys) are
	# all of the ones that exist in the database.
	#
	set dupkeys 0
	set lastkey ""
	for {set d [$dbc get -first]} { [llength $d] != 0 } {
	    set d [$dbc get -next]} {
		set thiskey [lindex [lindex $d 0] 0]
		if { [string compare $lastkey $thiskey] != 0 } {
			incr dupkeys
		}
		set lastkey $thiskey
	}
	error_check_good key_check $allkeys $dupkeys
	error_check_good curs_close [$dbc close] 0
	error_check_good db_close [$db close] 0
}

proc recd010_split { db txn split nkeys mkeys } {
	global errorCode
	global kvals
	global kvals_dups
	source ./include.tcl

	set data "data"
	set key "recd010_key"

	set numdups [expr $mkeys / $nkeys]

	set kvals $nkeys
	set kvals_dups $numdups
	if { $split == 1 } {
		puts \
"\tRecd010_split: Add $nkeys keys, with $numdups duplicates each to force split."
		for {set k 0} { $k < $nkeys } { incr k } {
			for {set i 0} { $i < $numdups } { incr i } {
				if { $i < 10 } {
					set pdata $data.$k.00$i
				} elseif { $i < 100 } {
					set pdata $data.$k.0$i
				} else {
					set pdata $data.$k.$i
				}
				set ret [$db put -txn $txn $key$k $pdata]
				error_check_good dbput:more $ret 0
			}
		}
	} else {
		puts \
"\tRecd010_split: Delete $nkeys keys to force reverse split."
		for {set k 0} { $k < $nkeys } { incr k } {
			error_check_good db_del:$k [$db del -txn $txn $key$k] 0
		}
	}
	return 0
}

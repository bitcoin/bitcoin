# See the file LICENSE for redistribution information.
#
# Copyright (c) 1999-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	sdb006
# TEST	Tests intra-subdb join
# TEST
# TEST	We'll test 2-way, 3-way, and 4-way joins and figure that if those work,
# TEST	everything else does as well.  We'll create test databases called
# TEST	sub1.db, sub2.db, sub3.db, and sub4.db.  The number on the database
# TEST	describes the duplication -- duplicates are of the form 0, N, 2N, 3N,
# TEST	...  where N is the number of the database.  Primary.db is the primary
# TEST	database, and sub0.db is the database that has no matching duplicates.
# TEST	All of these are within a single database.
#
# We should test this on all btrees, all hash, and a combination thereof
proc sdb006 {method {nentries 100} args } {
	source ./include.tcl
	global rand_init

	# NB: these flags are internal only, ok
	set args [convert_args $method $args]
	set omethod [convert_method $method]

	if { [is_record_based $method] == 1 || [is_rbtree $method] } {
		puts "\tSubdb006 skipping for method $method."
		return
	}

	set txnenv 0
	set eindex [lsearch -exact $args "-env"]
	#
	# If we are using an env, then testfile should just be the db name.
	# Otherwise it is the test directory and the name.
	if { $eindex == -1 } {
		set testfile $testdir/subdb006.db
		set env NULL
	} else {
		set testfile subdb006.db
		incr eindex
		set env [lindex $args $eindex]
		set txnenv [is_txnenv $env]
		if { $txnenv == 1 } {
			append args " -auto_commit "
			if { $nentries == 100 } {
				# !!!
				# nentries must be greater than the number
				# of do_join_subdb calls below.
				#
				set nentries 35
			}
		}
		set testdir [get_home $env]
	}
	berkdb srand $rand_init

	set oargs $args
	foreach opt {" -dup" " -dupsort"} {
		append args $opt

		puts "Subdb006: $method ( $args ) Intra-subdb join"
		set txn ""
		#
		# Get a cursor in each subdb and move past the end of each
		# subdb.  Make sure we don't end up in another subdb.
		#
		puts "\tSubdb006.a: Intra-subdb join"

		if { $env != "NULL" } {
			set testdir [get_home $env]
		}
		cleanup $testdir $env

		set psize 8192
		set duplist {0 50 25 16 12}
		set numdb [llength $duplist]
		build_all_subdb $testfile [list $method] $psize \
		    $duplist $nentries $args

		# Build the primary
		puts "Subdb006: Building the primary database $method"
		set oflags "-create -mode 0644 [conv $omethod \
		    [berkdb random_int 1 2]]"
		set db [eval {berkdb_open} $oflags $oargs $testfile primary.db]
		error_check_good dbopen [is_valid_db $db] TRUE
		for { set i 0 } { $i < 1000 } { incr i } {
			if { $txnenv == 1 } {
				set t [$env txn]
				error_check_good txn [is_valid_txn $t $env] TRUE
				set txn "-txn $t"
			}
			set key [format "%04d" $i]
			set ret [eval {$db put} $txn {$key stub}]
			error_check_good "primary put" $ret 0
			if { $txnenv == 1 } {
				error_check_good txn [$t commit] 0
			}
		}
		error_check_good "primary close" [$db close] 0
		set did [open $dict]
		gets $did str
		do_join_subdb $testfile primary.db "1 0" $str $oargs
		gets $did str
		do_join_subdb $testfile primary.db "2 0" $str $oargs
		gets $did str
		do_join_subdb $testfile primary.db "3 0" $str $oargs
		gets $did str
		do_join_subdb $testfile primary.db "4 0" $str $oargs
		gets $did str
		do_join_subdb $testfile primary.db "1" $str $oargs
		gets $did str
		do_join_subdb $testfile primary.db "2" $str $oargs
		gets $did str
		do_join_subdb $testfile primary.db "3" $str $oargs
		gets $did str
		do_join_subdb $testfile primary.db "4" $str $oargs
		gets $did str
		do_join_subdb $testfile primary.db "1 2" $str $oargs
		gets $did str
		do_join_subdb $testfile primary.db "1 2 3" $str $oargs
		gets $did str
		do_join_subdb $testfile primary.db "1 2 3 4" $str $oargs
		gets $did str
		do_join_subdb $testfile primary.db "2 1" $str $oargs
		gets $did str
		do_join_subdb $testfile primary.db "3 2 1" $str $oargs
		gets $did str
		do_join_subdb $testfile primary.db "4 3 2 1" $str $oargs
		gets $did str
		do_join_subdb $testfile primary.db "1 3" $str $oargs
		gets $did str
		do_join_subdb $testfile primary.db "3 1" $str $oargs
		gets $did str
		do_join_subdb $testfile primary.db "1 4" $str $oargs
		gets $did str
		do_join_subdb $testfile primary.db "4 1" $str $oargs
		gets $did str
		do_join_subdb $testfile primary.db "2 3" $str $oargs
		gets $did str
		do_join_subdb $testfile primary.db "3 2" $str $oargs
		gets $did str
		do_join_subdb $testfile primary.db "2 4" $str $oargs
		gets $did str
		do_join_subdb $testfile primary.db "4 2" $str $oargs
		gets $did str
		do_join_subdb $testfile primary.db "3 4" $str $oargs
		gets $did str
		do_join_subdb $testfile primary.db "4 3" $str $oargs
		gets $did str
		do_join_subdb $testfile primary.db "2 3 4" $str $oargs
		gets $did str
		do_join_subdb $testfile primary.db "3 4 1" $str $oargs
		gets $did str
		do_join_subdb $testfile primary.db "4 2 1" $str $oargs
		gets $did str
		do_join_subdb $testfile primary.db "0 2 1" $str $oargs
		gets $did str
		do_join_subdb $testfile primary.db "3 2 0" $str $oargs
		gets $did str
		do_join_subdb $testfile primary.db "4 3 2 1" $str $oargs
		gets $did str
		do_join_subdb $testfile primary.db "4 3 0 1" $str $oargs

		close $did
	}
}

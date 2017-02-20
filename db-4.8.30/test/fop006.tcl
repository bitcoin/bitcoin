# See the file LICENSE for redistribution information.
#
# Copyright (c) 2003-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	fop006
# TEST	Test file system operations in multiple simultaneous
# TEST	transactions.  Start one transaction, do a file operation.
# TEST	Start a second transaction, do a file operation.  Abort
# TEST	or commit txn1, then abort or commit txn2, and check for
# TEST	appropriate outcome.
proc fop006 { method { inmem 0 } args } {
	source ./include.tcl

	# The variable inmem determines whether the test is being
	# run on regular named databases or named in-memory databases.
	if { $inmem == 0 } {
		set tnum "006"
		set string "regular named databases"
		set operator do_op
	} else {
		set tnum "008"
		set string "in-memory named databases"
		set operator do_inmem_op
	}

	if { [is_btree $method] != 1 } {
		puts "Skipping fop$tnum for method $method"
		return
	}

	set args [convert_args $method $args]
	set omethod [convert_method $method]

	env_cleanup $testdir
	puts "\nFop$tnum ($method): Two file system ops,\
	    each in its own transaction, for $string."

	set exists {a b}
	set noexist {foo bar}
	set open {}
	set cases {}
	set ops {open open_create open_excl rename remove truncate}

	# Set up cases where op1 is successful.
	foreach retval { 0 "file exists" "no such file" } {
		foreach end1 {abort commit} {
			foreach op1 $ops {
				foreach op2 $ops {
					append cases " " [create_tests\
					    $op1 $op2 $exists $noexist\
					    $open $retval $end1]
				}
			}
		}
	}

	# Set up evil two-op cases (op1 fails).  Omit open_create
	# and truncate from op1 list -- open_create always succeeds
	# and truncate requires a successful open.
	foreach retval { 0 "file exists" "no such file" } {
		foreach op1 { rename remove open open_excl } {
			foreach op2 $ops {
				append cases " " [create_badtests $op1 $op2 \
					$exists $noexist $open $retval $end1]
			}
		}
	}

	# The structure of each case is:
	# {{op1 {args} result end} {op2 {args} result}}
	# A result of "0" indicates no error is expected.  Otherwise,
	# the result is the expected error message.  The value of "end"
	# indicates whether the transaction will be aborted or committed.
	#
	# Comment this loop out to remove the list of cases.
#	set i 1
#	foreach case $cases {
#		puts "\tFop$tnum.$i: $case"
#		incr i
#	}

	# To run a particular case, add the case in this format and
	# uncomment.
#	set cases {
#		{{open_excl {foo} 0 abort} {rename {b foo} 0}}
#	}

	set testid 0

	# Run all the cases
	foreach case $cases {
		incr testid

		# Extract elements of the case
		set op1 [lindex [lindex $case 0] 0]
		set names1 [lindex [lindex $case 0] 1]
		set res1 [lindex [lindex $case 0] 2]
		set end1 [lindex [lindex $case 0] 3]

		set op2 [lindex [lindex $case 1] 0]
		set names2 [lindex [lindex $case 1] 1]
		set res2 [lindex [lindex $case 1] 2]

		puts "\tFop$tnum.$testid: $op1 ($names1) $res1 $end1;\
		    $op2 ($names2) $res2."

		foreach end2 { abort commit } {
			# Create transactional environment.
			set env [berkdb_env -create -home $testdir -txn]
			error_check_good is_valid_env [is_valid_env $env] TRUE

			# Create databases
			if { $inmem == 0 } {
				set db [eval {berkdb_open -create} \
				    $omethod $args -env $env -auto_commit a]
			} else {
				set db [eval {berkdb_open -create} \
				    $omethod $args -env $env -auto_commit {""} a]
			}
			error_check_good db_open [is_valid_db $db] TRUE
			error_check_good db_put \
			    [$db put 1 [chop_data $method a]] 0
			error_check_good db_close [$db close] 0

			if { $inmem == 0 } {
				set db [eval {berkdb_open -create} \
				    $omethod $args -env $env -auto_commit b]
			} else {
				set db [eval {berkdb_open -create} \
				    $omethod $args -env $env -auto_commit {""} b]
			}
			error_check_good db_open [is_valid_db $db] TRUE
			error_check_good db_put \
			    [$db put 1 [chop_data $method a]] 0
			error_check_good db_close [$db close] 0

			# Start transaction 1 and perform a file op.
			set txn1 [$env txn]
			error_check_good \
			    txn_begin [is_valid_txn $txn1 $env] TRUE
			set result1 [$operator $omethod $op1 $names1 $txn1 $env $args]
			if { $res1 == 0 } {
				error_check_good \
				    op1_should_succeed $result1 $res1
			} else {
				set error [extract_error $result1]
				error_check_good op1_wrong_failure $error $res1
			}

			# Start transaction 2 before ending transaction 1.
			set pid [exec $tclsh_path $test_path/wrap.tcl \
			    fopscript.tcl $testdir/fop$tnum.log \
			    $operator $omethod $op2 $end2 $res2 $names2 &]

			# Sleep a bit to give txn2 a chance to block.
			tclsleep 2

			# End transaction 1 and close any open db handles.
			# Txn2 will now unblock and finish.
			error_check_good txn1_$end1 [$txn1 $end1] 0
			set handles [berkdb handles]
			foreach handle $handles {
				if {[string range $handle 0 1] == "db" } {
					error_check_good \
					    db_close [$handle close] 0
				}
			}
			watch_procs $pid 1 60 1

			# Clean up for next case
			error_check_good env_close [$env close] 0
			catch { [berkdb envremove -home $testdir] } res

			# Check for errors in log file.
			set errstrings [eval findfail $testdir/fop$tnum.log]
			foreach str $errstrings {
				puts "FAIL: error message in log file: $str"
			}
			env_cleanup $testdir
		}
	}
}


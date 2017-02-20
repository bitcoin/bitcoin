# See the file LICENSE for redistribution information.
#
# Copyright (c) 1999-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	sdb012
# TEST	Test subdbs with locking and transactions
# TEST  Tests creating and removing subdbs while handles
# TEST  are open works correctly, and in the face of txns.
#
proc sdb012 { method args } {
	source ./include.tcl

	set args [convert_args $method $args]
	set omethod [convert_method $method]

	if { [is_queue $method] == 1 } {
		puts "Subdb012: skipping for method $method"
		return
	}

	# If we are using an env, then skip this test.  It needs its own.
	set eindex [lsearch -exact $args "-env"]
	if { $eindex != -1 } {
		incr eindex
		set env [lindex $args $eindex]
		puts "Subdb012 skipping for env $env"
		return
	}
	set encargs ""
	set largs [split_encargs $args encargs]

	puts "Subdb012: $method ($largs $encargs) subdb txn/locking tests"

	#
	# sdb012_body takes a txn list containing 4 elements.
	# {txn command for first subdb
	#  txn command for second subdb
	#  txn command for first subdb removal
	#  txn command for second subdb removal}
	#
	# The allowed commands are 'none' 'one', 'auto', 'abort', 'commit'.
	# 'none' is a special case meaning run without a txn.  In the
	# case where all 4 items are 'none', we run in a lock-only env.
	# 'one' is a special case meaning we create the subdbs together
	# in one single transaction.  It is indicated as the value for t1,
	# and the value in t2 indicates if that single txn should be
	# aborted or committed.  It is not used and has no meaning
	# in the removal case.  'auto' means use the -auto_commit flag
	# to the operation, and 'abort' and 'commit' do the obvious.
	# "-auto" is applied only to the creation of the subdbs, since
	# it is done by default on database removes in transactional
	# environments.
	#
	# First test locking w/o txns.  If any in tlist are 'none',
	# all must be none.
	#
	# Now run through the txn-based operations
	set count 0
	set sdb "Subdb012."
	set teststr "abcdefghijklmnopqrstuvwxyz"
	set testlet [split $teststr {}]
	foreach t1 { none one abort auto commit } {
		foreach t2 { none abort auto commit } {
			if { $t1 == "one" } {
				if { $t2 == "none" || $t2 == "auto"} {
					continue
				}
			}
			set tlet [lindex $testlet $count]
			foreach r1 { none abort commit } {
				foreach r2 { none abort commit } {
					set tlist [list $t1 $t2 $r1 $r2]
					set nnone [llength \
					    [lsearch -all $tlist none]]
					if { $nnone != 0 && $nnone != 4 } {
						continue
					}
					sdb012_body $testdir $omethod $largs \
					    $encargs $sdb$tlet $tlist
				}
			}
			incr count
		}
	}

}

proc s012 { method args } {
	source ./include.tcl

	set omethod [convert_method $method]

	set encargs ""
	set largs ""

	puts "Subdb012: $method ($largs $encargs) subdb txn/locking tests"

	set sdb "Subdb012."
	set tlet X
	set tlist $args
	error_check_good tlist [llength $tlist] 4
	sdb012_body $testdir $omethod $largs $encargs $sdb$tlet $tlist
}

#
# This proc checks the tlist values and returns the flags
# that should be used when opening the env.  If we are running
# with no txns, then just -lock, otherwise -txn.
#
proc sdb012_subsys { tlist } {
	set t1 [lindex $tlist 0]
	#
	# If we have no txns, all elements of the list should be none.
	# In that case we only run with locking turned on.
	# Otherwise, we use the full txn subsystems.
	#
	set allnone {none none none none}
	if { $allnone == $tlist } {
		set subsys "-lock"
	} else {
		set subsys "-txn"
	}
	return $subsys
}

#
# This proc parses the tlist and returns a list of 4 items that
# should be used in operations.  I.e. it will begin the txns as
# needed, or return a -auto_commit flag, etc.
#
proc sdb012_tflags { env tlist } {
	set ret ""
	set t1 ""
	foreach t $tlist {
		switch $t {
		one {
			set t1 [$env txn]
			error_check_good txnbegin [is_valid_txn $t1 $env] TRUE
			lappend ret "-txn $t1"
			lappend ret "-txn $t1"
		}
		auto {
			lappend ret "-auto_commit"
		}
		abort -
		commit {
			#
			# If the previous command was a "one", skip over
			# this commit/abort.  Otherwise start a new txn
			# for the removal case.
			#
			if { $t1 == "" } {
				set txn [$env txn]
				error_check_good txnbegin [is_valid_txn $txn \
				    $env] TRUE
				lappend ret "-txn $txn"
			} else {
				set t1 ""
			}
		}
		none {
			lappend ret ""
		}
		default {
			error "Txn command $t not implemented"
		}
		}
	}
	return $ret
}

#
# This proc parses the tlist and returns a list of 4 items that
# should be used in the txn conclusion operations.  I.e. it will
# give "" if using auto_commit (i.e. no final txn op), or a single
# abort/commit if both subdb's are in one txn.
#
proc sdb012_top { tflags tlist } {
	set ret ""
	set t1 ""
	#
	# We know both lists have 4 items.  Iterate over them
	# using multiple value lists so we know which txn goes
	# with each op.
	#
	# The tflags list is needed to extract the txn command
	# out for the operation.  The tlist list is needed to
	# determine what operation we are doing.
	#
	foreach t $tlist tf $tflags {
		switch $t {
		one {
			set t1 [lindex $tf 1]
		}
		auto {
			lappend ret "sdb012_nop"
		}
		abort -
		commit {
			#
			# If the previous command was a "one" (i.e. t1
			# is set), append a correct command and then
			# an empty one.
			#
			if { $t1 == "" } {
				set txn [lindex $tf 1]
				set top "$txn $t"
				lappend ret $top
			} else {
				set top "$t1 $t"
				lappend ret "sdb012_nop"
				lappend ret $top
				set t1 ""
			}
		}
		none {
			lappend ret "sdb012_nop"
		}
		}
	}
	return $ret
}

proc sdb012_nop { } {
	return 0
}

proc sdb012_isabort { tlist item } {
	set i [lindex $tlist $item]
	if { $i == "one" } {
		set i [lindex $tlist [expr $item + 1]]
	}
	if { $i == "abort" } {
		return 1
	} else {
		return 0
	}
}

proc sdb012_body { testdir omethod largs encargs msg tlist } {

	puts "\t$msg: $tlist"
	set testfile subdb012.db
	set subdb1 sub1
	set subdb2 sub2

	set subsys [sdb012_subsys $tlist]
	env_cleanup $testdir
	set env [eval {berkdb_env -create -home} $testdir $subsys $encargs]
	error_check_good dbenv [is_valid_env $env] TRUE
	error_check_good test_lock [$env test abort subdb_lock] 0

	#
	# Convert from our tlist txn commands into real flags we
	# will pass to commands.  Use the multiple values feature
	# of foreach to do this efficiently.
	#
	set tflags [sdb012_tflags $env $tlist]
	foreach {txn1 txn2 rem1 rem2} $tflags {break}
	foreach {top1 top2 rop1 rop2} [sdb012_top $tflags $tlist] {break}

# puts "txn1 $txn1, txn2 $txn2, rem1 $rem1, rem2 $rem2"
# puts "top1 $top1, top2 $top2, rop1 $rop1, rop2 $rop2"
	puts "\t$msg.0: Create sub databases in env with $subsys"
	set s1 [eval {berkdb_open -env $env -create -mode 0644} \
	    $largs $txn1 {$omethod $testfile $subdb1}]
	error_check_good dbopen [is_valid_db $s1] TRUE

	set ret [eval $top1]
	error_check_good t1_end $ret 0

	set s2 [eval {berkdb_open -env $env -create -mode 0644} \
	    $largs $txn2 {$omethod $testfile $subdb2}]
	error_check_good dbopen [is_valid_db $s2] TRUE

	puts "\t$msg.1: Subdbs are open; resolve txns if necessary"
	set ret [eval $top2]
	error_check_good t2_end $ret 0

	set t1_isabort [sdb012_isabort $tlist 0]
	set t2_isabort [sdb012_isabort $tlist 1]
	set r1_isabort [sdb012_isabort $tlist 2]
	set r2_isabort [sdb012_isabort $tlist 3]

# puts "t1_isabort $t1_isabort, t2_isabort $t2_isabort, r1_isabort $r1_isabort, r2_isabort $r2_isabort"

	puts "\t$msg.2: Subdbs are open; verify removal failures"
	# Verify removes of subdbs with open subdb's fail
	#
	# We should fail no matter what.  If we aborted, then the
	# subdb should not exist.  If we didn't abort, we should fail
	# with DB_LOCK_NOTGRANTED.
	#
	# XXX - Do we need -auto_commit for all these failing ones?
	set r [ catch {berkdb dbremove -env $env $testfile $subdb1} result ]
	error_check_bad dbremove1_open $r 0
	if { $t1_isabort } {
		error_check_good dbremove1_open_ab [is_substr \
		    $result "no such file"] 1
	} else {
		error_check_good dbremove1_open [is_substr \
		    $result DB_LOCK_NOTGRANTED] 1
	}

	set r [ catch {berkdb dbremove -env $env $testfile $subdb2} result ]
	error_check_bad dbremove2_open $r 0
	if { $t2_isabort } {
		error_check_good dbremove2_open_ab [is_substr \
		    $result "no such file"] 1
	} else {
		error_check_good dbremove2_open [is_substr \
		    $result DB_LOCK_NOTGRANTED] 1
	}

	# Verify file remove fails
	set r [catch {berkdb dbremove -env $env $testfile} result]
	error_check_bad dbremovef_open $r 0

	#
	# If both aborted, there should be no file??
	#
	if { $t1_isabort && $t2_isabort } {
		error_check_good dbremovef_open_ab [is_substr \
		    $result "no such file"] 1
	} else {
		error_check_good dbremovef_open [is_substr \
		    $result DB_LOCK_NOTGRANTED] 1
	}

	puts "\t$msg.3: Close subdb2; verify removals"
	error_check_good close_s2 [$s2 close] 0
	set r [ catch {eval {berkdb dbremove -env} \
	    $env $rem2 $testfile $subdb2} result ]
	if { $t2_isabort } {
		error_check_bad dbrem2_ab $r 0
		error_check_good dbrem2_ab [is_substr \
		    $result "no such file"] 1
	} else {
		error_check_good dbrem2 $result 0
	}
	# Resolve subdb2 removal txn
	set r [eval $rop2]
	error_check_good rop2 $r 0

	set r [ catch {berkdb dbremove -env $env $testfile $subdb1} result ]
	error_check_bad dbremove1.2_open $r 0
	if { $t1_isabort } {
		error_check_good dbremove1.2_open_ab [is_substr \
		    $result "no such file"] 1
	} else {
		error_check_good dbremove1.2_open [is_substr \
		    $result DB_LOCK_NOTGRANTED] 1
	}

	# There are three cases here:
	# 1. if both t1 and t2 aborted, the file shouldn't exist
	# 2. if only t1 aborted, the file still exists and nothing is open
	# 3. if neither aborted a remove should fail because the first
	#	 subdb is still open
	# In case 2, don't try the remove, because it should succeed
	# and we won't be able to test anything else.
	if { !$t1_isabort || $t2_isabort } {
		set r [catch {berkdb dbremove -env $env $testfile} result]
		if { $t1_isabort && $t2_isabort } {
			error_check_bad dbremovef.2_open $r 0
			error_check_good dbremove.2_open_ab [is_substr \
			    $result "no such file"] 1
		} else {
			error_check_bad dbremovef.2_open $r 0
			error_check_good dbremove.2_open [is_substr \
			    $result DB_LOCK_NOTGRANTED] 1
		}
	}

	puts "\t$msg.4: Close subdb1; verify removals"
	error_check_good close_s1 [$s1 close] 0
	set r [ catch {eval {berkdb dbremove -env} \
	    $env $rem1 $testfile $subdb1} result ]
	if { $t1_isabort } {
		error_check_bad dbremove1_ab $r 0
		error_check_good dbremove1_ab [is_substr \
		    $result "no such file"] 1
	} else {
		error_check_good dbremove1 $result 0
	}
	# Resolve subdb1 removal txn
	set r [eval $rop1]
	error_check_good rop1 $r 0

	# Verify removal of subdb2.  All DB handles are closed now.
	# So we have two scenarios:
	#	1.  The removal of subdb2 above was successful and subdb2
	#	    doesn't exist and we should fail that way.
	#	2.  The removal of subdb2 above was aborted, and this
	#	    removal should succeed.
	#
	set r [ catch {berkdb dbremove -env $env $testfile $subdb2} result ]
	if { $r2_isabort && !$t2_isabort } {
		error_check_good dbremove2.1_ab $result 0
	} else {
		error_check_bad dbremove2.1 $r 0
		error_check_good dbremove2.1 [is_substr \
		    $result "no such file"] 1
	}

	# Verify removal of subdb1.  All DB handles are closed now.
	# So we have two scenarios:
	#	1.  The removal of subdb1 above was successful and subdb1
	#	    doesn't exist and we should fail that way.
	#	2.  The removal of subdb1 above was aborted, and this
	#	    removal should succeed.
	#
	set r [ catch {berkdb dbremove -env $env $testfile $subdb1} result ]
	if { $r1_isabort && !$t1_isabort } {
		error_check_good dbremove1.1 $result 0
	} else {
		error_check_bad dbremove_open $r 0
		error_check_good dbremove.1 [is_substr \
		    $result "no such file"] 1
	}

	puts "\t$msg.5: All closed; remove file"
	set r [catch {berkdb dbremove -env $env $testfile} result]
	if { $t1_isabort && $t2_isabort } {
		error_check_bad dbremove_final_ab $r 0
		error_check_good dbremove_file_abstr [is_substr \
		    $result "no such file"] 1
	} else {
		error_check_good dbremove_final $r 0
	}
	error_check_good envclose [$env close] 0
}

# See the file LICENSE for redistribution information.
#
# Copyright (c) 2009 Oracle.  All rights reserved.
#
# TEST	rep088
# TEST  Replication roll-back preserves checkpoint.
# TEST
# TEST  Create a situation where a client has to roll back its
# TEST  log, discarding some existing transactions, in order to sync
# TEST  with a new master.
# TEST
# TEST  1. When the client still has its entire log file history, all
# TEST     the way back to log file #1, it's OK if the roll-back discards
# TEST     any/all checkpoints.
# TEST  2. When old log files have been archived, if the roll-back would
# TEST     remove all existing checkpoints it must be forbidden.  The log
# TEST     must always have a checkpoint (or all files back through #1).
# TEST     The client must do internal init or return JOIN_FAILURE.
# TEST  3. (the normal case) Old log files archived, and a checkpoint
# TEST     still exists in the portion of the log which will remain after
# TEST     the roll-back: no internal-init/JOIN_FAILURE necessary.
#
# TODO: maybe just reject anything that doesn't comply with my simplified
# rep_test clone, like fixed-length record methods, etc.

proc rep088 { method { niter 20 } { tnum 088 } args } {
	source ./include.tcl

	if { $is_windows9x_test == 1 } {
		puts "Skipping replication test on Win 9x platform."
		return
	}

	# Run for btree only.
	if { $checking_valid_methods } {
		set test_methods { btree }
		return $test_methods
	}
	if { [is_btree $method] == 0 } {
		puts "\tRep$tnum: Skipping for method $method."
		return
	}

	set args [convert_args $method $args]

	puts "Rep$tnum ($method): Replication roll-back preserves checkpoint."
	# Note: expected result = "sync" means the client should be allowed to
	# synchronize normally (to the found sync point), without any need for
	# internal init.
	# 
	# Case #1.
	puts "Rep$tnum: Rollback without checkpoint, with log file 1"
	set archive false
	set ckpt false
	set result sync
	rep088_sub $method $niter $tnum $archive $ckpt $result $args

	# Case #2.(a).
	# 
	puts "Rep$tnum: Forbid rollback over only chkp: join failure"
	set archive true
	set ckpt false
	set result join_failure
	rep088_sub $method $niter $tnum $archive $ckpt $result $args

	# Case #2.(b): essentially the same, but allow the internal init to
	# happen, so that we verify that the subsequent restart with recovery
	# works fine.  NB: this is the obvious failure case prior to bug fix
	# #16732.
	# 
	puts "Rep$tnum: Forbid rollback over only chkp: internal init"
	set archive true
	set ckpt false
	set result internal_init
	rep088_sub $method $niter $tnum $archive $ckpt $result $args

	# Case #3.
	puts "Rep$tnum: Rollback with sufficient extra checkpoints"
	set archive true
	set ckpt true
	set result sync
	rep088_sub $method $niter $tnum $archive $ckpt $result $args
}

proc rep088_sub { method niter tnum archive ckpt result largs } {
	source ./include.tcl
	global testdir
	global util_path
	global rep_verbose
	global verbose_type

	set verbargs ""
	if { $rep_verbose == 1 } {
		set verbargs " -verbose {$verbose_type on} "
	}

	env_cleanup $testdir
	replsetup $testdir/MSGQUEUEDIR

	file mkdir [set dirA $testdir/A]
	file mkdir [set dirB $testdir/B]
	file mkdir [set dirC $testdir/C]

	set pagesize 4096
	append largs " -pagesize $pagesize "
	set log_buf [expr $pagesize * 2]
	set log_max [expr $log_buf * 4]

	puts "\tRep$tnum.a: Create master and two clients"
	repladd 1
	set env_A_cmd "berkdb_env -create -txn $verbargs \
              -log_buffer $log_buf -log_max $log_max \
	    -errpfx SITE_A -errfile /dev/stderr \
	    -home $dirA -rep_transport \[list 1 replsend\]"
	set envs(A) [eval $env_A_cmd -rep_master]

	repladd 2
	set env_B_cmd "berkdb_env -create -txn $verbargs \
              -log_buffer $log_buf -log_max $log_max \
              -errpfx SITE_B -errfile /dev/stderr \
              -home $dirB -rep_transport \[list 2 replsend\]"
	set envs(B) [eval $env_B_cmd -rep_client]

	repladd 3
	set env_C_cmd "berkdb_env -create -txn $verbargs \
              -log_buffer $log_buf -log_max $log_max \
              -errpfx SITE_C -errfile /dev/stderr \
              -home $dirC -rep_transport \[list 3 replsend\]"
	set envs(C) [eval $env_C_cmd -rep_client]

	set envlist "{$envs(A) 1} {$envs(B) 2} {$envs(C) 3}"
	process_msgs $envlist
	$envs(A) test force noarchive_timeout
	
	# Using small log file size, push into the second log file.
	#
	puts "\tRep$tnum.b: Write enough txns to exceed 1 log file"
	while { [lsn_file [next_expected_lsn $envs(C)]] == 1 } {
		eval rep088_reptest $method $envs(A) $niter $largs
		process_msgs $envlist
	}

	# To make sure everything still works in the normal case, put in a
	# checkpoint here before writing the transactions that will have to be
	# rolled back.  Later, when the client sees that it must roll back over
	# (and discard) the later checkpoint, the fact that this checkpoint is
	# here will allow it to proceed.
	#
	if { $ckpt } {
		puts "\tRep$tnum.c: put in an 'extra' checkpoint."
		$envs(A) txn_checkpoint
		process_msgs $envlist
	}

	# Turn off client TBM (the one that will become master later).
	# 
	puts "\tRep$tnum.d: Turn off client B and write more txns"
	$envs(B) close
	set envlist "{$envs(A) 1} {$envs(C) 3}"

	# Fill a bit more log, and then write a checkpoint.
	# 
	eval rep088_reptest $method $envs(A) $niter $largs
	$envs(A) txn_checkpoint
	replclear 2
	process_msgs $envlist
	
	# At the client under test, archive away the first log file.
	#
	if { $archive } {
		puts "\tRep$tnum.e: Archive log at client C"
		exec $util_path/db_archive -d -h $dirC
	}

	# Maybe another cycle of filling and checkpoint.
	# 
	eval rep088_reptest $method $envs(A) $niter $largs
	$envs(A) txn_checkpoint
	replclear 2
	process_msgs $envlist

	# Now turn off the master, and turn on the TBM site as master.  The
	# client under test has to sync with the new master.  Just to make sure
	# I understand what's going on, turn off auto-init.
	# 

	if { $result != "internal_init" } {
		$envs(C) rep_config {noautoinit on}
	}
	puts "\tRep$tnum.f: Switch master to site B, try to sync client C"
	$envs(A) close
	set envs(B) [eval $env_B_cmd -rep_master]
	set envlist "{$envs(B) 2} {$envs(C) 3}"
	replclear 1
	set succeeded [catch { process_msgs $envlist } ret]
	
	switch $result {
		internal_init {
			error_check_good inited $succeeded 0
        
			# Now stop the client, and try restarting it with
			# recovery.
			# 
			$envs(C) close
			set envs(C) [eval $env_C_cmd -rep_client -recover]
		}
		join_failure {
			error_check_bad no_autoinit $succeeded 0
			error_check_good join_fail \
			    [is_substr $ret DB_REP_JOIN_FAILURE] 1
		}
		sync {
			error_check_good sync_ok $succeeded 0
			error_check_good not_outdated \
			    [stat_field $envs(C) rep_stat \
				 "Outdated conditions"] 0
		}
		default {
			error "FAIL: unknown test result option $result"
		}
	}

	$envs(C) close
	$envs(B) close
	replclose $testdir/MSGQUEUEDIR
}

# A simplified clone of proc rep_test, with the crucial distinction that it
# doesn't do any of its own checkpointing.  For this test we need explicit
# control of when checkpoints should happen.  This proc doesn't support
# access methods using record numbers.
proc rep088_reptest { method env niter args } {
	source ./include.tcl

	set omethod [convert_method $method]
	set largs [convert_args $method $args]
	set db [eval berkdb_open_noerr -env $env -auto_commit \
		    -create $omethod $largs test.db]

	set did [open $dict]
	for { set i 0 } { $i < $niter && [gets $did str] >= 0 } { incr i } {
		set key $str
		set str [reverse $str]
		$db put $key $str
	}
	close $did
	$db close
}

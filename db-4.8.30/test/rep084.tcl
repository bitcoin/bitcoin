# See the file LICENSE for redistribution information.
#
# Copyright (c) 2008-2009 Oracle.  All rights reserved.
#
# TEST	rep084
# TEST  Abbreviated internal init for named in-memory databases (NIMDBs).
# TEST
#
proc rep084 { method { niter 200 } { tnum "084" } args } {
	source ./include.tcl

	if { $is_windows9x_test == 1 } {
		puts "Skipping replication test on Win9x platform."
		return
	}

	# As an internal init test, run for btree and queue only.
	# As an in-memory database test, skip queueext. 
	if { $checking_valid_methods } {
		set test_methods {}
		foreach method $valid_methods {
			if { [is_btree $method] == 1 || [is_queue $method] == 1 } {
				if { [is_queueext $method] == 0 } {
					lappend test_methods $method
				}
			}
		}
		return $test_methods
	}
	if { [is_btree $method] != 1 && [is_queue $method] != 1 } {
		puts "Skipping internal init test rep$tnum for method $method."
		return
	}
	if { [is_queueext $method] == 1 } {
		puts "Skipping in-memory db test rep$tnum for method $method."
		return
	}

	set args [convert_args $method $args]

	rep084_sub $method $niter $tnum $args
}

proc rep084_sub { method niter tnum  largs } {
	global testdir
	global util_path
	global rep_verbose
	global verbose_type

	set verbargs ""
	if { $rep_verbose == 1 } {
		set verbargs " -verbose {$verbose_type on} "
	}
	puts "Rep$tnum: ($method) Abbreviated internal init for NIMDBs."
	set omethod [convert_method $method]

	env_cleanup $testdir
	replsetup $testdir/MSGQUEUEDIR

	file mkdir [set dirA $testdir/A]
	file mkdir [set dirB $testdir/B]
	file mkdir [set dirC $testdir/C]

	repladd 1
	set env_A_cmd "berkdb_env_noerr -create -txn $verbargs \
	    -errpfx SITE_A \
	    -home $dirA -rep_transport \[list 1 replsend\]"
	set envs(A) [eval $env_A_cmd -rep_master]

	# Open two clients
	repladd 2
	set env_B_cmd "berkdb_env_noerr -create -txn $verbargs \
	    -errpfx SITE_B \
	    -home $dirB -rep_transport \[list 2 replsend\]"
	set envs(B) [eval $env_B_cmd -rep_client]

	repladd 3
	set env_C_cmd "berkdb_env_noerr -create -txn $verbargs \
	    -errpfx SITE_C \
	    -home $dirC -rep_transport \[list 3 replsend\]"
	set envs(C) [eval $env_C_cmd -rep_client]

	# Bring the clients online by processing the startup messages.
	set envlist "{$envs(A) 1} {$envs(B) 2} {$envs(C) 3}"
	process_msgs $envlist

	# Create some data in each of two databases, one a regular DB, and the
	# other a NIMDB.
	puts "\tRep$tnum.a: insert data."
	set start 0
	eval rep_test $method $envs(A) NULL $niter $start $start 0 $largs
	set db [eval berkdb_open -env $envs(A) -auto_commit $largs \
		    -create $omethod "{}" "mynimdb"]
	eval rep_test $method $envs(A) $db $niter $start $start 0 $largs
	process_msgs $envlist

	$db close
	$envs(B) close
	$envs(C) close

	# Restart the clients with recovery, which causes the NIMDB to
	# disappear.  Before syncing with the master, verify that the NIMDB is
	# gone.  Verify that the NOAUTOINIT setting does not inhibit NIMDB
	# materialization.
	puts "\tRep$tnum.b: restart with recovery; \
check expected database existence."
	set envs(B) [eval $env_B_cmd -rep_client -recover]
	set envs(C) [eval $env_C_cmd -rep_client -recover]
	$envs(C) rep_config {noautoinit on}

	[berkdb_open -env $envs(B) -auto_commit "test.db"] close
	[berkdb_open -env $envs(C) -auto_commit "test.db"] close
	error_check_good "NIMDB doesn't exist after recovery" \
	    [catch {berkdb_open -env $envs(B) -auto_commit "" "mynimdb"}] 1

	puts "\tRep$tnum.c: sync with master, NIMDB reappears."
	set envlist "{$envs(A) 1} {$envs(B) 2} {$envs(C) 3}"
	process_msgs $envlist

	# After syncing with the master, the client should have copies of all
	# databases.
	# 
	[berkdb_open -env $envs(B) -auto_commit "test.db"] close
	[berkdb_open -env $envs(B) -auto_commit "" "mynimdb"] close
	[berkdb_open -env $envs(C) -auto_commit "test.db"] close
	[berkdb_open -env $envs(C) -auto_commit "" "mynimdb"] close

	# Run some more updates into the NIMDB at the master, and replicate them
	# to the client, to make sure the client can apply transactions onto a
	# NIMDB that had disappeared (and is now back).
	# 
	incr start $niter
	set db [berkdb_open -env $envs(A) -auto_commit "" "mynimdb"]
	eval rep_test $method $envs(A) $db $niter $start $start 0 $largs
	process_msgs $envlist
	$db close

	$envs(C) close
	$envs(B) close
	$envs(A) close
	replclose $testdir/MSGQUEUEDIR
}

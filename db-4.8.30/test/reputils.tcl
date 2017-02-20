# See the file LICENSE for redistribution information.
#
# Copyright (c) 2001-2009 Oracle.  All rights reserved.
#
# $Id$
#
# Replication testing utilities

# Environment handle for the env containing the replication "communications
# structure" (really a CDB environment).

# The test environment consists of a queue and a # directory (environment)
# per replication site.  The queue is used to hold messages destined for a
# particular site and the directory will contain the environment for the
# site.  So the environment looks like:
#				$testdir
#			 ___________|______________________________
#			/           |              \               \
#		MSGQUEUEDIR	MASTERDIR	CLIENTDIR.0 ...	CLIENTDIR.N-1
#		| | ... |
#		1 2 .. N+1
#
# The master is site 1 in the MSGQUEUEDIR and clients 1-N map to message
# queues 2 - N+1.
#
# The globals repenv(1-N) contain the environment handles for the sites
# with a given id (i.e., repenv(1) is the master's environment.


# queuedbs is an array of DB handles, one per machine ID/machine ID pair,
# for the databases that contain messages from one machine to another.
# We omit the cases where the "from" and "to" machines are the same.
# Since tcl does not have real two-dimensional arrays, we use this
# naming convention:  queuedbs(1.2) has the handle for the database
# containing messages to machid 1 from machid 2.
#
global queuedbs
global machids
global perm_response_list
set perm_response_list {}
global perm_sent_list
set perm_sent_list {}
global elect_timeout
unset -nocomplain elect_timeout
set elect_timeout(default) 5000000
global electable_pri
set electable_pri 5
set drop 0
global anywhere
set anywhere 0

global rep_verbose
set rep_verbose 0
global verbose_type
set verbose_type "rep"

# To run a replication test with verbose messages, type
# 'run_verbose' and then the usual test command string enclosed
# in double quotes or curly braces.  For example:
# 
# run_verbose "rep001 btree"
# 
# run_verbose {run_repmethod btree test001}
#
# To run a replication test with one of the subsets of verbose 
# messages, use the same syntax with 'run_verbose_elect', 
# 'run_verbose_lease', etc. 

proc run_verbose { commandstring } {
	global verbose_type
	set verbose_type "rep"
	run_verb $commandstring
}

proc run_verbose_elect { commandstring } {
	global verbose_type
	set verbose_type "rep_elect"
	run_verb $commandstring
}

proc run_verbose_lease { commandstring } {
	global verbose_type
	set verbose_type "rep_lease"
	run_verb $commandstring
}

proc run_verbose_misc { commandstring } {
	global verbose_type
	set verbose_type "rep_misc"
	run_verb $commandstring
}

proc run_verbose_msgs { commandstring } {
	global verbose_type
	set verbose_type "rep_msgs"
	run_verb $commandstring
}

proc run_verbose_sync { commandstring } {
	global verbose_type
	set verbose_type "rep_sync"
	run_verb $commandstring
}

proc run_verbose_test { commandstring } {
	global verbose_type
	set verbose_type "rep_test"
	run_verb $commandstring
}

proc run_verbose_repmgr_misc { commandstring } {
	global verbose_type
	set verbose_type "repmgr_misc"
	run_verb $commandstring
}

proc run_verb { commandstring } {
	global rep_verbose
	global verbose_type

	set rep_verbose 1
	if { [catch {
		eval $commandstring
		flush stdout
		flush stderr
	} res] != 0 } {
		global errorInfo

		set rep_verbose 0
		set fnl [string first "\n" $errorInfo]
		set theError [string range $errorInfo 0 [expr $fnl - 1]]
		if {[string first FAIL $errorInfo] == -1} {
			error "FAIL:[timestamp]\
			    run_verbose: $commandstring: $theError"
		} else {
			error $theError;
		}
	}
	set rep_verbose 0
}

# Databases are on-disk by default for replication testing. 
# Some replication tests have been converted to run with databases
# in memory instead. 

global databases_in_memory
set databases_in_memory 0

proc run_inmem_db { test method } {
	run_inmem $test $method 1 0 0 0
}

# Replication files are on-disk by default for replication testing. 
# Some replication tests have been converted to run with rep files
# in memory instead. 

global repfiles_in_memory
set repfiles_in_memory 0

proc run_inmem_rep { test method } {
	run_inmem $test $method 0 0 1 0
}

# Region files are on-disk by default for replication testing. 
# Replication tests can force the region files in-memory by setting 
# the -private flag when opening an env.

global env_private
set env_private 0

proc run_env_private { test method } {
	run_inmem $test $method 0 0 0 1
}

# Logs are on-disk by default for replication testing. 
# Mixed-mode log testing provides a mixture of on-disk and
# in-memory logging, or even all in-memory.  When testing on a
# 1-master/1-client test, we try all four options.  On a test
# with more clients, we still try four options, randomly
# selecting whether the later clients are on-disk or in-memory.
#

global mixed_mode_logging
set mixed_mode_logging 0

proc create_logsets { nsites } {
	global mixed_mode_logging
	global logsets
	global rand_init

	error_check_good set_random_seed [berkdb srand $rand_init] 0
	if { $mixed_mode_logging == 0 || $mixed_mode_logging == 2 } {
		if { $mixed_mode_logging == 0 } {
			set logmode "on-disk"
		} else {
			set logmode "in-memory"
		}
		set loglist {}
		for { set i 0 } { $i < $nsites } { incr i } {
			lappend loglist $logmode
		}
		set logsets [list $loglist]
	}
	if { $mixed_mode_logging == 1 } {
		set set1 {on-disk on-disk}
		set set2 {on-disk in-memory}
		set set3 {in-memory on-disk}
		set set4 {in-memory in-memory}

		# Start with nsites at 2 since we already set up
		# the master and first client.
		for { set i 2 } { $i < $nsites } { incr i } {
			foreach set { set1 set2 set3 set4 } {
				if { [berkdb random_int 0 1] == 0 } {
					lappend $set "on-disk"
				} else {
					lappend $set "in-memory"
				}
			}
		}
		set logsets [list $set1 $set2 $set3 $set4]
	}
	return $logsets
}

proc run_inmem_log { test method } {
	run_inmem $test $method 0 1 0 0
}

# Run_mixedmode_log is a little different from the other run_inmem procs:
# it provides a mixture of in-memory and on-disk logging on the different
# hosts in a replication group.
proc run_mixedmode_log { test method {display 0} {run 1} \
    {outfile stdout} {largs ""} } {
	global mixed_mode_logging
	set mixed_mode_logging 1

	set prefix [string range $test 0 2]
	if { $prefix != "rep" } {
		puts "Skipping mixed-mode log testing for non-rep test."
		set mixed_mode_logging 0
		return
	}

	eval run_method $method $test $display $run $outfile $largs

	# Reset to default values after run.
	set mixed_mode_logging 0
}

# The procs run_inmem_db, run_inmem_log, run_inmem_rep, and run_env_private 
# put databases, logs, rep files, or region files in-memory.  (Setting up 
# an env with the -private flag puts region files in memory.)
# The proc run_inmem allows you to put any or all of these in-memory 
# at the same time. 

proc run_inmem { test method\
     {dbinmem 1} {logsinmem 1} {repinmem 1} {envprivate 1} } {

	set prefix [string range $test 0 2]
	if { $prefix != "rep" } {
		puts "Skipping in-memory testing for non-rep test."
		return
	}
	global databases_in_memory 
	global mixed_mode_logging
	global repfiles_in_memory
	global env_private
	global test_names

	if { $dbinmem } {
		if { [is_substr $test_names(rep_inmem) $test] == 0 } {
                	puts "Test $test does not support in-memory databases."
			puts "Putting databases on-disk."
                	set databases_in_memory 0
		} else {
			set databases_in_memory 1
		}
	}
	if { $logsinmem } {
		set mixed_mode_logging 2
	}
	if { $repinmem } {
		set repfiles_in_memory 1
	}
	if { $envprivate } { 
		set env_private 1
	}

	if { [catch {eval run_method $method $test} res]  } {
		set databases_in_memory 0
		set mixed_mode_logging 0 
		set repfiles_in_memory 0 
		set env_private 0
		puts "FAIL: $res"
	}

	set databases_in_memory 0 
	set mixed_mode_logging 0 
	set repfiles_in_memory 0
	set env_private 0
}

# The proc run_diskless runs run_inmem with its default values.
# It's useful to have this name to remind us of its testing purpose, 
# which is to mimic a diskless host.

proc run_diskless { test method } {
	run_inmem $test $method 1 1 1 1
}

# Open the master and client environments; store these in the global repenv
# Return the master's environment: "-env masterenv"
proc repl_envsetup { envargs largs test {nclients 1} {droppct 0} { oob 0 } } {
	source ./include.tcl
	global clientdir
	global drop drop_msg
	global masterdir
	global repenv
	global rep_verbose
	global verbose_type

	set verbargs ""
	if { $rep_verbose == 1 } {
		set verbargs " -verbose {$verbose_type on}"
	}

	env_cleanup $testdir

	replsetup $testdir/MSGQUEUEDIR

	set masterdir $testdir/MASTERDIR
	file mkdir $masterdir
	if { $droppct != 0 } {
		set drop 1
		set drop_msg [expr 100 / $droppct]
	} else {
		set drop 0
	}

	for { set i 0 } { $i < $nclients } { incr i } {
		set clientdir($i) $testdir/CLIENTDIR.$i
		file mkdir $clientdir($i)
	}

	# Open a master.
	repladd 1
	#
	# Set log smaller than default to force changing files,
	# but big enough so that the tests that use binary files
	# as keys/data can run.  Increase the size of the log region --
	# sdb004 needs this, now that subdatabase names are stored 
	# in the env region.
	#
	set logmax [expr 3 * 1024 * 1024]
	set lockmax 40000
	set logregion 2097152

	set ma_cmd "berkdb_env_noerr -create -log_max $logmax $envargs \
	    -cachesize { 0 4194304 1 } -log_regionmax $logregion \
	    -lock_max_objects $lockmax -lock_max_locks $lockmax \
	    -errpfx $masterdir $verbargs \
	    -home $masterdir -txn nosync -rep_master -rep_transport \
	    \[list 1 replsend\]"
	set masterenv [eval $ma_cmd]
	error_check_good master_env [is_valid_env $masterenv] TRUE
	set repenv(master) $masterenv

	# Open clients
	for { set i 0 } { $i < $nclients } { incr i } {
		set envid [expr $i + 2]
		repladd $envid
                set cl_cmd "berkdb_env_noerr -create $envargs -txn nosync \
		    -cachesize { 0 10000000 0 } -log_regionmax $logregion \
		    -lock_max_objects $lockmax -lock_max_locks $lockmax \
		    -errpfx $clientdir($i) $verbargs \
		    -home $clientdir($i) -rep_client -rep_transport \
		    \[list $envid replsend\]"
                set clientenv [eval $cl_cmd]
		error_check_good client_env [is_valid_env $clientenv] TRUE
		set repenv($i) $clientenv
	}
	set repenv($i) NULL
	append largs " -env $masterenv "

	# Process startup messages
	repl_envprocq $test $nclients $oob

	# Clobber replication's 30-second anti-archive timer, which
	# will have been started by client sync-up internal init, in
	# case the test we're about to run wants to do any log
	# archiving, or database renaming and/or removal.
	$masterenv test force noarchive_timeout

	return $largs
}

# Process all incoming messages.  Iterate until there are no messages left
# in anyone's queue so that we capture all message exchanges. We verify that
# the requested number of clients matches the number of client environments
# we have.  The oob parameter indicates if we should process the queue
# with out-of-order delivery.  The replprocess procedure actually does
# the real work of processing the queue -- this routine simply iterates
# over the various queues and does the initial setup.
proc repl_envprocq { test { nclients 1 } { oob 0 }} {
	global repenv
	global drop

	set masterenv $repenv(master)
	for { set i 0 } { 1 } { incr i } {
		if { $repenv($i) == "NULL"} {
			break
		}
	}
	error_check_good i_nclients $nclients $i

	berkdb debug_check
	puts -nonewline "\t$test: Processing master/$i client queues"
	set rand_skip 0
	if { $oob } {
		puts " out-of-order"
	} else {
		puts " in order"
	}
	set droprestore $drop
	while { 1 } {
		set nproced 0

		if { $oob } {
			set rand_skip [berkdb random_int 2 10]
		}
		incr nproced [replprocessqueue $masterenv 1 $rand_skip]
		for { set i 0 } { $i < $nclients } { incr i } {
			set envid [expr $i + 2]
			if { $oob } {
				set rand_skip [berkdb random_int 2 10]
			}
			set n [replprocessqueue $repenv($i) \
			    $envid $rand_skip]
			incr nproced $n
		}

		if { $nproced == 0 } {
			# Now that we delay requesting records until
			# we've had a few records go by, we should always
			# see that the number of requests is lower than the
			# number of messages that were enqueued.
			for { set i 0 } { $i < $nclients } { incr i } {
				set clientenv $repenv($i)
				set queued [stat_field $clientenv rep_stat \
				   "Total log records queued"]
				error_check_bad queued_stats \
				    $queued -1
				set requested [stat_field $clientenv rep_stat \
				   "Log records requested"]
				error_check_bad requested_stats \
				    $requested -1

				#
				# Set to 100 usecs.  An average ping
				# to localhost should be a few 10s usecs.
				#
				$clientenv rep_request 100 400
			}

			# If we were dropping messages, we might need
			# to flush the log so that we get everything
			# and end up in the right state.
			if { $drop != 0 } {
				set drop 0
				$masterenv rep_flush
				berkdb debug_check
				puts "\t$test: Flushing Master"
			} else {
				break
			}
		}
	}

	# Reset the clients back to the default state in case we
	# have more processing to do.
	for { set i 0 } { $i < $nclients } { incr i } {
		set clientenv $repenv($i)
		$clientenv rep_request 40000 1280000
	}
	set drop $droprestore
}

# Verify that the directories in the master are exactly replicated in
# each of the client environments.
proc repl_envver0 { test method { nclients 1 } } {
	global clientdir
	global masterdir
	global repenv

	# Verify the database in the client dir.
	# First dump the master.
	set t1 $masterdir/t1
	set t2 $masterdir/t2
	set t3 $masterdir/t3
	set omethod [convert_method $method]

	#
	# We are interested in the keys of whatever databases are present
	# in the master environment, so we just call a no-op check function
	# since we have no idea what the contents of this database really is.
	# We just need to walk the master and the clients and make sure they
	# have the same contents.
	#
	set cwd [pwd]
	cd $masterdir
	set stat [catch {glob test*.db} dbs]
	cd $cwd
	if { $stat == 1 } {
		return
	}
	foreach testfile $dbs {
		open_and_dump_file $testfile $repenv(master) $masterdir/t2 \
		    repl_noop dump_file_direction "-first" "-next"

		if { [string compare [convert_method $method] -recno] != 0 } {
			filesort $t2 $t3
			file rename -force $t3 $t2
		}
		for { set i 0 } { $i < $nclients } { incr i } {
	puts "\t$test: Verifying client $i database $testfile contents."
			open_and_dump_file $testfile $repenv($i) \
			    $t1 repl_noop dump_file_direction "-first" "-next"

			if { [string compare $omethod "-recno"] != 0 } {
				filesort $t1 $t3
			} else {
				catch {file copy -force $t1 $t3} ret
			}
			error_check_good diff_files($t2,$t3) [filecmp $t2 $t3] 0
		}
	}
}

# Remove all the elements from the master and verify that these
# deletions properly propagated to the clients.
proc repl_verdel { test method { nclients 1 } } {
	global clientdir
	global masterdir
	global repenv

	# Delete all items in the master.
	set cwd [pwd]
	cd $masterdir
	set stat [catch {glob test*.db} dbs]
	cd $cwd
	if { $stat == 1 } {
		return
	}
	foreach testfile $dbs {
		puts "\t$test: Deleting all items from the master."
		set txn [$repenv(master) txn]
		error_check_good txn_begin [is_valid_txn $txn \
		    $repenv(master)] TRUE
		set db [eval berkdb_open -txn $txn -env $repenv(master) \
		    $testfile]
		error_check_good reopen_master [is_valid_db $db] TRUE
		set dbc [$db cursor -txn $txn]
		error_check_good reopen_master_cursor \
		    [is_valid_cursor $dbc $db] TRUE
		for { set dbt [$dbc get -first] } { [llength $dbt] > 0 } \
		    { set dbt [$dbc get -next] } {
			error_check_good del_item [$dbc del] 0
		}
		error_check_good dbc_close [$dbc close] 0
		error_check_good txn_commit [$txn commit] 0
		error_check_good db_close [$db close] 0

		repl_envprocq $test $nclients

		# Check clients.
		for { set i 0 } { $i < $nclients } { incr i } {
			puts "\t$test: Verifying client database $i is empty."

			set db [eval berkdb_open -env $repenv($i) $testfile]
			error_check_good reopen_client($i) \
			    [is_valid_db $db] TRUE
			set dbc [$db cursor]
			error_check_good reopen_client_cursor($i) \
			    [is_valid_cursor $dbc $db] TRUE

			error_check_good client($i)_empty \
			    [llength [$dbc get -first]] 0

			error_check_good dbc_close [$dbc close] 0
			error_check_good db_close [$db close] 0
		}
	}
}

# Replication "check" function for the dump procs that expect to
# be able to verify the keys and data.
proc repl_noop { k d } {
	return
}

# Close all the master and client environments in a replication test directory.
proc repl_envclose { test envargs } {
	source ./include.tcl
	global clientdir
	global encrypt
	global masterdir
	global repenv
	global drop

	if { [lsearch $envargs "-encrypta*"] !=-1 } {
		set encrypt 1
	}

	# In order to make sure that we have fully-synced and ready-to-verify
	# databases on all the clients, do a checkpoint on the master and
	# process messages in order to flush all the clients.
	set drop 0
	berkdb debug_check
	puts "\t$test: Checkpointing master."
	error_check_good masterenv_ckp [$repenv(master) txn_checkpoint] 0

	# Count clients.
	for { set ncli 0 } { 1 } { incr ncli } {
		if { $repenv($ncli) == "NULL" } {
			break
		}
		$repenv($ncli) rep_request 100 100
	}
	repl_envprocq $test $ncli

	error_check_good masterenv_close [$repenv(master) close] 0
	verify_dir $masterdir "\t$test: " 0 0 1
	for { set i 0 } { $i < $ncli } { incr i } {
		error_check_good client($i)_close [$repenv($i) close] 0
		verify_dir $clientdir($i) "\t$test: " 0 0 1
	}
	replclose $testdir/MSGQUEUEDIR

}

# Replnoop is a dummy function to substitute for replsend
# when replication is off.
proc replnoop { control rec fromid toid flags lsn } {
	return 0
}

proc replclose { queuedir } {
	global queueenv queuedbs machids

	foreach m $machids {
		set db $queuedbs($m)
		error_check_good dbr_close [$db close] 0
	}
	error_check_good qenv_close [$queueenv close] 0
	set machids {}
}

# Create a replication group for testing.
proc replsetup { queuedir } {
	global queueenv queuedbs machids

	file mkdir $queuedir
	set max_locks 20000
	set queueenv [berkdb_env \
	     -create -txn nosync -lock_max_locks $max_locks -home $queuedir]
	error_check_good queueenv [is_valid_env $queueenv] TRUE

	if { [info exists queuedbs] } {
		unset queuedbs
	}
	set machids {}

	return $queueenv
}

# Send function for replication.
proc replsend { control rec fromid toid flags lsn } {
	global queuedbs queueenv machids
	global drop drop_msg
	global perm_sent_list
	global anywhere

	set permflags [lsearch $flags "perm"]
	if { [llength $perm_sent_list] != 0 && $permflags != -1 } {
#		puts "replsend sent perm message, LSN $lsn"
		lappend perm_sent_list $lsn
	}

	#
	# If we are testing with dropped messages, then we drop every
	# $drop_msg time.  If we do that just return 0 and don't do
	# anything.
	#
	if { $drop != 0 } {
		incr drop
		if { $drop == $drop_msg } {
			set drop 1
			return 0
		}
	}
	# XXX
	# -1 is DB_BROADCAST_EID
	if { $toid == -1 } {
		set machlist $machids
	} else {
		if { [info exists queuedbs($toid)] != 1 } {
			error "replsend: machid $toid not found"
		}
		set m NULL
		if { $anywhere != 0 } {
			#
			# If we can send this anywhere, send it to the first
			# id we find that is neither toid or fromid.
			#
			set anyflags [lsearch $flags "any"]
			if { $anyflags != -1 } {
				foreach m $machids {
					if { $m == $fromid || $m == $toid } {
						continue
					}
					set machlist [list $m]
					break
				}
			}
		}
		#
		# If we didn't find a different site, then we must
		# fallback to the toid.
		#
		if { $m == "NULL" } {
			set machlist [list $toid]
		}
	}

	foreach m $machlist {
		# do not broadcast to self.
		if { $m == $fromid } {
			continue
		}

		set db $queuedbs($m)
		set txn [$queueenv txn]
		$db put -txn $txn -append [list $control $rec $fromid]
		error_check_good replsend_commit [$txn commit] 0
	}

	queue_logcheck
	return 0
}

#
# If the message queue log files are getting too numerous, checkpoint
# and archive them.  Some tests are so large (particularly from
# run_repmethod) that they can consume far too much disk space.
proc queue_logcheck { } {
	global queueenv


	set logs [$queueenv log_archive -arch_log]
	set numlogs [llength $logs]
	if { $numlogs > 10 } {
		$queueenv txn_checkpoint
		$queueenv log_archive -arch_remove
	}
}

# Discard all the pending messages for a particular site.
proc replclear { machid } {
	global queuedbs queueenv

	if { [info exists queuedbs($machid)] != 1 } {
		error "FAIL: replclear: machid $machid not found"
	}

	set db $queuedbs($machid)
	set txn [$queueenv txn]
	set dbc [$db cursor -txn $txn]
	for { set dbt [$dbc get -rmw -first] } { [llength $dbt] > 0 } \
	    { set dbt [$dbc get -rmw -next] } {
		error_check_good replclear($machid)_del [$dbc del] 0
	}
	error_check_good replclear($machid)_dbc_close [$dbc close] 0
	error_check_good replclear($machid)_txn_commit [$txn commit] 0
}

# Add a machine to a replication environment.
proc repladd { machid } {
	global queueenv queuedbs machids

	if { [info exists queuedbs($machid)] == 1 } {
		error "FAIL: repladd: machid $machid already exists"
	}

	set queuedbs($machid) [berkdb open -auto_commit \
	    -env $queueenv -create -recno -renumber repqueue$machid.db]
	error_check_good repqueue_create [is_valid_db $queuedbs($machid)] TRUE

	lappend machids $machid
}

# Acquire a handle to work with an existing machine's replication
# queue.  This is for situations where more than one process
# is working with a message queue.  In general, having more than one
# process handle the queue is wrong.  However, in order to test some
# things, we need two processes (since Tcl doesn't support threads).  We
# go to great pain in the test harness to make sure this works, but we
# don't let customers do it.
proc repljoin { machid } {
	global queueenv queuedbs machids

	set queuedbs($machid) [berkdb open -auto_commit \
	    -env $queueenv repqueue$machid.db]
	error_check_good repqueue_create [is_valid_db $queuedbs($machid)] TRUE

	lappend machids $machid
}

# Process a queue of messages, skipping every "skip_interval" entry.
# We traverse the entire queue, but since we skip some messages, we
# may end up leaving things in the queue, which should get picked up
# on a later run.
proc replprocessqueue { dbenv machid { skip_interval 0 } { hold_electp NONE } \
    { dupmasterp NONE } { errp NONE } } {
	global queuedbs queueenv errorCode
	global perm_response_list
	global startup_done

	# hold_electp is a call-by-reference variable which lets our caller
	# know we need to hold an election.
	if { [string compare $hold_electp NONE] != 0 } {
		upvar $hold_electp hold_elect
	}
	set hold_elect 0

	# dupmasterp is a call-by-reference variable which lets our caller
	# know we have a duplicate master.
	if { [string compare $dupmasterp NONE] != 0 } {
		upvar $dupmasterp dupmaster
	}
	set dupmaster 0

	# errp is a call-by-reference variable which lets our caller
	# know we have gotten an error (that they expect).
	if { [string compare $errp NONE] != 0 } {
		upvar $errp errorp
	}
	set errorp 0

	set nproced 0

	set txn [$queueenv txn]

	# If we are running separate processes, the second process has
	# to join an existing message queue.
	if { [info exists queuedbs($machid)] == 0 } {
		repljoin $machid
	}

	set dbc [$queuedbs($machid) cursor -txn $txn]

	error_check_good process_dbc($machid) \
	    [is_valid_cursor $dbc $queuedbs($machid)] TRUE

	for { set dbt [$dbc get -first] } \
	    { [llength $dbt] != 0 } \
	    { } {
		set data [lindex [lindex $dbt 0] 1]
		set recno [lindex [lindex $dbt 0] 0]

		# If skip_interval is nonzero, we want to process messages
		# out of order.  We do this in a simple but slimy way--
		# continue walking with the cursor without processing the
		# message or deleting it from the queue, but do increment
		# "nproced".  The way this proc is normally used, the
		# precise value of nproced doesn't matter--we just don't
		# assume the queues are empty if it's nonzero.  Thus,
		# if we contrive to make sure it's nonzero, we'll always
		# come back to records we've skipped on a later call
		# to replprocessqueue.  (If there really are no records,
		# we'll never get here.)
		#
		# Skip every skip_interval'th record (and use a remainder other
		# than zero so that we're guaranteed to really process at least
		# one record on every call).
		if { $skip_interval != 0 } {
			if { $nproced % $skip_interval == 1 } {
				incr nproced
				set dbt [$dbc get -next]
				continue
			}
		}

		# We need to remove the current message from the queue,
		# because we're about to end the transaction and someone
		# else processing messages might come in and reprocess this
		# message which would be bad.
		error_check_good queue_remove [$dbc del] 0

		# We have to play an ugly cursor game here:  we currently
		# hold a lock on the page of messages, but rep_process_message
		# might need to lock the page with a different cursor in
		# order to send a response.  So save the next recno, close
		# the cursor, and then reopen and reset the cursor.
		# If someone else is processing this queue, our entry might
		# have gone away, and we need to be able to handle that.

		error_check_good dbc_process_close [$dbc close] 0
		error_check_good txn_commit [$txn commit] 0

		set ret [catch {$dbenv rep_process_message \
		    [lindex $data 2] [lindex $data 0] [lindex $data 1]} res]

		# Save all ISPERM and NOTPERM responses so we can compare their
		# LSNs to the LSN in the log.  The variable perm_response_list
		# holds the entire response so we can extract responses and
		# LSNs as needed.
		#
		if { [llength $perm_response_list] != 0 && \
		    ([is_substr $res ISPERM] || [is_substr $res NOTPERM]) } {
			lappend perm_response_list $res
		}

		if { $ret != 0 } {
			if { [string compare $errp NONE] != 0 } {
				set errorp "$dbenv $machid $res"
			} else {
				error "FAIL:[timestamp]\
				    rep_process_message returned $res"
			}
		}

		incr nproced

		# Now, re-establish the cursor position.  We fetch the
		# current record number.  If there is something there,
		# that is the record for the next iteration.  If there
		# is nothing there, then we've consumed the last item
		# in the queue.

		set txn [$queueenv txn]
		set dbc [$queuedbs($machid) cursor -txn $txn]
		set dbt [$dbc get -set_range $recno]

		if { $ret == 0 } {
			set rettype [lindex $res 0]
			set retval [lindex $res 1]
			#
			# Do nothing for 0 and NEWSITE
			#
			if { [is_substr $rettype STARTUPDONE] } {
				set startup_done 1
			}
			if { [is_substr $rettype HOLDELECTION] } {
				set hold_elect 1
			}
			if { [is_substr $rettype DUPMASTER] } {
				set dupmaster "1 $dbenv $machid"
			}
			if { [is_substr $rettype NOTPERM] || \
			    [is_substr $rettype ISPERM] } {
				set lsnfile [lindex $retval 0]
				set lsnoff [lindex $retval 1]
			}
		}

		if { $errorp != 0 } {
			# Break also on an error, caller wants to handle it.
			break
		}
		if { $hold_elect == 1 } {
			# Break also on a HOLDELECTION, for the same reason.
			break
		}
		if { $dupmaster == 1 } {
			# Break also on a DUPMASTER, for the same reason.
			break
		}

	}

	error_check_good dbc_close [$dbc close] 0
	error_check_good txn_commit [$txn commit] 0

	# Return the number of messages processed.
	return $nproced
}


set run_repl_flag "-run_repl"

proc extract_repl_args { args } {
	global run_repl_flag

	for { set arg [lindex $args [set i 0]] } \
	    { [string length $arg] > 0 } \
	    { set arg [lindex $args [incr i]] } {
		if { [string compare $arg $run_repl_flag] == 0 } {
			return [lindex $args [expr $i + 1]]
		}
	}
	return ""
}

proc delete_repl_args { args } {
	global run_repl_flag

	set ret {}

	for { set arg [lindex $args [set i 0]] } \
	    { [string length $arg] > 0 } \
	    { set arg [lindex $args [incr i]] } {
		if { [string compare $arg $run_repl_flag] != 0 } {
			lappend ret $arg
		} else {
			incr i
		}
	}
	return $ret
}

global elect_serial
global elections_in_progress
set elect_serial 0

# Start an election in a sub-process.
proc start_election \
    { pfx qdir envstring nsites nvotes pri timeout {err "none"} {crash 0}} {
	source ./include.tcl
	global elect_serial elections_in_progress machids
	global rep_verbose

	set filelist {}
	set ret [catch {glob $testdir/ELECTION*.$elect_serial} result]
	if { $ret == 0 } {
		set filelist [concat $filelist $result]
	}
	foreach f $filelist {
		fileremove -f $f
	}

	set oid [open $testdir/ELECTION_SOURCE.$elect_serial w]

	puts $oid "source $test_path/test.tcl"
	puts $oid "set elected_event 0"
	puts $oid "set elected_env \"NONE\""
	puts $oid "set is_repchild 1"
	puts $oid "replsetup $qdir"
	foreach i $machids { puts $oid "repladd $i" }
	puts $oid "set env_cmd \{$envstring\}"
	if { $rep_verbose == 1 } {
		puts $oid "set dbenv \[eval \$env_cmd -errfile \
		    /dev/stdout -errpfx $pfx \]"
	} else {
		puts $oid "set dbenv \[eval \$env_cmd -errfile \
		    $testdir/ELECTION_ERRFILE.$elect_serial -errpfx $pfx \]"
	}
	puts $oid "\$dbenv test abort $err"
	puts $oid "set res \[catch \{\$dbenv rep_elect $nsites \
	    $nvotes $pri $timeout\} ret\]"
	puts $oid "set r \[open \$testdir/ELECTION_RESULT.$elect_serial w\]"
	puts $oid "if \{\$res == 0 \} \{"
	puts $oid "puts \$r \"SUCCESS \$ret\""
	puts $oid "\} else \{"
	puts $oid "puts \$r \"ERROR \$ret\""
	puts $oid "\}"
	#
	# This loop calls rep_elect a second time with the error cleared.
	# We don't want to do that if we are simulating a crash.
	if { $err != "none" && $crash != 1 } {
		puts $oid "\$dbenv test abort none"
		puts $oid "set res \[catch \{\$dbenv rep_elect $nsites \
		    $nvotes $pri $timeout\} ret\]"
		puts $oid "if \{\$res == 0 \} \{"
		puts $oid "puts \$r \"SUCCESS \$ret\""
		puts $oid "\} else \{"
		puts $oid "puts \$r \"ERROR \$ret\""
		puts $oid "\}"
	}

	puts $oid "if \{ \$elected_event == 1 \} \{"
	puts $oid "puts \$r \"ELECTED \$elected_env\""
	puts $oid "\}"

	puts $oid "close \$r"
	close $oid

	set t [open "|$tclsh_path >& $testdir/ELECTION_OUTPUT.$elect_serial" w]
	if { $rep_verbose } {
		set t [open "|$tclsh_path" w]
	}
	puts $t "source ./include.tcl"
	puts $t "source $testdir/ELECTION_SOURCE.$elect_serial"
	flush $t

	set elections_in_progress($elect_serial) $t
	return $elect_serial
}

#
# If we are doing elections during upgrade testing, set
# upgrade to 1.  Doing that sets the priority to the
# test priority in rep_elect, which will simulate a
# 0-priority but electable site.
#
proc setpriority { priority nclients winner {start 0} {upgrade 0} } {
	global electable_pri
	upvar $priority pri

	for { set i $start } { $i < [expr $nclients + $start] } { incr i } {
		if { $i == $winner } {
			set pri($i) 100
		} else {
			if { $upgrade } {
				set pri($i) $electable_pri
			} else {
				set pri($i) 10
			}
		}
	}
}

# run_election has the following arguments:
# Arrays:
#	ecmd 		Array of the commands for setting up each client env.
#	cenv		Array of the handles to each client env.
#	errcmd		Array of where errors should be forced.
#	priority	Array of the priorities of each client env.
#	crash		If an error is forced, should we crash or recover?
# The upvar command takes care of making these arrays available to
# the procedure.
#
# Ordinary variables:
# 	qdir		Directory where the message queue is located.
#	msg		Message prefixed to the output.
#	elector		This client calls the first election.
#	nsites		Number of sites in the replication group.
#	nvotes		Number of votes required to win the election.
# 	nclients	Number of clients participating in the election.
#	win		The expected winner of the election.
#	reopen		Should the new master (i.e. winner) be closed
#			and reopened as a client?
#	dbname		Name of the underlying database.  The caller
#			should send in "NULL" if the database has not
# 			yet been created.
# 	ignore		Should the winner ignore its own election?
#			If ignore is 1, the winner is not made master.
#	timeout_ok	We expect that this election will not succeed
# 			in electing a new master (perhaps because there 
#			already is a master). 

proc run_election { ecmd celist errcmd priority crsh\
    qdir msg elector nsites nvotes nclients win reopen\
    dbname {ignore 0} {timeout_ok 0} } {

	global elect_timeout elect_serial
	global is_hp_test
	global is_windows_test
	global rand_init
	upvar $ecmd env_cmd
	upvar $celist cenvlist
	upvar $errcmd err_cmd
	upvar $priority pri
	upvar $crsh crash

	set elect_timeout(default) 15000000
	# Windows and HP-UX require a longer timeout.
	if { $is_windows_test == 1 || $is_hp_test == 1 } {
		set elect_timeout(default) [expr $elect_timeout(default) * 2]
	}

	set long_timeout $elect_timeout(default)
	#
	# Initialize tries based on the default timeout.
	# We use tries to loop looking for messages because
	# as sites are sleeping waiting for their timeout
	# to expire we need to keep checking for messages.
	#
	set tries [expr [expr $long_timeout * 4] / 1000000]
	#
	# Retry indicates whether the test should retry the election
	# if it gets a timeout.  This is primarily used for the
	# varied timeout election test because we expect short timeouts
	# to timeout when interacting with long timeouts and the
	# short timeout sites need to call elections again.
	#
	set retry 0
	foreach pair $cenvlist {
		set id [lindex $pair 1]
		set i [expr $id - 2]
		set elect_pipe($i) INVALID
		#
		# Array get should return us a list of 1 element:
		# { {$i timeout_value} }
		# If that doesn't exist, use the default.
		#
		set this_timeout [array get elect_timeout $i]
		if { [llength $this_timeout] } {
			set e_timeout($i) [lindex $this_timeout 1]
			#
			# Set number of tries based on the biggest
			# timeout we see in this group if using
			# varied timeouts.
			#
			set retry 1
			if { $e_timeout($i) > $long_timeout } {
				set long_timeout $e_timeout($i)
				set tries [expr $long_timeout / 1000000]
			}
		} else {
			set e_timeout($i) $elect_timeout(default)
		}
		replclear $id
	}

	#
	# XXX
	# We need to somehow check for the warning if nvotes is not
	# a majority.  Problem is that warning will go into the child
	# process' output.  Furthermore, we need a mechanism that can
	# handle both sending the output to a file and sending it to
	# /dev/stderr when debugging without failing the
	# error_check_good check.
	#
	puts "\t\t$msg.1: Election with nsites=$nsites,\
	    nvotes=$nvotes, nclients=$nclients"
	puts "\t\t$msg.2: First elector is $elector,\
	    expected winner is $win (eid [expr $win + 2])"
	incr elect_serial
	set pfx "CHILD$elector.$elect_serial"
	set elect_pipe($elector) [start_election \
	    $pfx $qdir $env_cmd($elector) $nsites $nvotes $pri($elector) \
	    $e_timeout($elector) $err_cmd($elector) $crash($elector)]
	tclsleep 2

	set got_newmaster 0
	set max_retry $tries

	# If we're simulating a crash, skip the while loop and
	# just give the initial election a chance to complete.
	set crashing 0
	for { set i 0 } { $i < $nclients } { incr i } {
		if { $crash($i) == 1 } {
			set crashing 1
		}
	}

	global elected_event
	global elected_env
	set elected_event 0
	set c_elected_event 0
	set elected_env "NONE"

	set orig_tries $tries
	if { $crashing == 1 } {
		tclsleep 10
	} else {
		set retry_cnt 0
		while { 1 } {
			set nproced 0
			set he 0
			set winning_envid -1
			set c_winning_envid -1

			foreach pair $cenvlist {
				set he 0
				set unavail 0
				set envid [lindex $pair 1]
				set i [expr $envid - 2]
				set clientenv($i) [lindex $pair 0]

				# If the "elected" event is received by the
				# child process, the env set up in that child
				# is the elected env. 
				set child_done [check_election $elect_pipe($i)\
				    unavail c_elected_event c_elected_env]
				if { $c_elected_event != 0 } {
					set elected_event 1
					set c_winning_envid $envid
					set c_elected_event 0
				}
		
				incr nproced [replprocessqueue \
				    $clientenv($i) $envid 0 he]
# puts "Tries $tries:\
# Processed queue for client $i, $nproced msgs he $he unavail $unavail"

				# Check for completed election.  If it's the
				# first time we've noticed it, deal with it.
				if { $elected_event == 1 && \
				    $got_newmaster == 0 } {
					set got_newmaster 1

					# Find env id of winner.
					if { $c_winning_envid != -1 } {
						set winning_envid \
						    $c_winning_envid
						set c_winning_envid -1
					} else {
						foreach pair $cenvlist {
							if { [lindex $pair 0]\
							    == $elected_env } {
								set winning_envid \
								    [lindex $pair 1]
								break
							}
						}
					}

					# Make sure it's the expected winner.
					error_check_good right_winner \
					    $winning_envid [expr $win + 2]

					# Reconfigure winning env as master.
					if { $ignore == 0 } {
						$clientenv($i) errpfx \
						    NEWMASTER
						error_check_good \
						    make_master($i) \
					    	    [$clientenv($i) \
						    rep_start -master] 0

						# Don't hold another election
						# yet if we are setting up a 
						# new master. This could 
						# cause the new master to
						# declare itself a client
						# during internal init.
						set he 0
					}

					# Occasionally force new log records
					# to be written, unless the database 
					# has not yet been created.
					set write [berkdb random_int 1 10]
					if { $write == 1 && $dbname != "NULL" } {
						set db [eval berkdb_open_noerr \
						    -env $clientenv($i) \
						    -auto_commit $dbname]
						error_check_good dbopen \
						    [is_valid_db $db] TRUE
						error_check_good dbclose \
						    [$db close] 0
					}
				}

				# If the previous election failed with a
				# timeout and we need to retry because we
				# are testing varying site timeouts, force
				# a hold election to start a new one.
				if { $unavail && $retry && $retry_cnt < $max_retry} {
					incr retry_cnt
					puts "\t\t$msg.2.b: Client $i timed\
					    out. Retry $retry_cnt\
					    of max $max_retry"
					set he 1
					set tries $orig_tries
				}
				if { $he == 1 && $got_newmaster == 0 } {
					#
					# Only close down the election pipe if the
					# previously created one is done and
					# waiting for new commands, otherwise
					# if we try to close it while it's in
					# progress we hang this main tclsh.
					#
					if { $elect_pipe($i) != "INVALID" && \
					    $child_done == 1 } {
						close_election $elect_pipe($i)
						set elect_pipe($i) "INVALID"
					}
# puts "Starting election on client $i"
					if { $elect_pipe($i) == "INVALID" } {
						incr elect_serial
						set pfx "CHILD$i.$elect_serial"
						set elect_pipe($i) [start_election \
						    $pfx $qdir \
						    $env_cmd($i) $nsites \
						    $nvotes $pri($i) $e_timeout($i)]
						set got_hold_elect($i) 1
					}
				}
			}

			# We need to wait around to make doubly sure that the
			# election has finished...
			if { $nproced == 0 } {
				incr tries -1
				#
				# If we have a newmaster already, set tries
				# down to just allow straggling messages to
				# be processed.  Tries could be a very large
				# number if we have long timeouts.
				#
				if { $got_newmaster != 0 && $tries > 10 } {
					set tries 10
				}
				if { $tries == 0 } {
					break
				} else {
					tclsleep 1
				}
			} else {
				set tries $tries
			}
		}

		# If we did get a new master, its identity was checked 
		# at that time.  But we still have to make sure that we 
		# didn't just time out. 

		if { $got_newmaster == 0 && $timeout_ok == 0 } {
			error "FAIL: Did not elect new master."
		}
	}
	cleanup_elections

	#
	# Make sure we've really processed all the post-election
	# sync-up messages.  If we're simulating a crash, don't process
	# any more messages.
	#
	if { $crashing == 0 } {
		process_msgs $cenvlist
	}

	if { $reopen == 1 } {
		puts "\t\t$msg.3: Closing new master and reopening as client"
		error_check_good log_flush [$clientenv($win) log_flush] 0
		error_check_good newmaster_close [$clientenv($win) close] 0

		set clientenv($win) [eval $env_cmd($win)]
		error_check_good cl($win) [is_valid_env $clientenv($win)] TRUE
		set newelector "$clientenv($win) [expr $win + 2]"
		set cenvlist [lreplace $cenvlist $win $win $newelector]
		if { $crashing == 0 } {
			process_msgs $cenvlist
		}
	}
}

proc check_election { id unavailp elected_eventp elected_envp } {
	source ./include.tcl

	if { $id == "INVALID" } {
		return 0
	}
	upvar $unavailp unavail
	upvar $elected_eventp elected_event
	upvar $elected_envp elected_env

	set unavail 0
	set elected_event 0
	set elected_env "NONE"

	set res [catch {open $testdir/ELECTION_RESULT.$id} nmid]
	if { $res != 0 } {
		return 0
	}
	while { [gets $nmid val] != -1 } {
#		puts "result $id: $val"
		set str [lindex $val 0]
		if { [is_substr $val UNAVAIL] } {
			set unavail 1
		}
		if { [is_substr $val ELECTED] } {
			set elected_event 1
			set elected_env [lindex $val 1]
		}
	}
	close $nmid
	return 1
}

proc close_election { i } {
	global elections_in_progress
	global noenv_messaging
	global qtestdir

	if { $noenv_messaging == 1 } {
		set testdir $qtestdir
	}

	set t $elections_in_progress($i)
	puts $t "replclose \$testdir/MSGQUEUEDIR"
	puts $t "\$dbenv close"
	close $t
	unset elections_in_progress($i)
}

proc cleanup_elections { } {
	global elect_serial elections_in_progress

	for { set i 0 } { $i <= $elect_serial } { incr i } {
		if { [info exists elections_in_progress($i)] != 0 } {
			close_election $i
		}
	}

	set elect_serial 0
}

#
# This is essentially a copy of test001, but it only does the put/get
# loop AND it takes an already-opened db handle.
#
proc rep_test { method env repdb {nentries 10000} \
    {start 0} {skip 0} {needpad 0} args } {

	source ./include.tcl
	global databases_in_memory

	#
	# Open the db if one isn't given.  Close before exit.
	#
	if { $repdb == "NULL" } {
		if { $databases_in_memory == 1 } {
			set testfile { "" "test.db" }
		} else {
			set testfile "test.db"
		}
		set largs [convert_args $method $args]
		set omethod [convert_method $method]
		set db [eval {berkdb_open_noerr} -env $env -auto_commit\
		    -create -mode 0644 $omethod $largs $testfile]
		error_check_good reptest_db [is_valid_db $db] TRUE
	} else {
		set db $repdb
	}

	puts "\t\tRep_test: $method $nentries key/data pairs starting at $start"
	set did [open $dict]

	# The "start" variable determines the record number to start
	# with, if we're using record numbers.  The "skip" variable
	# determines which dictionary entry to start with.  In normal
	# use, skip is equal to start.

	if { $skip != 0 } {
		for { set count 0 } { $count < $skip } { incr count } {
			gets $did str
		}
	}
	set pflags ""
	set gflags ""
	set txn ""

	if { [is_record_based $method] == 1 } {
		append gflags " -recno"
	}
	puts "\t\tRep_test.a: put/get loop"
	# Here is the loop where we put and get each key/data pair
	set count 0

	# Checkpoint 10 times during the run, but not more
	# frequently than every 5 entries.
	set checkfreq [expr $nentries / 10]

	# Abort occasionally during the run.
	set abortfreq [expr $nentries / 15]

	while { [gets $did str] != -1 && $count < $nentries } {
		if { [is_record_based $method] == 1 } {
			global kvals

			set key [expr $count + 1 + $start]
			if { 0xffffffff > 0 && $key > 0xffffffff } {
				set key [expr $key - 0x100000000]
			}
			if { $key == 0 || $key - 0xffffffff == 1 } {
				incr key
				incr count
			}
			set kvals($key) [pad_data $method $str]
		} else {
			set key $str
			set str [reverse $str]
		}
		#
		# We want to make sure we send in exactly the same
		# length data so that LSNs match up for some tests
		# in replication (rep021).
		#
		if { [is_fixed_length $method] == 1 && $needpad } {
			#
			# Make it something visible and obvious, 'A'.
			#
			set p 65
			set str [make_fixed_length $method $str $p]
			set kvals($key) $str
		}
		set t [$env txn]
		error_check_good txn [is_valid_txn $t $env] TRUE
		set txn "-txn $t"
		set ret [eval \
		    {$db put} $txn $pflags {$key [chop_data $method $str]}]
		error_check_good put $ret 0
		error_check_good txn [$t commit] 0

		if { $checkfreq < 5 } {
			set checkfreq 5
		}
		if { $abortfreq < 3 } {
			set abortfreq 3
		}
		#
		# Do a few aborted transactions to test that
		# aborts don't get processed on clients and the
		# master handles them properly.  Just abort
		# trying to delete the key we just added.
		#
		if { $count % $abortfreq == 0 } {
			set t [$env txn]
			error_check_good txn [is_valid_txn $t $env] TRUE
			set ret [$db del -txn $t $key]
			error_check_good txn [$t abort] 0
		}
		if { $count % $checkfreq == 0 } {
			error_check_good txn_checkpoint($count) \
			    [$env txn_checkpoint] 0
		}
		incr count
	}
	close $did
	if { $repdb == "NULL" } {
		error_check_good rep_close [$db close] 0
	}
}

#
# This is essentially a copy of rep_test, but it only does the put/get
# loop in a long running txn to an open db.  We use it for bulk testing
# because we want to fill the bulk buffer some before sending it out.
# Bulk buffer gets transmitted on every commit.
#
proc rep_test_bulk { method env repdb {nentries 10000} \
    {start 0} {skip 0} {useoverflow 0} args } {
	source ./include.tcl

	global overflowword1
	global overflowword2
	global databases_in_memory

	if { [is_fixed_length $method] && $useoverflow == 1 } {
		puts "Skipping overflow for fixed length method $method"
		return
	}
	#
	# Open the db if one isn't given.  Close before exit.
	#
	if { $repdb == "NULL" } {
		if { $databases_in_memory == 1 } {
			set testfile { "" "test.db" }
		} else {
			set testfile "test.db"
		}
		set largs [convert_args $method $args]
		set omethod [convert_method $method]
		set db [eval {berkdb_open_noerr -env $env -auto_commit -create \
		    -mode 0644} $largs $omethod $testfile]
		error_check_good reptest_db [is_valid_db $db] TRUE
	} else {
		set db $repdb
	}

	#
	# If we are using an env, then testfile should just be the db name.
	# Otherwise it is the test directory and the name.
	# If we are not using an external env, then test setting
	# the database cache size and using multiple caches.
	puts \
"\t\tRep_test_bulk: $method $nentries key/data pairs starting at $start"
	set did [open $dict]

	# The "start" variable determines the record number to start
	# with, if we're using record numbers.  The "skip" variable
	# determines which dictionary entry to start with.  In normal
	# use, skip is equal to start.

	if { $skip != 0 } {
		for { set count 0 } { $count < $skip } { incr count } {
			gets $did str
		}
	}
	set pflags ""
	set gflags ""
	set txn ""

	if { [is_record_based $method] == 1 } {
		append gflags " -recno"
	}
	puts "\t\tRep_test_bulk.a: put/get loop in 1 txn"
	# Here is the loop where we put and get each key/data pair
	set count 0

	set t [$env txn]
	error_check_good txn [is_valid_txn $t $env] TRUE
	set txn "-txn $t"
	set pid [pid]
	while { [gets $did str] != -1 && $count < $nentries } {
		if { [is_record_based $method] == 1 } {
			global kvals

			set key [expr $count + 1 + $start]
			if { 0xffffffff > 0 && $key > 0xffffffff } {
				set key [expr $key - 0x100000000]
			}
			if { $key == 0 || $key - 0xffffffff == 1 } {
				incr key
				incr count
			}
			set kvals($key) [pad_data $method $str]
			if { [is_fixed_length $method] == 0 } {
				set str [repeat $str 100]
			}
		} else {
			set key $str.$pid
			set str [repeat $str 100]
		}
		#
		# For use for overflow test.
		#
		if { $useoverflow == 0 } {
			if { [string length $overflowword1] < \
			    [string length $str] } {
				set overflowword2 $overflowword1
				set overflowword1 $str
			}
		} else {
			if { $count == 0 } {
				set len [string length $overflowword1]
				set word $overflowword1
			} else {
				set len [string length $overflowword2]
				set word $overflowword1
			}
			set rpt [expr 1024 * 1024 / $len]
			incr rpt
			set str [repeat $word $rpt]
		}
		set ret [eval \
		    {$db put} $txn $pflags {$key [chop_data $method $str]}]
		error_check_good put $ret 0
		incr count
	}
	error_check_good txn [$t commit] 0
	error_check_good txn_checkpoint [$env txn_checkpoint] 0
	close $did
	if { $repdb == "NULL" } {
		error_check_good rep_close [$db close] 0
	}
}

proc rep_test_upg { method env repdb {nentries 10000} \
    {start 0} {skip 0} {needpad 0} {inmem 0} args } {

	source ./include.tcl

	#
	# Open the db if one isn't given.  Close before exit.
	#
	if { $repdb == "NULL" } {
		if { $inmem == 1 } {
			set testfile { "" "test.db" }
		} else {
			set testfile "test.db"
		}
		set largs [convert_args $method $args]
		set omethod [convert_method $method]
		set db [eval {berkdb_open_noerr} -env $env -auto_commit\
		    -create -mode 0644 $omethod $largs $testfile]
		error_check_good reptest_db [is_valid_db $db] TRUE
	} else {
		set db $repdb
	}

	set pid [pid]
	puts "\t\tRep_test_upg($pid): $method $nentries key/data pairs starting at $start"
	set did [open $dict]

	# The "start" variable determines the record number to start
	# with, if we're using record numbers.  The "skip" variable
	# determines which dictionary entry to start with.  In normal
	# use, skip is equal to start.

	if { $skip != 0 } {
		for { set count 0 } { $count < $skip } { incr count } {
			gets $did str
		}
	}
	set pflags ""
	set gflags ""
	set txn ""

	if { [is_record_based $method] == 1 } {
		append gflags " -recno"
	}
	puts "\t\tRep_test.a: put/get loop"
	# Here is the loop where we put and get each key/data pair
	set count 0

	# Checkpoint 10 times during the run, but not more
	# frequently than every 5 entries.
	set checkfreq [expr $nentries / 10]

	# Abort occasionally during the run.
	set abortfreq [expr $nentries / 15]

	while { [gets $did str] != -1 && $count < $nentries } {
		if { [is_record_based $method] == 1 } {
			global kvals

			set key [expr $count + 1 + $start]
			if { 0xffffffff > 0 && $key > 0xffffffff } {
				set key [expr $key - 0x100000000]
			}
			if { $key == 0 || $key - 0xffffffff == 1 } {
				incr key
				incr count
			}
			set kvals($key) [pad_data $method $str]
		} else {
			#
			# With upgrade test, we run the same test several
			# times with the same database.  We want to have
			# some overwritten records and some new records.
			# Therefore append our pid to half the keys.
			#
			if { $count % 2 } {
				set key $str.$pid
			} else {
				set key $str
			}
			set str [reverse $str]
		}
		#
		# We want to make sure we send in exactly the same
		# length data so that LSNs match up for some tests
		# in replication (rep021).
		#
		if { [is_fixed_length $method] == 1 && $needpad } {
			#
			# Make it something visible and obvious, 'A'.
			#
			set p 65
			set str [make_fixed_length $method $str $p]
			set kvals($key) $str
		}
		set t [$env txn]
		error_check_good txn [is_valid_txn $t $env] TRUE
		set txn "-txn $t"
# puts "rep_test_upg: put $count of $nentries: key $key, data $str"
		set ret [eval \
		    {$db put} $txn $pflags {$key [chop_data $method $str]}]
		error_check_good put $ret 0
		error_check_good txn [$t commit] 0

		if { $checkfreq < 5 } {
			set checkfreq 5
		}
		if { $abortfreq < 3 } {
			set abortfreq 3
		}
		#
		# Do a few aborted transactions to test that
		# aborts don't get processed on clients and the
		# master handles them properly.  Just abort
		# trying to delete the key we just added.
		#
		if { $count % $abortfreq == 0 } {
			set t [$env txn]
			error_check_good txn [is_valid_txn $t $env] TRUE
			set ret [$db del -txn $t $key]
			error_check_good txn [$t abort] 0
		}
		if { $count % $checkfreq == 0 } {
			error_check_good txn_checkpoint($count) \
			    [$env txn_checkpoint] 0
		}
		incr count
	}
	close $did
	if { $repdb == "NULL" } {
		error_check_good rep_close [$db close] 0
	}
}

proc rep_test_upg.check { key data } {
	#
	# If the key has the pid attached, strip it off before checking.
	# If the key does not have the pid attached, then it is a recno
	# and we're done.
	#
	set i [string first . $key]
	if { $i != -1 } {
		set key [string replace $key $i end]
	}
	error_check_good "key/data mismatch" $data [reverse $key]
}

proc rep_test_upg.recno.check { key data } {
	#
	# If we're a recno database we better not have a pid in the key.
	# Otherwise we're done.
	#
	set i [string first . $key]
	error_check_good pid $i -1
}

#
# This is the basis for a number of simple repmgr test cases. It creates
# an appointed master and two clients, calls rep_test to process some records 
# and verifies the resulting databases. The following parameters control 
# runtime options:
#     niter    - number of records to process
#     inmemdb  - put databases in-memory (0, 1)
#     inmemlog - put logs in-memory (0, 1)
#     peer     - make the second client a peer of the first client (0, 1)
#     bulk     - use bulk processing (0, 1)
#     inmemrep - put replication files in-memory (0, 1)
#
proc basic_repmgr_test { method niter tnum inmemdb inmemlog peer bulk \
    inmemrep largs } {
	global testdir 
	global rep_verbose 
	global verbose_type
	global overflowword1
	global overflowword2
	global databases_in_memory
	set overflowword1 "0"
	set overflowword2 "0"
	set nsites 3

	# Set databases_in_memory for this test, preserving original value.
	if { $inmemdb } {
		set restore_dbinmem $databases_in_memory
		set databases_in_memory 1
	}

	set verbargs ""
	if { $rep_verbose == 1 } {
		set verbargs " -verbose {$verbose_type on} "
	}

	env_cleanup $testdir
	set ports [available_ports $nsites]

	set masterdir $testdir/MASTERDIR
	set clientdir $testdir/CLIENTDIR
	set clientdir2 $testdir/CLIENTDIR2

	file mkdir $masterdir
	file mkdir $clientdir
	file mkdir $clientdir2

	# In-memory logs require a large log buffer, and cannot
	# be used with -txn nosync.  Adjust the args.
	if { $inmemlog } {
		set logtype "in-memory"
	} else {
		set logtype "on-disk"
	}
	set logargs [adjust_logargs $logtype]
	set txnargs [adjust_txnargs $logtype]

	# Determine in-memory replication argument for environments.
	if { $inmemrep } {
		set repmemarg "-rep_inmem_files "
	} else {
		set repmemarg ""
	}

	# Use different connection retry timeout values to handle any
	# collisions from starting sites at the same time by retrying
	# at different times.

	# Open a master.
	puts "\tRepmgr$tnum.a: Start an appointed master."
	set ma_envcmd "berkdb_env_noerr -create $logargs $verbargs \
	    -errpfx MASTER -home $masterdir $txnargs -rep -thread \
	    -lock_max_locks 10000 -lock_max_objects 10000 $repmemarg"
	set masterenv [eval $ma_envcmd]
	$masterenv repmgr -ack all -nsites $nsites \
	    -timeout {conn_retry 20000000} \
	    -local [list localhost [lindex $ports 0]] \
	    -start master

	# Open first client
	puts "\tRepmgr$tnum.b: Start first client."
	set cl_envcmd "berkdb_env_noerr -create $verbargs $logargs \
	    -errpfx CLIENT -home $clientdir $txnargs -rep -thread \
	    -lock_max_locks 10000 -lock_max_objects 10000 $repmemarg"
	set clientenv [eval $cl_envcmd]
	$clientenv repmgr -ack all -nsites $nsites \
	    -timeout {conn_retry 10000000} \
	    -local [list localhost [lindex $ports 1]] \
	    -remote [list localhost [lindex $ports 0]] \
	    -remote [list localhost [lindex $ports 2]] \
	    -start client
	await_startup_done $clientenv

	# Open second client
	puts "\tRepmgr$tnum.c: Start second client."
	set cl2_envcmd "berkdb_env_noerr -create $verbargs $logargs \
	    -errpfx CLIENT2 -home $clientdir2 $txnargs -rep -thread \
	    -lock_max_locks 10000 -lock_max_objects 10000 $repmemarg"
	set clientenv2 [eval $cl2_envcmd]
	if { $peer } {
		$clientenv2 repmgr -ack all -nsites $nsites \
		    -timeout {conn_retry 5000000} \
		    -local [list localhost [lindex $ports 2]] \
		    -remote [list localhost [lindex $ports 0]] \
		    -remote [list localhost [lindex $ports 1] peer] \
		    -start client
	} else {
		$clientenv2 repmgr -ack all -nsites $nsites \
		    -timeout {conn_retry 5000000} \
		    -local [list localhost [lindex $ports 2]] \
		    -remote [list localhost [lindex $ports 0]] \
		    -remote [list localhost [lindex $ports 1]] \
		    -start client
	}
	await_startup_done $clientenv2

	#
	# Use of -ack all guarantees replication complete before repmgr send
	# function returns and rep_test finishes.
	#
	puts "\tRepmgr$tnum.d: Run some transactions at master."
	if { $bulk } {
		# Turn on bulk processing on master.
		error_check_good set_bulk [$masterenv rep_config {bulk on}] 0

		eval rep_test_bulk $method $masterenv NULL $niter 0 0 0 $largs

		# Must turn off bulk because some configs (debug_rop/wop)
		# generate log records when verifying databases.
		error_check_good set_bulk [$masterenv rep_config {bulk off}] 0
	} else {
		eval rep_test $method $masterenv NULL $niter 0 0 0 $largs
	}

	puts "\tRepmgr$tnum.e: Verifying client database contents."
	rep_verify $masterdir $masterenv $clientdir $clientenv 1 1 1
	rep_verify $masterdir $masterenv $clientdir2 $clientenv2 1 1 1

	# For in-memory replication, verify replication files not there.
	if { $inmemrep } {
		puts "\tRepmgr$tnum.f: Verify no replication files on disk."
		no_rep_files_on_disk $masterdir
		no_rep_files_on_disk $clientdir
		no_rep_files_on_disk $clientdir2
	}

	# Restore original databases_in_memory value.
	if { $inmemdb } {
		set databases_in_memory $restore_dbinmem
	}

	error_check_good client2_close [$clientenv2 close] 0
	error_check_good client_close [$clientenv close] 0
	error_check_good masterenv_close [$masterenv close] 0
}

#
# This is the basis for simple repmgr election test cases.  It opens three
# clients of different priorities and makes sure repmgr elects the
# expected master.  Then it shuts the master down and makes sure repmgr
# elects the expected remaining client master.  Then it makes sure the former
# master can join as a client.  The following parameters control 
# runtime options:
#     niter    - number of records to process
#     inmemrep - put replication files in-memory (0, 1)
#
proc basic_repmgr_election_test { method niter tnum inmemrep largs } {
	global rep_verbose
	global testdir
	global verbose_type
	set nsites 3

	set verbargs ""
	if { $rep_verbose == 1 } {
		set verbargs " -verbose {$verbose_type on} "
	}

	env_cleanup $testdir
	set ports [available_ports $nsites]

	set clientdir $testdir/CLIENTDIR
	set clientdir2 $testdir/CLIENTDIR2
	set clientdir3 $testdir/CLIENTDIR3

	file mkdir $clientdir
	file mkdir $clientdir2
	file mkdir $clientdir3

	# Determine in-memory replication argument for environments.
	if { $inmemrep } {
		set repmemarg "-rep_inmem_files "
	} else {
		set repmemarg ""
	}

	# Use different connection retry timeout values to handle any
	# collisions from starting sites at the same time by retrying
	# at different times.

	puts "\tRepmgr$tnum.a: Start three clients."

	# Open first client
	set cl_envcmd "berkdb_env_noerr -create $verbargs \
	    -errpfx CLIENT -home $clientdir -txn -rep -thread $repmemarg"
	set clientenv [eval $cl_envcmd]
	$clientenv repmgr -ack all -nsites $nsites -pri 100 \
	    -timeout {conn_retry 20000000} \
	    -local [list localhost [lindex $ports 0]] \
	    -remote [list localhost [lindex $ports 1]] \
	    -remote [list localhost [lindex $ports 2]] \
	    -start elect

	# Open second client
	set cl2_envcmd "berkdb_env_noerr -create $verbargs \
	    -errpfx CLIENT2 -home $clientdir2 -txn -rep -thread $repmemarg"
	set clientenv2 [eval $cl2_envcmd]
	$clientenv2 repmgr -ack all -nsites $nsites -pri 30 \
	    -timeout {conn_retry 10000000} \
	    -local [list localhost [lindex $ports 1]] \
	    -remote [list localhost [lindex $ports 0]] \
	    -remote [list localhost [lindex $ports 2]] \
	    -start elect

	# Open third client
	set cl3_envcmd "berkdb_env_noerr -create $verbargs \
	    -errpfx CLIENT3 -home $clientdir3 -txn -rep -thread $repmemarg"
	set clientenv3 [eval $cl3_envcmd]
	$clientenv3 repmgr -ack all -nsites $nsites -pri 20 \
	    -timeout {conn_retry 5000000} \
	    -local [list localhost [lindex $ports 2]] \
	    -remote [list localhost [lindex $ports 0]] \
	    -remote [list localhost [lindex $ports 1]] \
	    -start elect

	puts "\tRepmgr$tnum.b: Elect first client master."
	await_expected_master $clientenv
	set masterenv $clientenv
	set masterdir $clientdir
	await_startup_done $clientenv2
	await_startup_done $clientenv3

	#
	# Use of -ack all guarantees replication complete before repmgr send
	# function returns and rep_test finishes.
	#
	puts "\tRepmgr$tnum.c: Run some transactions at master."
	eval rep_test $method $masterenv NULL $niter 0 0 0 $largs

	puts "\tRepmgr$tnum.d: Verify client database contents."
	rep_verify $masterdir $masterenv $clientdir2 $clientenv2 1 1 1
	rep_verify $masterdir $masterenv $clientdir3 $clientenv3 1 1 1

	puts "\tRepmgr$tnum.e: Shut down master, elect second client master."
	error_check_good client_close [$clientenv close] 0
	await_expected_master $clientenv2
	set masterenv $clientenv2
	await_startup_done $clientenv3

	puts "\tRepmgr$tnum.f: Restart former master as client."
	# Open -recover to clear env region, including startup_done value.
	set clientenv [eval $cl_envcmd -recover]
	$clientenv repmgr -ack all -nsites $nsites -pri 100 \
	    -timeout {conn_retry 20000000} \
	    -local [list localhost [lindex $ports 0]] \
	    -remote [list localhost [lindex $ports 1]] \
	    -remote [list localhost [lindex $ports 2]] \
	    -start client
	await_startup_done $clientenv

	puts "\tRepmgr$tnum.g: Run some transactions at new master."
	eval rep_test $method $masterenv NULL $niter $niter 0 0 $largs

	puts "\tRepmgr$tnum.h: Verify client database contents."
	set masterdir $clientdir2
	rep_verify $masterdir $masterenv $clientdir $clientenv 1 1 1
	rep_verify $masterdir $masterenv $clientdir3 $clientenv3 1 1 1

	# For in-memory replication, verify replication files not there.
	if { $inmemrep } {
		puts "\tRepmgr$tnum.i: Verify no replication files on disk."
		no_rep_files_on_disk $clientdir
		no_rep_files_on_disk $clientdir2
		no_rep_files_on_disk $clientdir3
	}

	error_check_good client3_close [$clientenv3 close] 0
	error_check_good client_close [$clientenv close] 0
	error_check_good client2_close [$clientenv2 close] 0
}

#
# This is the basis for simple repmgr internal init test cases.  It starts
# an appointed master and two clients, processing transactions between each
# additional site.  Then it verifies all expected transactions are 
# replicated.  The following parameters control runtime options:
#     niter    - number of records to process
#     inmemrep - put replication files in-memory (0, 1)
#
proc basic_repmgr_init_test { method niter tnum inmemrep largs } {
	global rep_verbose
	global testdir
	global verbose_type
	set nsites 3

	set verbargs ""
	if { $rep_verbose == 1 } {
		set verbargs " -verbose {$verbose_type on} "
	}

	env_cleanup $testdir
	set ports [available_ports $nsites]

	set masterdir $testdir/MASTERDIR
	set clientdir $testdir/CLIENTDIR
	set clientdir2 $testdir/CLIENTDIR2

	file mkdir $masterdir
	file mkdir $clientdir
	file mkdir $clientdir2

	# Determine in-memory replication argument for environments.
	if { $inmemrep } {
		set repmemarg "-rep_inmem_files "
	} else {
		set repmemarg ""
	}

	# Use different connection retry timeout values to handle any
	# collisions from starting sites at the same time by retrying
	# at different times.

	# Open a master.
	puts "\tRepmgr$tnum.a: Start a master."
	set ma_envcmd "berkdb_env_noerr -create $verbargs \
	    -errpfx MASTER -home $masterdir -txn -rep -thread $repmemarg"
	set masterenv [eval $ma_envcmd]
	$masterenv repmgr -ack all -nsites $nsites \
	    -timeout {conn_retry 20000000} \
	    -local [list localhost [lindex $ports 0]] \
	    -start master

	puts "\tRepmgr$tnum.b: Run some transactions at master."
	eval rep_test $method $masterenv NULL $niter 0 0 0 $largs

	# Open first client
	puts "\tRepmgr$tnum.c: Start first client."
	set cl_envcmd "berkdb_env_noerr -create $verbargs \
	    -errpfx CLIENT -home $clientdir -txn -rep -thread $repmemarg"
	set clientenv [eval $cl_envcmd]
	$clientenv repmgr -ack all -nsites $nsites \
	    -timeout {conn_retry 10000000} \
	    -local [list localhost [lindex $ports 1]] \
	    -remote [list localhost [lindex $ports 0]] \
	    -remote [list localhost [lindex $ports 2]] \
	    -start client
	await_startup_done $clientenv

	#
	# Use of -ack all guarantees replication complete before repmgr send
	# function returns and rep_test finishes.
	#
	puts "\tRepmgr$tnum.d: Run some more transactions at master."
	eval rep_test $method $masterenv NULL $niter $niter 0 0 $largs

	# Open second client
	puts "\tRepmgr$tnum.e: Start second client."
	set cl_envcmd "berkdb_env_noerr -create $verbargs \
	    -errpfx CLIENT2 -home $clientdir2 -txn -rep -thread $repmemarg"
	set clientenv2 [eval $cl_envcmd]
	$clientenv2 repmgr -ack all -nsites $nsites \
	    -timeout {conn_retry 5000000} \
	    -local [list localhost [lindex $ports 2]] \
	    -remote [list localhost [lindex $ports 0]] \
	    -remote [list localhost [lindex $ports 1]] \
	    -start client
	await_startup_done $clientenv2

	puts "\tRepmgr$tnum.f: Verifying client database contents."
	rep_verify $masterdir $masterenv $clientdir $clientenv 1 1 1
	rep_verify $masterdir $masterenv $clientdir2 $clientenv2 1 1 1

	# For in-memory replication, verify replication files not there.
	if { $inmemrep } {
		puts "\tRepmgr$tnum.g: Verify no replication files on disk."
		no_rep_files_on_disk $masterdir
		no_rep_files_on_disk $clientdir
		no_rep_files_on_disk $clientdir2
	}

	error_check_good client2_close [$clientenv2 close] 0
	error_check_good client_close [$clientenv close] 0
	error_check_good masterenv_close [$masterenv close] 0
}

#
# Verify that no replication files are present in a given directory.
# This checks for the gen, egen, internal init, temp db and page db
# files.
#
proc no_rep_files_on_disk { dir } {
    error_check_good nogen [file exists "$dir/__db.rep.gen"] 0
    error_check_good noegen [file exists "$dir/__db.rep.egen"] 0
    error_check_good noinit [file exists "$dir/__db.rep.init"] 0
    error_check_good notmpdb [file exists "$dir/__db.rep.db"] 0
    error_check_good nopgdb [file exists "$dir/__db.reppg.db"] 0
}

proc process_msgs { elist {perm_response 0} {dupp NONE} {errp NONE} \
    {upg 0} } {
	if { $perm_response == 1 } {
		global perm_response_list
		set perm_response_list {{}}
	}

	if { [string compare $dupp NONE] != 0 } {
		upvar $dupp dupmaster
		set dupmaster 0
	} else {
		set dupmaster NONE
	}

	if { [string compare $errp NONE] != 0 } {
		upvar $errp errorp
		set errorp 0
		set var_name errorp
	} else {
		set errorp NONE
		set var_name NONE
	}

	set upgcount 0
	while { 1 } {
		set nproced 0
		incr nproced [proc_msgs_once $elist dupmaster $var_name]
		#
		# If we're running the upgrade test, we are running only
		# our own env, we need to loop a bit to allow the other
		# upgrade procs to run and reply to our messages.
		#
		if { $upg == 1 && $upgcount < 10 } {
			tclsleep 2
			incr upgcount
			continue
		}
		if { $nproced == 0 } {
			break
		} else {
			set upgcount 0
		}
	}
}


proc proc_msgs_once { elist {dupp NONE} {errp NONE} } {
	global noenv_messaging

	if { [string compare $dupp NONE] != 0 } {
		upvar $dupp dupmaster
		set dupmaster 0
	} else {
		set dupmaster NONE
	}

	if { [string compare $errp NONE] != 0 } {
		upvar $errp errorp
		set errorp 0
		set var_name errorp
	} else {
		set errorp NONE
		set var_name NONE
	}

	set nproced 0
	foreach pair $elist {
		set envname [lindex $pair 0]
		set envid [lindex $pair 1]
		#
		# If we need to send in all the other args
# puts "Call replpq with on $envid"
		if { $noenv_messaging } {
			incr nproced [replprocessqueue_noenv $envname $envid \
			    0 NONE dupmaster $var_name]
		} else {
			incr nproced [replprocessqueue $envname $envid \
			    0 NONE dupmaster $var_name]
		}
		#
		# If the user is expecting to handle an error and we get
		# one, return the error immediately.
		#
		if { $dupmaster != 0 && $dupmaster != "NONE" } {
			return 0
		}
		if { $errorp != 0 && $errorp != "NONE" } {
# puts "Returning due to error $errorp"
			return 0
		}
	}
	return $nproced
}

proc rep_verify { masterdir masterenv clientdir clientenv \
    {compare_shared_portion 0} {match 1} {logcompare 1} \
    {dbname "test.db"} {datadir ""} } {
	global util_path
	global encrypt
	global passwd
	global databases_in_memory
	global repfiles_in_memory
	global env_private

	# Whether a named database is in-memory or on-disk, only the 
	# the name itself is passed in.  Here we do the syntax adjustment
	# from "test.db" to { "" "test.db" } for in-memory databases.
	# 
	if { $databases_in_memory && $dbname != "NULL" } {
		set dbname " {} $dbname "
	} 

	# Check locations of dbs, repfiles, region files.
	if { $dbname != "NULL" } {
		check_db_location $masterenv $dbname $datadir
		check_db_location $clientenv $dbname $datadir
	}

	if { $repfiles_in_memory } {
		no_rep_files_on_disk $masterdir
		no_rep_files_on_disk $clientdir
	}
	if { $env_private } {
		no_region_files_on_disk $masterdir
		no_region_files_on_disk $clientdir
	}
	
	# The logcompare flag indicates whether to compare logs.
	# Sometimes we run a test where rep_verify is run twice with
	# no intervening processing of messages.  If that test is
	# on a build with debug_rop enabled, the master's log is
	# altered by the first rep_verify, and the second rep_verify
	# will fail.
	# To avoid this, skip the log comparison on the second rep_verify
	# by specifying logcompare == 0.
	#
	if { $logcompare } {
		set msg "Logs and databases"
	} else {
		set msg "Databases ($dbname)"
	}

	if { $match } {
		puts "\t\tRep_verify: $clientdir: $msg should match"
	} else {
		puts "\t\tRep_verify: $clientdir: $msg should not match"
	}
	# Check that master and client logs and dbs are identical.

	# Logs first, if specified ...
	#
	# If compare_shared_portion is set, run db_printlog on the log
	# subset that both client and master have.  Either the client or
	# the master may have more (earlier) log files, due to internal
	# initialization, in-memory log wraparound, or other causes.
	#
	if { $logcompare } {
		error_check_good logcmp \
		    [logcmp $masterenv $clientenv $compare_shared_portion] 0
	
		if { $dbname == "NULL" } {
			return
		}
	}

	# ... now the databases.
	#
	# We're defensive here and throw an error if a database does
	# not exist.  If opening the first database succeeded but the
	# second failed, we close the first before reporting the error.
	#
	if { [catch {eval {berkdb_open_noerr} -env $masterenv\
	    -rdonly $dbname} db1] } {
		error "FAIL:\
		    Unable to open first db $dbname in rep_verify: $db1"
	}
	if { [catch {eval {berkdb_open_noerr} -env $clientenv\
	    -rdonly $dbname} db2] } {
		error_check_good close_db1 [$db1 close] 0
		error "FAIL:\
		    Unable to open second db $dbname in rep_verify: $db2"
	}

	# db_compare uses the database handles to do the comparison, and 
	# we pass in the $mumbledir/$dbname string as a label to make it 
	# easier to identify the offending database in case of failure. 
	# Therefore this will work for both in-memory and on-disk databases.
	if { $match } {
		error_check_good [concat comparedbs. $dbname] [db_compare \
		    $db1 $db2 $masterdir/$dbname $clientdir/$dbname] 0
	} else {
		error_check_bad comparedbs [db_compare \
		    $db1 $db2 $masterdir/$dbname $clientdir/$dbname] 0
	}
	error_check_good db1_close [$db1 close] 0
	error_check_good db2_close [$db2 close] 0
}

proc rep_event { env eventlist } {
	global startup_done
	global elected_event
	global elected_env

	set event [lindex $eventlist 0]
# puts "rep_event: Got event $event on env $env"
	set eventlength [llength $eventlist]

	if { $event == "startupdone" } {
		error_check_good event_nodata $eventlength 1
		set startup_done 1
	}
	if { $event == "elected" } {
		error_check_good event_nodata $eventlength 1
		set elected_event 1
		set elected_env $env
	}
	if { $event == "newmaster" } {
		error_check_good eiddata $eventlength 2
		set event_newmasterid [lindex $eventlist 1]
	}
	return
}

# Return a list of TCP port numbers that are not currently in use on
# the local system.  Note that this doesn't actually reserve the
# ports, so it's possible that by the time the caller tries to use
# them, another process could have taken one of them.  But for our
# purposes that's unlikely enough that this is still useful: it's
# still better than trying to find hard-coded port numbers that will
# always be available.
#
proc available_ports { n } {
    set ports {}
    set socks {}

    while {[incr n -1] >= 0} {
        set sock [socket -server Unused -myaddr localhost 0]
        set port [lindex [fconfigure $sock -sockname] 2]

        lappend socks $sock
        lappend ports $port
    }

    foreach sock $socks {
        close $sock
    }
    return $ports
}

# Wait (a limited amount of time) for an arbitrary condition to become true,
# polling once per second.  If time runs out we throw an error: a successful
# return implies the condition is indeed true.
# 
proc await_condition { cond { limit 20 } } {
	for {set i 0} {$i < $limit} {incr i} {
		if {[uplevel 1 [list expr $cond]]} {
			return
		}
		tclsleep 1
	}
	error "FAIL: condition \{$cond\} not achieved in $limit seconds."
}

proc await_startup_done { env { limit 20 } } {
	await_condition {[stat_field $env rep_stat "Startup complete"]} $limit
}

# Wait (a limited amount of time) for an election to yield the expected
# environment as winner.
#
proc await_expected_master { env { limit 20 } } {
	await_condition {[stat_field $env rep_stat "Role"] == "master"} $limit
}

proc do_leaseop { env db method key envlist { domsgs 1 } } {
	global alphabet

	#
	# Put a txn to the database.  Process messages to envlist
	# if directed to do so.  Read data on the master, ignoring
	# leases (should always succeed).
	#
	set num [berkdb random_int 1 100]
	set data $alphabet.$num
	set t [$env txn]
	error_check_good txn [is_valid_txn $t $env] TRUE
	set txn "-txn $t"
	set ret [eval \
	    {$db put} $txn {$key [chop_data $method $data]}]
	error_check_good put $ret 0
	error_check_good txn [$t commit] 0

	if { $domsgs } {
		process_msgs $envlist
	}

	#
	# Now make sure we can successfully read on the master
	# if we ignore leases.  That should always work.  The
	# caller will do any lease related calls and checks
	# that are specific to the test.
	#
	set kd [$db get -nolease $key]
	set curs [$db cursor]
	set ckd [$curs get -nolease -set $key]
	$curs close
	error_check_good kd [llength $kd] 1
	error_check_good ckd [llength $ckd] 1
}

#
# Get the given key, expecting status depending on whether leases
# are currently expected to be valid or not.
#
proc check_leaseget { db key getarg status } {
	set stat [catch {eval {$db get} $getarg $key} kd]
	if { $status != 0 } {
		error_check_good get_result $stat 1
		error_check_good kd_check \
		    [is_substr $kd $status] 1
	} else {
		error_check_good get_result_good $stat $status
		error_check_good dbkey [lindex [lindex $kd 0] 0] $key
	}
	set curs [$db cursor]
	set stat [catch {eval {$curs get} $getarg -set $key} kd]
	if { $status != 0 } {
		error_check_good get_result2 $stat 1
		error_check_good kd_check \
		    [is_substr $kd $status] 1
	} else {
		error_check_good get_result2_good $stat $status
		error_check_good dbckey [lindex [lindex $kd 0] 0] $key
	}
	$curs close
}

# Simple utility to check a client database for expected values.  It does not
# handle dup keys.
# 
proc verify_client_data { env db items } {
	set dbp [berkdb open -env $env $db]
	foreach i $items {
		foreach {key expected_value} $i {
			set results [$dbp get $key]
			error_check_good result_length [llength $results] 1
			set value [lindex $results 0 1]
			error_check_good expected_value $value $expected_value
		}
	}
	$dbp close
}

proc make_dbconfig { dir cnfs } {
	global rep_verbose

	set f [open "$dir/DB_CONFIG" "w"]
	foreach line $cnfs {
		puts $f $line
	}
	if {$rep_verbose} {
		puts $f "set_verbose DB_VERB_REPLICATION"
	}
	close $f
}

proc open_site_prog { cmds } {

	set site_prog [setup_site_prog]

	set s [open "| $site_prog" "r+"]
	fconfigure $s -buffering line
	set synced yes
	foreach cmd $cmds {
		puts $s $cmd
		if {[lindex $cmd 0] == "start"} {
			gets $s
			set synced yes
		} else {
			set synced no
		}
	}
	if {! $synced} {
		puts $s "echo done"
		gets $s
	}
	return $s
}

proc setup_site_prog { } {
	source ./include.tcl
	
	# Generate the proper executable name for the system. 
	if { $is_windows_test } {
		set repsite_executable db_repsite.exe
	} else {
		set repsite_executable db_repsite
	}

	# Check whether the executable exists. 
	if { [file exists $util_path/$repsite_executable] == 0 } {
		error "Skipping: db_repsite executable\
		    not found.  Is it built?"
	} else {
		set site_prog $util_path/$repsite_executable
	}
	return $site_prog
}

proc next_expected_lsn { env } {
	return [stat_field $env rep_stat "Next LSN expected"]
}

proc lsn_file { lsn } {
	if { [llength $lsn] != 2 } {
		error "not a valid LSN: $lsn"
	}

	return [lindex $lsn 0]
}

proc assert_rep_flag { dir flag value } {
	global util_path

	set stat [exec $util_path/db_stat -N -RA -h $dir]
	set present [is_substr $stat $flag]
	error_check_good expected.flag.$flag $present $value
}

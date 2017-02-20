# See the file LICENSE for redistribution information.
#
# Copyright (c) 2004-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	rep060
# TEST	Test of normally running clients and internal initialization.
# TEST	Have a client running normally, but slow/far behind the master.
# TEST	Then the master checkpoints and archives, causing the client
# TEST	to suddenly be thrown into internal init.  This test tests
# TEST	that we clean up the old files/pages in mpool and dbreg.
# TEST	Also test same thing but the app holding an open dbp as well.
#
proc rep060 { method { niter 200 } { tnum "060" } args } {

	source ./include.tcl
	global databases_in_memory
	global repfiles_in_memory

	# Run for btree and queue only.
	if { $checking_valid_methods } {
		set test_methods {}
		foreach method $valid_methods {
			if { [is_btree $method] == 1 || \
			    [is_queue $method] == 1 } {
				lappend test_methods $method
			}
		}
		return $test_methods
	}
	if { [is_btree $method] != 1 && [is_queue $method] != 1 } {
		puts "Skipping rep060 for method $method."
		return
	}

	set args [convert_args $method $args]

	# This test needs to set its own pagesize.
	set pgindex [lsearch -exact $args "-pagesize"]
        if { $pgindex != -1 } {
                puts "Rep$tnum: skipping for specific pagesizes"
                return
        }

	set logsets [create_logsets 2]

	# Set up for on-disk or in-memory databases.
	set msg "using on-disk databases"
	if { $databases_in_memory } {
		set msg "using named in-memory databases"
		if { [is_queueext $method] } { 
			puts -nonewline "Skipping rep$tnum for method "
			puts "$method with named in-memory databases."
			return
		}
	}

	set msg2 "and on-disk replication files"
	if { $repfiles_in_memory } {
		set msg2 "and in-memory replication files"
	}

	# Run the body of the test with and without recovery,
	# and with and without cleaning.  Skip recovery with in-memory
	# logging - it doesn't make sense.
	#
	# 'user' means that the "app" (the test in this case) has
	# its own handle open to the database.
	set opts { "" user }
	foreach r $test_recopts {
		foreach o $opts {
			foreach l $logsets {
				set logindex [lsearch -exact $l "in-memory"]
				if { $r == "-recover" && $logindex != -1 } {
					puts "Skipping rep$tnum for -recover\
					    with in-memory logs."
					continue
				}
				puts "Rep$tnum ($method $r $o $args):\
				    Test of internal initialization and\
				    slow client $msg $msg2."
				puts "Rep$tnum: Master logs are [lindex $l 0]"
				puts "Rep$tnum: Client logs are [lindex $l 1]"
				rep060_sub $method $niter $tnum $l $r $o $args
			}
		}
	}
}

proc rep060_sub { method niter tnum logset recargs opt largs } {
	source ./include.tcl
	global databases_in_memory
	global repfiles_in_memory
	global rep_verbose
	global verbose_type

	set verbargs ""
	if { $rep_verbose == 1 } {
		set verbargs " -verbose {$verbose_type on} "
	}

	set repmemargs ""
	if { $repfiles_in_memory } {
		set repmemargs "-rep_inmem_files "
	}

	env_cleanup $testdir

	replsetup $testdir/MSGQUEUEDIR

	set masterdir $testdir/MASTERDIR
	set clientdir $testdir/CLIENTDIR

	file mkdir $masterdir
	file mkdir $clientdir

	# Log size is small so we quickly create more than one.
	# The documentation says that the log file must be at least
	# four times the size of the in-memory log buffer.
	set pagesize 4096
	append largs " -pagesize $pagesize "
	set log_max [expr $pagesize * 4]

	set m_logtype [lindex $logset 0]
	set c_logtype [lindex $logset 1]

	# In-memory logs cannot be used with -txn nosync.
	set m_logargs [adjust_logargs $m_logtype]
	set c_logargs [adjust_logargs $c_logtype]
	set m_txnargs [adjust_txnargs $m_logtype]
	set c_txnargs [adjust_txnargs $c_logtype]

	# Open a master.
	repladd 1
	set ma_envcmd "berkdb_env_noerr -create $m_txnargs \
	    $repmemargs \
	    $m_logargs -log_max $log_max -errpfx MASTER $verbargs \
	    -home $masterdir -rep_transport \[list 1 replsend\]"
	set masterenv [eval $ma_envcmd $recargs -rep_master]

	# Open a client
	puts "\tRep$tnum.a: Open client."
	repladd 2
	set cl_envcmd "berkdb_env_noerr -create $c_txnargs \
	    $repmemargs \
	    $c_logargs -log_max $log_max -errpfx CLIENT $verbargs \
	    -home $clientdir -rep_transport \[list 2 replsend\]"
	set clientenv [eval $cl_envcmd $recargs -rep_client]

	# Bring the client online by processing the startup messages.
	set envlist "{$masterenv 1} {$clientenv 2}"
	process_msgs $envlist

	# Clobber replication's 30-second anti-archive timer, which will have
	# been started by client sync-up internal init.
	#
	$masterenv test force noarchive_timeout

	# Set a low limit so that there are lots of reps between
	# master and client.  This allows greater control over
	# the test.
	error_check_good thr [$masterenv rep_limit 0 [expr 10 * 1024]] 0

	# It is *key* to this test that we have a database handle
	# open for the duration of the test.  The problem this
	# test checks for regards internal init when there are open
	# database handles around.
	#
	if { $databases_in_memory } {
		set dbname { "" "test.db" }
	} else { 
		set dbname "test.db"
	} 
	
	set omethod [convert_method $method]
	set db [eval {berkdb_open_noerr -env $masterenv -auto_commit \
	    -create -mode 0644} $largs $omethod $dbname]
	error_check_good dbopen [is_valid_db $db] TRUE

	# Put some data into the database, running the master up past
	# log file 10, discarding messages to the client so that it will
	# be forced to request them as a gap.
	#
	puts "\tRep$tnum.c: Run rep_test in master env."
	set start 0

	set stop 0
	set endlog 10
	while { $stop == 0 } {
		# Run test in the master (don't update client).
		eval rep_test $method \
		    $masterenv $db $niter $start $start 0 $largs
		incr start $niter
		replclear 2

		if { $m_logtype != "in-memory" } {
			set res \
			    [eval exec $util_path/db_archive -l -h $masterdir]
		}
		# Make sure the master has gone as far as we requested.
		set last_master_log [get_logfile $masterenv last]
		if { $last_master_log > $endlog } {
			set stop 1
		}
	}

	# Do one more set of txns at the master, replicating log records
	# normally, to give the client a chance to notice how many messages
	# it is missing.
	#
	eval rep_test $method $masterenv $db $niter $start $start 0 $largs
	incr start $niter

	set stop 0
	set client_endlog 5
	set last_client_log 0
	set nproced 0
	incr nproced [proc_msgs_once $envlist NONE err]
	incr nproced [proc_msgs_once $envlist NONE err]

	puts "\tRep$tnum.d: Client catches up partway."
	error_check_good ckp [$masterenv txn_checkpoint] 0

	# We have checkpointed on the master, but we want to get the
	# client a healthy way through the logs before archiving on
	# the master.
	while { $stop == 0 } {
		set nproced 0
		incr nproced [proc_msgs_once $envlist NONE err]
		if { $nproced == 0 } {
			error_check_good \
			    ckp [$masterenv txn_checkpoint -force] 0
		}

		# Stop processing when the client is partway through.
		if { $c_logtype != "in-memory" } {
			set res \
			    [eval exec $util_path/db_archive -l -h $clientdir]
		}
		set last_client_log [get_logfile $clientenv last]
		set first_client_log [get_logfile $clientenv first]
		if { $last_client_log > $client_endlog } {
			set stop 1
		}
	}

	#
	# The user may have the database open itself.
	#
	if { $opt == "user" } {
		set cdb [eval {berkdb_open_noerr -env} $clientenv $dbname]
		error_check_good dbopen [is_valid_db $cdb] TRUE
		set ccur [$cdb cursor]
		error_check_good curs [is_valid_cursor $ccur $cdb] TRUE
		set ret [$ccur get -first]
		set kd [lindex $ret 0]
		set key [lindex $kd 0]
		error_check_good cclose [$ccur close] 0
	} else {
		set cdb NULL
	}

	# Now that the client is well on its way of normal processing,
	# simply fairly far behind the master, archive on the master,
	# removing the log files the client needs, sending it into
	# internal init with the database pages reflecting the client's
	# current LSN.
	#
	puts "\tRep$tnum.e: Force internal initialization."
	if { $m_logtype != "in-memory" } {
		puts "\tRep$tnum.e1: Archive on master."
		set res [eval exec $util_path/db_archive -d -h $masterdir]
	} else {
		# Master is in-memory, and we'll need a different
		# technique to create the gap forcing internal init.
		puts "\tRep$tnum.e1: Run rep_test until gap is created."
		set stop 0
		while { $stop == 0 } {
			eval rep_test $method $masterenv \
			    NULL $niter $start $start 0 $largs
			incr start $niter
			set first_master_log [get_logfile $masterenv first]
			if { $first_master_log > $last_client_log } {
				set stop 1
			}
		}
	}

	puts "\tRep$tnum.f: Process messages."
	if { $opt == "user" } {
		for { set loop 0 } { $loop < 5 } { incr loop } {
			set nproced 0
			incr nproced [proc_msgs_once $envlist]
			if { $cdb == "NULL" } {
				continue
			}
			puts "\tRep$tnum.g.$loop: Check user database."
			set status [catch {$cdb get $key} ret]
			if { $status != 0 } {
				#
				# For db operations, DB doesn't block, but
				# returns DEADLOCK.
				#
				set is_lock [is_substr $ret DB_LOCK_DEADLOCK]
				set is_dead [is_substr $ret DB_REP_HANDLE_DEAD]
				error_check_good lock_dead \
				    [expr $is_lock || $is_dead] 1
				if { $is_dead } {
					error_check_good cclose [$cdb close] 0
					set cdb NULL
				}
			}
		}
	}
	process_msgs $envlist

	#
	# If we get through the user loop with a valid db, then it better
	# be a dead handle after we've completed processing all the
	# messages and running recovery.
	#
	if { $cdb != "NULL" } {
		puts "\tRep$tnum.h: Check dead handle."
		set status [catch {$cdb get $key} ret]
		error_check_good status $status 1
		error_check_good is_dead [is_substr $ret DB_REP_HANDLE_DEAD] 1
		error_check_good cclose [$cdb close] 0
		puts "\tRep$tnum.i: Verify correct internal initialization."
	} else {
		puts "\tRep$tnum.h: Verify correct internal initialization."
	}
	error_check_good close [$db close] 0
	process_msgs $envlist

	# We have now forced an internal initialization.  Verify it is correct.
	rep_verify $masterdir $masterenv $clientdir $clientenv 1 1 1

	# Check that logs are in-memory or on-disk as expected.
	check_log_location $masterenv
	check_log_location $clientenv

	error_check_good masterenv_close [$masterenv close] 0
	error_check_good clientenv_close [$clientenv close] 0
	replclose $testdir/MSGQUEUEDIR
}

# See the file LICENSE for redistribution information.
#
# Copyright (c) 2002-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST  	rep003
# TEST	Repeated shutdown/restart replication test
# TEST
# TEST	Run a quick put test in a replicated master environment;
# TEST	start up, shut down, and restart client processes, with
# TEST	and without recovery.  To ensure that environment state
# TEST	is transient, use DB_PRIVATE.

proc rep003 { method { tnum "003" } args } {
	source ./include.tcl
	global rep003_dbname rep003_omethod rep003_oargs
	global repfiles_in_memory

	if { $is_windows9x_test == 1 } {
		puts "Skipping replication test on Win 9x platform."
		return
	}

	# Skip for record-based methods.
	if { $checking_valid_methods } {
		set test_methods {}
		foreach method $valid_methods {
			if { [is_record_based $method] != 1 } {
				lappend test_methods $method
			}
		}
		return $test_methods
	}
	if { [is_record_based $method] } {
		puts "Rep$tnum: Skipping for method $method"
		return
	}

	set msg2 "with on-disk replication files"
	if { $repfiles_in_memory } {
		set msg2 "with in-memory replication files"
	}

	set rep003_dbname rep003.db
	set rep003_omethod [convert_method $method]
	set rep003_oargs [convert_args $method $args]

	# Run the body of the test with and without recovery.  If we're
	# testing in-memory logging, skip the combination of recovery
	# and in-memory logging -- it doesn't make sense.

	set logsets [create_logsets 2]
	foreach recopt $test_recopts {
		foreach l $logsets {
			set logindex [lsearch -exact $l "in-memory"]
			if { $recopt == "-recover" && $logindex != -1 } {
				puts "Rep$tnum: Skipping for\
				    in-memory logs with -recover."
				continue
			}
			puts "Rep$tnum ($method $recopt):\
			    Replication repeated-startup test $msg2."
			puts "Rep$tnum: Master logs are [lindex $l 0]"
			puts "Rep$tnum: Client logs are [lindex $l 1]"
			rep003_sub $method $tnum $l $recopt $args
		}
	}
}

proc rep003_sub { method tnum logset recargs largs } {
	source ./include.tcl
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

	set m_logtype [lindex $logset 0]
	set c_logtype [lindex $logset 1]

	# In-memory logs require a large log buffer, and cannot
	# be used with -txn nosync.  This test already requires
	# -txn, so adjust the logargs only.
	set m_logargs [adjust_logargs $m_logtype]
	set c_logargs [adjust_logargs $c_logtype]

	# Open a master.
	repladd 1
	set env_cmd(M) "berkdb_env_noerr -create -log_max 1000000 \
	    -errpfx MASTER $verbargs $repmemargs \
	    -home $masterdir -txn $m_logargs -rep_master \
	    -rep_transport \[list 1 replsend\]"
	set masterenv [eval $env_cmd(M) $recargs]
	error_check_good master_env [is_valid_env $masterenv] TRUE

	puts "\tRep$tnum.a: Simple client startup test."

	# Put item one.
	rep003_put $masterenv A1 a-one

	# Open a client.
	repladd 2
	set env_cmd(C) "berkdb_env_noerr -create -private -home $clientdir \
	    -txn $c_logargs -errpfx CLIENT $verbargs $repmemargs \
	    -rep_client -rep_transport \[list 2 replsend\]"
	set clientenv [eval $env_cmd(C) $recargs]
	error_check_good client_env [is_valid_env $clientenv] TRUE

	# Put another quick item.
	rep003_put $masterenv A2 a-two

	# Loop, processing first the master's messages, then the client's,
	# until both queues are empty.
	set envlist "{$masterenv 1} {$clientenv 2}"
	process_msgs $envlist

	rep003_check $clientenv A1 a-one
	rep003_check $clientenv A2 a-two

	error_check_good clientenv_close [$clientenv close] 0
	replclear 2

	# Now reopen the client after doing another put.
	puts "\tRep$tnum.b: Client restart."
	rep003_put $masterenv B1 b-one

	set clientenv [eval $env_cmd(C)]
	error_check_good client_env [is_valid_env $clientenv] TRUE

	# Loop letting the client and master sync up and get the
	# environment initialized.  It's a new client env so
	# reinitialize the envlist as well.
	set envlist "{$masterenv 1} {$clientenv 2}"
	process_msgs $envlist

	# The items from part A should be present at all times--
	# if we roll them back, we've screwed up. [#5709]
	rep003_check $clientenv A1 a-one
	rep003_check $clientenv A2 a-two

	rep003_put $masterenv B2 b-two

	# Loop, processing first the master's messages, then the client's,
	# until both queues are empty.
	while { 1 } {
		set nproced 0

		incr nproced [replprocessqueue $masterenv 1]
		incr nproced [replprocessqueue $clientenv 2]

		# The items from part A should be present at all times--
		# if we roll them back, we've screwed up. [#5709]
		rep003_check $clientenv A1 a-one
		rep003_check $clientenv A2 a-two

		if { $nproced == 0 } {
			break
		}
	}

	rep003_check $clientenv B1 b-one
	rep003_check $clientenv B2 b-two

	error_check_good clientenv_close [$clientenv close] 0

	replclear 2

	# Now reopen the client after a recovery.
	puts "\tRep$tnum.c: Client restart after recovery."
	rep003_put $masterenv C1 c-one

	set clientenv [eval $env_cmd(C) -recover]
	error_check_good client_env [is_valid_env $clientenv] TRUE

	# Loop, processing first the master's messages, then the client's,
	# until both queues are empty.
	set envlist "{$masterenv 1} {$clientenv 2}"
	process_msgs $envlist

	# The items from part A should be present at all times--
	# if we roll them back, we've screwed up. [#5709]
	rep003_check $clientenv A1 a-one
	rep003_check $clientenv A2 a-two
	rep003_check $clientenv B1 b-one
	rep003_check $clientenv B2 b-two

	rep003_put $masterenv C2 c-two

	# Loop, processing first the master's messages, then the client's,
	# until both queues are empty.
	while { 1 } {
		set nproced 0

		# The items from part A should be present at all times--
		# if we roll them back, we've screwed up. [#5709]
		rep003_check $clientenv A1 a-one
		rep003_check $clientenv A2 a-two
		rep003_check $clientenv B1 b-one
		rep003_check $clientenv B2 b-two

		incr nproced [replprocessqueue $masterenv 1]
		incr nproced [replprocessqueue $clientenv 2]

		if { $nproced == 0 } {
			break
		}
	}

	rep003_check $clientenv C1 c-one
	rep003_check $clientenv C2 c-two

	error_check_good clientenv_close [$clientenv close] 0

	replclear 2

	# Now reopen the client after a catastrophic recovery.
	puts "\tRep$tnum.d: Client restart after catastrophic recovery."
	rep003_put $masterenv D1 d-one

	set clientenv [eval $env_cmd(C) -recover_fatal]
	error_check_good client_env [is_valid_env $clientenv] TRUE

	# Loop, processing first the master's messages, then the client's,
	# until both queues are empty.
	set envlist "{$masterenv 1} {$clientenv 2}"
	process_msgs $envlist
	rep003_put $masterenv D2 d-two

	# Loop, processing first the master's messages, then the client's,
	# until both queues are empty.
	while { 1 } {
		set nproced 0

		# The items from part A should be present at all times--
		# if we roll them back, we've screwed up. [#5709]
		rep003_check $clientenv A1 a-one
		rep003_check $clientenv A2 a-two
		rep003_check $clientenv B1 b-one
		rep003_check $clientenv B2 b-two
		rep003_check $clientenv C1 c-one
		rep003_check $clientenv C2 c-two

		incr nproced [replprocessqueue $masterenv 1]
		incr nproced [replprocessqueue $clientenv 2]

		if { $nproced == 0 } {
			break
		}
	}

	rep003_check $clientenv D1 d-one
	rep003_check $clientenv D2 d-two

	error_check_good clientenv_close [$clientenv close] 0

	error_check_good masterenv_close [$masterenv close] 0
	replclose $testdir/MSGQUEUEDIR
}

proc rep003_put { masterenv key data } {
	global rep003_dbname rep003_omethod rep003_oargs

	set db [eval {berkdb_open_noerr -create -env $masterenv -auto_commit} \
	    $rep003_omethod $rep003_oargs $rep003_dbname]
	error_check_good rep3_put_open($key,$data) [is_valid_db $db] TRUE

	set txn [$masterenv txn]
	error_check_good rep3_put($key,$data) [$db put -txn $txn $key $data] 0
	error_check_good rep3_put_txn_commit($key,$data) [$txn commit] 0

	error_check_good rep3_put_close($key,$data) [$db close] 0
}

proc rep003_check { env key data } {
	global rep003_dbname

	set db [berkdb_open_noerr -rdonly -env $env $rep003_dbname]
	error_check_good rep3_check_open($key,$data) [is_valid_db $db] TRUE

	set dbt [$db get $key]
	error_check_good rep3_check($key,$data) \
	    [lindex [lindex $dbt 0] 1] $data

	error_check_good rep3_put_close($key,$data) [$db close] 0
}

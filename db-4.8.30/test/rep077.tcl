# See the file LICENSE for redistribution information.
#
# Copyright (c) 2001-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST  rep077
# TEST
# TEST	Replication, recovery and applying log records immediately.
# TEST	Master and 1 client.  Start up both sites.
# TEST	Close client and run rep_test on the master so that the
# TEST	log record is the same LSN the client would be expecting.
# TEST	Reopen client with recovery and verify the client does not
# TEST	try to apply that "expected" record before it synchronizes
# TEST	with the master.
#
proc rep077 { method { tnum "077"} args} {
	source ./include.tcl
	global databases_in_memory
	global repfiles_in_memory

	if { $is_windows9x_test == 1 } {
		puts "Skipping replication test on Win9x platform."
		return
	}

	# Valid for all access methods.
	if { $checking_valid_methods } {
		return "ALL"
	}

	set args [convert_args $method $args]
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

	foreach l $logsets {
		puts "Rep$tnum ($method): Recovered client\
		    getting immediate log records $msg $msg2."
		puts "Rep$tnum: Master logs are [lindex $l 0]"
		puts "Rep$tnum: Client logs are [lindex $l 1]"
		rep077_sub $method $tnum $l $args
	}
}

proc rep077_sub { method tnum logset largs} {
	global testdir
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

	set niter 5
	env_cleanup $testdir

	replsetup $testdir/MSGQUEUEDIR

	set masterdir $testdir/MASTERDIR
	set clientdir $testdir/CLIENTDIR

	file mkdir $masterdir
	file mkdir $clientdir

        set m_logtype [lindex $logset 0]
	set c_logtype [lindex $logset 1]

	# In-memory logs cannot be used with -txn nosync.
	set m_logargs [adjust_logargs $m_logtype]
	set c_logargs [adjust_logargs $c_logtype]
	set m_txnargs [adjust_txnargs $m_logtype]
	set c_txnargs [adjust_txnargs $c_logtype]

	# Open a master.
	repladd 1
	set env_cmd(M) "berkdb_env_noerr -create \
	    $verbargs $repmemargs \
	    -home $masterdir -errpfx MASTER -txn nosync -rep_master \
	    -rep_transport \[list 1 replsend\]"
	set masterenv [eval $env_cmd(M)]

	eval rep_test $method $masterenv NULL $niter 0 0 0 $largs

	# Open a client
	repladd 2
	set env_cmd(C) "berkdb_env_noerr -create \
	    $verbargs $repmemargs \
	    -home $clientdir -errpfx CLIENT -txn nosync -rep_client \
	    -rep_transport \[list 2 replsend\]"
	set clientenv [eval $env_cmd(C)]

	puts "\tRep$tnum.a: Start up master and client."
	# Bring the client online by processing the startup messages.
	set envlist "{$masterenv 1} {$clientenv 2}"
	process_msgs $envlist

	puts "\tRep$tnum.b: Close client."
	$clientenv close

	#
	# We want to run rep_test now and DO NOT replclear the
	# messages for the closed client.  We want to make sure
	# that the first message the client sees upon restarting
	# is a log record that exactly matches the current
	# expected LSN.
	#
	puts "\tRep$tnum.c: Run rep_test on master with client closed."
	#
	# Move it forward by sending in niter as start and skip.
	#
	eval rep_test $method $masterenv NULL $niter $niter $niter 0 $largs

	# We need to reopen with recovery to blow away our idea of
	# who the master is, because this client will start up with
	# the right generation number and the ready_lsn will be
	# set to the right value for the first log record to apply.
	#
	# However, this client is running recovery and will have
	# written its own recovery log records.  So, until this
	# client finds and synchronizes with the master after
	# restarting, its ready_lsn and lp->lsn will not be
	# in sync and this client better not try to apply the records.
	#
	puts "\tRep$tnum.d: Restart client with recovery and process messages."
	set clientenv [eval $env_cmd(C) -recover]
	set envlist "{$masterenv 1} {$clientenv 2}"
	process_msgs $envlist

	#
	# If we didn't crash at this point, we're okay.  
	#
	$masterenv close
	$clientenv close
	replclose $testdir/MSGQUEUEDIR
}

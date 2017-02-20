# See the file LICENSE for redistribution information.
#
# Copyright (c) 2003-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	rep011
# TEST	Replication: test open handle across an upgrade.
# TEST
# TEST	Open and close test database in master environment.
# TEST	Update the client.  Check client, and leave the handle
# TEST 	to the client open as we close the masterenv and upgrade
# TEST	the client to master.  Reopen the old master as client
# TEST	and catch up.  Test that we can still do a put to the
# TEST	handle we created on the master while it was still a
# TEST	client, and then make sure that the change can be
# TEST 	propagated back to the new client.

proc rep011 { method { tnum "011" } args } {
	global has_crypto
	global passwd
	global repfiles_in_memory

	source ./include.tcl
	if { $is_windows9x_test == 1 } {
		puts "Skipping replication test on Win 9x platform."
		return
	}

	# Run for all access methods.
	if { $checking_valid_methods } {
		return "ALL"
	}

	set msg2 "and on-disk replication files"
	if { $repfiles_in_memory } {
		set msg2 "and in-memory replication files"
	}

	set logsets [create_logsets 2]
	foreach r $test_recopts {
		foreach l $logsets {
			set logindex [lsearch -exact $l "in-memory"]
			if { $r == "-recover" && $logindex != -1 } {
				puts "Rep$tnum: Skipping\
				    for in-memory logs with -recover."
				continue
			}
			set envargs ""
			puts "Rep$tnum.a ($r $envargs $method):\
			    Test upgrade of open handles $msg2."
			puts "Rep$tnum: Master logs are [lindex $l 0]"
			puts "Rep$tnum: Client logs are [lindex $l 1]"
			rep011_sub $method $tnum $envargs $l $r $args

			if { $has_crypto == 0 } {
				continue
			}
			append envargs " -encryptaes $passwd "
			append args " -encrypt "

			puts "Rep$tnum.b ($r $envargs):\
			    Open handle upgrade test with encryption ($method)."
			puts "Rep$tnum: Master logs are [lindex $l 0]"
			puts "Rep$tnum: Client logs are [lindex $l 1]"
			rep011_sub $method $tnum $envargs $l $r $args
		}
	}
}

proc rep011_sub { method tnum envargs logset recargs largs } {
	source ./include.tcl
	global testdir
	global encrypt
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
	# be used with -txn nosync.
	set m_logargs [adjust_logargs $m_logtype]
	set c_logargs [adjust_logargs $c_logtype]
	set m_txnargs [adjust_txnargs $m_logtype]
	set c_txnargs [adjust_txnargs $c_logtype]

	# Open a master.
	repladd 1
	set env_cmd(M) "berkdb_env_noerr -create -log_max 1000000 \
	    $m_logargs $envargs $verbargs -home $masterdir $repmemargs \
	    $m_txnargs -errpfx MASTER -rep_master -rep_transport \
	    \[list 1 replsend\]"
	set masterenv [eval $env_cmd(M) $recargs]

	# Open a client
	repladd 2
	set env_cmd(C) "berkdb_env_noerr -create \
	    $c_logargs $envargs $verbargs -home $clientdir $repmemargs \
	    $c_txnargs -errpfx CLIENT -rep_client -rep_transport \
	    \[list 2 replsend\]"
	set clientenv [eval $env_cmd(C) $recargs]

	# Bring the client online by processing the startup messages.
	set envlist "{$masterenv 1} {$clientenv 2}"
	process_msgs $envlist

	# Open a test database on the master so we can test having
	# handles open across an upgrade.
	puts "\tRep$tnum.a:\
	    Opening test database for post-upgrade client logging test."
	set master_upg_db [berkdb_open_noerr \
	    -create -auto_commit -btree -env $masterenv rep$tnum-upg.db]
	set puttxn [$masterenv txn]
	error_check_good master_upg_db_put \
	    [$master_upg_db put -txn $puttxn hello world] 0
	error_check_good puttxn_commit [$puttxn commit] 0
	error_check_good master_upg_db_close [$master_upg_db close] 0

	# Update the client.
	process_msgs $envlist

	# Open the cross-upgrade database on the client and check its contents.
	set client_upg_db [berkdb_open_noerr \
	     -create -auto_commit -btree -env $clientenv rep$tnum-upg.db]
	error_check_good client_upg_db_get [$client_upg_db get hello] \
	     [list [list hello world]]
	# !!! We use this handle later.  Don't close it here.

	# Close master.
	puts "\tRep$tnum.b: Close master."
	error_check_good masterenv_close [$masterenv close] 0

	puts "\tRep$tnum.c: Upgrade client."
	set newmasterenv $clientenv
	error_check_good upgrade_client [$newmasterenv rep_start -master] 0

	puts "\tRep$tnum.d: Reopen old master as client and catch up."
	set newclientenv [eval {berkdb_env_noerr -create -recover} $envargs \
	    -txn nosync -errpfx NEWCLIENT $verbargs \
	    {-home $masterdir -rep_client -rep_transport [list 1 replsend]}]
	set envlist "{$newclientenv 1} {$newmasterenv 2}"
	process_msgs $envlist

	# Test put to the database handle we opened back when the new master
	# was a client.
	puts "\tRep$tnum.e: Test put to handle opened before upgrade."
	set puttxn [$newmasterenv txn]
	error_check_good client_upg_db_put \
	    [$client_upg_db put -txn $puttxn hello there] 0
	error_check_good puttxn_commit [$puttxn commit] 0
	process_msgs $envlist

	# Close the new master's handle for the upgrade-test database;  we
	# don't need it.  Then check to make sure the client did in fact
	# update the database.
	puts "\tRep$tnum.f: Test that client did update the database."
	error_check_good client_upg_db_close [$client_upg_db close] 0
	set newclient_upg_db \
	    [berkdb_open_noerr -env $newclientenv rep$tnum-upg.db]
	error_check_good newclient_upg_db_get [$newclient_upg_db get hello] \
	    [list [list hello there]]
	error_check_good newclient_upg_db_close [$newclient_upg_db close] 0

	error_check_good newmasterenv_close [$newmasterenv close] 0
	error_check_good newclientenv_close [$newclientenv close] 0

	if { [lsearch $envargs "-encrypta*"] !=-1 } {
		set encrypt 1
	}
	error_check_good verify \
	    [verify_dir $clientdir "\tRep$tnum.g: " 0 0 1] 0
	replclose $testdir/MSGQUEUEDIR
}

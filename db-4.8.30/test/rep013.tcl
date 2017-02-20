# See the file LICENSE for redistribution information.
#
# Copyright (c) 2001-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	rep013
# TEST	Replication and swapping master/clients with open dbs.
# TEST
# TEST	Run a modified version of test001 in a replicated master env.
# TEST	Make additional changes to master, but not to the client.
# TEST	Swap master and client.
# TEST	Verify that the roll back on clients gives dead db handles.
# TEST	Rerun the test, turning on client-to-client synchronization.
# TEST	Swap and verify several times.
proc rep013 { method { niter 10 } { tnum "013" } args } {

	source ./include.tcl
	global databases_in_memory 
	global repfiles_in_memory

	if { $is_windows9x_test == 1 } {
		puts "Skipping replication test on Win 9x platform."
		return
	}

	# Run for all access methods.
	if { $checking_valid_methods } {
		return "ALL"
	}

	set args [convert_args $method $args]
	set logsets [create_logsets 3]

	# Set up named in-memory database testing. 
	set msg "using on-disk databases"
	if { $databases_in_memory } {
		set msg "using named in-memory databases"
		if { [is_queueext $method] } { 
			puts -nonewline "Skipping rep$tnum for method "
			puts "$method with named in-memory databases"
			return
		}
	}

	set msg2 "and on-disk replication files"
	if { $repfiles_in_memory } {
		set msg2 "and in-memory replication files"
	}

	# Run the body of the test with and without recovery.
	set anyopts { "" "anywhere" }
	foreach r $test_recopts {
		foreach l $logsets {
			foreach a $anyopts {
				set logindex [lsearch -exact $l "in-memory"]
				if { $r == "-recover" && $logindex != -1 } {
					puts "Rep$tnum: Skipping\
					    for in-memory logs with -recover."
					continue
				}
				puts "Rep$tnum ($r $a): Replication and \
				    ($method) master/client swapping $msg $msg2."
				puts "Rep$tnum: Master logs are [lindex $l 0]"
				puts "Rep$tnum: Client 0 logs are [lindex $l 1]"
				puts "Rep$tnum: Client 1 logs are [lindex $l 2]"
				rep013_sub $method $niter $tnum $l $r $a $args
			}
		}
	}
}

proc rep013_sub { method niter tnum logset recargs anyopt largs } {
	global testdir
	global anywhere
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
	set orig_tdir $testdir

	replsetup $testdir/MSGQUEUEDIR

	set masterdir $testdir/MASTERDIR
	set clientdir $testdir/CLIENTDIR
	set clientdir2 $testdir/CLIENTDIR.2
	file mkdir $masterdir
	file mkdir $clientdir
	file mkdir $clientdir2

	if { $anyopt == "anywhere" } {
		set anywhere 1
	} else {
		set anywhere 0
	}
	set m_logtype [lindex $logset 0]
	set c_logtype [lindex $logset 1]
	set c2_logtype [lindex $logset 2]

	# In-memory logs require a large log buffer, and cannot
	# be used with -txn nosync.
	set m_logargs [adjust_logargs $m_logtype]
	set c_logargs [adjust_logargs $c_logtype]
	set c2_logargs [adjust_logargs $c2_logtype]
	set m_txnargs [adjust_txnargs $m_logtype]
	set c_txnargs [adjust_txnargs $c_logtype]
	set c2_txnargs [adjust_txnargs $c2_logtype]

	# Set number of swaps between master and client.
	set nswap 6

	# Open a master.
	repladd 1
	set ma_envcmd "berkdb_env_noerr -create $m_txnargs \
	    $m_logargs -errpfx ENV1 $verbargs $repmemargs \
	    -cachesize {0 4194304 3} \
	    -home $masterdir -rep_transport \[list 1 replsend\]"
	set env1 [eval $ma_envcmd $recargs -rep_master]

	# Open two clients
	repladd 2
	set cl_envcmd "berkdb_env_noerr -create $c_txnargs \
	    $c_logargs -errpfx ENV2 $verbargs $repmemargs \
	    -cachesize {0 2097152 2} \
	    -home $clientdir -rep_transport \[list 2 replsend\]"
	set env2 [eval $cl_envcmd $recargs -rep_client]

	repladd 3
	set cl2_envcmd "berkdb_env_noerr -create $c2_txnargs \
	    $c2_logargs -errpfx ENV3 $verbargs $repmemargs \
	    -cachesize {0 1048576 1} \
	    -home $clientdir2 -rep_transport \[list 3 replsend\]"
	set cl2env [eval $cl2_envcmd $recargs -rep_client]

	# Set database name for in-memory or on-disk.
	if { $databases_in_memory } {
		set testfile { "" "test.db" }
	} else { 
		set testfile "test.db"
	} 

	set omethod [convert_method $method]

	set env1db_cmd "berkdb_open_noerr -env $env1 -auto_commit \
	     -create -mode 0644 $largs $omethod $testfile"
	set env1db [eval $env1db_cmd]
	error_check_good dbopen [is_valid_db $env1db] TRUE

	#
	# Verify that a client creating a database gets an error.
	#
	set stat [catch {berkdb_open_noerr -env $env2 -auto_commit \
	     -create -mode 0644 $largs $omethod $testfile} ret]
	error_check_good create_cl $stat 1
	error_check_good cr_str [is_substr $ret "invalid"] 1

	# Bring the clients online by processing the startup messages.
	set envlist "{$env1 1} {$env2 2} {$cl2env 3}"
	process_msgs $envlist

	set env2db_cmd "berkdb_open_noerr -env $env2 -auto_commit \
	     -mode 0644 $largs $omethod $testfile"
	set env2db [eval $env2db_cmd]
	error_check_good dbopen [is_valid_db $env2db] TRUE
	set env3db_cmd "berkdb_open_noerr -env $cl2env -auto_commit \
	     -mode 0644 $largs $omethod $testfile"
	set env3db [eval $env3db_cmd]
	error_check_good dbopen [is_valid_db $env3db] TRUE

	#
	# Set up all the master/client data we're going to need
	# to keep track of and swap.
	#
	set masterenv $env1
	set masterdb $env1db
	set mid 1
	set clientenv $env2
	set clientdb $env2db
	set cid 2
	set mdb_cmd "berkdb_open_noerr -env $masterenv -auto_commit \
	     -mode 0644 $largs $omethod $testfile"
	set cdb_cmd "berkdb_open_noerr -env $clientenv -auto_commit \
	     -mode 0644 $largs $omethod $testfile"

	# Run a modified test001 in the master (and update clients).
	puts "\tRep$tnum.a: Running test001 in replicated env."
	eval rep_test $method $masterenv $masterdb $niter 0 0 0 $largs
	set envlist "{$env1 1} {$env2 2} {$cl2env 3}"
	process_msgs $envlist

	set nstart 0
	for { set i 0 } { $i < $nswap } { incr i } {
		puts "\tRep$tnum.b.$i: Check for bad db handles"
		set dbl {masterdb clientdb env3db}
		set dbcmd {$mdb_cmd $cdb_cmd $env3db_cmd}

		set stat [catch {$masterdb stat} ret]
		if { $stat == 1 } {
			error_check_good dead [is_substr $ret \
			    DB_REP_HANDLE_DEAD] 1
			error_check_good close [$masterdb close] 0
			set masterdb [eval $mdb_cmd]
			error_check_good dbopen [is_valid_db $masterdb] TRUE
		}

		set stat [catch {$clientdb stat} ret]
		if { $stat == 1 } {
			error_check_good dead [is_substr $ret \
			    DB_REP_HANDLE_DEAD] 1
			error_check_good close [$clientdb close] 0
			set clientdb [eval $cdb_cmd]
			error_check_good dbopen [is_valid_db $clientdb] TRUE
		}

		set stat [catch {$env3db stat} ret]
		if { $stat == 1 } {
			error_check_good dead [is_substr $ret \
			    DB_REP_HANDLE_DEAD] 1
			error_check_good close [$env3db close] 0
			set env3db [eval $env3db_cmd]
			error_check_good dbopen [is_valid_db $env3db] TRUE
		}

		set nstart [expr $nstart + $niter]
		puts "\tRep$tnum.c.$i: Run test in master and client2 only"
		eval rep_test \
		    $method $masterenv $masterdb $niter $nstart $nstart 0 $largs
		set envlist "{$masterenv $mid} {$cl2env 3}"
		process_msgs $envlist

		# Nuke those for client about to become master.
		replclear $cid

		# Swap all the info we need.
		set tmp $masterenv
		set masterenv $clientenv
		set clientenv $tmp

		set tmp $masterdb
		set masterdb $clientdb
		set clientdb $tmp

		set tmp $mid
		set mid $cid
		set cid $tmp

		set tmp $mdb_cmd
		set mdb_cmd $cdb_cmd
		set cdb_cmd $tmp

		puts "\tRep$tnum.d.$i: Swap: master $mid, client $cid"
		error_check_good downgrade [$clientenv rep_start -client] 0
		error_check_good upgrade [$masterenv rep_start -master] 0
		set envlist "{$env1 1} {$env2 2} {$cl2env 3}"
		process_msgs $envlist
	}
	puts "\tRep$tnum.e: Check message handling of client."
	set req3 [stat_field $cl2env rep_stat "Client service requests"]
	set rereq1 [stat_field $env1 rep_stat "Client rerequests"]
	set rereq2 [stat_field $env2 rep_stat "Client rerequests"]
	if { $anyopt == "anywhere" } {
		error_check_bad req $req3 0
		error_check_bad rereq1 $rereq1 0
		error_check_bad rereq2 $rereq2 0
	} else {
		error_check_good req $req3 0
		error_check_good rereq1 $rereq1 0
		error_check_good rereq2 $rereq2 0
	}

	# Check that databases are in-memory or on-disk as expected.
	check_db_location $env1 
	check_db_location $env2 
	check_db_location $cl2env
	
	puts "\tRep$tnum.f: Closing"
	error_check_good masterdb [$masterdb close] 0
	error_check_good clientdb [$clientdb close] 0
	error_check_good cl2db [$env3db close] 0
	error_check_good env1_close [$env1 close] 0
	error_check_good env2_close [$env2 close] 0
	error_check_good cl2_close [$cl2env close] 0
	replclose $testdir/MSGQUEUEDIR
	set testdir $orig_tdir
	set anywhere 0
	return
}

# See the file LICENSE for redistribution information.
#
# Copyright (c) 2001-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	rep012
# TEST	Replication and dead DB handles.
# TEST
# TEST	Run a modified version of test001 in a replicated master env.
# TEST	Run in replicated environment with secondary indices too.
# TEST	Make additional changes to master, but not to the client.
# TEST	Downgrade the master and upgrade the client with open db handles.
# TEST	Verify that the roll back on clients gives dead db handles.
proc rep012 { method { niter 10 } { tnum "012" } args } {

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

	# Run the body of the test with and without recovery.
	foreach r $test_recopts {
		foreach l $logsets {
			set logindex [lsearch -exact $l "in-memory"]
			if { $r == "-recover" && $logindex != -1 } {
				puts "Rep$tnum: Skipping\
				    for in-memory logs with -recover."
				continue
			}
			puts "Rep$tnum ($method $r):\
			    Replication and dead db handles $msg $msg2."
			puts "Rep$tnum: Master logs are [lindex $l 0]"
			puts "Rep$tnum: Client 0 logs are [lindex $l 1]"
			puts "Rep$tnum: Client 1 logs are [lindex $l 2]"
			rep012_sub $method $niter $tnum $l $r $args
		}
	}
}

proc rep012_sub { method niter tnum logset recargs largs } {
	global testdir
	global databases_in_memory
	global repfiles_in_memory
	global verbose_check_secondaries
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

	# Open a master.
	repladd 1
	set ma_envcmd "berkdb_env_noerr -create $m_txnargs \
	    $m_logargs -errpfx ENV0 $verbargs $repmemargs \
	    -home $masterdir -rep_transport \[list 1 replsend\]"
	set env0 [eval $ma_envcmd $recargs -rep_master]
	set masterenv $env0

	# Open two clients
	repladd 2
	set cl_envcmd "berkdb_env_noerr -create $c_txnargs \
	    $c_logargs -errpfx ENV1 $verbargs $repmemargs \
	    -home $clientdir -rep_transport \[list 2 replsend\]"
	set env1 [eval $cl_envcmd $recargs -rep_client]
	set clientenv $env1

	repladd 3
	set cl2_envcmd "berkdb_env_noerr -create $c2_txnargs \
	    $c2_logargs -errpfx ENV2 $verbargs $repmemargs \
	    -home $clientdir2 -rep_transport \[list 3 replsend\]"
	set cl2env [eval $cl2_envcmd $recargs -rep_client]

	if { $databases_in_memory } {
		set testfile { "" test$tnum.db }
		set pname { "" primary$tnum.db }
		set sname { "" secondary$tnum.db }
	} else {
		set testfile "test$tnum.db"
		set pname "primary$tnum.db"
		set sname "secondary$tnum.db"
	}
	set omethod [convert_method $method]
	set env0db [eval {berkdb_open_noerr -env $env0 -auto_commit \
	    -create -mode 0644} $largs $omethod $testfile]
	error_check_good dbopen [is_valid_db $env0db] TRUE
	set masterdb $env0db

	set do_secondary 0
	if { [is_btree $method] || [is_hash $method] } {
		set do_secondary 1
		# Open the primary
		set mpdb [eval {berkdb_open_noerr -env $env0 -auto_commit \
		    -create -mode 0644} $largs $omethod $pname]
		error_check_good dbopen [is_valid_db $mpdb] TRUE

		# Open the secondary
		# Open a 2nd handle to the same secondary
		set msdb [eval {berkdb_open_noerr -env $env0 -auto_commit \
		    -create -mode 0644} $largs $omethod $sname]
		error_check_good dbopen [is_valid_db $msdb] TRUE
		error_check_good associate [$mpdb associate \
		    [callback_n 0] $msdb] 0
	}

	# Bring the clients online by processing the startup messages.
	set envlist "{$env0 1} {$env1 2} {$cl2env 3}"
	process_msgs $envlist

	set env1db [eval {berkdb_open_noerr -env $env1 -auto_commit \
	     -mode 0644} $largs $omethod $testfile]
	set clientdb $env1db
	error_check_good dbopen [is_valid_db $env1db] TRUE
	set env2db [eval {berkdb_open_noerr -env $cl2env -auto_commit \
	     -mode 0644} $largs $omethod $testfile]
	error_check_good dbopen [is_valid_db $env2db] TRUE

	# Run a modified test001 in the master (and update clients).
	puts "\tRep$tnum.a.0: Running rep_test in replicated env."
	eval rep_test $method $masterenv $masterdb $niter 0 0 0 $largs
	process_msgs $envlist

	if { $do_secondary } {
		# Put some data into the primary
		puts "\tRep$tnum.a.1: Putting primary/secondary data on master."
		eval rep012_sec $method $mpdb $niter keys data
		process_msgs $envlist

		set verbose_check_secondaries 1
		check_secondaries $mpdb $msdb $niter keys data "Rep$tnum.b"
	} else {
		puts "\tRep$tnum.b: Skipping secondaries for method $method"
	}

	# Check that databases are in-memory or on-disk as expected.
	# We can only check the secondaries if secondaries are allowed for 
	# this access method. 
	set names [list $testfile]
	if { $do_secondary } {
		lappend names $pname $sname
	}
	foreach name $names {
		eval check_db_location $masterenv $name
		eval check_db_location $clientenv $name
		eval check_db_location $cl2env $name
	}
	
	puts "\tRep$tnum.c: Run test in master and client 2 only"
	set nstart $niter
	eval rep_test\
	    $method $masterenv $masterdb $niter $nstart $nstart 0 $largs

	# Ignore messages for $env1.
	set envlist "{$env0 1} {$cl2env 3}"
	process_msgs $envlist

	# Nuke those for client about to become master.
	replclear 2
	tclsleep 3
	puts "\tRep$tnum.d: Swap envs"
	set tmp $masterenv
	set masterenv $clientenv
	set clientenv $tmp
	error_check_good downgrade [$clientenv rep_start -client] 0
	error_check_good upgrade [$masterenv rep_start -master] 0
	set envlist "{$env0 1} {$env1 2} {$cl2env 3}"
	process_msgs $envlist

	#
	# At this point, env0 should have rolled back across a txn commit.
	# If we do any operation on env0db, we should get an error that
	# the handle is dead.
	puts "\tRep$tnum.e: Try to access db handle after rollback"
	set stat1 [catch {$env0db stat} ret1]
	error_check_good stat1 $stat1 1
	error_check_good dead1 [is_substr $ret1 DB_REP_HANDLE_DEAD] 1

	set stat3 [catch {$env2db stat} ret3]
	error_check_good stat3 $stat3 1
	error_check_good dead3 [is_substr $ret3 DB_REP_HANDLE_DEAD] 1

	if { $do_secondary } {
		#
		# Check both secondary get and close to detect DEAD_HANDLE.
		#
		puts "\tRep$tnum.f: Try to access secondary db handles after rollback"
		set verbose_check_secondaries 1
		check_secondaries $mpdb $msdb $niter \
		    keys data "Rep$tnum.f" errp errs errsg
		error_check_good deadp [is_substr $errp DB_REP_HANDLE_DEAD] 1
		error_check_good deads [is_substr $errs DB_REP_HANDLE_DEAD] 1
		error_check_good deadsg [is_substr $errsg DB_REP_HANDLE_DEAD] 1
		puts "\tRep$tnum.g: Closing"
		error_check_good mpdb [$mpdb close] 0
		error_check_good msdb [$msdb close] 0
	} else {
		puts "\tRep$tnum.f: Closing"
	}

	error_check_good env0db [$env0db close] 0
	error_check_good env1db [$env1db close] 0
	error_check_good cl2db [$env2db close] 0
	error_check_good env0_close [$env0 close] 0
	error_check_good env1_close [$env1 close] 0
	error_check_good cl2_close [$cl2env close] 0
	replclose $testdir/MSGQUEUEDIR
	set verbose_check_secondaries 0
	set testdir $orig_tdir
	return
}

proc rep012_sec {method pdb niter keysp datap} {
	source ./include.tcl

	upvar $keysp keys
	upvar $datap data
	set did [open $dict]
	for { set n 0 } { [gets $did str] != -1 && $n < $niter } { incr n } {
		if { [is_record_based $method] == 1 } {
			set key [expr $n + 1]
			set datum $str
		} else {
			set key $str
			gets $did datum
		}
		set keys($n) $key
		set data($n) [pad_data $method $datum]

		set ret [$pdb put $key [chop_data $method $datum]]
		error_check_good put($n) $ret 0
	}
	close $did
}

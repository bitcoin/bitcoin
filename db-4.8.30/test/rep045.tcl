# See the file LICENSE for redistribution information.
#
# Copyright (c) 2005-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	rep045
# TEST
# TEST	Replication with versions.
# TEST
# TEST	Mimic an application where a database is set up in the
# TEST	background and then put into a replication group for use.
# TEST	The "version database" identifies the current live
# TEST	version, the database against which queries are made.
# TEST	For example, the version database might say the current
# TEST	version is 3, and queries would then be sent to db.3.
# TEST	Version 4 is prepared for use while version 3 is in use.
# TEST	When version 4 is complete, the version database is updated
# TEST	to point to version 4 so queries can be directed there.
# TEST
# TEST	This test has a master and two clients.  One client swaps
# TEST	roles with the master, and the other client runs constantly
# TEST	in another process.

proc rep045 { method { tnum "045" } args } {

	source ./include.tcl
	global databases_in_memory
	global repfiles_in_memory

	if { $is_windows9x_test == 1 } {
		puts "Skipping replication test on Win 9x platform."
		return
	}

	# Valid for all access methods.
	if { $checking_valid_methods } {
		return "ALL"
	}

	set args [convert_args $method $args]
	set logsets [create_logsets 3]

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
		set logindex [lsearch -exact $l "in-memory"]
		puts "Rep$tnum ($method): Replication with version\
		    databases $msg $msg2."
		puts "Rep$tnum: Master logs are [lindex $l 0]"
		puts "Rep$tnum: Client 0 logs are [lindex $l 1]"
		puts "Rep$tnum: Client 1 logs are [lindex $l 2]"
		rep045_sub $method $tnum $l $args
	}
}

proc rep045_sub { method tnum logset largs } {
	source ./include.tcl
	set orig_tdir $testdir
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

	set masterdir $testdir/MASTERDIR
	set clientdir0 $testdir/CLIENTDIR0
	set clientdir1 $testdir/CLIENTDIR1

	env_cleanup $testdir
	replsetup $testdir/MSGQUEUEDIR
	file mkdir $masterdir
	file mkdir $clientdir0
	file mkdir $clientdir1

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

	set omethod [convert_method $method]

	# Open a master.
	repladd 1
	set envcmd(M0) "berkdb_env_noerr -create $m_txnargs \
	    $m_logargs -errpfx ENV.M0 $verbargs $repmemargs \
	    -errfile /dev/stderr -lock_detect default \
	    -home $masterdir -rep_transport \[list 1 replsend\]"
	set menv [eval $envcmd(M0) -rep_master]

	# Open a client
	repladd 2
	set envcmd(C0) "berkdb_env_noerr -create $c_txnargs \
	    $c_logargs -errpfx ENV.C0 $verbargs $repmemargs \
	    -errfile /dev/stderr -lock_detect default \
	    -home $clientdir0 -rep_transport \[list 2 replsend\]"
	set cenv0 [eval $envcmd(C0) -rep_client]

	# Open second client.
	repladd 3
	set envcmd(C1) "berkdb_env_noerr -create $c2_txnargs \
	    $c2_logargs -errpfx ENV.C1 $verbargs $repmemargs \
	    -errfile /dev/stderr -lock_detect default \
	    -home $clientdir1 -rep_transport \[list 3 replsend\]"
	set cenv1 [eval $envcmd(C1) -rep_client]

	# Bring the clients online by processing the startup messages.
	set envlist "{$menv 1} {$cenv0 2} {$cenv1 3}"
	process_msgs $envlist

	# Clobber replication's 30-second anti-archive timer, which will have
	# been started by client sync-up internal init, so that we can do a
	# db_remove in a moment.
	#
	$menv test force noarchive_timeout

	puts "\tRep$tnum.a: Initialize version database."
	# Set up variables so we cycle through version numbers 1
	# through maxversion several times.
	if { $databases_in_memory } {
		set vname { "" "version.db" }
	} else {
		set vname "version.db"
	}
	set version 0
	set maxversion 5
	set iter 12
	set nentries 100
	set start 0

	# The version db is always btree.
	set vdb [eval {berkdb_open_noerr -env $menv -create \
	    -auto_commit -mode 0644} -btree $vname]
	error_check_good init_version [$vdb put VERSION $version] 0
	error_check_good vdb_close [$vdb close] 0
	process_msgs $envlist

	# Start up a separate process that constantly reads data
	# from the current official version.
	puts "\tRep$tnum.b: Spawn a child tclsh to do client work."
	set pid [exec $tclsh_path $test_path/wrap.tcl \
	    rep045script.tcl $testdir/rep045script.log \
		   $clientdir1 $vname $databases_in_memory &]

	# Main loop: update query database, process messages (or don't,
	# simulating a failure), announce the new version, process
	# messages (or don't), and swap masters.
	set version 1
	for { set i 1 } { $i < $iter } { incr i } {

		# If database.N exists on disk, clean it up.
		if { $databases_in_memory } {
			set dbname { "" "db.$version" }
		} else {
			set dbname "db.$version"
		}
		if { [file exists $masterdir/$dbname] == 1 } {
			puts "\tRep$tnum.c.$i: Removing old version $version."
			error_check_good dbremove \
			   [$menv dbremove -auto_commit $dbname] 0
		}

		puts "\tRep$tnum.c.$i: Set up query database $version."
		set db [eval berkdb_open_noerr -create -env $menv\
		    -auto_commit -mode 0644 $largs $omethod $dbname]
		error_check_good db_open [is_valid_db $db] TRUE
		eval rep_test $method $menv $db $nentries $start $start 0 $largs
		incr start $nentries
		error_check_good db_close [$db close] 0

		# We alternate between processing the messages and
		# clearing the messages to simulate a failure.

		set process [expr $i % 2]
		if { $process == 1 } {
			process_msgs $envlist
		} else {
			replclear 2
			replclear 3
		}

		# Announce new version.
		puts "\tRep$tnum.d.$i: Announce new version $version."
		set vdb [eval {berkdb_open_noerr -env $menv \
		    -auto_commit -mode 0644} $vname]
		error_check_good update_version [$vdb put VERSION $version] 0
		error_check_good vdb_close [$vdb close] 0

		# Process messages or simulate failure.
		if { $process == 1 } {
			process_msgs $envlist
		} else {
			replclear 2
			replclear 3
		}

		# Switch master, update envlist.
		puts "\tRep$tnum.e.$i: Switch masters."
		set envlist [switch_master $envlist]

		# Update values for next iteration.
		set menv [lindex [lindex $envlist 0] 0]
		set cenv0 [lindex [lindex $envlist 1] 0]
		incr version
		if { $version > $maxversion } {
			set version 1
		}
	}

	# Signal to child that we are done.
	set vdb [eval {berkdb_open_noerr -env $menv \
	    -auto_commit -mode 0644} $vname]
	error_check_good version_done [$vdb put VERSION DONE] 0
	error_check_good vdb_close [$vdb close] 0
	process_msgs $envlist

	# Watch for child to finish.
	watch_procs $pid 5

	puts "\tRep$tnum.f: Clean up."
	error_check_good menv_close [$menv close] 0
	error_check_good cenv0_close [$cenv0 close] 0
	error_check_good cenv1_close [$cenv1 close] 0

	replclose $testdir/MSGQUEUEDIR

	# Check for failures in child's log file.
	set errstrings [eval findfail $testdir/rep045script.log]
	foreach str $errstrings {
		puts "FAIL: error message in log file: $str"
	}

	set testdir $orig_tdir
	return
}

proc switch_master { envlist } {
	# Find env handles and machine ids.
	set menv [lindex [lindex $envlist 0] 0]
	set mid [lindex [lindex $envlist 0] 1]
	set cenv [lindex [lindex $envlist 1] 0]
	set cid [lindex [lindex $envlist 1] 1]
	set cenv1 [lindex [lindex $envlist 2] 0]
	set cid1 [lindex [lindex $envlist 2] 1]

	# Downgrade master, upgrade client.
	error_check_good master_downgrade [$menv rep_start -client] 0
	error_check_good client_upgrade [$cenv rep_start -master] 0
	process_msgs $envlist

	# Adjust envlist.  The former client env is the new master,
	# and vice versa.
	set newenvlist "{$cenv $cid} {$menv $mid} {$cenv1 $cid1}"
	return $newenvlist
}

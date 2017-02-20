# See the file LICENSE for redistribution information.
#
# Copyright (c) 2004-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	rep030
# TEST	Test of internal initialization multiple files and pagesizes.
# TEST  	Hold some databases open on master.
# TEST
# TEST	One master, one client using a data_dir for internal init.
# TEST	Generate several log files.
# TEST	Remove old master log files.
# TEST	Delete client files and restart client.
# TEST	Put one more record to the master.
#
proc rep030 { method { niter 500 } { tnum "030" } args } {

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
	# and with and without cleaning.
	set opts { noclean clean bulk }
	foreach r $test_recopts {
		foreach c $opts {
			foreach l $logsets {
				set logindex [lsearch -exact $l "in-memory"]
				if { $r == "-recover" && $logindex != -1 } {
					puts "Skipping rep$tnum for -recover\
					    with in-memory logs."
					continue
				}
				puts "Rep$tnum ($method $r $c):\
				    Internal initialization - hold some\
				    databases open on master $msg $msg2."
				puts "Rep$tnum: Master logs are [lindex $l 0]"
				puts "Rep$tnum: Client logs are [lindex $l 1]"
				rep030_sub $method $niter $tnum $l $r $c $args
			}
		}
	}
}

proc rep030_sub { method niter tnum logset recargs opts largs } {
	global testdir
	global util_path
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
	set maxpg 16384
	set log_max [expr $maxpg * 8]
	set cache [expr $maxpg * 32 ]

	set m_logtype [lindex $logset 0]
	set c_logtype [lindex $logset 1]

	# In-memory logs cannot be used with -txn nosync.
	set m_logargs [adjust_logargs $m_logtype]
	set c_logargs [adjust_logargs $c_logtype]
	set m_txnargs [adjust_txnargs $m_logtype]
	set c_txnargs [adjust_txnargs $c_logtype]

	# Run internal init using a data directory
	#
	file mkdir $masterdir/data
	file mkdir $masterdir/data2
	file mkdir $clientdir/data
	file mkdir $clientdir/data2
	#
	# Set it twice to test duplicates data_dirs as well
	# as multiple, different data dirs
	#
	set data_diropts " -data_dir data -data_dir data -data_dir data2"

	# Open a master.
	repladd 1
	set ma_envcmd "berkdb_env_noerr -create $m_txnargs \
	    $repmemargs \
	    $m_logargs -log_max $log_max -errpfx MASTER \
	    -cachesize { 0 $cache 1 } $data_diropts $verbargs \
	    -home $masterdir -rep_transport \[list 1 replsend\]"
	set masterenv [eval $ma_envcmd $recargs -rep_master]

	# Open a client
	repladd 2
	set cl_envcmd "berkdb_env_noerr -create $c_txnargs \
	    $repmemargs \
	    $c_logargs -log_max $log_max -errpfx CLIENT \
	    -cachesize { 0 $cache 1 } $data_diropts $verbargs \
	    -home $clientdir -rep_transport \[list 2 replsend\]"
	set clientenv [eval $cl_envcmd $recargs -rep_client]

	# Bring the clients online by processing the startup messages.
	set envlist "{$masterenv 1} {$clientenv 2}"
	process_msgs $envlist

	# Clobber replication's 30-second anti-archive timer, which will have
	# been started by client sync-up internal init, so that we can do a
	# log_archive in a moment.
	#
	$masterenv test force noarchive_timeout

	# Run rep_test in the master (and update client).
	set startpgsz 512
	set pglist ""
	for { set pgsz $startpgsz } { $pgsz <= $maxpg } \
	    { set pgsz [expr $pgsz * 2] } {
		lappend pglist $pgsz
	}
	set nfiles [llength $pglist]
	puts "\tRep$tnum.a.0: Running rep_test $nfiles times in replicated env."
	set dbopen ""
	for { set i 0 } { $i < $nfiles } { incr i } {
		set mult [expr $i * 10]
		set nentries [expr $niter + $mult]
		set pagesize [lindex $pglist $i]
		set largs " -pagesize $pagesize "
		eval rep_test $method $masterenv NULL $nentries $mult $mult \
		    0 $largs
		process_msgs $envlist

		#
		# Everytime we run 'rep_test' we create 'test.db'.  So
		# rename it each time through the loop.
		#
		set old "test.db"
		set new "test.$i.db"
		if { $databases_in_memory == 0 } {
			error_check_good rename [$masterenv dbrename \
			    -auto_commit $old $new] 0
		} else {
			error_check_good inmem_rename [$masterenv dbrename \
			    "" $old $new] 0
		}
		process_msgs $envlist
		#
		# We want to keep some databases open so that we test the
		# code finding the files in the data dir as well as finding
		# them in dbreg list.
		#
		if { [expr $i % 2 ] == 0 } {
			if { $databases_in_memory == 1 } {
				set db [berkdb_open_noerr\
				    -env $masterenv "" $new]
			} else {
				set db [berkdb_open_noerr\
				    -env $masterenv $new]
			}
			error_check_good dbopen.$i [is_valid_db $db] TRUE
			lappend dbopen $db
		}
	}

	# Set up a few special databases too.  We want one with a subdatabase
	# and we want an empty database, in addition to in-memory dbs.

	# Set up databases in-memory or on-disk as expected.
	if { $databases_in_memory } {
		set testfile { "" "test.db" }
		set emptyfile { "" "empty.db" }
	} else { 
		set testfile "test.db"
		set emptyfile "empty.db"
	} 
	
	if { [is_queue $method] } {
		set sub ""
	} else {
		set sub "subdb"
	}

	set omethod [convert_method $method]
	set largs " -pagesize $maxpg "
	set largs [convert_args $method $largs]

	#
	# Create/close an empty database.
	#
	set db [eval {berkdb_open_noerr -env $masterenv -auto_commit -create \
	    -mode 0644} $largs $omethod $emptyfile]
	error_check_good emptydb [is_valid_db $db] TRUE
	error_check_good empty_close [$db close] 0
	#
	# If we're not using in-mem named databases, open a subdb and 
	# keep it open.  (Do a regular db if method is queue.)
	# We need it a few times later on.
	#
	if { $databases_in_memory } {
		set db [eval {berkdb_open_noerr -env $masterenv -auto_commit \
		    -create -mode 0644} $largs $omethod $testfile]
	} else {
		set db [eval {berkdb_open_noerr -env $masterenv -auto_commit \
		     -create -mode 0644} $largs $omethod $testfile $sub]
	}
	error_check_good subdb [is_valid_db $db] TRUE
	set start 0
	eval rep_test $method $masterenv $db $niter $start $start 0 $largs
	incr start $niter
	process_msgs $envlist

	puts "\tRep$tnum.b: Close client."
	# First save the log number of the latest client log.
	set last_client_log [get_logfile $clientenv last]
	error_check_good client_close [$clientenv close] 0

	# Run rep_test in the master (don't update client).
	set stop 0
	while { $stop == 0 } {
		# Run rep_test in the master (don't update client).
		puts "\tRep$tnum.c: Running rep_test in replicated env."
	 	eval rep_test \
		    $method $masterenv $db $niter $start $start 0 $largs
		incr start $niter
		replclear 2

		puts "\tRep$tnum.d: Run db_archive on master."
		if { $m_logtype != "in-memory"} {
			set res [eval exec $util_path/db_archive -d -h $masterdir]
			set res [eval exec $util_path/db_archive -l -h $masterdir]
		}
		set first_master_log [get_logfile $masterenv first]
		if { $first_master_log > $last_client_log } {
			set stop 1
		}
	}

	puts "\tRep$tnum.e: Reopen client ($opts)."
	if { $opts == "clean" } {
		env_cleanup $clientdir
		file mkdir $clientdir/data
		file mkdir $clientdir/data2
	}
	if { $opts == "bulk" } {
		error_check_good bulk [$masterenv rep_config {bulk on}] 0
	}

	set clientenv [eval $cl_envcmd $recargs -rep_client]
	error_check_good client_env [is_valid_env $clientenv] TRUE
	set envlist "{$masterenv 1} {$clientenv 2}"
	process_msgs $envlist 0 NONE err
	if { $opts != "clean" } {
		puts "\tRep$tnum.e.1: Trigger log request"
		#
		# When we don't clean, starting the client doesn't
		# trigger any events.  We need to generate some log
		# records so that the client requests the missing
		# logs and that will trigger it.
		#
		set entries 100
		eval rep_test $method $masterenv $db $entries $start $start 0 $largs
		incr start $entries
		process_msgs $envlist 0 NONE err
	}
	error_check_good subdb_close [$db close] 0
	process_msgs $envlist 0 NONE err

	puts "\tRep$tnum.f: Verify logs and databases"
	#
	# If doing bulk testing, turn it off now so that it forces us
	# to flush anything currently in the bulk buffer.  We need to
	# do this because rep_test might have aborted a transaction on
	# its last iteration and those log records would still be in
	# the bulk buffer causing the log comparison to fail.
	#
	if { $opts == "bulk" } {
		puts "\tRep$tnum.f.1: Turn off bulk transfers."
		error_check_good bulk [$masterenv rep_config {bulk off}] 0
		process_msgs $envlist 0 NONE err
	}

	rep_verify $masterdir $masterenv $clientdir $clientenv\
	    1 1 1 test.db $masterdir/data
	for { set i 0 } { $i < $nfiles } { incr i } {
		set dbname "test.$i.db"
		rep_verify $masterdir $masterenv $clientdir $clientenv \
		    1 1 0 $dbname $masterdir/data
	}

	# Close the database held open on master for initialization.
	foreach db $dbopen {
		error_check_good db_close [$db close] 0
	}

	# Add records to the master and update client.
	puts "\tRep$tnum.g: Add more records and check again."
	set entries 10
	if { $databases_in_memory }  {
		set db [eval {berkdb_open_noerr -env $masterenv -auto_commit \
		    -mode 0644} $largs $omethod $testfile]
	} else {
		set db [eval {berkdb_open_noerr -env $masterenv -auto_commit \
		    -mode 0644} $largs $omethod $testfile $sub]

	}
	error_check_good subdb [is_valid_db $db] TRUE
	eval rep_test $method $masterenv $db $entries $niter 0 0 $largs
	error_check_good subdb_close [$db close] 0
	process_msgs $envlist 0 NONE err

	rep_verify $masterdir $masterenv $clientdir $clientenv \
	    1 1 0 test.db $masterdir/data
	for { set i 0 } { $i < $nfiles } { incr i } {
		set dbname "test.$i.db"
		rep_verify $masterdir $masterenv $clientdir $clientenv \
		    1 1 0 $dbname $masterdir/data
	}
	set bulkxfer [stat_field $masterenv rep_stat "Bulk buffer transfers"]
	if { $opts == "bulk" } {
		error_check_bad bulkxferon $bulkxfer 0
	} else {
		error_check_good bulkxferoff $bulkxfer 0
	}

	# Check that databases and logs are in-memory or on-disk as expected.
	check_db_location $masterenv $dbname $masterdir/data
	check_db_location $clientenv $dbname $clientdir/data
	
	check_log_location $masterenv
	check_log_location $clientenv

	error_check_good masterenv_close [$masterenv close] 0
	error_check_good clientenv_close [$clientenv close] 0
	replclose $testdir/MSGQUEUEDIR
}

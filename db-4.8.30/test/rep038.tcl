# See the file LICENSE for redistribution information.
#
# Copyright (c) 2004-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	rep038
# TEST	Test of internal initialization and ongoing master updates.
# TEST
# TEST	One master, one client.
# TEST	Generate several log files.
# TEST	Remove old master log files.
# TEST	Delete client files and restart client.
# TEST	Put more records on master while initialization is in progress.
#
proc rep038 { method { niter 200 } { tnum "038" } args } {

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
	# and with various options, such as in-memory databases,
	# forcing an archive during the middle of init, and normal.
	# Skip recovery with in-memory logging - it doesn't make sense.
	set testopts { normal archive }
	foreach r $test_recopts {
		foreach t $testopts {
			foreach l $logsets {
				set logindex [lsearch -exact $l "in-memory"]
				if { $r == "-recover" && $logindex != -1 } {
					puts "Skipping rep$tnum for -recover\
					    with in-memory logs."
					continue
				}
				puts "Rep$tnum ($method $t $r $args): Test of\
				    internal init with new records $msg $msg2."
				puts "Rep$tnum: Master logs are [lindex $l 0]"
				puts "Rep$tnum: Client logs are [lindex $l 1]"
				rep038_sub $method $niter $tnum $l $r $t $args
			}
		}
	}
}

proc rep038_sub { method niter tnum logset recargs testopt largs } {
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
	set pagesize 4096
	append largs " -pagesize $pagesize "
	set log_max [expr $pagesize * 8]

	set m_logtype [lindex $logset 0]
	set c_logtype [lindex $logset 1]

	# In-memory logs cannot be used with -txn nosync.
	set m_logargs [adjust_logargs $m_logtype]
	set c_logargs [adjust_logargs $c_logtype]
	set m_txnargs [adjust_txnargs $m_logtype]
	set c_txnargs [adjust_txnargs $c_logtype]

	# Open a master.
	repladd 1
	set ma_envcmd "berkdb_env_noerr -create $m_txnargs $repmemargs \
	    $m_logargs -log_max $log_max -errpfx MASTER $verbargs \
	    -home $masterdir -rep_transport \[list 1 replsend\]"
	set masterenv [eval $ma_envcmd $recargs -rep_master]
	$masterenv rep_limit 0 0

	# Run rep_test in the master only.
	puts "\tRep$tnum.a: Running rep_test in replicated env."
	set start 0
	if { $databases_in_memory } {
		set testfile { "" "test.db" }
	} else {
		set testfile "test.db"
	}
	set omethod [convert_method $method]
	set dbargs [convert_args $method $largs]
	set mdb [eval {berkdb_open} -env $masterenv -auto_commit\
          	-create -mode 0644 $omethod $dbargs $testfile ]
	error_check_good reptest_db [is_valid_db $mdb] TRUE

	set stop 0
	while { $stop == 0 } {
		# Run rep_test in the master beyond the first log file.
		eval rep_test\
		    $method $masterenv $mdb $niter $start $start 0 $largs
		incr start $niter

		puts "\tRep$tnum.a.1: Run db_archive on master."
		if { $m_logtype == "on-disk" } {
			set res \
			    [eval exec $util_path/db_archive -d -h $masterdir]
		}
		#
		# Make sure we have moved beyond the first log file.
		#
		set first_master_log [get_logfile $masterenv first]
		if { $first_master_log > 1 } {
			set stop 1
		}

	}

	puts "\tRep$tnum.b: Open client."
	repladd 2
	set cl_envcmd "berkdb_env_noerr -create $c_txnargs $repmemargs \
	    $c_logargs -log_max $log_max -errpfx CLIENT $verbargs \
	    -home $clientdir -rep_transport \[list 2 replsend\]"
	set clientenv [eval $cl_envcmd $recargs -rep_client]
	$clientenv rep_limit 0 0
	set envlist "{$masterenv 1} {$clientenv 2}"
	#
	# We want to simulate a master continually getting new
	# records while an update is going on.  Simulate that
	# for several iterations and then let the messages finish
	# all their processing.
	#
	set loop 10
	set i 0
	set entries 100
	set archived 0
	set start $niter
	set init 0
	while { $i < $loop } {
		set nproced 0
		set start [expr $start + $entries]
		eval rep_test \
		    $method $masterenv $mdb $entries $start $start 0 $largs
		incr start $entries
		incr nproced [proc_msgs_once $envlist NONE err]
		error_check_bad nproced $nproced 0
		#
		# If we are testing archiving, we need to make sure that
		# the first_lsn for internal init (the last log file we
		# have when we first enter init) is no longer available.
		# So, the first time through we record init_log, and then
		# on subsequent iterations we'll wait for the last log
		# to move further.  Force a checkpoint and archive.
		#
		if { $testopt == "archive" && $archived == 0 } {
			set clstat [exec $util_path/db_stat \
			    -N -r -R A -h $clientdir]
			if { $init == 0 && \
			    [is_substr $clstat "REP_F_RECOVER_PAGE"] } {
				set init_log [get_logfile $masterenv last]
				set init 1
			}
			if { $init == 0 && \
			    [is_substr $clstat "REP_F_RECOVER_LOG"] } {
				set init_log [get_logfile $masterenv last]
				set init 1
			}
			set last_master_log [get_logfile $masterenv last]
			set first_master_log [get_logfile $masterenv first]
			if { $init && $m_logtype == "on-disk" && \
			    $last_master_log > $init_log } {
				$masterenv txn_checkpoint -force
				$masterenv test force noarchive_timeout
				set res [eval exec  $util_path/db_archive \
				    -d -h $masterdir]
				set newlog [get_logfile $masterenv first]
				set archived 1
				error_check_good logs \
				    [expr $newlog > $init_log] 1
			} elseif { $init && $m_logtype == "in-memory" && \
			    $first_master_log > $init_log } {
				$masterenv txn_checkpoint -force
				$masterenv test force noarchive_timeout
				set archived 1
			}
		}
		incr i
	}
	set cdb [eval {berkdb_open_noerr} -env $clientenv -auto_commit\
	    -create -mode 0644 $omethod $dbargs $testfile]
	error_check_good reptest_db [is_valid_db $cdb] TRUE
	process_msgs $envlist

	puts "\tRep$tnum.c: Verify logs and databases"
	if { $databases_in_memory } {
		rep038_verify_inmem $masterenv $clientenv $mdb $cdb
	} else {
		rep_verify $masterdir $masterenv $clientdir $clientenv 1
	}

	# Add records to the master and update client.
	puts "\tRep$tnum.d: Add more records and check again."
	eval rep_test $method $masterenv $mdb $entries $start $start 0 $largs
	incr start $entries
	process_msgs $envlist 0 NONE err
	if { $databases_in_memory } {
		rep038_verify_inmem $masterenv $clientenv $mdb $cdb
	} else {
		rep_verify $masterdir $masterenv $clientdir $clientenv 1
	}

	# Make sure log file are on-disk or not as expected.
	check_log_location $masterenv
	check_log_location $clientenv

	error_check_good mdb_close [$mdb close] 0
	error_check_good cdb_close [$cdb close] 0
	error_check_good masterenv_close [$masterenv close] 0
	error_check_good clientenv_close [$clientenv close] 0
	replclose $testdir/MSGQUEUEDIR
}

proc rep038_verify_inmem { masterenv clientenv mdb cdb } {
	#
	# Can't use rep_verify to compare the logs because each
	# commit record from db_printlog shows the database name
	# as text on the master and as the file uid on the client
	# because the client cannot find the "file".  
	#
	# !!! Check the LSN first.  Otherwise the DB->stat for the
	# number of records will write a log record on the master if
	# the build is configured for debug_rop.  Work around that issue.
	#
	set mlsn [next_expected_lsn $masterenv]
	set clsn [next_expected_lsn $clientenv]
	error_check_good lsn $mlsn $clsn

	set mrecs [stat_field $mdb stat "Number of records"]
	set crecs [stat_field $cdb stat "Number of records"]
	error_check_good recs $mrecs $crecs
}

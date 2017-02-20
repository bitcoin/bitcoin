# See the file LICENSE for redistribution information.
#
# Copyright (c)-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	rep081
# TEST	Test of internal initialization and missing database files.
# TEST
# TEST	One master, one client, two databases.
# TEST	Generate several log files.
# TEST	Remove old master log files.
# TEST	Start up client.
# TEST	Remove or replace one master database file while client initialization 
# TEST  is in progress, make sure other master database can keep processing.
#
proc rep081 { method { niter 200 } { tnum "081" } args } {

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

	# Run with options to remove or replace the master database file.
	set testopts { removefile replacefile }
	foreach t $testopts {
		foreach l $logsets {
			puts "Rep$tnum ($method $t $args): Test of\
			    internal init with missing db file $msg $msg2."
			puts "Rep$tnum: Master logs are [lindex $l 0]"
			puts "Rep$tnum: Client logs are [lindex $l 1]"
			rep081_sub $method $niter $tnum $l $t $args
		}
	}
}

proc rep081_sub { method niter tnum logset testopt largs } {
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
	set masterenv [eval $ma_envcmd -rep_master]
	$masterenv rep_limit 0 0

	# Run rep_test in the master only.
	puts "\tRep$tnum.a: Running rep_test in replicated env."
	set start 0
	if { $databases_in_memory } {
		set testfile { "" "test.db" }
		set testfile2 { "" "test2.db" }
	} else {
		set testfile "test.db"
		set testfile2 "test2.db"
	}
	set omethod [convert_method $method]
	set dbargs [convert_args $method $largs]
	set mdb [eval {berkdb_open_noerr} -env $masterenv -auto_commit\
		-create -mode 0644 $omethod $dbargs $testfile ]
	error_check_good reptest_db [is_valid_db $mdb] TRUE
	set mdb2 [eval {berkdb_open_noerr} -env $masterenv -auto_commit\
		-create -mode 0644 $omethod $dbargs $testfile2 ]
	error_check_good reptest_db2 [is_valid_db $mdb2] TRUE

	set stop 0
	while { $stop == 0 } {
		# Run rep_test in the master beyond the first log file.
		eval rep_test $method \
		    $masterenv $mdb $niter $start $start 0 $largs
		eval rep_test $method \
		    $masterenv $mdb2 $niter $start $start 0 $largs
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
	set clientenv [eval $cl_envcmd -rep_client]
	$clientenv rep_limit 0 0
	set envlist "{$masterenv 1} {$clientenv 2}"

	# Check initial value for number of FILE_FAIL internal init cleanups.
	error_check_good ff_cleanup \
	    [stat_field $clientenv rep_stat "File fail cleanups done"] 0

	#
	# Process messages in a controlled manner until the update (internal
	# init) starts and we can remove or replace the database file.
	#
	set loop 10
	set i 0
	set entries 100
	set in_rec_page 0
	set dbrem_init 0
	if { $testopt == "replacefile" } {
		set errstr "invalid argument"
	} else {
		set errstr "no such file or directory"
	}
	while { $i < $loop } {
		set nproced 0
		incr nproced [proc_msgs_once $envlist NONE err]
		#
		# Last time through the loop the mdb database file
		# is gone. The master is processing the client's PAGE_REQ 
		# and not finding the database file it needs so it sends a 
		# FILE_FAIL and returns an error. Break out of loop if
		# expected error seen.
		#
		if { [is_substr $err $errstr] } {
			error_check_good nproced $nproced 0
			break
		} else {
			error_check_bad nproced $nproced 0
			error_check_good errchk $err 0
		}
		# Internal init file is very transient, but exists in
		# the rep files on-disk case during the second iteration
		# of this loop. Take this chance to make sure the internal
		# init file doesn't exist when rep files are in-memory.
		if { $i == 1 && $repfiles_in_memory == 1 } {
			error_check_good noinit \
			    [file exists "$clientdir/__db.rep.init"] 0
		}
		#
		# When we are in internal init, remove the mdb database file.
		# This causes the master to send a FILE_FAIL that will cause
		# the client to clean up its internal init.
		#
		if { $in_rec_page == 0 } {
			set clstat [exec $util_path/db_stat \
			    -N -r -R A -h $clientdir]
			if { $dbrem_init == 0 && \
			    [is_substr $clstat "REP_F_RECOVER_PAGE"] } {
				set in_rec_page 1
				set dbrem_init 1
				#
				# Turn off timer so that client sync doesn't
				# prevent db operations.
				#
				$masterenv test force noarchive_timeout

				# Close and remove mdb.
				puts "\tRep$tnum.c: Remove a database file."
				error_check_good mdb_close [$mdb close] 0
				error_check_good remove_x [$masterenv \
				    dbremove -auto_commit $testfile] 0

				# Make sure mdb file is really gone.
				set dfname [file join $masterdir $testfile]
				error_check_good gone [file exists $dfname] 0

				# Replace mdb file with non-db content.
				if { $testopt == "replacefile" } {
					puts \
	"\tRep$tnum.c.1: Replace database file."
					set repfileid [open $dfname w+]
					puts -nonewline $repfileid \
					    "This is not a database file."
					close $repfileid
				}
			}
		}
		incr i
	}

	#
	# Process two more batches of messages so client can process 
	# the FILE_FAIL message and the resulting new internal init.
	#
	puts "\tRep$tnum.d: Process messages including FILE_FAIL."
	process_msgs $envlist 0 NONE err
	if { $err != 0 } {
		error_check_good errchk [is_substr $err $errstr] 1
	}
	puts "\tRep$tnum.d.1: Process messages including new internal init."
	process_msgs $envlist 0 NONE err
	error_check_good errchk $err 0

	puts "\tRep$tnum.e: Verify logs and databases."
	rep_verify $masterdir $masterenv $clientdir $clientenv 1 1 1 test2.db

	# Make sure we have seen a FILE_FAIL internal init cleanup.
	error_check_good ff_cleanup \
	    [stat_field $clientenv rep_stat "File fail cleanups done"] 1

	error_check_good mdb_close2 [$mdb2 close] 0
	error_check_good masterenv_close [$masterenv close] 0
	error_check_good clientenv_close [$clientenv close] 0
	replclose $testdir/MSGQUEUEDIR
}



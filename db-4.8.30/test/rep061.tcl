# See the file LICENSE for redistribution information.
#
# Copyright (c) 2004-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	rep061
# TEST	Test of internal initialization multiple files and pagesizes
# TEST	with page gaps.
# TEST
# TEST	One master, one client.
# TEST	Generate several log files.
# TEST	Remove old master log files.
# TEST	Delete client files and restart client.
# TEST	Put one more record to the master.
# TEST  Force some page messages to get dropped.
#
proc rep061 { method { niter 500 } { tnum "061" } args } {

	source ./include.tcl
	global databases_in_memory
	global repfiles_in_memory

	if { $is_windows9x_test == 1 } {
		puts "Skipping replication test on Win 9x platform."
		return
	}

	# Run for btree and queue only.
	if { $checking_valid_methods } {
		set test_methods {}
		foreach method $valid_methods {
			if { [is_btree $method] == 1 || [is_queue $method] == 1 } {
				lappend test_methods $method
			}
		}
		return $test_methods
	}
	if { [is_btree $method] != 1 && [is_queue $method] != 1 } {
		puts "Skipping rep061 for method $method."
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
	# and with and without cleaning.
	set opts { noclean clean bulk }
	# Try varying drop percentages.
	set dpct { 10 5 }
	foreach r $test_recopts {
		foreach c $opts {
			foreach l $logsets {
				foreach d $dpct {
					set logindex [lsearch -exact $l \
					    "in-memory"]
					if { $r == "-recover" && \
					    $logindex != -1 } {
						puts "Skipping rep$tnum \
						    for -recover\
						    with in-memory logs."
						continue
					}
					puts "Rep$tnum ($method $r $c):\
					    Internal initialization - with\
					    $d pct page gaps, $msg $msg2."
					puts "Rep$tnum: Master logs are \
					    [lindex $l 0]"
					puts "Rep$tnum: Client logs are \
					    [lindex $l 1]"
					rep061_sub $method $niter $tnum \
					    $l $r $c $d $args
				}
			}
		}
	}
}

proc rep061_sub { method niter tnum logset recargs opts dpct largs } {
	global testdir
	global util_path
	global drop drop_msg
	global startup_done
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
	set cache [expr $maxpg * 32]
	if { $repfiles_in_memory } {
		set cache [expr ($maxpg * 32) + (3000 * 1024)]
	}

	set m_logtype [lindex $logset 0]
	set c_logtype [lindex $logset 1]

	# In-memory logs cannot be used with -txn nosync.
	set m_txnargs [adjust_txnargs $m_logtype]
	set c_txnargs [adjust_txnargs $c_logtype]

	# Open a master.
	repladd 1
	set ma_envcmd "berkdb_env_noerr -create $m_txnargs $verbargs \
	    $repmemargs \
	    -log_max $log_max -cachesize { 0 $cache 1 } -errpfx MASTER \
	    -home $masterdir -rep_transport \[list 1 replsend\]"
	set masterenv [eval $ma_envcmd $recargs -rep_master]

	# Open a client
	repladd 2
	set cl_envcmd "berkdb_env_noerr -create $c_txnargs $verbargs \
	    $repmemargs \
	    -log_max $log_max -cachesize { 0 $cache 1 } -errpfx CLIENT \
	    -home $clientdir -rep_transport \[list 2 replsend\]"
	set clientenv [eval $cl_envcmd $recargs -rep_client]

	#
	# Since we're dropping messages, set the rerequest values
	# lower so we don't wait too long to request what we're
	# missing.
	#
	#
	# Set to 200/800 usecs.  An average ping to localhost should
	# be a few 10s usecs.
	#
	$clientenv rep_request 200 800
	# Bring the clients online by processing the startup messages.
	set envlist "{$masterenv 1} {$clientenv 2}"
	process_msgs $envlist

	# Clobber replication's 30-second anti-archive timer, which will have
	# been started by client sync-up internal init, so that we can do a
	# log_archive in a moment.
	#
	$masterenv test force noarchive_timeout

	#
	# Note that by setting these 2 globals below, message dropping
	# is automatically enabled.  By setting 'drop' to 0, further
	# down in the test, we disable message dropping.
	#
	set drop 1
	set drop_msg [expr 100 / $dpct]

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

		if { $databases_in_memory } {
			error_check_good rename [$masterenv dbrename \
			    -auto_commit "" $old $new] 0
		} else {
			error_check_good rename [$masterenv dbrename \
			    -auto_commit $old $new] 0
		}
		process_msgs $envlist

		#
		# We want to keep some databases open so that we test the
		# code finding the files in the data dir as well as finding
		# them in dbreg list.
		#
		if { [expr $i % 2 ] == 0 } {
			if { $databases_in_memory } {
				set db [berkdb_open_noerr -env $masterenv "" $new]
			} else {
				set db [berkdb_open_noerr -env $masterenv $new]
			}
			error_check_good dbopen.$i [is_valid_db $db] TRUE
			lappend dbopen $db
		}
	}
	#
	# Set up a few special databases too.  We want one with a subdatabase
	# and we want an empty database.
	#
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

	# If we're not using in-mem named databases, open a subdb and keep 
	# it open. (Do a regular db if method is queue.) 
	# We need it a few times later on.
	#
	if { $databases_in_memory } {
		set db [eval {berkdb_open_noerr -env $masterenv -auto_commit\
		    -create -mode 0644} $largs $omethod $testfile]
	} else {
		set db [eval {berkdb_open_noerr -env $masterenv -auto_commit\
		    -create -mode 0644} $largs $omethod $testfile $sub]
	}
	error_check_good subdb [is_valid_db $db] TRUE
	eval rep_test $method $masterenv $db $niter 0 0 0 $largs
	process_msgs $envlist

	puts "\tRep$tnum.b: Close client."
	error_check_good client_close [$clientenv close] 0

	#
	# Run rep_test in the master (don't update client).
	# Need to guarantee that we will change log files during
	# this run so run with the largest pagesize and double
	# the number of entries.
	#
	puts "\tRep$tnum.c: Running rep_test ( $largs) in replicated env."
	set nentries [expr $niter * 2]
	eval rep_test $method $masterenv $db $nentries 0 0 0 $largs
	replclear 2

	puts "\tRep$tnum.d: Run db_archive on master."
	set res [eval exec $util_path/db_archive -l -h $masterdir]
	error_check_bad log.1.present [lsearch -exact $res log.0000000001] -1
	set res [eval exec $util_path/db_archive -d -h $masterdir]
	set res [eval exec $util_path/db_archive -l -h $masterdir]
	error_check_good log.1.gone [lsearch -exact $res log.0000000001] -1

	puts "\tRep$tnum.e: Reopen client ($opts)."
	if { $opts == "clean" } {
		env_cleanup $clientdir
	}
	if { $opts == "bulk" } {
		error_check_good bulk [$masterenv rep_config {bulk on}] 0
	}

	set clientenv [eval $cl_envcmd $recargs -rep_client]
	error_check_good client_env [is_valid_env $clientenv] TRUE
	#
	# Since we are dropping frequent messages, we set the
	# rerequest rate low to make sure the test finishes.
	#
	$clientenv rep_request 200 800
	set envlist "{$masterenv 1} {$clientenv 2}"
	process_msgs $envlist 0 NONE err
	set done 0
	#
	# We are done with this loop when the client has achieved
	# startup_done and we've looped one more time after turning
	# off dropping messages.  Otherwise we might get a few
	# straggling log records that don't make it over.
	#
	# Set a maximum iteration count because some methods can get
	# into a state where if we're regularly dropping messages we
	# can never catch up (queue) and we loop forever.
	#
	set iter 1
	set max_drop_iter 200
	if { $opts == "bulk" } {
		#
		# Since bulk is sending several messages at once we need to
		# loop more times to allow rerequests to get through.
		#
		set max_drop_iter [expr $max_drop_iter * 2]
		$clientenv rep_request 100 400
	}
	while { $done == 0 } {
		puts "\tRep$tnum.e.1.$iter: Trigger log request"
		#
		# When we don't clean, starting the client doesn't
		# trigger any events.  We need to generate some log
		# records so that the client requests the missing
		# logs and that will trigger it.
		#
		set entries 4
		eval rep_test $method $masterenv $db $entries $niter 0 0 $largs
		process_msgs $envlist 0 NONE err
		set stat [exec $util_path/db_stat -N -r -R A -h $clientdir]
		#
		# Loop until we are done with the RECOVER_PAGE phase.
		#
		set in_page [is_substr $stat "REP_F_RECOVER_PAGE"]
		if { !$in_page || $iter >= $max_drop_iter } {
			#
			# If we're dropping, stop doing so.
			# If we're not dropping, we're done.
			#
			if { $drop != 0 } {
				set drop 0
			} else {
				set done 1
			}
		}
		incr iter
	}
	error_check_good subdb_close [$db close] 0
	#
	# Stop dropping records, we've sent all the pages.
	# We need to do that in order to make sure we get
	# all the log records there and can accurately compare.  Also, make sure
	# enough time has passed so that the client's rep_request timer has
	# expired, and make sure there are any messages to send to the client,
	# so that there is something to trigger any needed final rerequest.
	#
	set drop 0
	tclsleep 2
	$masterenv txn_checkpoint -force
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
	rep_verify $masterdir $masterenv $clientdir $clientenv 1 1 1

	for { set i 0 } { $i < $nfiles } { incr i } {
		set dbname "test.$i.db"
		rep_verify $masterdir $masterenv $clientdir $clientenv \
		    1 1 0 $dbname
	}

	#
	# Close the database held open on master for initialization.
	#
	foreach db $dbopen {
		error_check_good db_close [$db close] 0
	}

	# Add records to the master and update client.
	puts "\tRep$tnum.g: Add more records and check again."
	set entries 10
	set db [eval {berkdb_open_noerr -env $masterenv -auto_commit \
	    -mode 0644} $largs $omethod $testfile $sub]
	error_check_good subdb [is_valid_db $db] TRUE
	eval rep_test $method $masterenv $db $entries $niter 0 0 $largs
	error_check_good subdb_close [$db close] 0
	process_msgs $envlist 0 NONE err

	rep_verify $masterdir $masterenv $clientdir $clientenv 1 1 1
	for { set i 0 } { $i < $nfiles } { incr i } {
		rep_verify $masterdir $masterenv $clientdir $clientenv \
		    1 1 0
	}
	set bulkxfer [stat_field $masterenv rep_stat "Bulk buffer transfers"]
	if { $opts == "bulk" } {
		error_check_bad bulkxferon $bulkxfer 0
	} else {
		error_check_good bulkxferoff $bulkxfer 0
	}

	error_check_good masterenv_close [$masterenv close] 0
	error_check_good clientenv_close [$clientenv close] 0
	replclose $testdir/MSGQUEUEDIR
}

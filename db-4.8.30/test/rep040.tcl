# See the file LICENSE for redistribution information.
#
# Copyright (c) 2004-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	rep040
# TEST	Test of racing rep_start and transactions.
# TEST
# TEST	One master, one client.
# TEST	Have master in the middle of a transaction.
# TEST  Call rep_start to make master a client.
# TEST  Commit the transaction.
# TEST  Call rep_start to make master the master again.
#
proc rep040 { method { niter 200 } { tnum "040" } args } {

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
	# and with and without cleaning.  Skip recovery with in-memory
	# logging - it doesn't make sense.
	foreach r $test_recopts {
		foreach l $logsets {
			set logindex [lsearch -exact $l "in-memory"]
			if { $r == "-recover" && $logindex != -1 } {
				puts "Skipping rep$tnum for -recover\
				    with in-memory logs."
				continue
			}
			puts "Rep$tnum ($method $r $args):\
			    Test of rep_start racing txns $msg $msg2."
			puts "Rep$tnum: Master logs are [lindex $l 0]"
			puts "Rep$tnum: Client logs are [lindex $l 1]"
			rep040_sub $method $niter $tnum $l $r $args
		}
	}
}

proc rep040_sub { method niter tnum logset recargs largs } {
	source ./include.tcl
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

	set omethod [convert_method $method]
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
	set ma_envcmd "berkdb_env_noerr -create $m_txnargs $m_logargs \
	    -errpfx MASTER $repmemargs \
	    -home $masterdir $verbargs -rep_transport \[list 1 replsend\]"
	set masterenv [eval $ma_envcmd $recargs -rep_master]

	# Open a client
	repladd 2
	set cl_envcmd "berkdb_env_noerr -create $c_txnargs $c_logargs \
	    -errpfx CLIENT $repmemargs \
	    -home $clientdir $verbargs -rep_transport \[list 2 replsend\]"
	set clientenv [eval $cl_envcmd $recargs -rep_client]
	error_check_good client_env [is_valid_env $clientenv] TRUE

	# Set up databases in-memory or on-disk.
	if { $databases_in_memory } {
		set testfile { "" "rep040.db" }
		set testfile1 { "" "rep040A.db" }
	} else { 
		set testfile "rep040.db"
		set testfile1 "rep040A.db"
	} 

	set db [eval {berkdb_open_noerr -env $masterenv -auto_commit -create \
	    -mode 0644} $largs $omethod $testfile]
	error_check_good rep_db [is_valid_db $db] TRUE

	set db1 [eval {berkdb_open_noerr -env $masterenv -auto_commit -create \
	    -mode 0644} $largs $omethod $testfile1]
	error_check_good rep_db [is_valid_db $db1] TRUE

	set key [expr $niter + 100]
	set key2 [expr $niter + 200]
	set data "data1"
	set newdata "rep040test"

	# Bring the clients online by processing the startup messages.
	set envlist "{$masterenv 1} {$clientenv 2}"
	process_msgs $envlist

	# Run rep_test in the master (and update client).
	puts "\tRep$tnum.a: Running rep_test in replicated env."
	eval rep_test $method $masterenv $db $niter 0 0 0 $largs
	process_msgs $envlist

	# Get some data on a page
	set t [$masterenv txn]
	error_check_good txn [is_valid_txn $t $masterenv] TRUE
	set ret [$db put -txn $t $key [chop_data $method $data]]
	error_check_good put $ret 0
	error_check_good txn [$t commit] 0
	process_msgs $envlist

	#
	# Start 2 txns.  One that will commit early and one we'll hold
	# open a while to test for the warning message.
	#
	# Now modify the data but don't commit it yet.  This will
	# update the same page and update the page LSN.
	#
	set t [$masterenv txn]
	error_check_good txn [is_valid_txn $t $masterenv] TRUE
	set t2 [$masterenv txn]
	error_check_good txn [is_valid_txn $t2 $masterenv] TRUE
	set ret [$db put -txn $t $key [chop_data $method $newdata]]
	error_check_good put $ret 0
	set ret [$db1 put -txn $t2 $key2 [chop_data $method $newdata]]
	error_check_good put $ret 0
	process_msgs $envlist

	# Fork child process and then sleep for more than 1 minute so
	# that the child process must block on the open transaction and
	# it will print out the wait message.
	#
	set outfile "$testdir/rep040script.log"
	puts "\tRep$tnum.b: Fork master child process and sleep 90 seconds"
	set pid [exec $tclsh_path $test_path/wrap.tcl \
	    rep040script.tcl $outfile $masterdir $databases_in_memory &]

	tclsleep 10
	process_msgs $envlist
	error_check_good txn [$t commit] 0
	tclsleep 80

	error_check_good txn [$t2 commit] 0
	puts "\tRep$tnum.c: Waiting for child ..."
	process_msgs $envlist
	watch_procs $pid 5

	process_msgs $envlist

	set t [$masterenv txn]
	error_check_good txn [is_valid_txn $t $masterenv] TRUE
	set ret [$db put -txn $t $key [chop_data $method $data]]
	error_check_good put $ret 0
	error_check_good txn [$t commit] 0
	error_check_good dbclose [$db close] 0
	error_check_good dbclose [$db1 close] 0
	process_msgs $envlist
 
	# Check that databases are in-memory or on-disk as expected.
	check_db_location $masterenv $testfile
	check_db_location $masterenv $testfile1
	check_db_location $clientenv $testfile
	check_db_location $clientenv $testfile1

	check_log_location $masterenv
	check_log_location $clientenv

	error_check_good masterenv_close [$masterenv close] 0
	error_check_good clientenv_close [$clientenv close] 0

	#
	# Check we detected outstanding txn (t2).
	# The message we check for is produced only if the build was 
	# configured with --enable-diagnostic. 
	set conf [berkdb getconfig]
	if { [is_substr $conf "diagnostic"] == 1 } {
		puts "\tRep$tnum.d: Verify waiting and logs"
		set ret [catch {open $outfile} ofid]
		error_check_good open $ret 0
		set contents [read $ofid]
		error_check_good \
		    detect [is_substr $contents "Waiting for op_cnt"] 1
		close $ofid
	}

	# Check that master and client logs and dbs are identical.
	set stat [catch {eval exec $util_path/db_printlog \
	    -h $masterdir > $masterdir/prlog} result]
	error_check_good stat_mprlog $stat 0
	set stat [catch {eval exec $util_path/db_printlog \
	    -h $clientdir > $clientdir/prlog} result]
	error_check_good stat_cprlog $stat 0
	error_check_good log_cmp \
	    [filecmp $masterdir/prlog $clientdir/prlog] 0

	replclose $testdir/MSGQUEUEDIR
}

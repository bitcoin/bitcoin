# See the file LICENSE for redistribution information.
#
# Copyright (c) 2001-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	rep075
# TEST	Replication and prepared transactions.
# TEST	Test having outstanding prepared transactions and simulating
# TEST	crashing or upgrading or downgrading sites.
# TEST
#
proc rep075 { method { tnum "075" } args } {

	source ./include.tcl
	global databases_in_memory
	global mixed_mode_logging
	global repfiles_in_memory

	if { $is_windows9x_test == 1 } {
		puts "Skipping replication test on Win 9x platform."
		return
	}

	# Run for all access methods.
	if { $checking_valid_methods } {
		set test_methods { btree }
		return $test_methods
	}
	if { [is_btree $method] == 0 } {
		puts "Rep075: Skipping for method $method"
		return
	}

	set args [convert_args $method $args]
	set logsets [create_logsets 2]

	# Swapping the envs is the only thing that should
	# work for:
	#   HP, old Windows: can't open two handles on same env.
	#   in-memory logs: prepared txns don't survive recovery
	#   NIM databases: can't be recovered
	#
	if { $is_hp_test == 1  || $is_windows9x_test == 1 ||
	     $mixed_mode_logging > 0 || $databases_in_memory == 1 } {
		set prep {swap}
	} else {
		set prep {dbrecover swap resolve recover envrecover}
	}
	set ops {commit abort both}

	# Set up for on-disk or in-memory databases.
	set msg "using on-disk databases"
	if { $databases_in_memory } {
		set msg "using named in-memory databases"

	}
	set msg2 "and on-disk replication files"
	if { $repfiles_in_memory } {
		set msg2 "and in-memory replication files"
	}

	# Run the body of the test with and without recovery.
	foreach l $logsets {
		foreach p $prep {
			foreach o $ops {
				puts "Rep$tnum ($method $p $o):\
				    Replication and prepared txns $msg $msg2."
				puts "Rep$tnum: Master logs are [lindex $l 0]"
				puts "Rep$tnum: Client logs are [lindex $l 1]"
				puts "Rep$tnum: close DBs after prepare"
				rep075_sub $method $tnum $l $p $o 1 $args
				puts "Rep$tnum: close DBs before prepare"
				rep075_sub $method $tnum $l $p $o 0 $args
			}
		}
	}
}

proc rep075_sub { method tnum logset prep op after largs } {
	global testdir
	global databases_in_memory
	global repfiles_in_memory
	global rep_verbose
	global verbose_type
	global util_path

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
	set clientdir2 $testdir/CLIENTDIR2
	file mkdir $masterdir
	file mkdir $clientdir
	file mkdir $clientdir2

        # Log size is small so we quickly create more than one.
	# The documentation says that the log file must be at least
	# four times the size of the in-memory log buffer.
	set pagesize 4096
	append largs " -pagesize $pagesize "
	set log_buf [expr $pagesize * 2]
	set log_max [expr $log_buf * 4]
	set m_logargs " -log_buffer $log_buf "
	set c_logargs " -log_buffer $log_buf "

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
	set ma_envcmd "berkdb_env_noerr -create $m_txnargs \
	    $repmemargs \
	    $m_logargs -errpfx ENV0 -log_max $log_max $verbargs \
	    -home $masterdir -rep_transport \[list 1 replsend\]"
	set env0 [eval $ma_envcmd -rep_master]
	set masterenv $env0
	error_check_good master_env [is_valid_env $env0] TRUE

	# Open a client.
	repladd 2
	set cl_envcmd "berkdb_env_noerr -create $c_txnargs \
	    $repmemargs \
	    $c_logargs -errpfx ENV1 -log_max $log_max $verbargs \
	    -home $clientdir -rep_transport \[list 2 replsend\]"
	set env1 [eval $cl_envcmd -rep_client]
	set clientenv $env1
	error_check_good client_env [is_valid_env $env1] TRUE

	repladd 3
	set cl2_envcmd "berkdb_env_noerr -create $c_txnargs \
	    $repmemargs \
	    $c_logargs -errpfx ENV2 -log_max $log_max $verbargs \
	    -home $clientdir2 -rep_transport \[list 3 replsend\]"
	set env2 [eval $cl2_envcmd -rep_client]
	set clientenv2 $env2
	error_check_good client_env [is_valid_env $env2] TRUE

	set omethod [convert_method $method]

	# Bring the clients online by processing the startup messages.
	set envlist "{$env0 1} {$env1 2} {$env2 3}"
	process_msgs $envlist

	#
	# Run rep_test in a database with a sub database, or in a
	# named in-memory database.
	#
	if { $databases_in_memory } {
		set testfile { "" "test1.db" }
		set testfile2 { "" "test2.db" }
		set db1 [eval {berkdb_open_noerr -env $masterenv -auto_commit \
		    -create -mode 0644} $largs $omethod $testfile]
	} else {
		set testfile "test1.db"
		set testfile2 "test2.db"
		set sub "subdb"
		set db1 [eval {berkdb_open_noerr -env $masterenv -auto_commit \
		    -create -mode 0644} $largs $omethod $testfile $sub]
	}
	error_check_good dbopen [is_valid_db $db1] TRUE

	puts "\tRep$tnum.a: Running rep_test in replicated env."
	set niter 1
	eval rep_test $method $masterenv $db1 $niter 0 0 0 $largs
	process_msgs $envlist

	set db [eval {berkdb_open_noerr -env $masterenv -auto_commit \
	    -create -mode 0644} $largs $omethod $testfile2]
	error_check_good dbopen [is_valid_db $db] TRUE

	#
	# Create and prepare 2 transactions:
	# One txn is for the first database and one txn for the
	# second database.  We want to test that we can detect
	# when the last restored txn has been resolved.  And we
	# want to test various files being open.
	#
	puts "\tRep$tnum.b: Prepare some txns."
	set pbnyc 2
	set key key
	set data some_data
	set txn1 [$masterenv txn]
	error_check_good txn [is_valid_txn $txn1 $masterenv] TRUE
	error_check_good put [$db1 put -txn $txn1 $key $data] 0

	set gid [make_gid rep075:$txn1]
	error_check_good commit [$txn1 prepare $gid] 0

	set txn2 [$masterenv txn]
	error_check_good txn [is_valid_txn $txn2 $masterenv] TRUE
	error_check_good put [$db put -txn $txn2 $key $data] 0

	set gid [make_gid rep075:$txn2]
	error_check_good commit [$txn2 prepare $gid] 0
	if { $after == 0 } {
		$db1 close
		$db close
	}
	process_msgs $envlist

	#
	# Now we have txns on a master that are PBNYC (prepared but
	# not yet committed).  Alter the replication system now
	# based on what we're testing this time through.
	#
	puts "\tRep$tnum.c: Reset replication ($prep)."

	if { $op == "commit" } {
		set op1 commit
		set op2 commit
	} elseif { $op == "abort" } {
		set op1 abort
		set op2 abort
	} else {
		set i [berkdb random_int 0 1]
		if { $i == 0 } {
			set op1 commit
			set op2 abort
		} else {
			set op1 abort
			set op2 commit
		}
	}
	set oplist [list $op1 $op2]
	#
	# If we are doing a swap, swap roles between master and client
	# and then call txn recover.  Master should then commit.
	# This operation tests handling prepared txns in replication code.
	#
	# If we are doing a recover, each site stops using its old
	# env handle and then opens a new one, with recovery.
	# This operation tests handling prepared txns and then
	# starting replication.
	#
	# If we are doing an envrecover, each site stops using its old
	# env handle and then opens a new one, with recovery.
	# Each site then opens a 2nd dbenv handle to run txn_recover
	# and resolve each operation.
	# This operation tests handling prepared txns and then
	# starting replication.
	#
	# If we are doing a resolve, each site prepares the txns
	# and then resolves the txns and then stops using the old
	# env handle to cause a "crash".  We then open a new one
	# with recovery.  This operation tests handling prepared
	# txns and having them resolved.
	#
	if { $prep == "swap" } {
		puts "\tRep$tnum.c.0: Swap roles master->client."
		#
		# A downgrading master must resolve the txns.  So, commit
		# them here, but don't send the messages to the client that
		# is about to become master.
		#
		error_check_good commit [$txn1 commit] 0
		error_check_good commit [$txn2 commit] 0
		if { $after == 1 } {
			$db1 close
			$db close
		}
		replclear 2
		replclear 3
		set newclient $env0
		error_check_good downgrade [$newclient rep_start -client] 0
		set ctxnlist [$newclient txn_recover]
		set newmaster $env1
		puts "\tRep$tnum.c.1: Swap roles client->master."
		error_check_good upgrade [$newmaster rep_start -master] 0
		set txnlist [$newmaster txn_recover]

		puts "\tRep$tnum.c.2: Check status of prepared txn."
		error_check_good txnlist_len [llength $txnlist] $pbnyc
		error_check_good txnlist_len [llength $ctxnlist] 0

		#
		# Now commit that old prepared txn.
		#
		puts "\tRep$tnum.c.3: Resolve prepared txn ($op)."
		rep075_resolve $txnlist $oplist
	} elseif { $prep == "recover" } {
		#
		# To simulate a crash, simply stop using the old handles
		# and reopen new ones, with recovery.  First flush both
		# the log and mpool to disk.
		#
		set origenv0 $env0
		set origenv1 $env1
		set origtxn1 $txn1
		set origtxn2 $txn2
		puts "\tRep$tnum.c.0: Sync and recover master environment."
		error_check_good flush1 [$env0 log_flush] 0
		error_check_good sync1 [$env0 mpool_sync] 0
		if { $after == 1 } {
			$db1 close
			$db close
		}
		set env0 [eval $ma_envcmd -recover]
		error_check_good master_env [is_valid_env $env0] TRUE
		puts "\tRep$tnum.c.1: Run txn_recover on master env."
		set txnlist [$env0 txn_recover]
		error_check_good txnlist_len [llength $txnlist] $pbnyc
		puts "\tRep$tnum.c.2: Resolve txn ($op) on master env."
		rep075_resolve $txnlist $oplist

		puts "\tRep$tnum.c.3: Sync and recover client environment."
		error_check_good flush1 [$env1 log_flush] 0
		error_check_good sync1 [$env1 mpool_sync] 0
		set env1 [eval $cl_envcmd -recover]
		error_check_good client_env [is_valid_env $env1] TRUE
		puts "\tRep$tnum.c.4: Run txn_recover on client env."
		set txnlist [$env1 txn_recover]
		error_check_good txnlist_len [llength $txnlist] $pbnyc

		puts "\tRep$tnum.c.5: Resolve txn ($op) on client env."
		rep075_resolve $txnlist $oplist

		puts "\tRep$tnum.c.6: Restart replication on both envs."
		error_check_good master [$env0 rep_start -master] 0
		error_check_good client [$env1 rep_start -client] 0
		set newmaster $env0
		set envlist "{$env0 1} {$env1 2} {$env2 3}"
		#
		# Clean up old Tcl handles.
		#
		catch {$origenv0 close} res
		catch {$origenv1 close} res
		catch {$origtxn1 close} res
		catch {$origtxn2 close} res
	} elseif { $prep == "resolve" } {
		#
		# Check having prepared txns in the log, but they are
		# also resolved before we "crash".
		# To simulate a crash, simply stop using the old handles
		# and reopen new ones, with recovery.  First flush both
		# the log and mpool to disk.
		#
		set origenv0 $env0
		set origenv1 $env1
		set origdb1 $db1
		set origdb $db
		puts "\tRep$tnum.c.0: Resolve ($op1 $op2) and recover master."
		error_check_good resolve1 [$txn1 $op1] 0
		error_check_good resolve2 [$txn2 $op2] 0
		error_check_good flush0 [$env0 log_flush] 0
		error_check_good sync0 [$env0 mpool_sync] 0
		process_msgs $envlist
		set env0 [eval $ma_envcmd -recover]
		error_check_good master_env [is_valid_env $env0] TRUE
		puts "\tRep$tnum.c.1: Run txn_recover on master env."
		set txnlist [$env0 txn_recover]
		error_check_good txnlist_len [llength $txnlist] 0

		puts "\tRep$tnum.c.2: Sync and recover client environment."
		error_check_good flush1 [$env1 log_flush] 0
		error_check_good sync1 [$env1 mpool_sync] 0
		set env1 [eval $cl_envcmd -recover]
		error_check_good client_env [is_valid_env $env1] TRUE
		puts "\tRep$tnum.c.3: Run txn_recover on client env."
		set txnlist [$env1 txn_recover]
		error_check_good txnlist_len [llength $txnlist] 0

		puts "\tRep$tnum.c.4: Restart replication on both envs."
		error_check_good master [$env0 rep_start -master] 0
		error_check_good client [$env1 rep_start -client] 0
		set newmaster $env0
		set envlist "{$env0 1} {$env1 2} {$env2 3}"
		catch {$origenv0 close} res
		catch {$origenv1 close} res
		catch {$origdb close} res
		catch {$origdb1 close} res
	} elseif { $prep == "envrecover" || $prep == "dbrecover" } {
		#
		# To simulate a crash, simply stop using the old handles
		# and reopen new ones, with recovery.  First flush both
		# the log and mpool to disk.
		#
		set origenv0 $env0
		set origenv1 $env1
		set origtxn1 $txn1
		set origtxn2 $txn2
		puts "\tRep$tnum.c.0: Sync and recover master environment."
		error_check_good flush1 [$env0 log_flush] 0
		error_check_good sync1 [$env0 mpool_sync] 0
		set oldgen [stat_field $env0 rep_stat "Generation number"]
		error_check_good flush1 [$env1 log_flush] 0
		error_check_good sync1 [$env1 mpool_sync] 0
		if { $after == 1 } {
			$db1 close
			$db close
		}
		if { $prep == "dbrecover" } {
			set recargs "-h $masterdir -c "
			set stat [catch {eval exec $util_path/db_recover \
			    -e $recargs} result]
			if { $stat == 1 } {
				error "FAIL: Recovery error: $result."
			}
			set recargs "-h $clientdir -c "
			set stat [catch {eval exec $util_path/db_recover \
			    -e $recargs} result]
			if { $stat == 1 } {
				error "FAIL: Recovery error: $result."
			}
		}
		#
		# !!!
		# We still need to open with recovery, even if 'dbrecover'
		# because db_recover cannot open the env with replication
		# enabled.  But db_recover will be the real recovery that
		# needs to deal with the prepared txn.  This recovery below
		# for db_recover, should be a no-op essentially.
		#
		set recenv0 [eval $ma_envcmd -recover]
		error_check_good master_env [is_valid_env $recenv0] TRUE
		puts "\tRep$tnum.c.1: Run txn_recover on master env."
		set env0 [eval $ma_envcmd]
		error_check_good master_env [is_valid_env $env0] TRUE
		set txnlist [$env0 txn_recover]
		error_check_good txnlist_len [llength $txnlist] $pbnyc
		puts "\tRep$tnum.c.2: Resolve txn ($op) on master env."
		rep075_resolve $txnlist $oplist
		error_check_good recenv0_close [$recenv0 close] 0

		puts "\tRep$tnum.c.3: Recover client environment."
		set recenv1 [eval $cl_envcmd -recover -errpfx "ENV1REC"]
		error_check_good client_env [is_valid_env $recenv1] TRUE
		puts "\tRep$tnum.c.4: Run txn_recover on client env."
		set env1 [eval $cl_envcmd -errpfx "ENV1NEW"]
		error_check_good client_env [is_valid_env $env1] TRUE
		set txnlist [$env1 txn_recover]
		error_check_good txnlist_len [llength $txnlist] $pbnyc

		puts "\tRep$tnum.c.5: Resolve txns ($oplist) on client env."
		rep075_resolve $txnlist $oplist
		error_check_good recenv1_close [$recenv1 close] 0

		puts "\tRep$tnum.c.6: Restart replication on both envs."
		if { $prep == "dbrecover" } {
			#
			# XXX Since we ran db_recover, we lost the rep gen
			# and clientenv2 cannot detect the change.  Until
			# SR 15396 is fixed, we'll fake it by becoming
			# master, downgrading and then upgrading again to
			# advance the generation number.
			#
			error_check_good master [$env0 rep_start -master] 0
			error_check_good master [$env0 rep_start -client] 0
			replclear 2
			replclear 3
		}
		error_check_good master [$env0 rep_start -master] 0
		set gen [stat_field $env0 rep_stat "Generation number"]
		#
		# If in-memory rep, restarting environment puts gen back
		# to 1, the same as oldgen. envrecover doesn't do the extra
		# rep_start, so gen is expected to stay at 1 in this case.
		#
		if { $repfiles_in_memory != 0 && $prep == "envrecover" } {
			error_check_good gen $gen $oldgen
		} else {
			error_check_bad gen $gen $oldgen
		}
		error_check_good client [$env1 rep_start -client] 0
		set newmaster $env0
		set envlist "{$env0 1} {$env1 2} {$env2 3}"
		process_msgs $envlist
		#
		# Clean up old Tcl handles.
		#
		catch {$origenv0 close} res
		catch {$origenv1 close} res
		catch {$origtxn1 close} res
		catch {$origtxn2 close} res
	}
	#
	# Run a standard rep_test creating test.db now.
	#
	eval rep_test $method $newmaster NULL $niter 0 0 0 $largs
	process_msgs $envlist

	#
	# Verify whether or not the key exists in the databases both
	# on the client and the master.
	#
	puts "\tRep$tnum.d: Verify prepared data."
	foreach e $envlist {
		set env [lindex $e 0]
		if { $databases_in_memory } {
			set db1 [eval {berkdb_open_noerr -env $env\
			    -auto_commit -create -mode 0644} $largs\
			    $omethod $testfile]
		} else {
			set db1 [eval {berkdb_open_noerr -env $env\
			    -auto_commit -create -mode 0644} $largs\
			    $omethod $testfile $sub]
		}
		error_check_good dbopen [is_valid_db $db1] TRUE
		set db2 [eval {berkdb_open_noerr -env $env -auto_commit \
		    -create -mode 0644} $largs $omethod $testfile2]
		error_check_good dbopen [is_valid_db $db2] TRUE
		set k1 [$db1 get $key]
		set k2 [$db2 get $key]
		if { $op1 == "commit" } {
			error_check_good key [llength $k1] 1
		} else {
			error_check_good key [llength $k1] 0
		}
		if { $op2 == "commit" } {
			error_check_good key [llength $k2] 1
		} else {
			error_check_good key [llength $k2] 0
		}

		error_check_good db_close [$db1 close] 0
		error_check_good db_close [$db2 close] 0
	}
	error_check_good env0_close [$env0 close] 0
	error_check_good env1_close [$env1 close] 0
	error_check_good env2_close [$env2 close] 0

	replclose $testdir/MSGQUEUEDIR
	return
}

proc rep075_resolve { txnlist ops } {
	error_check_good resolve_lists [llength $txnlist] [llength $ops]
	foreach trec $txnlist op $ops {
		set txn [lindex $trec 0]
		error_check_good commit [$txn $op] 0
	}
}

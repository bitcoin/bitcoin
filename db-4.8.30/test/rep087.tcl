# See the file LICENSE for redistribution information.
#
# Copyright (c) 2009 Oracle.  All rights reserved.
#
# TEST	rep087
# TEST  Abbreviated internal init with open file handles.
# TEST
# TEST  Client has open handle to an on-disk DB when abbreviated
# TEST  internal init starts.  Make sure we lock out access, and make sure
# TEST  it ends up as HANDLE_DEAD.  Also, make sure that if there are
# TEST  no NIMDBs, that we *don't* get HANDLE_DEAD.

proc rep087 { method { niter 200 } { tnum "087" } args } {
	source ./include.tcl

	if { $is_windows9x_test == 1 } {
		puts "Skipping replication test on Win9x platform."
		return
	}

	# Run for btree and queue only.  Since this is a NIMDB test, 
	# explicitly exclude queueext. 
	if { $checking_valid_methods } {
		set test_methods {}
		foreach method $valid_methods {
			if { [is_btree $method] == 1 || [is_queue $method] == 1 } {
				if { [is_queueext $method] == 0 } {	
					lappend test_methods $method
				}
			}
		}
		return $test_methods
	}
	if { [is_btree $method] != 1 && [is_queue $method] != 1 } {
		puts "Skipping internal init test rep$tnum for method $method."
		return
	}
	if { [is_queueext $method] == 1 } {
		puts "Skipping in-memory database test rep$tnum for method $method."
		return
	}

	set args [convert_args $method $args]

	# Run test with and without a NIMDB present.
	rep087_sub $method $niter $tnum "true" $args
	rep087_sub $method $niter $tnum "false" $args
}	

proc rep087_sub { method niter tnum with_nimdb largs } {
	global testdir
	global util_path
	global rep_verbose
	global verbose_type
	
	set verbargs ""
	if { $rep_verbose == 1 } {
		set verbargs " -verbose {$verbose_type on} "
	}
	if { $with_nimdb} {
		set msg "with"
	} else {
		set msg "without"
	}
	puts "Rep$tnum ($method):\
	    Abbreviated internal init and dead handles, $msg NIMDB."
	if { $niter < 3 } {
		set niter 3
		puts "\tRep$tnum: the minimum 'niter' value is 3."
	}

	set omethod [convert_method $method]

	env_cleanup $testdir
	replsetup $testdir/MSGQUEUEDIR

	file mkdir [set dirs(A) $testdir/SITE_A]
	file mkdir [set dirs(B) $testdir/SITE_B]

	puts "\tRep$tnum: Create master and client"
	repladd 1
	set env_A_cmd "berkdb_env_noerr -create -txn \
	    $verbargs \
	    -errpfx SITE_A \
	    -home $dirs(A) -rep_transport \[list 1 replsend\]"
	set envs(A) [eval $env_A_cmd -rep_master]

	# Open a client
	repladd 2
	set env_B_cmd "berkdb_env_noerr -create -txn \
	    $verbargs \
	    -errpfx SITE_B \
	    -home $dirs(B) -rep_transport \[list 2 replsend\]"
	set envs(B) [eval $env_B_cmd -rep_client]

	set envlist "{$envs(A) 1} {$envs(B) 2}"
	process_msgs $envlist

	if { $with_nimdb } {
		set msg "and a NIMDB"
	} else {
		set msg ""
	}
	puts "\tRep$tnum: Create a regular DB $msg"
	set start 0
	eval rep_test $method $envs(A) NULL $niter $start $start 0 $largs

	if { $with_nimdb } {
		set nimdb [eval {berkdb_open} -env $envs(A) -auto_commit \
			    -create $largs $omethod {"" "mynimdb"}]
		eval rep_test $method $envs(A) \
		    $nimdb $niter $start $start 0 $largs
		$nimdb close
	}
	process_msgs $envlist
	
	puts "\tRep$tnum: Restart client with recovery"
	#
	# In the NIMDB case, this forces the rematerialization of the NIMDB.
	# 
	$envs(B) close
	set envs(B) [eval $env_B_cmd -rep_client -recover]
	set envlist "{$envs(A) 1} {$envs(B) 2}"

	# Before seeking the master, open a DB handle onto the regular DB.
	# At this point, we should be allowed to read it.  
	#
	# Try reading a few records.  (How many?  We arbitrarily choose to try
	# reading three.)  Save one of the keys so that we can use it later in a
	# "$db get" call.  (Superstitiously skip over the first key, in deciding
	# which one to save, because it is usually a zero-length string.)
	# 
	set db [berkdb_open_noerr -env $envs(B) -auto_commit test.db]
	set c [$db cursor]
	$c get -next
	set pairs [$c get -next]
	set a_key [lindex $pairs 0 0]
	$c get -next
	$c close

	if { $with_nimdb} {
		# At this point, the NIMDB is obviously not available, since it
		# was blown away by the recovery/recreation of regions.  Let's
		# just make sure.
		#
		error_check_bad no_nimdb \
		    [catch {berkdb_open_noerr -env $envs(B) \
				-auto_commit "" "mynimdb"}] 0

		# Use the usual idiom of processing just one message cycle at a
		# time, so that we can check access during the middle of
		# internal init.  (If no NIMDB, there is no internal init, so
		# there's no point in doing this for that case.)

		# 1. NEWCLIENT -> NEWMASTER -> VERIFY_REQ (the checkpoint 
		#                 written by regular recovery)
		# 2.   -> VERIFY -> (no match) VERIFY_REQ (last txn commit in 
		#                 common)
		# 3.   -> VERIFY -> (match, but need NIMDBS) UPDATE_REQ
		# 4.   -> UPDATE -> PAGE_REQ
		# 5.   -> PAGE -> (limited to partial NIMDB content by 
		#                 rep_limit)
		
		proc_msgs_once $envlist
		proc_msgs_once $envlist
		proc_msgs_once $envlist
		proc_msgs_once $envlist
		
		# Before doing cycle # 5, set a ridiculously low limit, so that
		# only the first page of the database will be received on this
		# next cycle.
		# 
		$envs(A) rep_limit 0 4
		proc_msgs_once $envlist
		
		# Now we should be blocked from reading from our DB.
		puts "\tRep$tnum: Try blocked access (5 second delay)."
		error_check_bad should_block [catch {$db get $a_key} ret] 0
		error_check_good deadlock [is_substr $ret DB_LOCK_DEADLOCK] 1
		
		# Get rid of any limit for the remainder of the test.
		# 
		$envs(A) rep_limit 0 0
	}

	# Finish off all pending message processing.
	# 
	process_msgs $envlist
	
	if { $with_nimdb } { 
		# We should of course be able to open, and read a few
		# records from, the NIMDB, now that we've completed the
		# abbreviated internal init.
		# 
		set imdb [berkdb_open_noerr -env $envs(B) \
			      -auto_commit "" "mynimdb"]
		set c [$imdb cursor]
		$c get -next
		$c get -next
		$c get -next
		$c close
		$imdb close

		puts "\tRep$tnum: Try access to dead handle."
		error_check_bad handle_dead [catch {$db get $a_key} ret] 0
		error_check_good $ret [is_substr $ret DB_REP_HANDLE_DEAD] 1

		$db close
		set db [berkdb_open_noerr -env $envs(B) -auto_commit test.db]
		error_check_good reaccess_ok [catch {$db get $a_key} ret] 0
	} else {
		puts "\tRep$tnum: Try access to still-valid handle"
		error_check_good access_ok [catch {$db get $a_key} ret] 0
	}

	puts "\tRep$tnum: Clean up."
	$db close
	$envs(A) close
	$envs(B) close
	replclose $testdir/MSGQUEUEDIR
}

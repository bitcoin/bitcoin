# See the file LICENSE for redistribution information.
#
# Copyright (c) 2003-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	test106
# TEST
# TEST
# TEST
proc test106 { method {nitems 100} {niter 200} {tnum "106"} args } {
	source ./include.tcl
	global dict
	global rand_init

	# Set random seed for use in t106script procs op2 and create_data.
	error_check_good set_random_seed [berkdb srand $rand_init] 0

	set args [convert_args $method $args]
	set omethod [convert_method $method]
	set pageargs ""
	split_pageargs $args pageargs

	if { [is_btree $method] != 1 } {
		puts "\tTest$tnum: Skipping for method $method."
		return
	}

	# Skip for specified pagesizes.  This test runs at the native
	# pagesize.  (For SR #7964 testing, we may need to force
	# to 8192.)
	set pgindex [lsearch -exact $args "-pagesize"]
	if { $pgindex != -1 } {
		puts "Test$tnum: Skipping for specific pagesizes"
		return
	}

	# This test needs a txn-enabled environment.  If one is not
	# provided, create it.
	#
	set eindex [lsearch -exact $args "-env"]
	if { $eindex == -1 } {
		set env \
		    [eval {berkdb_env -create -home $testdir -txn} $pageargs]
	} else {
		incr eindex
		set env [lindex $args $eindex]
		set txnenv [is_txnenv $env]
		if { $txnenv != 1 } {
			puts "Skipping test$tnum for non-txn environment."
			return
		}
		set testdir [get_home $env]
	}

	cleanup $testdir $env

	# The bulk of the work of this test is done in t106script.tcl.
	# Here we kick off one consumer, then five producers, then sit
	# back and wait for them to finish.
	foreach order { ordered random } {
		set nproducers 5

		puts "\tTest$tnum.a: Start deadlock detector ($order)."
		set dpid [exec $util_path/db_deadlock -a o -v -t 5\
		    -h $testdir >& $testdir/dd.out &]

		puts "\tTest$tnum.b: Start consumer process ($order)."
		sentinel_init
		set pidlist {}
		set cpid [exec $tclsh_path $test_path/wrap.tcl t106script.tcl \
		    $testdir/t106script.log.cons.$order.1 $testdir WAIT \
		    0 $nproducers $testdir/CONSUMERLOG 1 $tnum $order $niter \
		    $args &]
		lappend pidlist $cpid

		puts "\tTest$tnum.c: Initialize producers ($order)."
		for { set p 1 } { $p <= $nproducers } { incr p } {
			set ppid [exec $tclsh_path $test_path/wrap.tcl \
			    t106script.tcl \
			    $testdir/t106script.log.init.$order.$p \
			    $testdir INITIAL $nitems $nproducers \
			    $testdir/INITLOG.$p $p $tnum \
			    $order $niter $args &]
			lappend pidlist $ppid
		}

		# Wait for all producers to be initialized before continuing
		# to the RMW portion of the test.
		watch_procs $pidlist 10

		sentinel_init
		set pidlist {}
		puts "\tTest$tnum.d: Run producers in RMW mode ($order)."
		for { set p 1 } { $p <= $nproducers } { incr p } {
			set ppid [exec $tclsh_path $test_path/wrap.tcl \
			    t106script.tcl \
			    $testdir/t106script.log.prod.$order.$p \
			    $testdir PRODUCE $nitems $nproducers \
			    $testdir/PRODUCERLOG.$p $p $tnum \
			    $order $niter $args &]
			lappend pidlist $ppid
		}

		watch_procs $pidlist 10
		tclkill $dpid
	}

	# If this test created the env, close it.
	if { $eindex == -1 } {
		error_check_good env_close [$env close] 0
	}
}

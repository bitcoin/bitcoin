# See the file LICENSE for redistribution information.
#
# Copyright (c) 1996-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	test042
# TEST	Concurrent Data Store test (CDB)
# TEST
# TEST	Multiprocess DB test; verify that locking is working for the
# TEST	concurrent access method product.
# TEST
# TEST	Use the first "nentries" words from the dictionary.  Insert each with
# TEST	self as key and a fixed, medium length data string.  Then fire off
# TEST	multiple processes that bang on the database.  Each one should try to
# TEST	read and write random keys.  When they rewrite, they'll append their
# TEST	pid to the data string (sometimes doing a rewrite sometimes doing a
# TEST	partial put).  Some will use cursors to traverse through a few keys
# TEST	before finding one to write.

proc test042 { method {nentries 1000} args } {
	global encrypt

	#
	# If we are using an env, then skip this test.  It needs its own.
	set eindex [lsearch -exact $args "-env"]
	if { $eindex != -1 } {
		incr eindex
		set env [lindex $args $eindex]
		puts "Test042 skipping for env $env"
		return
	}

	set args [convert_args $method $args]
	if { $encrypt != 0 } {
		puts "Test042 skipping for security"
		return
	}
	test042_body $method $nentries 0 $args
	test042_body $method $nentries 1 $args
}

proc test042_body { method nentries alldb args } {
	source ./include.tcl

	if { $alldb } {
		set eflag "-cdb -cdb_alldb"
	} else {
		set eflag "-cdb"
	}
	puts "Test042: CDB Test ($eflag) $method $nentries"

	# Set initial parameters
	set do_exit 0
	set iter 10000
	set procs 5

	# Process arguments
	set oargs ""
	for { set i 0 } { $i < [llength $args] } {incr i} {
		switch -regexp -- [lindex $args $i] {
			-dir	{ incr i; set testdir [lindex $args $i] }
			-iter	{ incr i; set iter [lindex $args $i] }
			-procs	{ incr i; set procs [lindex $args $i] }
			-exit	{ set do_exit 1 }
			default { append oargs " " [lindex $args $i] }
		}
	}

	set pageargs ""
	set args [split_pageargs $args pageargs]

	# Create the database and open the dictionary
	set basename test042
	set t1 $testdir/t1
	set t2 $testdir/t2
	set t3 $testdir/t3

	env_cleanup $testdir

	set env [eval {berkdb_env -create} $eflag $pageargs -home $testdir]
	error_check_good dbenv [is_valid_env $env] TRUE

	# Env is created, now set up database
	test042_dbinit $env $nentries $method $oargs $basename.0.db
	if { $alldb } {
		for { set i 1 } {$i < $procs} {incr i} {
			test042_dbinit $env $nentries $method $oargs \
			    $basename.$i.db
		}
	}

	# Remove old mpools and Open/create the lock and mpool regions
	error_check_good env:close:$env [$env close] 0
	set ret [berkdb envremove -home $testdir]
	error_check_good env_remove $ret 0

	set env [eval {berkdb_env \
	    -create -cachesize {0 1048576 1}} $pageargs $eflag -home $testdir]
	error_check_good dbenv [is_valid_widget $env env] TRUE

	if { $do_exit == 1 } {
		return
	}

	# Now spawn off processes
	berkdb debug_check
	puts "\tTest042.b: forking off $procs children"
	set pidlist {}

	for { set i 0 } {$i < $procs} {incr i} {
		if { $alldb } {
			set tf $basename.$i.db
		} else {
			set tf $basename.0.db
		}
		puts "exec $tclsh_path $test_path/wrap.tcl \
		    mdbscript.tcl $testdir/test042.$i.log \
		    $method $testdir $tf $nentries $iter $i $procs $args &"
		set p [exec $tclsh_path $test_path/wrap.tcl \
		    mdbscript.tcl $testdir/test042.$i.log $method \
		    $testdir $tf $nentries $iter $i $procs $args &]
		lappend pidlist $p
	}
	puts "Test042: $procs independent processes now running"
	watch_procs $pidlist

	# Make sure we haven't added or lost any entries.
	set dblist [glob $testdir/$basename.*.db]
	foreach file $dblist {
		set tf [file tail $file]
		set db [eval {berkdb_open -env $env} $oargs $tf]
		set statret [$db stat]
		foreach pair $statret {
			set fld [lindex $pair 0]
			if { [string compare $fld {Number of records}] == 0 } {
				set numrecs [lindex $pair 1]
				break
			}
		}
		error_check_good nentries $numrecs $nentries
		error_check_good db_close [$db close] 0
	}

	# Check for test failure
	set errstrings [eval findfail [glob $testdir/test042.*.log]]
	foreach str $errstrings {
		puts "FAIL: error message in log file: $str"
	}

	# Test is done, blow away lock and mpool region
	reset_env $env
}

proc test042_dbinit { env nentries method oargs tf } {
	global datastr
	source ./include.tcl

	set omethod [convert_method $method]
	set db [eval {berkdb_open -env $env -create \
	    -mode 0644 $omethod} $oargs $tf]
	error_check_good dbopen [is_valid_db $db] TRUE

	set did [open $dict]

	set pflags ""
	set gflags ""
	set txn ""
	set count 0

	# Here is the loop where we put each key/data pair
	puts "\tTest042.a: put loop $tf"
	while { [gets $did str] != -1 && $count < $nentries } {
		if { [is_record_based $method] == 1 } {
			set key [expr $count + 1]
		} else {
			set key $str
		}
		set ret [eval {$db put} \
		    $txn $pflags {$key [chop_data $method $datastr]}]
		error_check_good put:$db $ret 0
		incr count
	}
	close $did
	error_check_good close:$db [$db close] 0
}

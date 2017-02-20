# See the file LICENSE for redistribution information.
#
# Copyright (c) 1996-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	test045
# TEST	Small random tester
# TEST		Runs a number of random add/delete/retrieve operations.
# TEST		Tests both successful conditions and error conditions.
# TEST
# TEST	Run the random db tester on the specified access method.
#
# Options are:
#	-adds <maximum number of keys before you disable adds>
#	-cursors <number of cursors>
#	-dataavg <average data size>
#	-delete <minimum number of keys before you disable deletes>
#	-dups <allow duplicates in file>
#	-errpct <Induce errors errpct of the time>
#	-init <initial number of entries in database>
#	-keyavg <average key size>
proc test045 { method {nops 10000} args } {
	source ./include.tcl
	global encrypt

	#
	# If we are using an env, then skip this test.  It needs its own.
	set eindex [lsearch -exact $args "-env"]
	if { $eindex != -1 } {
		incr eindex
		set env [lindex $args $eindex]
		puts "Test045 skipping for env $env"
		return
	}
	set args [convert_args $method $args]
	if { $encrypt != 0 } {
		puts "Test045 skipping for security"
		return
	}
	set omethod [convert_method $method]

	puts "Test045: Random tester on $method for $nops operations"

	# Set initial parameters
	set adds [expr $nops * 10]
	set cursors 5
	set dataavg 40
	set delete $nops
	set dups 0
	set errpct 0
	set init 0
	if { [is_record_based $method] == 1 } {
		set keyavg 10
	} else {
		set keyavg 25
	}

	# Process arguments
	set oargs ""
	for { set i 0 } { $i < [llength $args] } {incr i} {
		switch -regexp -- [lindex $args $i] {
			-adds	 { incr i; set adds [lindex $args $i] }
			-cursors { incr i; set cursors [lindex $args $i] }
			-dataavg { incr i; set dataavg [lindex $args $i] }
			-delete	 { incr i; set delete [lindex $args $i] }
			-dups	 { incr i; set dups [lindex $args $i] }
			-errpct	 { incr i; set errpct [lindex $args $i] }
			-init	 { incr i; set init [lindex $args $i] }
			-keyavg	 { incr i; set keyavg [lindex $args $i] }
			-extent	 { incr i;
				    lappend oargs "-extent" "100" }
			default	 { lappend oargs [lindex $args $i] }
		}
	}

	# Create the database and and initialize it.
	set root $testdir/test045
	set f $root.db
	env_cleanup $testdir

	# Run the script with 3 times the number of initial elements to
	# set it up.
	set db [eval {berkdb_open \
	     -create -mode 0644 $omethod} $oargs {$f}]
	error_check_good dbopen:$f [is_valid_db $db] TRUE

	set r [$db close]
	error_check_good dbclose:$f $r 0

	# We redirect standard out, but leave standard error here so we
	# can see errors.

	puts "\tTest045.a: Initializing database"
	if { $init != 0 } {
		set n [expr 3 * $init]
		exec $tclsh_path \
		    $test_path/dbscript.tcl $method $f $n \
		    1 $init $n $keyavg $dataavg $dups 0 -1 \
		    > $testdir/test045.init
	}
	# Check for test failure
	set initerrs [findfail $testdir/test045.init]
	foreach str $initerrs {
		puts "FAIL: error message in .init file: $str"
	}

	puts "\tTest045.b: Now firing off berkdb rand dbscript, running: "
	# Now the database is initialized, run a test
	puts "$tclsh_path\
	    $test_path/dbscript.tcl $method $f $nops $cursors $delete $adds \
	    $keyavg $dataavg $dups $errpct $oargs > $testdir/test045.log"

	exec $tclsh_path \
	    $test_path/dbscript.tcl $method $f \
	    $nops $cursors $delete $adds $keyavg \
	    $dataavg $dups $errpct $oargs\
	    > $testdir/test045.log

	# Check for test failure
	set logerrs [findfail $testdir/test045.log]
	foreach str $logerrs {
		puts "FAIL: error message in log file: $str"
	}
}

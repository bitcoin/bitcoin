# See the file LICENSE for redistribution information.
#
# Copyright (c) 2006-2009 Oracle.  All rights reserved.
#
# $Id$
#
# rep065script - procs to use at each replication site in the
# replication upgrade test.
#
# type: START, PROCMSGS, VERIFY
# 	START starts up a replication site and performs an operation.
#		the operations are:
#		REPTEST runs the rep_test_upg procedure on the master.
#		REPTEST_GET run a read-only test on a client.
#		REPTEST_ELECT runs an election on the site.
# 	PROCMSGS processes messages until none are left.
#	VERIFY dumps the log and database contents.
# role: master or client
# op: operation to perform
# envid: environment id number for use in replsend
# allids: all env ids we need for sending
# ctldir: controlling directory
# mydir: directory where this participant runs
# reputils_path: location of reputils.tcl

proc rep065scr_elect { repenv oplist } {
	set ver [lindex $oplist 1]
	set pri [lindex $oplist 2]
}

proc rep065scr_reptest { repenv oplist markerdb } {

	set method [lindex $oplist 1]
	set niter [lindex $oplist 2]
	set loop [lindex $oplist 3]
	set start 0
	puts "REPTEST: method $method, niter $niter, loop $loop"

	for {set n 0} {$n < $loop} {incr n} {
		puts "REPTEST: call rep_test_upg $n"
		eval rep_test_upg $method $repenv NULL $niter $start $start 0 0
		incr start $niter
		tclsleep 3
	}
	#
	# Sleep a bunch to help get the messages worked through.
	#
	tclsleep 10
	puts "put DONE to marker"
       	error_check_good marker_done [$markerdb put DONE DONE] 0
       	error_check_good marker_sync [$markerdb sync] 0
}

proc rep065scr_repget { repenv oplist mydir markerfile } {
	set dbname "$mydir/test.db"
	set i 0
	while { [file exists $dbname] == 0 } {
		tclsleep 2
		incr i
		if { $i >= 15 && $i % 5 == 0 } {
			puts "After $i seconds, no database exists."
		}
		if { $i > 180 } {
			error "Database never created."
		}
	}
	set loop 1
	while { 1 } {
		set markerdb [berkdb_open $markerfile]
		error_check_good marker [is_valid_db $markerdb] TRUE
		set kd [$markerdb get DONE]
		error_check_good marker_close [$markerdb close] 0
		if { [llength $kd] != 0 } {
			break
		}
		set db [berkdb_open -env $repenv $dbname]
		error_check_good dbopen [is_valid_db $db] TRUE
		set dbc [$db cursor]
		set i 0
		error_check_good curs [is_valid_cursor $dbc $db] TRUE
		for { set dbt [$dbc get -first ] } \
		    { [llength $dbt] > 0 } \
		    { set dbt [$dbc get -next] } {
			incr i
		}
		error_check_good dbc_close [$dbc close] 0
		error_check_good db_close [$db close] 0
		puts "REPTEST_GET: after $loop loops: key count $i"
		incr loop
		tclsleep 2
	}
}
proc rep065scr_starttest { role oplist envid msgdir mydir allids markerfile } {
	global qtestdir
	global util_path
	global repfiles_in_memory

	puts "repladd_noenv $allids"
	set qtestdir $msgdir
	foreach id $allids {
		repladd_noenv $id
	}

	set repmemargs ""
	if { $repfiles_in_memory } {
		set repmemargs "-rep_inmem_files "
	}

	set markerdb [berkdb_open -create -btree $markerfile]
	error_check_good marker [is_valid_db $markerdb] TRUE
	puts "set up env cmd"
	set lockmax 40000
	set logbuf [expr 16 * 1024]
	set logmax [expr $logbuf * 4]
	if { $role == "MASTER" } {
		set rep_env_cmd "berkdb_env_noerr -create -home $mydir \
		    -log_max $logmax -log_buffer $logbuf $repmemargs \
		    -lock_max_objects $lockmax -lock_max_locks $lockmax \
		    -errpfx MASTER -txn -rep_master \
		    -rep_transport \[list $envid replsend_noenv\]"
		set rep_env_cmd "berkdb_env_noerr -create -home $mydir \
		    -log_max $logmax -log_buffer $logbuf $repmemargs \
		    -lock_max_objects $lockmax -lock_max_locks $lockmax \
		    -errpfx MASTER -txn -rep_master \
		    -verbose {rep on} -errfile /dev/stderr \
		    -rep_transport \[list $envid replsend_noenv\]"
	} elseif { $role == "CLIENT" } {
		set rep_env_cmd "berkdb_env_noerr -create -home $mydir \
		    -log_max $logmax -log_buffer $logbuf $repmemargs \
		    -lock_max_objects $lockmax -lock_max_locks $lockmax \
		    -errpfx CLIENT -txn -rep_client \
		    -rep_transport \[list $envid replsend_noenv\]"
		set rep_env_cmd "berkdb_env_noerr -create -home $mydir \
		    -log_max $logmax -log_buffer $logbuf $repmemargs \
		    -lock_max_objects $lockmax -lock_max_locks $lockmax \
		    -errpfx CLIENT -txn -rep_client \
		    -verbose {rep on} -errfile /dev/stderr \
		    -rep_transport \[list $envid replsend_noenv\]"
	} else {
		puts "FAIL: unrecognized replication role $role"
		return
	}

	# Change directories to where this will run.
	# !!!
	# mydir is an absolute path of the form
	# <path>/build_unix/TESTDIR/MASTERDIR or
	# <path>/build_unix/TESTDIR/CLIENTDIR.0
	#
	# So we want to run relative to the build_unix directory
	cd $mydir/../..

	puts "open repenv $rep_env_cmd"
	set repenv [eval $rep_env_cmd]
	error_check_good repenv_open [is_valid_env $repenv] TRUE

	puts "repenv is $repenv"
	#
	# Indicate that we're done starting up.  Sleep to let
	# others do the same.
	#
	puts "put START$envid to marker"
        error_check_good marker_done [$markerdb put START$envid START$envid] 0
        error_check_good marker_sync [$markerdb sync] 0
	puts "sleeping after marker"
	tclsleep 3

	# Here is where the real test starts.
	#
	# Different operations may have different args in their list.
	# REPTEST: Args are method, niter, nloops
	set op [lindex $oplist 0]
	if { $op == "REPTEST" } {
		#
		# This test writes the marker, so close after it runs.
		#
		rep065scr_reptest $repenv $oplist $markerdb
		error_check_good marker_close [$markerdb close] 0
	}
	if { $op == "REPTEST_GET" } {
		#
		# This test needs to poll the marker.  So close it now.
		#
		error_check_good marker_close [$markerdb close] 0
		rep065scr_repget $repenv $oplist $mydir $markerfile
	}
	if { $op == "REP_ELECT" } {
		#
		# This test writes the marker, so close after it runs.
		#
		rep065scr_elect $repenv $oplist $markerdb
	}
	puts "Closing env"
	$repenv mpool_sync
	error_check_good envclose [$repenv close] 0

}

proc rep065scr_msgs { role envid msgdir mydir allids markerfile } {
	global qtestdir
	global repfiles_in_memory

	set repmemargs ""
	if { $repfiles_in_memory } {
		set repmemargs "-rep_inmem_files "
	}

	#
	# The main test process will write the marker file when it
	# has started and when it has completed.  We need to
	# open/close the marker file because we are in a separate
	# process from the writer and we cannot share an env because
	# we might be a different BDB release version.
	#
	set markerdb [berkdb_open -create -btree $markerfile]
	error_check_good marker [is_valid_db $markerdb] TRUE
	set s [$markerdb get START$envid]
	while { [llength $s] == 0 } {
		error_check_good marker_close [$markerdb close] 0
		tclsleep 1
		set markerdb [berkdb_open $markerfile]
		error_check_good marker [is_valid_db $markerdb] TRUE
		set s [$markerdb get START$envid]
	}

	puts "repladd_noenv $allids"
	set qtestdir $msgdir
	foreach id $allids {
		repladd_noenv $id
	}

	puts "set up env cmd"
	if { $role == "MASTER" } {
		set rep_env_cmd "berkdb_env_noerr -home $mydir \
		    -errpfx MASTER -txn -rep_master $repmemargs \
		    -rep_transport \[list $envid replsend_noenv\]"
		set rep_env_cmd "berkdb_env_noerr -home $mydir \
		    -errpfx MASTER -txn -rep_master $repmemargs \
		    -verbose {rep on} -errfile /dev/stderr \
		    -rep_transport \[list $envid replsend_noenv\]"
	} elseif { $role == "CLIENT" } {
		set rep_env_cmd "berkdb_env_noerr -home $mydir \
		    -errpfx CLIENT -txn -rep_client $repmemargs \
		    -rep_transport \[list $envid replsend_noenv\]"
		set rep_env_cmd "berkdb_env_noerr -home $mydir \
		    -errpfx CLIENT -txn -rep_client $repmemargs \
		    -verbose {rep on} -errfile /dev/stderr \
		    -rep_transport \[list $envid replsend_noenv\]"
	} else {
		puts "FAIL: unrecognized replication role $role"
		return
	}

	# Change directories to where this will run.
	cd $mydir

	puts "open repenv $rep_env_cmd"
	set repenv [eval $rep_env_cmd]
	error_check_good repenv_open [is_valid_env $repenv] TRUE

	set envlist "{$repenv $envid}"
	puts "repenv is $repenv"
	while { 1 } {
		if { [llength [$markerdb get DONE]] != 0 } {
			break
		}
		process_msgs $envlist 0 NONE NONE 1
		error_check_good marker_close [$markerdb close] 0
		set markerdb [berkdb_open $markerfile]
		error_check_good marker [is_valid_db $markerdb] TRUE
		tclsleep 1
	}
	#
	# Process messages in case there are a few more stragglers.
	# Just because the main test is done doesn't mean that all
	# the messaging is done.  Loop for messages as long as
	# progress is being made.
	#
	set nummsg 1
	while { $nummsg != 0 } {
		process_msgs $envlist 0 NONE NONE 1
		tclsleep 1
		# First look at messages from us
		set nummsg [replmsglen_noenv $envid from]
		puts "Still have $nummsg not yet processed by others"
	}
	error_check_good marker_close [$markerdb close] 0
	replclear_noenv $envid from
	tclsleep 1
	replclear_noenv $envid
	$repenv mpool_sync
	error_check_good envclose [$repenv close] 0
}

proc rep065scr_verify { oplist mydir id } {
	global util_path

	set rep_env_cmd "berkdb_env_noerr -home $mydir -txn \
	    -rep_transport \[list $id replnoop\]"

	# Change directories to where this will run.
	# !!!
	# mydir is an absolute path of the form
	# <path>/build_unix/TESTDIR/MASTERDIR or
	# <path>/build_unix/TESTDIR/CLIENTDIR.0
	#
	# So we want to run relative to the build_unix directory
	cd $mydir/../..

	foreach op $oplist {
		set repenv [eval $rep_env_cmd]
		error_check_good env_open [is_valid_env $repenv] TRUE
		if { $op == "DB" } {
			set dbname "$mydir/test.db"
			set db [berkdb_open -env $repenv -rdonly $dbname]
			error_check_good dbopen [is_valid_db $db] TRUE
			set txn ""
			set method [$db get_type]
			if { [is_record_based $method] == 1 } {
				dump_file $db $txn $mydir/VERIFY/dbdump \
				    rep_test_upg.recno.check
			} else {
				dump_file $db $txn $mydir/VERIFY/dbdump \
				    rep_test_upg.check
			}
			error_check_good dbclose [$db close] 0
		}
		if { $op == "LOG" } {
			set lgstat [$repenv log_stat]
			set lgfile [stat_field $repenv log_stat "Current log file number"]
			set lgoff [stat_field $repenv log_stat "Current log file offset"]
			puts "Current LSN: $lgfile $lgoff"
			set f [open $mydir/VERIFY/loglsn w]
			puts $f $lgfile
			puts $f $lgoff
			close $f

			set stat [catch {eval exec $util_path/db_printlog \
			    -h $mydir > $mydir/VERIFY/prlog} result]
			if { $stat != 0 } {
				puts "PRINTLOG: $result"
			}
			error_check_good stat_prlog $stat 0
		}
		error_check_good envclose [$repenv close] 0
	}
	#
	# Run recovery locally so that any later upgrades are ready
	# to be upgraded.
	#
	set stat [catch {eval exec $util_path/db_recover -h $mydir} result]
	if { $stat != 0 } {
		puts "RECOVERY: $result"
	}
	error_check_good stat_rec $stat 0

}

set usage "upgradescript type role op envid allids ctldir mydir reputils_path"

# Verify usage
if { $argc != 8 } {
	puts stderr "Argc $argc, argv $argv"
	puts stderr "FAIL:[timestamp] Usage: $usage"
	exit
}

# Initialize arguments
set type [ lindex $argv 0 ]
set role [ lindex $argv 1 ]
set op [ lindex $argv 2 ]
set envid [ lindex $argv 3 ]
set allids [ lindex $argv 4 ]
set ctldir [ lindex $argv 5 ]
set mydir [ lindex $argv 6 ]
set reputils_path [ lindex $argv 7 ]

set histdir $mydir/../..
puts "Histdir $histdir"

set msgtestdir $ctldir/TESTDIR

global env
cd $histdir
set stat [catch {eval exec ./db_printlog -V} result]
if { $stat != 0 } {
	set env(LD_LIBRARY_PATH) ":$histdir:$histdir/.libs:$env(LD_LIBRARY_PATH)"
}
source ./include.tcl
source $test_path/test.tcl

# The global variable noenv_messaging must be set after sourcing
# test.tcl or its value will be wrong.
global noenv_messaging
set noenv_messaging 1

set is_repchild 1
puts "Did args. now source reputils"
source $reputils_path/reputils.tcl
source $reputils_path/reputilsnoenv.tcl

set markerdir $msgtestdir/MARKER
set markerfile $markerdir/marker.db

puts "Calling proc for type $type"
if { $type == "START" } {
	rep065scr_starttest $role $op $envid $msgtestdir $mydir $allids $markerfile
} elseif { $type == "PROCMSGS" } {
	rep065scr_msgs $role $envid $msgtestdir $mydir $allids $markerfile
} elseif { $type == "VERIFY" } {
	file mkdir $mydir/VERIFY
	rep065scr_verify $op $mydir $envid
} else {
	puts "FAIL: unknown type $type"
	return
}

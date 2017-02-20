# See the file LICENSE for redistribution information.
#
# Copyright (c) 2005-2009 Oracle.  All rights reserved.
#
# $Id$
#
# Rep045 script - replication with version dbs.
#
# Usage: rep045script clientdir vfile
# clientdir: client env directory
# vfile: name of version file
#
source ./include.tcl
source $test_path/test.tcl
source $test_path/testutils.tcl
source $test_path/reputils.tcl

set usage "repscript clientdir vfile"

# Verify usage
if { $argc != 3 } {
	puts stderr "FAIL:[timestamp] Usage: $usage"
	exit
}

# Initialize arguments
set clientdir [ lindex $argv 0 ]
set vfile [ lindex $argv 1 ]
global databases_in_memory
set databases_in_memory [ lindex $argv 2 ]
set niter 50

# Join the queue env.  We assume the rep test convention of
# placing the messages in $testdir/MSGQUEUEDIR.
set queueenv [eval berkdb_env -home $testdir/MSGQUEUEDIR]
error_check_good script_qenv_open [is_valid_env $queueenv] TRUE

# We need to set up our own machids.
repladd 3

# Join the client env.
set cl_cmd "berkdb_env_noerr -home $clientdir \
	-txn -rep_client -rep_transport \[list 3 replsend\]"
# set cl_cmd "berkdb_env_noerr -home $clientdir \
# 	-verbose {rep on} -errfile /dev/stderr \
# 	-txn -rep_client -rep_transport \[list 3 replsend\]"
set clientenv [eval $cl_cmd]
error_check_good script_cenv_open [is_valid_env $clientenv] TRUE

# Start up deadlock detector.
set dpid [exec $util_path/db_deadlock \
    -a o -v -t 5 -h $clientdir >& $testdir/dd.out &]

# Initialize version number.  Don't try to open the first
# version database until the master has completed setting it up.
set version 0
while {[catch {berkdb_open_noerr -env $clientenv -rdonly $vfile} vdb]} {
	puts "FAIL: vdb open failed: $vdb"
	tclsleep 1
}

while { $version == 0 } {
	tclsleep 1
	if { [catch {$vdb get VERSION} res] } {
		# If we encounter an error, check what kind of
		# error it is.
		if { [is_substr $res DB_LOCK_DEADLOCK] == 1 } {
			# We're deadlocked.  Just wait for the
			# deadlock detector to break the deadlock.
		} elseif { [is_substr $res DB_REP_HANDLE_DEAD] == 1 } {
			# Handle is dead.  Get a new handle.
			error_check_good vdb_close [$vdb close] 0
			set vdb [eval berkdb_open -env $clientenv\
			    -rdonly $vfile]
		} else {
			# We got something we didn't expect.
			puts "FAIL: Trying to get version, got $res"
			break
		}
	} else {
		# No error was encountered.
		set version [lindex [lindex $res 0] 1]
	}
}
error_check_good close_vdb [$vdb close] 0
set dbfile db.$version

# Open completed database version $version.
if {[catch {berkdb_open -rdonly -env $clientenv $dbfile} db]} {
	puts "FAIL: db open failed: $db"
}
error_check_good db_open [is_valid_db $db] TRUE

# While parent process is not done, read from current database.
# Periodically check version and update current database when
# necessary.
while { 1 } {
	set dbc [$db cursor]
	set i 0
	error_check_good cursor_open [is_valid_cursor $dbc $db] TRUE
	for { set dbt [$dbc get -first] } { $i < $niter } \
	    { set dbt [$dbc get -next] } {
		incr i
	}
	error_check_good cursor_close [$dbc close] 0

	while {[catch {berkdb_open -env $clientenv -rdonly $vfile} vdb]} {
		puts "open failed: vdb is $vdb"
		tclsleep 1
	}
	set ret [$vdb get VERSION]

	set newversion [lindex [lindex $ret 0] 1]
	error_check_good close_vdb [$vdb close] 0
	error_check_bad check_newversion $newversion ""
	if { $newversion != $version } {
		if { $newversion == "DONE" } {
			break
		} elseif { $newversion == 0 } {
			puts "FAIL: version has reverted to 0"
			continue
		} else {
			error_check_good db_close [$db close] 0
			set version $newversion
			set dbfile db.$version
			while {[catch \
			    {berkdb_open -env $clientenv -rdonly $dbfile} db]} {
				puts "db open of new db failed: $db"
				tclsleep 1
			}
			error_check_good db_open [is_valid_db $db] TRUE
		}
	}

	# Pause a few seconds to allow the parent to do some work.
	tclsleep 3
}

# Clean up.
error_check_good kill_deadlock_detector [tclkill $dpid] ""
error_check_good db_close [$db close] 0
error_check_good script_client_close [$clientenv close] 0

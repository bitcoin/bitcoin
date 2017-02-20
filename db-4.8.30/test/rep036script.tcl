# See the file LICENSE for redistribution information.
#
# Copyright (c) 2004-2009 Oracle.  All rights reserved.
#
# $Id$
#
# Rep036 script - create additional writers in master env.
#
# Usage: masterdir writerid
# masterdir: Directory of replication master
# writerid: i.d. number for writer
source ./include.tcl
source $test_path/test.tcl
source $test_path/testutils.tcl
source $test_path/reputils.tcl

global rand_init
set usage "repscript masterdir writerid nentries method"

# Verify usage
if { $argc != 4 } {
	puts stderr "FAIL:[timestamp] Usage: $usage"
	exit
}

# Initialize arguments
set masterdir [ lindex $argv 0 ]
set writerid [ lindex $argv 1 ]
set nentries [ lindex $argv 2 ]
set method [ lindex $argv 3 ]

# Join the queue env.  We assume the rep test convention of
# placing the messages in $testdir/MSGQUEUEDIR.
set queueenv [eval berkdb_env -home $testdir/MSGQUEUEDIR]
error_check_good script_qenv_open [is_valid_env $queueenv] TRUE

# We need to set up our own machid.
repladd 1
repladd 2

# Start up deadlock detector.
# Commented out - see #15049.
#set dpid [eval {exec $util_path/db_deadlock} \
#    -a o -v -t 2.0 -h $masterdir >& $testdir/dd.writer.$writerid.out &]

# Join the master env.
set envid 1
set env_cmd "berkdb_env_noerr -home $masterdir \
	-errfile /dev/stderr -errpfx WRITER.$writerid \
	-txn -rep_master -rep_transport \[list $envid replsend\]"
# set env_cmd "berkdb_env_noerr -home $masterdir \
# 	-errfile /dev/stderr -errpfx WRITER.$writerid \
# 	-verbose {rep on} \
# 	-txn -rep_master -rep_transport \[list $envid replsend\]"
set masterenv [eval $env_cmd]
error_check_good script_env_open [is_valid_env $masterenv] TRUE

# Open database.
set testfile "rep036.db"
set omethod [convert_method $method]
set mdb [eval {berkdb_open_noerr} -env $masterenv -auto_commit \
	    -create $omethod $testfile]
error_check_good dbopen [is_valid_db $mdb] TRUE

# Write records to the database.
set did [open $dict]
set count 0
set dictsize 10000
berkdb srand $rand_init
while { $count < $nentries } {
	#
	# If nentries exceeds the dictionary size, close
	# and reopen to start from the beginning again.
	if { [expr [expr $count + 1] % $dictsize] == 0 } {
		close $did
		set did [open $dict]
	}

	gets $did str
	set key WRITER.$writerid.$str
	set str [reverse $str]

	set t [$masterenv txn]
	error_check_good txn [is_valid_txn $t $masterenv] TRUE
	set txn "-txn $t"

# 	If using deadlock detection, uncomment this and comment the
#	following put statement.
#	# Writing to this database can deadlock.  If we do, let the
#	# deadlock detector break the lock, wait a second, and try again.
#	while { [catch {eval {$mdb put}\
#	    $txn {$key [chop_data $method $str]}} ret] } {
#		error_check_good deadlock [is_substr $ret DB_LOCK_DEADLOCK] 1
#		tclsleep 1
#	}
									 
	set ret [eval \
	    {$mdb put} $txn {$key [chop_data $method $str]}]
	error_check_good put $ret 0
	error_check_good txn [$t commit] 0

	if { [expr $count % 100] == 1 } {
		puts "Wrote WRITER.$writerid record $count"
		set sleep [berkdb random_int 0 10]
		puts "Writer.$writerid sleeping $sleep seconds"
		tclsleep $sleep
	}
	incr count
}
close $did

# Clean up.
# Uncomment following line if using deadlock detector.
#error_check_good kill_deadlock_detector [tclkill $dpid] ""
error_check_good mdb_close [$mdb close] 0
error_check_good masterenv_close [$masterenv close] 0
replclose $testdir/MSGQUEUEDIR

# Communicate with parent by creating a marker file.
set markerenv [berkdb_env -create -home $testdir -txn]
error_check_good markerenv_open [is_valid_env $markerenv] TRUE
set marker [eval "berkdb_open \
    -create -btree -auto_commit -env $markerenv $writerid.db"]
error_check_good marker_close [$marker close] 0
error_check_good markerenv_close [$markerenv close] 0

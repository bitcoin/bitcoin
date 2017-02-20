# See the file LICENSE for redistribution information.
#
# Copyright (c) 2003-2009 Oracle.  All rights reserved.
#
# $Id$
#
# Rep042 script - concurrency with updates.

# Usage: repscript masterdir sleepval dbname
# masterdir: master env directory
# sleepval: sleep value (in secs) to send to env test_check
# dbname: name of database to use
# op: operation: one of del or truncate
#
source ./include.tcl
source $test_path/reputils.tcl

set usage "repscript masterdir sleepval dbname op"

# Verify usage
if { $argc != 4 } {
	puts stderr "FAIL:[timestamp] Usage: $usage"
	exit
}

# Initialize arguments
set masterdir [ lindex $argv 0 ]
set sleepval [ lindex $argv 1 ]
set dbname [lindex $argv 2]
set op [lindex $argv 3]

# Join the queue env.  We assume the rep test convention of
# placing the messages in $testdir/MSGQUEUEDIR.
set queueenv [eval berkdb_env -home $testdir/MSGQUEUEDIR]
error_check_good script_qenv_open [is_valid_env $queueenv] TRUE

# We need to set up our own machids.
# Add 1 for master env id, and 2 for the clientenv id.
#
repladd 1
repladd 2

# Join the master env.
set ma_cmd "berkdb_env_noerr -home $masterdir \
	-txn -rep_master -rep_transport \[list 1 replsend\]"
# set ma_cmd "berkdb_env_noerr -home $masterdir  \
# 	-verbose {rep on} -errfile /dev/stderr  \
# 	-txn -rep_master -rep_transport \[list 1 replsend\]"
set masterenv [eval $ma_cmd]
error_check_good script_menv_open [is_valid_env $masterenv] TRUE

puts "Master open"
set db [eval "berkdb_open -auto_commit -env $masterenv $dbname"]
error_check_good dbopen [is_valid_db $db] TRUE

# Make it so that the process sleeps in the middle of a delete.
$masterenv test check $sleepval

# Create marker file
set marker [open $masterdir/marker.db w]
close $marker

if { $op == "del" } {
	# Just delete record 1 - we know that one is in there.
	set stat [catch {$db del 1} ret]
	puts "Stat: $stat"
	puts "Ret: $ret"
} elseif { $op == "truncate" } {
	set stat [catch {$db truncate} ret]
	puts "Stat: $stat"
	puts "Ret: $ret"
} else {
	puts "Stat: FAIL: invalid operation specified"
}
# Close the envs
error_check_good script_db_close [$db close] 0
error_check_good script_master_close [$masterenv close] 0
puts "\tRepscript completed successfully"

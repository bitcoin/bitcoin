# See the file LICENSE for redistribution information.
#
# Copyright (c) 2003-2009 Oracle.  All rights reserved.
#
# $Id$
#
# Rep078 script - Master leases.
#
# Test master leases and write operations.
#
# Usage: rep078script masterdir dbfile method
# masterdir: master env directory
# dbfile: name of database file
# method: access method
#
source ./include.tcl
source $test_path/test.tcl
source $test_path/testutils.tcl
source $test_path/reputils.tcl

set usage "repscript masterdir dbfile method"

# Verify usage
if { $argc != 3 } {
	puts stderr "FAIL:[timestamp] Usage: $usage"
	exit
}

# Initialize arguments
set masterdir [ lindex $argv 0 ]
set dbfile [ lindex $argv 1 ]
set method [ lindex $argv 2 ]

# Join the queue env.  We assume the rep test convention of
# placing the messages in $testdir/MSGQUEUEDIR.
set queueenv [eval berkdb_env -home $testdir/MSGQUEUEDIR]
error_check_good script_qenv_open [is_valid_env $queueenv] TRUE

#
# We need to set up our own machids.
# Add 2 for master env id, and 3 and 4 for the clientenv ids.
#
repladd 2
repladd 3
repladd 4

# Join the master env.
set ma_cmd "berkdb_env_noerr -home $masterdir \
	-txn -rep_transport \[list 2 replsend\]"
# set ma_cmd "berkdb_env_noerr -home $masterdir \
#	-verbose {rep on} -errfile /dev/stderr \
#	-txn -rep_transport \[list 2 replsend\]"
puts "Joining master env"
set masterenv [eval $ma_cmd]
error_check_good script_menv_open [is_valid_env $masterenv] TRUE

# Create a marker file.  Don't put anything in it yet.  The parent
# process will be processing messages while it looks for our
# marker.


puts "Create marker file"
set markerenv [berkdb_env -create -home $testdir -txn]
error_check_good markerenv_open [is_valid_env $markerenv] TRUE
set marker \
    [eval "berkdb_open -create -btree -auto_commit -env $markerenv marker.db"]

#
# Create the database and then do a lease operation.  Don't 
# process messages in the child process.
#
puts "Open database"
set args [convert_args $method]
puts "args is $args"
set omethod [convert_method $method]
set db [eval "berkdb_open -env $masterenv -auto_commit -create \
    $omethod $args $dbfile"]
error_check_good script_db_open [is_valid_db $db] TRUE

puts "Do lease op"
set key 1
do_leaseop $masterenv $db $method $key NULL 0

puts "Put CHILD1"
error_check_good child_key \
    [$marker put CHILD1 $key] 0

puts "Wait for PARENT1"
# Give the parent a chance to process messages and check leases.
while { [llength [$marker get PARENT1]] == 0 } {
	tclsleep 1
}

puts "Do lease op 2"
incr key
do_leaseop $masterenv $db $method $key NULL 0
puts "Put CHILD2"
error_check_good child2_key \
    [$marker put CHILD2 $key] 0

puts "Wait for PARENT2"
# Give the parent a chance to process messages and check leases.
while { [llength [$marker get PARENT2]] == 0 } {
	tclsleep 1
}

#
# After we get PARENT2, do a checkpoint.
# Then our work is done and we clean up.
#
puts "Write a checkpoint"
$masterenv txn_checkpoint
puts "Put CHILD3"
error_check_good child2_key \
    [$marker put CHILD3 $key] 0

puts "Clean up and exit"
# Clean up the child so the parent can go forward.
error_check_good master_db_close [$db close] 0
error_check_good marker_db_close [$marker close] 0
error_check_good markerenv_close [$markerenv close] 0
error_check_good script_master_close [$masterenv close] 0


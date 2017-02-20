# See the file LICENSE for redistribution information.
#
# Copyright (c) 2003-2009 Oracle.  All rights reserved.
#
# $Id$
#
# Rep017 script - concurrency with checkpoints.
#
# Repscript exists to process checkpoints, though the
# way it is currently written, it will process whatever
# it finds in the message queue.  It requires a one-master
# one-client setup.
#
# Usage: repscript masterdir clientdir rep_verbose verbose_type
# masterdir: master env directory
# clientdir: client env directory
#
source ./include.tcl
source $test_path/test.tcl
source $test_path/testutils.tcl
source $test_path/reputils.tcl

set usage "repscript masterdir clientdir rep_verbose verbose_type"

# Verify usage
if { $argc != 4 } {
	puts stderr "FAIL:[timestamp] Usage: $usage"
	exit
}

# Initialize arguments
set masterdir [ lindex $argv 0 ]
set clientdir [ lindex $argv 1 ]
set rep_verbose [ lindex $argv 2 ]
set verbose_type [ lindex $argv 3 ]
set verbargs "" 
if { $rep_verbose == 1 } {
	set verbargs " -verbose {$verbose_type on} "
}

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
set ma_cmd "berkdb_env_noerr -home $masterdir $verbargs \
	-txn -rep_master -rep_transport \[list 1 replsend\]"
set masterenv [eval $ma_cmd]
error_check_good script_menv_open [is_valid_env $masterenv] TRUE

puts "Master open"

# Join the client env.
set cl_cmd "berkdb_env_noerr -home $clientdir $verbargs \
	-txn -rep_client -rep_transport \[list 2 replsend\]"
set clientenv [eval $cl_cmd]
error_check_good script_cenv_open [is_valid_env $clientenv] TRUE

puts "Everyone open"
tclsleep 10

# Make it so that the client sleeps in the middle of checkpoints
$clientenv test check 10

puts "Client set"

# Update the client, in order to process the checkpoint
process_msgs "{$masterenv 1} {$clientenv 2}"


puts "Processed messages"

# Close the envs
error_check_good script_master_close [$masterenv close] 0
error_check_good script_client_close [$clientenv close] 0
puts "\tRepscript completed successfully"

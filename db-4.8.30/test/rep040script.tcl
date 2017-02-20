# See the file LICENSE for redistribution information.
#
# Copyright (c) 2003-2009 Oracle.  All rights reserved.
#
# $Id$
#
# Rep040 script - transaction concurrency with rep_start
#
# Repscript exists to call rep_start.  The main script will immediately
# start a transaction, do an operation, then sleep a long time before
# commiting the transaction.  We should be blocked on the transaction
# when we call rep_start.  The main process should sleep long enough
# that we get a diagnostic message.
#
# Usage: repscript masterdir clientdir
# masterdir: master env directory
# clientdir: client env directory
#
source ./include.tcl
source $test_path/test.tcl
source $test_path/testutils.tcl
source $test_path/reputils.tcl
global databases_in_memory

set usage "repscript masterdir"

# Verify usage
if { $argc != 2 } {
	puts stderr "FAIL:[timestamp] Usage: $usage"
	exit
}

# Initialize arguments
set masterdir [ lindex $argv 0 ]
set databases_in_memory [ lindex $argv 1 ]

puts "databases_in_memory is $databases_in_memory"

# Join the queue env.  We assume the rep test convention of
# placing the messages in $testdir/MSGQUEUEDIR.
set queueenv [eval berkdb_env -home $testdir/MSGQUEUEDIR]
error_check_good script_qenv_open [is_valid_env $queueenv] TRUE

# We need to set up our own machids.
# Add 1 for master env id, and 2 for the clientenv id.
repladd 1
repladd 2

# Join the master env.
set ma_cmd "berkdb_env_noerr -home $masterdir \
	-errfile /dev/stderr -errpfx CHILD.MA \
	-txn -rep_master -rep_transport \[list 1 replsend\]"
# set ma_cmd "berkdb_env_noerr -home $masterdir  \
#  	-verbose {rep on} -errfile /dev/stderr -errpfx CHILD.MA \
#  	-txn -rep_master -rep_transport \[list 1 replsend\]"
set masterenv [eval $ma_cmd]
error_check_good script_menv_open [is_valid_env $masterenv] TRUE

puts "Master open"
# Downgrade while transaction is open
error_check_good downgrade [$masterenv rep_start -client] 0

tclsleep 10
# Upgrade again
error_check_good upgrade [$masterenv rep_start -master] 0
#
# Create a btree database now.
#
rep_test btree $masterenv NULL 10 0 0 0

# Close the envs
puts "Closing Masterenv $masterenv"
error_check_good script_master_close [$masterenv close] 0
puts "\tRepscript completed successfully"

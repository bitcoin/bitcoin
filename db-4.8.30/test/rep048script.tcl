# See the file LICENSE for redistribution information.
#
# Copyright (c) 2003-2009 Oracle.  All rights reserved.
#
# $Id$
#
# Rep048 script - toggle bulk transfer while updates are going on.

# Usage: repscript masterdir
# masterdir: master env directory
# databases_in_memory: are we using named in-memory databases?
#
source ./include.tcl
source $test_path/reputils.tcl

set usage "repscript masterdir"

# Verify usage
if { $argc != 2 } {
	puts stderr "FAIL:[timestamp] Usage: $usage"
	exit
}

# Initialize arguments
set masterdir [ lindex $argv 0 ]
global databases_in_memory
set databases_in_memory [ lindex $argv 1 ]

# Join the queue env.  We assume the rep test convention of
# placing the messages in $testdir/MSGQUEUEDIR.
set queueenv [eval berkdb_env -home $testdir/MSGQUEUEDIR]
error_check_good script_qenv_open [is_valid_env $queueenv] TRUE

#
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
if { $databases_in_memory } {
	set dbname { "" "child.db" }
} else {
	set dbname "child.db"
}
set db [eval "berkdb_open -create -btree -auto_commit -env $masterenv $dbname"]
error_check_good dbopen [is_valid_db $db] TRUE

# Create marker file
set marker [open $masterdir/marker.file w]
close $marker

#
# Keep toggling until the parent indicates it's done.
#
set tog "on"
for { set i 0 } { [file exists $masterdir/done.file] == 0 } { incr i } {
puts "Iter $i: Turn bulk $tog"
	error_check_good bulk$tog [$masterenv rep_config [list bulk $tog]] 0
	set t [$masterenv txn]
	error_check_good db_put \
	    [eval $db put -txn $t $i data$i] 0
	error_check_good txn_commit [$t commit] 0
	if { $tog == "on" } {
		set tog "off"
	} else {
		set tog "on"
	}
	tclsleep 1
}
# Close the envs
error_check_good script_db_close [$db close] 0
error_check_good script_master_close [$masterenv close] 0
puts "\tRepscript completed successfully"

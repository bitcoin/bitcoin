# See the file LICENSE for redistribution information.
#
# Copyright (c) 1996-2009 Oracle.  All rights reserved.
#
# $Id$
#
# Deadlock detector script tester.
# Usage: ddscript dir test lockerid objid numprocs
# dir: DBHOME directory
# test: Which test to run
# lockerid: Lock id for this locker
# objid: Object id to lock.
# numprocs: Total number of processes running

source ./include.tcl
source $test_path/test.tcl
source $test_path/testutils.tcl

set usage "ddscript dir test lockerid objid numprocs"

# Verify usage
if { $argc != 5 } {
	puts stderr "FAIL:[timestamp] Usage: $usage"
	exit
}

# Initialize arguments
set dir [lindex $argv 0]
set test [ lindex $argv 1 ]
set lockerid [ lindex $argv 2 ]
set objid [ lindex $argv 3 ]
set numprocs [ lindex $argv 4 ]

set myenv [berkdb_env -lock -home $dir -create -mode 0644 ]
error_check_bad lock_open $myenv NULL
error_check_good lock_open [is_substr $myenv "env"] 1

puts [eval $test $myenv $lockerid $objid $numprocs]

error_check_good lock_id_free [$myenv lock_id_free $lockerid] 0
error_check_good envclose [$myenv close] 0

exit

# See the file LICENSE for redistribution information.
#
# Copyright (c) 2009 Oracle.  All rights reserved.
#
# $Id$
#
# Mut002script: for use with mut002, a 2-process mutex test.
# Usage:	mut002script testdir
# testdir: 	directory containing the env we are joining.
# mutex:	id of mutex

source ./include.tcl

set usage "mut002script testdir mutex"

# Verify usage
if { $argc != 2 } {
	puts stderr "FAIL:[timestamp] Usage: $usage"
	exit
}

# Initialize arguments.
set testdir [ lindex $argv 0 ]
set mutex [ lindex $argv 1 ]

# Open environment.
if {[catch {eval {berkdb_env} -create -home $testdir } dbenv]} {
    	puts "FAIL: opening env returned $dbenv"
}
error_check_good envopen [is_valid_env $dbenv] TRUE

# Pause for a while to let the original process block. 
tclsleep 10

# Unlock the mutex and let the original process proceed. 
$dbenv mutex_unlock $mutex

# Clean up.
error_check_good env_close [$dbenv close] 0

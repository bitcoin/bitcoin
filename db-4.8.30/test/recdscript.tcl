# See the file LICENSE for redistribution information.
#
# Copyright (c) 1996-2009 Oracle.  All rights reserved.
#
# $Id$
#
# Recovery txn prepare script
# Usage: recdscript op dir envcmd dbfile cmd
# op: primary txn operation
# dir: test directory
# envcmd: command to open env
# dbfile: name of database file
# gidf: name of global id file
# cmd: db command to execute

source ./include.tcl
source $test_path/test.tcl

set usage "recdscript op dir envcmd dbfile gidfile cmd"

# Verify usage
if { $argc < 6 } {
	puts stderr "FAIL:[timestamp] Usage: $usage"
	exit
}

# Initialize arguments
set op [ lindex $argv 0 ]
set dir [ lindex $argv 1 ]
set envcmd [ lindex $argv 2 ]
set dbfile [ lindex $argv 3 ]
set gidfile [ lindex $argv 4 ]
set cmd [ lindex $argv 5 ]
set args [ lindex $argv 6 ]

eval {op_recover_prep $op $dir $envcmd $dbfile $gidfile $cmd} $args
flush stdout

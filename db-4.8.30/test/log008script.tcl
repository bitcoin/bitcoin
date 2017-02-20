# See the file LICENSE for redistribution information.
#
# Copyright (c) 2005-2009 Oracle.  All rights reserved.
#
# $Id$
#
# Log008 script - dbreg_ckp and txn_ckp records spanning log files.
#
# Usage: log008script

source ./include.tcl
set tnum "008"
set usage "log008script nhandles"

# Verify usage
if { $argc != 1 } {
	puts stderr "FAIL:[timestamp] Usage: $usage"
	exit
}

# Initialize arguments
set nhandles [ lindex $argv 0 ]

# We make the log files small so it's likely that the
# records will end up in different files.
set maxbsize [expr 8 * 1024]
set maxfile [expr 32 * 1024]

# Set up environment.
set envcmd "berkdb_env -create -txn -home $testdir \
    -log_buffer $maxbsize -log_max $maxfile"
set dbenv [eval $envcmd]
error_check_good dbenv [is_valid_env $dbenv] TRUE

# Open a lot of database handles.
set filename TESTFILE
set handlelist {}
for { set i 0 } { $i < $nhandles } { incr i } {
	set db [berkdb_open \
	    -create -env $dbenv -auto_commit -btree $filename]
	lappend handlelist $db
}

# Fill log files, checking LSNs before and after a checkpoint,
# until we generate a case where the records span two log files.
set i 0
while { 1 } {
	set txn [$dbenv txn]
	foreach handle $handlelist {
		error_check_good \
		    db_put [$handle put -txn $txn key.$i data.$i] 0
		incr i
	}
	error_check_good txn_commit [$txn commit] 0

	# Find current LSN file number.
	set filenum [stat_field $dbenv log_stat "Current log file number"]

	# Checkpoint.
	error_check_good checkpoint [$dbenv txn_checkpoint] 0

	# Find current LSN.
	set newfilenum [stat_field $dbenv log_stat "Current log file number"]
	if { [expr $newfilenum > $filenum] } {
		break
	}
}

# Do one more transactional operation per fileid.
set txn [$dbenv txn]
foreach handle $handlelist {
	error_check_good \
	    db_put [$handle put -txn $txn key.$i data.$i] 0
	incr i
}
error_check_good txn_commit [$txn commit] 0

# Archive, deleting the log files we think we no longer need.
set stat [eval exec $util_path/db_archive -d -h $testdir]

# Child is done.  Exit, abandoning the env instead of closing it.
exit

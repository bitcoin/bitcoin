# See the file LICENSE for redistribution information.
#
# Copyright (c) 2005-2009 Oracle.  All rights reserved.
#
# $Id$
#
# Rep043 script - constant writes to an env which may be
# either a master or a client, or changing between the
# two states
#
# Usage: 	dir writerid
# dir: 	Directory of writer
# writerid: i.d. number for writer

set usage "rep043script dir writerid"

# Verify usage
if { $argc != 2 } {
	puts stderr "FAIL:[timestamp] Usage: $usage"
	exit
}

# Initialize arguments
set dir [ lindex $argv 0 ]
set writerid [ lindex $argv 1 ]
set nentries 50

# Join the queue env.  We assume the rep test convention of
# placing the messages in $testdir/MSGQUEUEDIR.
set queueenv [eval berkdb_env -home $testdir/MSGQUEUEDIR]
error_check_good script_qenv_open [is_valid_env $queueenv] TRUE

# We need to set up our own machids.
set envid [expr $writerid + 1]
repladd $envid
set name "WRITER.$writerid"

# Pause a bit to give the master a chance to create the database 
# before we try to join.
tclsleep 3

# Join the env.
set env_cmd "berkdb_env_noerr -home $dir -lock_detect default \
	-errfile /dev/stderr -errpfx WRITER.$writerid \
	-txn -rep_transport \[list $envid replsend\]"
# set env_cmd "berkdb_env_noerr -home $dir \
#	-errfile /dev/stderr -errpfx WRITER.$writerid \
#  	-verbose {rep on} -lock_detect default \
#  	-txn -rep_transport \[list $envid replsend\]"
set dbenv [eval $env_cmd]
error_check_good script_env_open [is_valid_env $dbenv] TRUE

# Open database.  It's still possible under heavy load that the 
# master hasn't created the database, so pause even longer if it's 
# not there.
set testfile "rep043.db"
while {[catch {berkdb_open_noerr -errpfx $name -errfile /dev/stderr\
    -env $dbenv -auto_commit $testfile} db]} {
	puts "Could not open handle $db, sleeping 1 second."
	tclsleep 1
}
error_check_good dbopen [is_valid_db $db] TRUE

# Communicate with parent in marker file.
set markerenv [berkdb_env -home $testdir -txn]
error_check_good markerenv_open [is_valid_env $markerenv] TRUE
set marker [eval "berkdb_open \
    -create -btree -auto_commit -env $markerenv marker.db"]

# Write records to the database.
set iter INIT
set olditer $iter
while { [llength [$marker get DONE]] == 0 } {
	for { set i 0 } { $i < $nentries } { incr i } {
	 	set kd [$marker get ITER]
		if { [llength $kd] == 0 } {
			set iter X
		} else {
			set iter [lindex [lindex $kd 0] 1]
		}
		if { $iter != $olditer } {
			puts "Entry $i: Iter changed from $olditer to $iter"
			set olditer $iter
		}

		set key WRITER.$writerid.$iter.$i
		set str string.$i

		set t [$dbenv txn]
		error_check_good txn [is_valid_txn $t $dbenv] TRUE
		set stat [catch {$db put -txn $t $key $str} res]
		if { $stat == 0 } {
puts "res is $res, commit"
			error_check_good txn_commit [$t commit] 0
		} else {
puts "res is $res, abort"
			error_check_good txn_abort [$t abort] 0
		}

		# If the handle is dead, get a new one.
		if { [is_substr $res DB_REP_HANDLE_DEAD] == 1 } {
puts "Close - dead handle."
			error_check_good db_close [$db close] 0
puts "Getting new handle"
			while {[catch {berkdb_open_noerr -env $dbenv\
			    -auto_commit $testfile} db]} {
				puts "Could not open handle: $db"
				tclsleep 1
			}
			error_check_good db_open [is_valid_db $db] TRUE
		}

		if { [expr $i % 10] == 1 } {
			puts "Wrote WRITER.$writerid.$iter.$i record $i"
		}
	}
	tclsleep 1
}

# Clean up.
error_check_good db_close [$db close] 0
error_check_good dbenv_close [$dbenv close] 0
replclose $testdir/MSGQUEUEDIR
error_check_good marker_close [$marker close] 0
error_check_good markerenv_close [$markerenv close] 0

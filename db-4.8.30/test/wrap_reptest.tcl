# See the file LICENSE for redistribution information.
#
# Copyright (c) 2000-2009 Oracle.  All rights reserved.
#
# $Id$
#
# This is a very cut down version of wrap.tcl.  We don't want to
# use wrap.tcl because that will create yet another Tcl subprocess
# to execute the test.  We want to open the test program directly
# here so that we get the pid for the program (not the Tcl shell)
# and watch_procs can kill the program if needed.

source ./include.tcl
source $test_path/test.tcl

# Arguments:
if { $argc != 2 } {
	puts "FAIL: wrap_reptest.tcl: Usage: wrap_reptest.tcl argfile log"
	exit
}

set argfile [lindex $argv 0]
set logfile [lindex $argv 1]

# Create a sentinel file to mark our creation and signal that watch_procs
# should look for us.
set parentpid [pid]
set parentsentinel $testdir/begin.$parentpid
set f [open $parentsentinel w]
close $f

# Create a Tcl subprocess that will actually run the test.
set argf [open $argfile r]
set progargs [read $argf]
close $argf
set cmd [open "| $util_path/db_reptest $progargs >& $logfile" w]
set childpid [pid $cmd]

puts "Script watcher process $parentpid launching db_reptest process $childpid to $logfile."
set childsentinel $testdir/begin.$childpid
set f [open $childsentinel w]
close $f

# Close the pipe.  This will flush the above commands and actually run the
# test, and will also return an error a la exec if anything bad happens
# to the subprocess.  The magic here is that closing a pipe blocks
# and waits for the exit of processes in the pipeline, at least according
# to Ousterhout (p. 115).
set ret [catch {close $cmd} res]

# Write ending sentinel files--we're done.
set f [open $testdir/end.$childpid w]
close $f
set f [open $testdir/end.$parentpid w]
close $f

error_check_good "($childpid: db_reptest $progargs: logfile $logfile)"\
    $ret 0
exit $ret

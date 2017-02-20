# See the file LICENSE for redistribution information.
#
# Copyright (c) 2000-2009 Oracle.  All rights reserved.
#
# $Id$
#
# Sentinel file wrapper for multi-process tests.  This is designed to avoid a
# set of nasty bugs, primarily on Windows, where pid reuse causes watch_procs
# to sit around waiting for some random process that's not DB's and is not
# exiting.

source ./include.tcl
source $test_path/testutils.tcl

# Arguments:
if { $argc < 2 } {
	puts "FAIL: wrap.tcl: Usage: wrap.tcl script log [scriptargs]"
	exit
}

set script [lindex $argv 0]
set logfile [lindex $argv 1]
if { $argc >= 2 } {
	set skip [lindex $argv 2]
	set args [lrange $argv 3 end]
} else {
	set skip ""
	set args ""
}
#
# Account in args for SKIP command, or not.
#
if { $skip != "SKIP" && $argc >= 2 } {
	set args [lrange $argv 2 end]
}

# Create a sentinel file to mark our creation and signal that watch_procs
# should look for us.
set parentpid [pid]
set parentsentinel $testdir/begin.$parentpid
set f [open $parentsentinel w]
close $f

# Create a Tcl subprocess that will actually run the test.
set t [open "|$tclsh_path >& $logfile" w]

# Create a sentinel for the subprocess.
set childpid [pid $t]
puts "Script watcher process $parentpid launching $script process $childpid."
set childsentinel $testdir/begin.$childpid
set f [open $childsentinel w]
close $f

#
# For the upgrade tests where a current release tclsh is starting up
# a tclsh in an older release, we cannot tell it to source the current
# test.tcl because new things may not exist in the old release.  So,
# we need to skip that and the script we're running in the old
# release will have to take care of itself.
#
if { $skip != "SKIP" } {
	puts $t "source $test_path/test.tcl"
}
puts $t "set script $script"

# Set up argv for the subprocess, since the args aren't passed in as true
# arguments thanks to the pipe structure.
puts $t "set argc [llength $args]"
puts $t "set argv [list $args]"

set has_path [file dirname $script]
if { $has_path != "." } {
	set scr $script
} else {
	set scr $test_path/$script
}
#puts "Script $script: path $has_path, scr $scr"
puts $t "set scr $scr"
puts $t {set ret [catch { source $scr } result]}
puts $t {if { [string length $result] > 0 } { puts $result }}
puts $t {error_check_good "$scr run: $result: pid [pid]" $ret 0}

# Close the pipe.  This will flush the above commands and actually run the
# test, and will also return an error a la exec if anything bad happens
# to the subprocess.  The magic here is that closing a pipe blocks
# and waits for the exit of processes in the pipeline, at least according
# to Ousterhout (p. 115).

set ret [catch {close $t} res]

# Write ending sentinel files--we're done.
set f [open $testdir/end.$childpid w]
close $f
set f [open $testdir/end.$parentpid w]
close $f

error_check_good "Pipe close ($childpid: $script $argv: logfile $logfile)"\
    $ret 0
exit $ret

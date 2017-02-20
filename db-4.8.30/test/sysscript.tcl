# See the file LICENSE for redistribution information.
#
# Copyright (c) 1996-2009 Oracle.  All rights reserved.
#
# $Id$
#
# System integration test script.
# This script runs a single process that tests the full functionality of
# the system.  The database under test contains nfiles files.  Each process
# randomly generates a key and some data.  Both keys and data are bimodally
# distributed between small keys (1-10 characters) and large keys (the avg
# length is indicated via the command line parameter.
# The process then decides on a replication factor between 1 and nfiles.
# It writes the key and data to that many files and tacks on the file ids
# of the files it writes to the data string.  For example, let's say that
# I randomly generate the key dog and data cat.  Then I pick a replication
# factor of 3.  I pick 3 files from the set of n (say 1, 3, and 5).  I then
# rewrite the data as 1:3:5:cat.  I begin a transaction, add the key/data
# pair to each file and then commit.  Notice that I may generate replication
# of the form 1:3:3:cat in which case I simply add a duplicate to file 3.
#
# Usage: sysscript dir nfiles key_avg data_avg
#
# dir: DB_HOME directory
# nfiles: number of files in the set
# key_avg: average big key size
# data_avg: average big data size

source ./include.tcl
source $test_path/test.tcl
source $test_path/testutils.tcl

set mypid [pid]

set usage "sysscript dir nfiles key_avg data_avg method args"

# Verify usage
if { $argc < 5 } {
	puts stderr "FAIL:[timestamp] Usage: $usage"
	exit
}

puts [concat "Argc: " $argc " Argv: " $argv]

# Initialize arguments
set dir [lindex $argv 0]
set nfiles [ lindex $argv 1 ]
set key_avg [ lindex $argv 2 ]
set data_avg [ lindex $argv 3 ]
set method [ lindex $argv 4 ]
set args [ lindex $argv 5 ]

# Initialize seed
global rand_init
berkdb srand $rand_init

puts "Beginning execution for $mypid"
puts "$dir DB_HOME"
puts "$nfiles files"
puts "$key_avg average key length"
puts "$data_avg average data length"

flush stdout

# Create local environment
set dbenv [berkdb_env -txn -home $dir]
set err [catch {error_check_good $mypid:dbenv [is_substr $dbenv env] 1} ret]
if {$err != 0} {
	puts $ret
	return
}

# Now open the files
for { set i 0 } { $i < $nfiles } { incr i } {
	set file test044.$i.db
	set db_set($i) [eval {berkdb open} -auto_commit -env $dbenv $args $method $file ]
	set err [catch {error_check_bad $mypid:dbopen $db_set($i) NULL} ret]
	if {$err != 0} {
		puts $ret
		return
	}
	set err [catch {error_check_bad $mypid:dbopen [is_substr $db_set($i) \
	    error] 1} ret]
	if {$err != 0} {
		puts $ret
		return
	}
}

set record_based [is_record_based $method]
while { 1 } {
	# Decide if we're going to create a big key or a small key
	# We give small keys a 70% chance.
	if { [berkdb random_int 1 10] < 8 } {
		set k [random_data 5 0 0 $record_based]
	} else {
		set k [random_data $key_avg 0 0 $record_based]
	}
	set data [chop_data $method [random_data $data_avg 0 0]]

	set txn [$dbenv txn]
	set err [catch {error_check_good $mypid:txn_begin [is_substr $txn \
	    $dbenv.txn] 1} ret]
	if {$err != 0} {
		puts $ret
		return
	}

	# Open cursors
	for { set f 0 } {$f < $nfiles} {incr f} {
		set cursors($f) [$db_set($f) cursor -txn $txn]
		set err [catch {error_check_good $mypid:cursor_open \
		    [is_substr $cursors($f) $db_set($f)] 1} ret]
		if {$err != 0} {
			puts $ret
			return
		}
	}
	set aborted 0

	# Check to see if key is already in database
	set found 0
	for { set i 0 } { $i < $nfiles } { incr i } {
		set r [$db_set($i) get -txn $txn $k]
		set r [$db_set($i) get -txn $txn $k]
		if { $r == "-1" } {
			for {set f 0 } {$f < $nfiles} {incr f} {
				set err [catch {error_check_good \
				    $mypid:cursor_close \
				    [$cursors($f) close] 0} ret]
				if {$err != 0} {
					puts $ret
					return
				}
			}
			set err [catch {error_check_good $mypid:txn_abort \
			    [$txn abort] 0} ret]
			if {$err != 0} {
				puts $ret
				return
			}
			set aborted 1
			set found 2
			break
		} elseif { $r != "Key $k not found." } {
			set found 1
			break
		}
	}
	switch $found {
	2 {
		# Transaction aborted, no need to do anything.
	}
	0 {
		# Key was not found, decide how much to replicate
		# and then create a list of that many file IDs.
		set repl [berkdb random_int 1 $nfiles]
		set fset ""
		for { set i 0 } { $i < $repl } {incr i} {
			set f [berkdb random_int 0 [expr $nfiles - 1]]
			lappend fset $f
			set data [chop_data $method $f:$data]
		}

		foreach i $fset {
			set r [$db_set($i) put -txn $txn $k $data]
			if {$r == "-1"} {
				for {set f 0 } {$f < $nfiles} {incr f} {
					set err [catch {error_check_good \
					    $mypid:cursor_close \
					    [$cursors($f) close] 0} ret]
					if {$err != 0} {
						puts $ret
						return
					}
				}
				set err [catch {error_check_good \
				    $mypid:txn_abort [$txn abort] 0} ret]
				if {$err != 0} {
					puts $ret
					return
				}
				set aborted 1
				break
			}
		}
	}
	1 {
		# Key was found.  Make sure that all the data values
		# look good.
		set f [zero_list $nfiles]
		set data $r
		while { [set ndx [string first : $r]] != -1 } {
			set fnum [string range $r 0 [expr $ndx - 1]]
			if { [lindex $f $fnum] == 0 } {
				#set flag -set
				set full [record $cursors($fnum) get -set $k]
			} else {
				#set flag -next
				set full [record $cursors($fnum) get -next]
			}
			if {[llength $full] == 0} {
				for {set f 0 } {$f < $nfiles} {incr f} {
					set err [catch {error_check_good \
					    $mypid:cursor_close \
					    [$cursors($f) close] 0} ret]
					if {$err != 0} {
						puts $ret
						return
					}
				}
				set err [catch {error_check_good \
				    $mypid:txn_abort [$txn abort] 0} ret]
				if {$err != 0} {
					puts $ret
					return
				}
				set aborted 1
				break
			}
			set err [catch {error_check_bad \
			    $mypid:curs_get($k,$data,$fnum,$flag) \
			    [string length $full] 0} ret]
			if {$err != 0} {
				puts $ret
				return
			}
			set key [lindex [lindex $full 0] 0]
			set rec [pad_data $method [lindex [lindex $full 0] 1]]
			set err [catch {error_check_good \
			    $mypid:dbget_$fnum:key $key $k} ret]
			if {$err != 0} {
				puts $ret
				return
			}
			set err [catch {error_check_good \
			    $mypid:dbget_$fnum:data($k) $rec $data} ret]
			if {$err != 0} {
				puts $ret
				return
			}
			set f [lreplace $f $fnum $fnum 1]
			incr ndx
			set r [string range $r $ndx end]
		}
	}
	}
	if { $aborted == 0 } {
		for {set f 0 } {$f < $nfiles} {incr f} {
			set err [catch {error_check_good $mypid:cursor_close \
			    [$cursors($f) close] 0} ret]
			if {$err != 0} {
				puts $ret
				return
			}
		}
		set err [catch {error_check_good $mypid:commit [$txn commit] \
		    0} ret]
		if {$err != 0} {
			puts $ret
			return
		}
	}
}

# Close files
for { set i 0 } { $i < $nfiles} { incr i } {
	set r [$db_set($i) close]
	set err [catch {error_check_good $mypid:db_close:$i $r 0} ret]
	if {$err != 0} {
		puts $ret
		return
	}
}

# Close tm and environment
$dbenv close

puts "[timestamp] [pid] Complete"
flush stdout

filecheck $file 0

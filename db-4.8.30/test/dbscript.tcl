# See the file LICENSE for redistribution information.
#
# Copyright (c) 1996-2009 Oracle.  All rights reserved.
#
# $Id$
#
# Random db tester.
# Usage: dbscript file numops min_del max_add key_avg data_avgdups
# method: method (we pass this in so that fixed-length records work)
# file: db file on which to operate
# numops: number of operations to do
# ncurs: number of cursors
# min_del: minimum number of keys before you disable deletes.
# max_add: maximum number of keys before you disable adds.
# key_avg: average key size
# data_avg: average data size
# dups: 1 indicates dups allowed, 0 indicates no dups
# errpct: What percent of operations should generate errors
# seed: Random number generator seed (-1 means use pid)

source ./include.tcl
source $test_path/test.tcl
source $test_path/testutils.tcl

set usage "dbscript file numops ncurs min_del max_add key_avg data_avg dups errpcnt args"

# Verify usage
if { $argc < 10 } {
	puts stderr "FAIL:[timestamp] Usage: $usage"
	exit
}

# Initialize arguments
set method [lindex $argv 0]
set file [lindex $argv 1]
set numops [ lindex $argv 2 ]
set ncurs [ lindex $argv 3 ]
set min_del [ lindex $argv 4 ]
set max_add [ lindex $argv 5 ]
set key_avg [ lindex $argv 6 ]
set data_avg [ lindex $argv 7 ]
set dups [ lindex $argv 8 ]
set errpct [ lindex $argv 9 ]
set args [ lindex $argv 10 ]

berkdb srand $rand_init

puts "Beginning execution for [pid]"
puts "$file database"
puts "$numops Operations"
puts "$ncurs cursors"
puts "$min_del keys before deletes allowed"
puts "$max_add or fewer keys to add"
puts "$key_avg average key length"
puts "$data_avg average data length"
puts "$method $args"
if { $dups != 1 } {
	puts "No dups"
} else {
	puts "Dups allowed"
}
puts "$errpct % Errors"

flush stdout

set db [eval {berkdb_open} $args $file]
set cerr [catch {error_check_good dbopen [is_substr $db db] 1} cret]
if {$cerr != 0} {
	puts $cret
	return
}
# set method [$db get_type]
set record_based [is_record_based $method]

# Initialize globals including data
global nkeys
global l_keys
global a_keys

set nkeys [db_init $db 1]
puts "Initial number of keys: $nkeys"

set pflags ""
set gflags ""
set txn ""

# Open the cursors
set curslist {}
for { set i 0 } { $i < $ncurs } { incr i } {
	set dbc [$db cursor]
	set cerr [catch {error_check_good dbcopen [is_substr $dbc $db.c] 1} cret]
	if {$cerr != 0} {
		puts $cret
		return
	}
	set cerr [catch {error_check_bad cursor_create $dbc NULL} cret]
	if {$cerr != 0} {
		puts $cret
		return
	}
	lappend curslist $dbc

}

# On each iteration we're going to generate random keys and
# data.  We'll select either a get/put/delete operation unless
# we have fewer than min_del keys in which case, delete is not
# an option or more than max_add in which case, add is not
# an option.  The tcl global arrays a_keys and l_keys keep track
# of key-data pairs indexed by key and a list of keys, accessed
# by integer.
set adds 0
set puts 0
set gets 0
set dels 0
set bad_adds 0
set bad_puts 0
set bad_gets 0
set bad_dels 0

for { set iter 0 } { $iter < $numops } { incr iter } {
	set op [pick_op $min_del $max_add $nkeys]
	set err [is_err $errpct]

	# The op0's indicate that there aren't any duplicates, so we
	# exercise regular operations.  If dups is 1, then we'll use
	# cursor ops.
	switch $op$dups$err {
		add00 {
			incr adds

			set k [random_data $key_avg 1 a_keys $record_based]
			set data [random_data $data_avg 0 0]
			set data [chop_data $method $data]
			set ret [eval {$db put} $txn $pflags \
			    {-nooverwrite $k $data}]
			set cerr [catch {error_check_good put $ret 0} cret]
			if {$cerr != 0} {
				puts $cret
				return
			}
			newpair $k [pad_data $method $data]
		}
		add01 {
			incr bad_adds
			set k [random_key]
			set data [random_data $data_avg 0 0]
			set data [chop_data $method $data]
			set ret [eval {$db put} $txn $pflags \
			    {-nooverwrite $k $data}]
			set cerr [catch {error_check_good put $ret 0} cret]
			if {$cerr != 0} {
				puts $cret
				return
			}
			# Error case so no change to data state
		}
		add10 {
			incr adds
			set dbcinfo [random_cursor $curslist]
			set dbc [lindex $dbcinfo 0]
			if { [berkdb random_int 1 2] == 1 } {
				# Add a new key
				set k [random_data $key_avg 1 a_keys \
				    $record_based]
				set data [random_data $data_avg 0 0]
				set data [chop_data $method $data]
				set ret [eval {$dbc put} $txn \
				    {-keyfirst $k $data}]
				newpair $k [pad_data $method $data]
			} else {
				# Add a new duplicate
				set dbc [lindex $dbcinfo 0]
				set k [lindex $dbcinfo 1]
				set data [random_data $data_avg 0 0]

				set op [pick_cursput]
				set data [chop_data $method $data]
				set ret [eval {$dbc put} $txn {$op $k $data}]
				adddup $k [lindex $dbcinfo 2] $data
			}
		}
		add11 {
			# TODO
			incr bad_adds
			set ret 1
		}
		put00 {
			incr puts
			set k [random_key]
			set data [random_data $data_avg 0 0]
			set data [chop_data $method $data]
			set ret [eval {$db put} $txn {$k $data}]
			changepair $k [pad_data $method $data]
		}
		put01 {
			incr bad_puts
			set k [random_key]
			set data [random_data $data_avg 0 0]
			set data [chop_data $method $data]
			set ret [eval {$db put} $txn $pflags \
			    {-nooverwrite $k $data}]
			set cerr [catch {error_check_good put $ret 0} cret]
			if {$cerr != 0} {
				puts $cret
				return
			}
			# Error case so no change to data state
		}
		put10 {
			incr puts
			set dbcinfo [random_cursor $curslist]
			set dbc [lindex $dbcinfo 0]
			set k [lindex $dbcinfo 1]
			set data [random_data $data_avg 0 0]
			set data [chop_data $method $data]

			set ret [eval {$dbc put} $txn {-current $data}]
			changedup $k [lindex $dbcinfo 2] $data
		}
		put11 {
			incr bad_puts
			set k [random_key]
			set data [random_data $data_avg 0 0]
			set data [chop_data $method $data]
			set dbc [$db cursor]
			set ret [eval {$dbc put} $txn {-current $data}]
			set cerr [catch {error_check_good curs_close \
			    [$dbc close] 0} cret]
			if {$cerr != 0} {
				puts $cret
				return
			}
			# Error case so no change to data state
		}
		get00 {
			incr gets
			set k [random_key]
			set val [eval {$db get} $txn {$k}]
			set data [pad_data $method [lindex [lindex $val 0] 1]]
			if { $data == $a_keys($k) } {
				set ret 0
			} else {
				set ret "FAIL: Error got |$data| expected |$a_keys($k)|"
			}
			# Get command requires no state change
		}
		get01 {
			incr bad_gets
			set k [random_data $key_avg 1 a_keys $record_based]
			set ret [eval {$db get} $txn {$k}]
			# Error case so no change to data state
		}
		get10 {
			incr gets
			set dbcinfo [random_cursor $curslist]
			if { [llength $dbcinfo] == 3 } {
				set ret 0
			else
				set ret 0
			}
			# Get command requires no state change
		}
		get11 {
			incr bad_gets
			set k [random_key]
			set dbc [$db cursor]
			if { [berkdb random_int 1 2] == 1 } {
				set dir -next
			} else {
				set dir -prev
			}
			set ret [eval {$dbc get} $txn {-next $k}]
			set cerr [catch {error_check_good curs_close \
			    [$dbc close] 0} cret]
			if {$cerr != 0} {
				puts $cret
				return
			}
			# Error and get case so no change to data state
		}
		del00 {
			incr dels
			set k [random_key]
			set ret [eval {$db del} $txn {$k}]
			rempair $k
		}
		del01 {
			incr bad_dels
			set k [random_data $key_avg 1 a_keys $record_based]
			set ret [eval {$db del} $txn {$k}]
			# Error case so no change to data state
		}
		del10 {
			incr dels
			set dbcinfo [random_cursor $curslist]
			set dbc [lindex $dbcinfo 0]
			set ret [eval {$dbc del} $txn]
			remdup [lindex dbcinfo 1] [lindex dbcinfo 2]
		}
		del11 {
			incr bad_dels
			set c [$db cursor]
			set ret [eval {$c del} $txn]
			set cerr [catch {error_check_good curs_close \
			    [$c close] 0} cret]
			if {$cerr != 0} {
				puts $cret
				return
			}
			# Error case so no change to data state
		}
	}
	if { $err == 1 } {
		# Verify failure.
		set cerr [catch {error_check_good $op$dups$err:$k \
		    [is_substr Error $ret] 1} cret]
		if {$cerr != 0} {
			puts $cret
			return
		}
	} else {
		# Verify success
		set cerr [catch {error_check_good $op$dups$err:$k $ret 0} cret]
		if {$cerr != 0} {
			puts $cret
			return
		}
	}

	flush stdout
}

# Close cursors and file
foreach i $curslist {
	set r [$i close]
	set cerr [catch {error_check_good cursor_close:$i $r 0} cret]
	if {$cerr != 0} {
		puts $cret
		return
	}
}

set r [$db close]
set cerr [catch {error_check_good db_close:$db $r 0} cret]
if {$cerr != 0} {
	puts $cret
	return
}

puts "[timestamp] [pid] Complete"
puts "Successful ops: $adds adds $gets gets $puts puts $dels dels"
puts "Error ops: $bad_adds adds $bad_gets gets $bad_puts puts $bad_dels dels"
flush stdout

eval filecheck $file {$txn} $args

exit

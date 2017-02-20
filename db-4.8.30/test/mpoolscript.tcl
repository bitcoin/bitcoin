# See the file LICENSE for redistribution information.
#
# Copyright (c) 1996-2009 Oracle.  All rights reserved.
#
# $Id$
#
# Random multiple process mpool tester.
# Usage: mpoolscript dir id numiters numfiles numpages sleepint
# dir: lock directory.
# id: Unique identifier for this process.
# maxprocs: Number of procs in this test.
# numiters: Total number of iterations.
# pgsizes: Pagesizes for the different files.  Length of this item indicates
#		how many files to use.
# numpages: Number of pages per file.
# sleepint: Maximum sleep interval.
# flags: Flags for env open

source ./include.tcl
source $test_path/test.tcl
source $test_path/testutils.tcl

set usage \
   "mpoolscript dir id maxprocs numiters pgsizes numpages sleepint flags"

# Verify usage
if { $argc != 8 } {
	puts stderr "FAIL:[timestamp] Usage: $usage"
	puts $argc
	exit
}

# Initialize arguments
set dir [lindex $argv 0]
set id [lindex $argv 1]
set maxprocs [lindex $argv 2]
set numiters [ lindex $argv 3 ]
set pgsizes [ lindex $argv 4 ]
set numpages [ lindex $argv 5 ]
set sleepint [ lindex $argv 6 ]
set flags [ lindex $argv 7]

# Initialize seed
global rand_init
berkdb srand $rand_init

# Give time for all processes to start up.
tclsleep 10

puts -nonewline "Beginning execution for $id: $maxprocs $dir $numiters"
puts " $pgsizes $numpages $sleepint"
flush stdout

# Figure out how small/large to make the cache
set max 0
foreach i $pgsizes {
	if { $i > $max } {
		set max $i
	}
}

set cache [list 0 [expr $maxprocs * ([lindex $pgsizes 0] + $max)] 1]
set env_cmd {berkdb_env -lock -cachesize $cache -home $dir}
set e [eval $env_cmd $flags]
error_check_good env_open [is_valid_env $e] TRUE

# Now open files
set mpools {}
set nfiles 0
foreach psize $pgsizes {
	set mp [$e mpool -create -mode 0644 -pagesize $psize file$nfiles]
	error_check_good memp_fopen:$nfiles [is_valid_mpool $mp $e] TRUE
	lappend mpools $mp
	incr nfiles
}

puts "Establishing long-term pin on file 0 page $id for process $id"

# Set up the long-pin page
set locker [$e lock_id]
set lock [$e lock_get write $locker 0:$id]
error_check_good lock_get [is_valid_lock $lock $e] TRUE

set mp [lindex $mpools 0]
set master_page [$mp get -create -dirty $id]
error_check_good mp_get:$master_page [is_valid_page $master_page $mp] TRUE

set r [$master_page init MASTER$id]
error_check_good page_init $r 0

# Release the lock but keep the page pinned
set r [$lock put]
error_check_good lock_put $r 0

# Main loop.  On each iteration, we'll check every page in each of
# of the files.  On any file, if we see the appropriate tag in the
# field, we'll rewrite the page, else we won't.  Keep track of
# how many pages we actually process.
set pages 0
for { set iter 0 } { $iter < $numiters } { incr iter } {
	puts "[timestamp]: iteration $iter, $pages pages set so far"
	flush stdout
	for { set fnum 1 } { $fnum < $nfiles } { incr fnum } {
		if { [expr $fnum % 2 ] == 0 } {
			set pred [expr ($id + $maxprocs - 1) % $maxprocs]
		} else {
			set pred [expr ($id + $maxprocs + 1) % $maxprocs]
		}

		set mpf [lindex $mpools $fnum]
		for { set p 0 } { $p < $numpages } { incr p } {
			set lock [$e lock_get write $locker $fnum:$p]
			error_check_good lock_get:$fnum:$p \
			    [is_valid_lock $lock $e] TRUE

			# Now, get the page
			set pp [$mpf get -create -dirty $p]
			error_check_good page_get:$fnum:$p \
			    [is_valid_page $pp $mpf] TRUE

			if { [$pp is_setto $pred] == 0 || [$pp is_setto 0] == 0 } {
				# Set page to self.
				set r [$pp init $id]
				error_check_good page_init:$fnum:$p $r 0
				incr pages
				set r [$pp put]
				error_check_good page_put:$fnum:$p $r 0
			} else {
				error_check_good page_put:$fnum:$p [$pp put] 0
			}
			error_check_good lock_put:$fnum:$p [$lock put] 0
		}
	}
	tclsleep [berkdb random_int 1 $sleepint]
}

# Now verify your master page, release its pin, then verify everyone else's
puts "$id: End of run verification of master page"
set r [$master_page is_setto MASTER$id]
error_check_good page_check $r 1
set r [$master_page put]
error_check_good page_put $r 0

set i [expr ($id + 1) % $maxprocs]
set mpf [lindex $mpools 0]

while { $i != $id } {
	set p [$mpf get -create $i]
	error_check_good mp_get [is_valid_page $p $mpf] TRUE

	if { [$p is_setto MASTER$i] != 1 } {
		puts "Warning: Master page $i not set."
	}
	error_check_good page_put:$p [$p put] 0

	set i [expr ($i + 1) % $maxprocs]
}

# Close files
foreach i $mpools {
	set r [$i close]
	error_check_good mpf_close $r 0
}

# Close environment system
set r [$e close]
error_check_good env_close $r 0

puts "[timestamp] $id Complete"
flush stdout

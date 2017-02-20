# See the file LICENSE for redistribution information.
#
# Copyright (c) 2000-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	recd012
# TEST	Test of log file ID management. [#2288]
# TEST	Test recovery handling of file opens and closes.
proc recd012 { method {start 0} \
    {niter 49} {noutiter 25} {niniter 100} {ndbs 5} args } {
	source ./include.tcl

	set tnum "012"
	set pagesize 512

	if { $is_qnx_test } {
		set niter 40
	}

	puts "Recd$tnum $method ($args): Test recovery file management."
	set pgindex [lsearch -exact $args "-pagesize"]
	if { $pgindex != -1 } {
		puts "Recd012: skipping for specific pagesizes"
		return
	}

	for { set i $start } { $i <= $niter } { incr i } {
		env_cleanup $testdir

		# For repeatability, we pass in the iteration number
		# as a parameter and use that in recd012_body to seed
		# the random number generator to randomize our operations.
		# This lets us re-run a potentially failing iteration
		# without having to start from the beginning and work
		# our way to it.
		#
		# The number of databases ranges from 4 to 8 and is
		# a function of $niter
		# set ndbs [expr ($i % 5) + 4]

		 recd012_body \
		    $method $ndbs $i $noutiter $niniter $pagesize $tnum  $args
	}
}

proc recd012_body { method {ndbs 5} iter noutiter niniter psz tnum {largs ""} } {
	global alphabet rand_init fixed_len recd012_ofkey recd012_ofckptkey
	source ./include.tcl

	set largs [convert_args $method $largs]
	set omethod [convert_method $method]

	puts "\tRecd$tnum $method ($largs): Iteration $iter"
	puts "\t\tRecd$tnum.a: Create environment and $ndbs databases."

	# We run out of lockers during some of the recovery runs, so
	# we need to make sure that we specify a DB_CONFIG that will
	# give us enough lockers.
	set f [open $testdir/DB_CONFIG w]
	puts $f "set_lk_max_lockers	5000"
	close $f

	set flags "-create -txn -home $testdir"
	set env_cmd "berkdb_env $flags"
	error_check_good env_remove [berkdb envremove -home $testdir] 0
	set dbenv [eval $env_cmd]
	error_check_good dbenv [is_valid_env $dbenv] TRUE

	# Initialize random number generator based on $iter.
	berkdb srand [expr $iter + $rand_init]

	# Initialize database that keeps track of number of open files (so
	# we don't run out of descriptors).
	set ofname of.db
	set txn [$dbenv txn]
	error_check_good open_txn_begin [is_valid_txn $txn $dbenv] TRUE
	set ofdb [berkdb_open -env $dbenv -txn $txn\
	    -create -dup -mode 0644 -btree -pagesize 512 $ofname]
	error_check_good of_open [is_valid_db $ofdb] TRUE
	error_check_good open_txn_commit [$txn commit] 0
	set oftxn [$dbenv txn]
	error_check_good of_txn [is_valid_txn $oftxn $dbenv] TRUE
	error_check_good of_put [$ofdb put -txn $oftxn $recd012_ofkey 1] 0
	error_check_good of_put2 [$ofdb put -txn $oftxn $recd012_ofckptkey 0] 0
	error_check_good of_put3 [$ofdb put -txn $oftxn $recd012_ofckptkey 0] 0
	error_check_good of_txn_commit [$oftxn commit] 0
	error_check_good of_close [$ofdb close] 0

	# Create ndbs databases to work in, and a file listing db names to
	# pick from.
	set f [open $testdir/dblist w]

	set oflags "-auto_commit -env $dbenv \
	    -create -mode 0644 -pagesize $psz $largs $omethod"
	for { set i 0 } { $i < $ndbs } { incr i } {
	# 50-50 chance of being a subdb, unless we're a queue or partitioned.
		if { [berkdb random_int 0 1] || \
		    [is_queue $method] || [is_partitioned $largs] } {
			# not a subdb
			set dbname recd$tnum-$i.db
		} else {
			# subdb
			set dbname "recd$tnum-subdb.db s$i"
		}
		puts $f $dbname
		set db [eval {berkdb_open} $oflags $dbname]
		error_check_good db($i) [is_valid_db $db] TRUE
		error_check_good db($i)_close [$db close] 0
	}
	close $f
	error_check_good env_close [$dbenv close] 0

	# Now we get to the meat of things.  Our goal is to do some number
	# of opens, closes, updates, and shutdowns (simulated here by a
	# close of all open handles and a close/reopen of the environment,
	# with or without an envremove), matching the regular expression
	#
	#	((O[OUC]+S)+R+V)
	#
	# We'll repeat the inner + a random number up to $niniter times,
	# and the outer + a random number up to $noutiter times.
	#
	# In order to simulate shutdowns, we'll perform the opens, closes,
	# and updates in a separate process, which we'll exit without closing
	# all handles properly.  The environment will be left lying around
	# before we run recovery 50% of the time.
	set out [berkdb random_int 1 $noutiter]
	puts \
    "\t\tRecd$tnum.b: Performing $out recoveries of up to $niniter ops."
	for { set i 0 } { $i < $out } { incr i } {
		set child [open "|$tclsh_path" w]

		# For performance, don't source everything,
		# just what we'll need.
		puts $child "load $tcllib"
		puts $child "set fixed_len $fixed_len"
		puts $child "source $src_root/test/testutils.tcl"
		puts $child "source $src_root/test/recd$tnum.tcl"

		set rnd [expr $iter * 10000 + $i * 100 + $rand_init]

		# Go.
		berkdb debug_check
		puts $child "recd012_dochild {$env_cmd} $rnd $i $niniter\
		    $ndbs $tnum $method $ofname $largs"
		close $child

		# Run recovery 0-3 times.
		set nrecs [berkdb random_int 0 3]
		for { set j 0 } { $j < $nrecs } { incr j } {
			berkdb debug_check
			set ret [catch {exec $util_path/db_recover \
			    -h $testdir} res]
			if { $ret != 0 } {
				puts "FAIL: db_recover returned with nonzero\
				    exit status, output as follows:"
				file mkdir /tmp/12out
				set fd [open /tmp/12out/[pid] w]
				puts $fd $res
				close $fd
			}
			error_check_good recover($j) $ret 0
		}
	}

	# Run recovery one final time;  it doesn't make sense to
	# check integrity if we do not.
	set ret [catch {exec $util_path/db_recover -h $testdir} res]
	if { $ret != 0 } {
		puts "FAIL: db_recover returned with nonzero\
		    exit status, output as follows:"
		puts $res
	}

	# Make sure each datum is the correct filename.
	puts "\t\tRecd$tnum.c: Checking data integrity."
	set dbenv [berkdb_env -create -private -home $testdir]
	error_check_good env_open_integrity [is_valid_env $dbenv] TRUE
	set f [open $testdir/dblist r]
	set i 0
	while { [gets $f dbinfo] > 0 } {
		set db [eval berkdb_open -env $dbenv $largs $dbinfo]
		error_check_good dbopen($dbinfo) [is_valid_db $db] TRUE

		set dbc [$db cursor]
		error_check_good cursor [is_valid_cursor $dbc $db] TRUE

		for { set dbt [$dbc get -first] } { [llength $dbt] > 0 } \
		    { set dbt [$dbc get -next] } {
			error_check_good integrity [lindex [lindex $dbt 0] 1] \
			    [pad_data $method $dbinfo]
		}
		error_check_good dbc_close [$dbc close] 0
		error_check_good db_close [$db close] 0
	}
	close $f
	error_check_good env_close_integrity [$dbenv close] 0

	# Verify
	error_check_good verify \
	    [verify_dir $testdir "\t\tRecd$tnum.d: " 0 0 1] 0
}

proc recd012_dochild { env_cmd rnd outiter niniter ndbs tnum method\
    ofname args } {
	global recd012_ofkey
	source ./include.tcl
	if { [is_record_based $method] } {
		set keybase ""
	} else {
		set keybase .[repeat abcdefghijklmnopqrstuvwxyz 4]
	}

	# Initialize our random number generator, repeatably based on an arg.
	berkdb srand $rnd

	# Open our env.
	set dbenv [eval $env_cmd]
	error_check_good env_open [is_valid_env $dbenv] TRUE

	# Find out how many databases appear to be open in the log--we
	# don't want recovery to run out of filehandles.
	set txn [$dbenv txn]
	error_check_good child_txn_begin [is_valid_txn $txn $dbenv] TRUE
	set ofdb [berkdb_open -env $dbenv -txn $txn $ofname]
	error_check_good child_txn_commit [$txn commit] 0

	set oftxn [$dbenv txn]
	error_check_good of_txn [is_valid_txn $oftxn $dbenv] TRUE
	set dbt [$ofdb get -txn $oftxn $recd012_ofkey]
	error_check_good of_get [lindex [lindex $dbt 0] 0] $recd012_ofkey
	set nopenfiles [lindex [lindex $dbt 0] 1]

	error_check_good of_commit [$oftxn commit] 0

	# Read our dbnames
	set f [open $testdir/dblist r]
	set i 0
	while { [gets $f dbname($i)] > 0 } {
		incr i
	}
	close $f

	# We now have $ndbs extant databases.
	# Open one of them, just to get us started.
	set opendbs {}
	set oflags "-env $dbenv $args"

	# Start a transaction, just to get us started.
	set curtxn [$dbenv txn]
	error_check_good txn [is_valid_txn $curtxn $dbenv] TRUE

	# Inner loop.  Do $in iterations of a random open, close, or
	# update, where $in is between 1 and $niniter.
	set in [berkdb random_int 1 $niniter]
	for { set j 0 }	{ $j < $in } { incr j } {
		set op [berkdb random_int 0 2]
		switch $op {
		0 {
			# Open.
			recd012_open
		}
		1 {
			# Update.  Put random-number$keybase as key,
			# filename as data, into random database.
			set num_open [llength $opendbs]
			if { $num_open == 0 } {
				# If none are open, do an open first.
				recd012_open
				set num_open [llength $opendbs]
			}
			set n [berkdb random_int 0 [expr $num_open - 1]]
			set pair [lindex $opendbs $n]
			set udb [lindex $pair 0]
			set uname [lindex $pair 1]

			set key [berkdb random_int 1000 1999]$keybase
			set data [chop_data $method $uname]
			error_check_good put($uname,$udb,$key,$data) \
			    [$udb put -txn $curtxn $key $data] 0

			# One time in four, commit the transaction.
			if { [berkdb random_int 0 3] == 0 && 0 } {
				error_check_good txn_recommit \
				    [$curtxn commit] 0
				set curtxn [$dbenv txn]
				error_check_good txn_reopen \
				    [is_valid_txn $curtxn $dbenv] TRUE
			}
		}
		2 {
			# Close.
			if { [llength $opendbs] == 0 } {
				# If none are open, open instead of closing.
				recd012_open
				continue
			}

			# Commit curtxn first, lest we self-deadlock.
			error_check_good txn_recommit [$curtxn commit] 0

			# Do it.
			set which [berkdb random_int 0 \
			    [expr [llength $opendbs] - 1]]

			set db [lindex [lindex $opendbs $which] 0]
			error_check_good db_choice [is_valid_db $db] TRUE
			global errorCode errorInfo

			error_check_good db_close \
			    [[lindex [lindex $opendbs $which] 0] close] 0

			set opendbs [lreplace $opendbs $which $which]
			incr nopenfiles -1

			# Reopen txn.
			set curtxn [$dbenv txn]
			error_check_good txn_reopen \
			    [is_valid_txn $curtxn $dbenv] TRUE
		}
		}

		# One time in two hundred, checkpoint.
		if { [berkdb random_int 0 199] == 0 } {
			puts "\t\t\tRecd$tnum:\
			    Random checkpoint after operation $outiter.$j."
			error_check_good txn_ckpt \
			    [$dbenv txn_checkpoint] 0
			set nopenfiles \
			    [recd012_nopenfiles_ckpt $dbenv $ofdb $nopenfiles]
		}
	}

	# We have to commit curtxn.  It'd be kind of nice not to, but
	# if we start in again without running recovery, we may block
	# ourselves.
	error_check_good curtxn_commit [$curtxn commit] 0

	# Put back the new number of open files.
	set oftxn [$dbenv txn]
	error_check_good of_txn [is_valid_txn $oftxn $dbenv] TRUE
	error_check_good of_del [$ofdb del -txn $oftxn $recd012_ofkey] 0
	error_check_good of_put \
	    [$ofdb put -txn $oftxn $recd012_ofkey $nopenfiles] 0
	error_check_good of_commit [$oftxn commit] 0
	error_check_good ofdb_close [$ofdb close] 0
}

proc recd012_open { } {
	# This is basically an inline and has to modify curtxn,
	# so use upvars.
	upvar curtxn curtxn
	upvar ndbs ndbs
	upvar dbname dbname
	upvar dbenv dbenv
	upvar oflags oflags
	upvar opendbs opendbs
	upvar nopenfiles nopenfiles

	# Return without an open if we've already opened too many files--
	# we don't want to make recovery run out of filehandles.
	if { $nopenfiles > 30 } {
		#puts "skipping--too many open files"
		return -code break
	}

	# Commit curtxn first, lest we self-deadlock.
	error_check_good txn_recommit \
	    [$curtxn commit] 0

	# Do it.
	set which [berkdb random_int 0 [expr $ndbs - 1]]

	set db [eval berkdb_open -auto_commit $oflags $dbname($which)]

	lappend opendbs [list $db $dbname($which)]

	# Reopen txn.
	set curtxn [$dbenv txn]
	error_check_good txn_reopen [is_valid_txn $curtxn $dbenv] TRUE

	incr nopenfiles
}

# Update the database containing the number of files that db_recover has
# to contend with--we want to avoid letting it run out of file descriptors.
# We do this by keeping track of the number of unclosed opens since the
# checkpoint before last.
# $recd012_ofkey stores this current value;  the two dups available
# at $recd012_ofckptkey store the number of opens since the last checkpoint
# previous.
# Thus, if the current value is 17 when we do a checkpoint, and the
# stored values are 3 and 8, the new current value (which we return)
# is 14, and the new stored values are 8 and 6.
proc recd012_nopenfiles_ckpt { env db nopenfiles } {
	global recd012_ofckptkey
	set txn [$env txn]
	error_check_good nopenfiles_ckpt_txn [is_valid_txn $txn $env] TRUE

	set dbc [$db cursor -txn $txn]
	error_check_good db_cursor [is_valid_cursor $dbc $db] TRUE

	# Get the first ckpt value and delete it.
	set dbt [$dbc get -set $recd012_ofckptkey]
	error_check_good set [llength $dbt] 1

	set discard [lindex [lindex $dbt 0] 1]
	error_check_good del [$dbc del] 0

	set nopenfiles [expr $nopenfiles - $discard]

	# Get the next ckpt value
	set dbt [$dbc get -nextdup]
	error_check_good set2 [llength $dbt] 1

	# Calculate how many opens we've had since this checkpoint before last.
	set onlast [lindex [lindex $dbt 0] 1]
	set sincelast [expr $nopenfiles - $onlast]

	# Put this new number at the end of the dup set.
	error_check_good put [$dbc put -keylast $recd012_ofckptkey $sincelast] 0

	# We should never deadlock since we're the only one in this db.
	error_check_good dbc_close [$dbc close] 0
	error_check_good txn_commit [$txn commit] 0

	return $nopenfiles
}

# globals -- it's not worth passing these around, as they're constants
set recd012_ofkey OPENFILES
set recd012_ofckptkey CKPTS

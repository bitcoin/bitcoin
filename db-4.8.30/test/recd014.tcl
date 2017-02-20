# See the file LICENSE for redistribution information.
#
# Copyright (c) 1999-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	recd014
# TEST	This is a recovery test for create/delete of queue extents.  We
# TEST	then need to recover and make sure the file is correctly existing
# TEST	or not, as the case may be.
proc recd014 { method args} {
	global fixed_len
	source ./include.tcl

	if { ![is_queueext $method] == 1 } {
		puts "Recd014: Skipping for method $method"
		return
	}
	set pgindex [lsearch -exact $args "-pagesize"]
	if { $pgindex != -1 } {
		puts "Recd014: skipping for specific pagesizes"
		return
	}

	set orig_fixed_len $fixed_len
	#
	# We will use 512-byte pages, to be able to control
	# when extents get created/removed.
	#
	set fixed_len 300

	set opts [convert_args $method $args]
	set omethod [convert_method $method]
	#
	# We want to set -extent 1 instead of what
	# convert_args gave us.
	#
	set exti [lsearch -exact $opts "-extent"]
	incr exti
	set opts [lreplace $opts $exti $exti 1]

	puts "Recd014: $method extent creation/deletion tests"

	# Create the database and environment.
	env_cleanup $testdir

	set testfile recd014.db
	set flags "-create -txn -home $testdir"

	puts "\tRecd014.a: creating environment"
	set env_cmd "berkdb_env $flags"

	puts "\tRecd014.b: Create test commit"
	ext_recover_create $testdir $env_cmd $omethod \
	    $opts $testfile commit
	puts "\tRecd014.b: Create test abort"
	ext_recover_create $testdir $env_cmd $omethod \
	    $opts $testfile abort

	puts "\tRecd014.c: Consume test commit"
	ext_recover_consume $testdir $env_cmd $omethod \
	    $opts $testfile commit
	puts "\tRecd014.c: Consume test abort"
	ext_recover_consume $testdir $env_cmd $omethod \
	    $opts $testfile abort

	set fixed_len $orig_fixed_len
	puts "\tRecd014.d: Verify db_printlog can read logfile"
	set tmpfile $testdir/printlog.out
	set stat [catch {exec $util_path/db_printlog -h $testdir \
	    > $tmpfile} ret]
	error_check_good db_printlog $stat 0
	fileremove $tmpfile
}

proc ext_recover_create { dir env_cmd method opts dbfile txncmd } {
	global log_log_record_types
	global fixed_len
	global alphabet
	source ./include.tcl

	# Keep track of the log types we've seen
	if { $log_log_record_types == 1} {
		logtrack_read $dir
	}

	env_cleanup $dir
	# Open the environment and set the copy/abort locations
	set env [eval $env_cmd]

	set init_file $dir/$dbfile.init
	set noenvflags "-create $method -mode 0644 -pagesize 512 $opts $dbfile"
	set oflags "-env $env $noenvflags"

	set t [$env txn]
	error_check_good txn_begin [is_valid_txn $t $env] TRUE

	set ret [catch {eval {berkdb_open} -txn $t $oflags} db]
	error_check_good txn_commit [$t commit] 0

	set t [$env txn]
	error_check_good txn_begin [is_valid_txn $t $env] TRUE

	#
	# The command to execute to create an extent is a put.
	# We are just creating the first one, so our extnum is 0.
	# extnum must be in the format that make_ext_file expects,
	# but we just leave out the file name.
	#
	set extnum "/__dbq..0"
	set data [chop_data $method [replicate $alphabet 512]]
	puts "\t\tExecuting command"
	set putrecno [$db put -txn $t -append $data]
	error_check_good db_put $putrecno 1

	# Sync the db so any changes to the file that are
	# in mpool get written to the disk file before the
	# diff.
	puts "\t\tSyncing"
	error_check_good db_sync [$db sync] 0

	catch { file copy -force $dir/$dbfile $dir/$dbfile.afterop } res
	copy_extent_file $dir $dbfile afterop

	error_check_good txn_$txncmd:$t [$t $txncmd] 0
	#
	# If we don't abort, then we expect success.
	# If we abort, we expect no file created.
	#
	set dbq [make_ext_filename $dir $dbfile $extnum]
	error_check_good extput:exists1 [file exists $dbq] 1
	set ret [$db get $putrecno]
	if {$txncmd == "abort"} {
		#
		# Operation was aborted.   Verify our entry is not there.
		#
		puts "\t\tCommand executed and aborted."
		error_check_good db_get [llength $ret] 0
	} else {
		#
		# Operation was committed, verify it exists.
		#
		puts "\t\tCommand executed and committed."
		error_check_good db_get [llength $ret] 1
		catch { file copy -force $dir/$dbfile $init_file } res
		copy_extent_file $dir $dbfile init
	}
	set t [$env txn]
	error_check_good txn_begin [is_valid_txn $t $env] TRUE
	error_check_good db_close [$db close] 0
	error_check_good txn_commit [$t commit] 0
	error_check_good env_close [$env close] 0

	#
	# Run recovery here.  Should be a no-op.  Verify that
	# the file still does/n't exist when we are done.
	#
	berkdb debug_check
	puts -nonewline "\t\tAbout to run recovery (no-op) ... "
	flush stdout

	set stat [catch {exec $util_path/db_recover -h $dir -c} result]
	if { $stat == 1 } {
		error "FAIL: Recovery error: $result."
		return
	}
	puts "complete"
	#
	# Verify it did not change.
	#
	error_check_good extput:exists2 [file exists $dbq] 1
	ext_create_check $dir $txncmd $init_file $dbfile $noenvflags $putrecno

	#
	# Need a new copy to get the right LSN into the file.
	#
	catch { file copy -force $dir/$dbfile $init_file } res
	copy_extent_file $dir $dbfile init

	#
	# Undo.
	# Now move the .afterop file to $dbfile.  Run recovery again.
	#
	file copy -force $dir/$dbfile.afterop $dir/$dbfile
	move_file_extent $dir $dbfile afterop copy

	berkdb debug_check
	puts -nonewline "\t\tAbout to run recovery (afterop) ... "
	flush stdout

	set stat [catch {exec $util_path/db_recover -h $dir -c} result]
	if { $stat == 1 } {
		error "FAIL: Recovery error: $result."
		return
	}
	puts "complete"
	ext_create_check $dir $txncmd $init_file $dbfile $noenvflags $putrecno

	#
	# To redo, remove the dbfiles.  Run recovery again.
	#
	catch { file rename -force $dir/$dbfile $dir/$dbfile.renamed } res
	copy_extent_file $dir $dbfile renamed rename

	berkdb debug_check
	puts -nonewline "\t\tAbout to run recovery (init) ... "
	flush stdout

	set stat [catch {exec $util_path/db_recover -h $dir -c} result]
	#
	# !!!
	# Even though db_recover exits with status 0, it should print out
	# a warning because the file didn't exist.  Db_recover writes this
	# to stderr.  Tcl assumes that ANYTHING written to stderr is an
	# error, so even though we exit with 0 status, we still get an
	# error back from 'catch'.  Look for the warning.
	#
	if { $stat == 1 && [is_substr $result "warning"] == 0 } {
		error "FAIL: Recovery error: $result."
		return
	}
	puts "complete"

	#
	# Verify it was redone.  However, since we removed the files
	# to begin with, recovery with abort will not recreate the
	# extent.  Recovery with commit will.
	#
	if {$txncmd == "abort"} {
		error_check_good extput:exists3 [file exists $dbq] 0
	} else {
		error_check_good extput:exists3 [file exists $dbq] 1
	}
}

proc ext_create_check { dir txncmd init_file dbfile oflags putrecno } {
	if { $txncmd == "commit" } {
		#
		# Operation was committed. Verify it did not change.
		#
		error_check_good \
		    diff(initial,post-recover2):diff($init_file,$dir/$dbfile) \
		    [dbdump_diff "-dar" $init_file $dir $dbfile] 0
	} else {
		#
		# Operation aborted.  The file is there, but make
		# sure the item is not.
		#
		set xdb [eval {berkdb_open} $oflags]
		error_check_good db_open [is_valid_db $xdb] TRUE
		set ret [$xdb get $putrecno]
		error_check_good db_get [llength $ret] 0
		error_check_good db_close [$xdb close] 0
	}
}

proc ext_recover_consume { dir env_cmd method opts dbfile txncmd} {
	global log_log_record_types
	global alphabet
	source ./include.tcl

	# Keep track of the log types we've seen
	if { $log_log_record_types == 1} {
		logtrack_read $dir
	}

	env_cleanup $dir
	# Open the environment and set the copy/abort locations
	set env [eval $env_cmd]

	set oflags "-create -auto_commit $method -mode 0644 -pagesize 512 \
	   -env $env $opts $dbfile"

	#
	# Open our db, add some data, close and copy as our
	# init file.
	#
	set db [eval {berkdb_open} $oflags]
	error_check_good db_open [is_valid_db $db] TRUE

	set extnum "/__dbq..0"
	set data [chop_data $method [replicate $alphabet 512]]

	set txn [$env txn]
	error_check_good txn_begin [is_valid_txn $txn $env] TRUE
	set putrecno [$db put -txn $txn -append $data]
	error_check_good db_put $putrecno 1
	error_check_good commit [$txn commit] 0
	error_check_good db_close [$db close] 0

	puts "\t\tExecuting command"

	set init_file $dir/$dbfile.init
	catch { file copy -force $dir/$dbfile $init_file } res
	copy_extent_file $dir $dbfile init

	#
	# If we don't abort, then we expect success.
	# If we abort, we expect no file removed until recovery is run.
	#
	set db [eval {berkdb_open} $oflags]
	error_check_good db_open [is_valid_db $db] TRUE

	set t [$env txn]
	error_check_good txn_begin [is_valid_txn $t $env] TRUE

	set dbcmd "$db get -txn $t -consume"
	set ret [eval $dbcmd]
	error_check_good db_sync [$db sync] 0

	catch { file copy -force $dir/$dbfile $dir/$dbfile.afterop } res
	copy_extent_file $dir $dbfile afterop

	error_check_good txn_$txncmd:$t [$t $txncmd] 0
	error_check_good db_sync [$db sync] 0
	set dbq [make_ext_filename $dir $dbfile $extnum]
	if {$txncmd == "abort"} {
		#
		# Operation was aborted, verify ext did not change.
		#
		puts "\t\tCommand executed and aborted."

		#
		# Check that the file exists.  Final state.
		# Since we aborted the txn, we should be able
		# to get to our original entry.
		#
		error_check_good postconsume.1 [file exists $dbq] 1
		error_check_good \
		    diff(init,postconsume.2):diff($init_file,$dir/$dbfile)\
		    [dbdump_diff "-dar" $init_file $dir $dbfile] 0
	} else {
		#
		# Operation was committed, verify it does
		# not exist.
		#
		puts "\t\tCommand executed and committed."
		#
		# Check file existence.  Consume operations remove
		# the extent when we move off, which we should have
		# done.
		error_check_good consume_exists [file exists $dbq] 0
	}
	error_check_good db_close [$db close] 0
	error_check_good env_close [$env close] 0

	#
	# Run recovery here on what we ended up with.  Should be a no-op.
	#
	berkdb debug_check
	puts -nonewline "\t\tAbout to run recovery (no-op) ... "
	flush stdout

	set stat [catch {exec $util_path/db_recover -h $dir -c} result]
	if { $stat == 1 } {
		error "FAIL: Recovery error: $result."
		return
	}
	puts "complete"
	if { $txncmd == "abort"} {
		#
		# Operation was aborted, verify it did not change.
		#
		error_check_good \
		    diff(initial,post-recover1):diff($init_file,$dir/$dbfile) \
		    [dbdump_diff "-dar" $init_file $dir $dbfile] 0
	} else {
		#
		# Operation was committed, verify it does
		# not exist.  Both operations should result
		# in no file existing now that we've run recovery.
		#
		error_check_good after_recover1 [file exists $dbq] 0
	}

	#
	# Run recovery here. Re-do the operation.
	# Verify that the file doesn't exist
	# (if we committed)  or change (if we aborted)
	# when we are done.
	#
	catch { file copy -force $dir/$dbfile $init_file } res
	copy_extent_file $dir $dbfile init
	berkdb debug_check
	puts -nonewline "\t\tAbout to run recovery (init) ... "
	flush stdout

	set stat [catch {exec $util_path/db_recover -h $dir -c} result]
	if { $stat == 1 } {
		error "FAIL: Recovery error: $result."
		return
	}
	puts "complete"
	if { $txncmd == "abort"} {
		#
		# Operation was aborted, verify it did not change.
		#
		error_check_good \
		    diff(initial,post-recover1):diff($init_file,$dir/$dbfile) \
		    [dbdump_diff "-dar" $init_file $dir $dbfile] 0
	} else {
		#
		# Operation was committed, verify it does
		# not exist.  Both operations should result
		# in no file existing now that we've run recovery.
		#
		error_check_good after_recover2 [file exists $dbq] 0
	}

	#
	# Now move the .afterop file to $dbfile.  Run recovery again.
	#
	set filecopy [glob $dir/*.afterop]
	set afterop [lindex $filecopy 0]
	file rename -force $afterop $dir/$dbfile
	set afterop [string range $afterop \
	    [expr [string last "/" $afterop] + 1]  \
	    [string last "." $afterop]]
	move_file_extent $dir $dbfile afterop rename

	berkdb debug_check
	puts -nonewline "\t\tAbout to run recovery (afterop) ... "
	flush stdout

	set stat [catch {exec $util_path/db_recover -h $dir -c} result]
	if { $stat == 1 } {
		error "FAIL: Recovery error: $result."
		return
	}
	puts "complete"

	if { $txncmd == "abort"} {
		#
		# Operation was aborted, verify it did not change.
		#
		error_check_good \
		    diff(initial,post-recover2):diff($init_file,$dir/$dbfile) \
		    [dbdump_diff "-dar" $init_file $dir $dbfile] 0
	} else {
		#
		# Operation was committed, verify it still does
		# not exist.
		#
		error_check_good after_recover3 [file exists $dbq] 0
	}
}

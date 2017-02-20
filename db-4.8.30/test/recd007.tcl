# See the file LICENSE for redistribution information.
#
# Copyright (c) 1999-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	recd007
# TEST	File create/delete tests.
# TEST
# TEST	This is a recovery test for create/delete of databases.  We have
# TEST	hooks in the database so that we can abort the process at various
# TEST	points and make sure that the transaction doesn't commit.  We
# TEST	then need to recover and make sure the file is correctly existing
# TEST	or not, as the case may be.
proc recd007 { method args } {
	global fixed_len
	source ./include.tcl

	env_cleanup $testdir
	set envargs ""
	set data_dir ""
	set dir_cmd ""
	set zero_idx [lsearch -exact $args "-zero_log"]
	if { $zero_idx != -1 } {
		set args [lreplace $args $zero_idx $zero_idx]
		set envargs "-zero_log"
	}
	set zero_idx [lsearch -exact $args "-data_dir"]
	if { $zero_idx != -1 } {
		set end [expr $zero_idx + 1]
		append envargs [lrange $args $zero_idx $end]
		set data_dir [lrange $args $end $end]
		set dir_cmd "if {\[file exists $testdir/$data_dir] == 0 } {exec mkdir $testdir/$data_dir} ; "
		set args [lreplace $args $zero_idx $end]
	}

	set orig_fixed_len $fixed_len
	set opts [convert_args $method $args]
	set omethod [convert_method $method]

	puts "Recd007: $method operation/transaction tests ($envargs)"

	# Create the database and environment.

	set testfile recd007.db
	set flags "-create -txn -home $testdir $envargs"

	puts "\tRecd007.a: creating environment"
	set env_cmd "$dir_cmd berkdb_env $flags"

	set env [eval $env_cmd]

	# We need to create a database to get the pagesize (either
	# the default or whatever might have been specified).
	# Then remove it so we can compute fixed_len and create the
	# real database.
	set oflags "-create $omethod -mode 0644 -env $env $opts $testfile"
	set db [eval {berkdb_open} $oflags]
	error_check_good db_open [is_valid_db $db] TRUE
	set stat [$db stat]
	#
	# Compute the fixed_len based on the pagesize being used.
	# We want the fixed_len to be 1/4 the pagesize.
	#
	set pg [get_pagesize $stat]
	error_check_bad get_pagesize $pg -1
	set fixed_len [expr $pg / 4]
	error_check_good db_close [$db close] 0
	error_check_good dbremove [berkdb dbremove -env $env $testfile] 0
	error_check_good log_flush [$env log_flush] 0
	error_check_good envclose [$env close] 0

	# Convert the args again because fixed_len is now real.
	set opts [convert_args $method $args]
	set save_opts $opts
	set moreopts {" -lorder 1234 " " -lorder 1234 -chksum " \
	    " -lorder 4321 " " -lorder 4321 -chksum "}

	# List of recovery tests: {HOOKS MSG} pairs
	# Where each HOOK is a list of {COPY ABORT}
	#
	set rlist {
	{ {"none" "preopen"}		"Recd007.b0: none/preopen"}
	{ {"none" "postopen"}		"Recd007.b1: none/postopen"}
	{ {"none" "postlogmeta"}	"Recd007.b2: none/postlogmeta"}
	{ {"none" "postlog"}		"Recd007.b3: none/postlog"}
	{ {"none" "postsync"}		"Recd007.b4: none/postsync"}
	{ {"postopen" "none"}		"Recd007.c0: postopen/none"}
	{ {"postlogmeta" "none"}	"Recd007.c1: postlogmeta/none"}
	{ {"postlog" "none"}		"Recd007.c2: postlog/none"}
	{ {"postsync" "none"}		"Recd007.c3: postsync/none"}
	{ {"postopen" "postopen"}	"Recd007.d: postopen/postopen"}
	{ {"postopen" "postlogmeta"}	"Recd007.e: postopen/postlogmeta"}
	{ {"postopen" "postlog"}	"Recd007.f: postopen/postlog"}
	{ {"postlog" "postlog"}		"Recd007.g: postlog/postlog"}
	{ {"postlogmeta" "postlogmeta"}	"Recd007.h: postlogmeta/postlogmeta"}
	{ {"postlogmeta" "postlog"}	"Recd007.i: postlogmeta/postlog"}
	{ {"postlog" "postsync"}	"Recd007.j: postlog/postsync"}
	{ {"postsync" "postsync"}	"Recd007.k: postsync/postsync"}
	}

	# These are all the data values that we're going to need to read
	# through the operation table and run the recovery tests.

	foreach pair $rlist {
		set cmd [lindex $pair 0]
		set msg [lindex $pair 1]
		#
		# Run natively
		#
		file_recover_create $testdir $env_cmd $omethod \
		    $save_opts $testfile $cmd $msg $data_dir
		foreach o $moreopts {
			set opts $save_opts
			append opts $o
			file_recover_create $testdir $env_cmd $omethod \
			    $opts $testfile $cmd $msg $data_dir
		}
	}

	set rlist {
	{ {"none" "predestroy"}		"Recd007.l0: none/predestroy"}
	{ {"none" "postdestroy"}	"Recd007.l1: none/postdestroy"}
	{ {"predestroy" "none"}		"Recd007.m0: predestroy/none"}
	{ {"postdestroy" "none"}	"Recd007.m1: postdestroy/none"}
	{ {"predestroy" "predestroy"}	"Recd007.n: predestroy/predestroy"}
	{ {"predestroy" "postdestroy"}	"Recd007.o: predestroy/postdestroy"}
	{ {"postdestroy" "postdestroy"}	"Recd007.p: postdestroy/postdestroy"}
	}

	foreach op { dbremove dbrename dbtruncate } {
		foreach pair $rlist {
			set cmd [lindex $pair 0]
			set msg [lindex $pair 1]
			file_recover_delete $testdir $env_cmd $omethod \
			    $save_opts $testfile $cmd $msg $op $data_dir
			foreach o $moreopts {
				set opts $save_opts
				append opts $o
				file_recover_delete $testdir $env_cmd $omethod \
				    $opts $testfile $cmd $msg $op $data_dir
			}
		}
	}

	if { $is_windows_test != 1 && $is_hp_test != 1 } {
		set env_cmd "$dir_cmd ; berkdb_env_noerr $flags"
		do_file_recover_delmk $testdir $env_cmd $method $opts $testfile $data_dir
	}

	puts "\tRecd007.r: Verify db_printlog can read logfile"
	set tmpfile $testdir/printlog.out
	set stat [catch {exec $util_path/db_printlog -h $testdir \
	    > $tmpfile} ret]
	error_check_good db_printlog $stat 0
	fileremove $tmpfile
	set fixed_len $orig_fixed_len
	return
}

proc file_recover_create { dir env_cmd method opts dbfile cmd msg data_dir} {
	#
	# We run this test on each of these scenarios:
	# 1.  Creating just a database
	# 2.  Creating a database with a subdb
	# 3.  Creating a 2nd subdb in a database
	puts "\t$msg ($opts) create with a database"
	do_file_recover_create $dir $env_cmd $method $opts $dbfile \
	    0 $cmd $msg $data_dir
	if { [is_queue $method] == 1 || [is_partitioned $opts] == 1} {
		puts "\tSkipping subdatabase tests for method $method"
		return
	}
	puts "\t$msg ($opts) create with a database and subdb"
	do_file_recover_create $dir $env_cmd $method $opts $dbfile \
	    1 $cmd $msg $data_dir 
	puts "\t$msg ($opts) create with a database and 2nd subdb"
	do_file_recover_create $dir $env_cmd $method $opts $dbfile \
	    2 $cmd $msg $data_dir

}

proc do_file_recover_create { dir env_cmd method opts dbfile sub cmd msg data_dir} {
	global log_log_record_types
	source ./include.tcl

	# Keep track of the log types we've seen
	if { $log_log_record_types == 1} {
		logtrack_read $dir
	}

	env_cleanup $dir
	set dflags "-dar"
	# Open the environment and set the copy/abort locations
	set env [eval $env_cmd]
	set copy [lindex $cmd 0]
	set abort [lindex $cmd 1]
	error_check_good copy_location [is_valid_create_loc $copy] 1
	error_check_good abort_location [is_valid_create_loc $abort] 1

	if {([string first "logmeta" $copy] != -1 || \
	    [string first "logmeta" $abort] != -1) && \
	    [is_btree $method] == 0 } {
		puts "\tSkipping for method $method"
		$env test copy none
		$env test abort none
		error_check_good log_flush [$env log_flush] 0
		error_check_good env_close [$env close] 0
		return
	}

	# Basically non-existence is our initial state.  When we
	# abort, it is also our final state.
	#
	switch $sub {
		0 {
			set oflags "-create $method -auto_commit -mode 0644 \
			    -env $env $opts $dbfile"
		}
		1 {
			set oflags "-create $method -auto_commit -mode 0644 \
			    -env $env $opts $dbfile sub0"
		}
		2 {
			#
			# If we are aborting here, then we need to
			# create a first subdb, then create a second
			#
			set oflags "-create $method -auto_commit -mode 0644 \
			    -env $env $opts $dbfile sub0"
			set db [eval {berkdb_open} $oflags]
			error_check_good db_open [is_valid_db $db] TRUE
			error_check_good db_close [$db close] 0
			set init_file $dir/$data_dir/$dbfile.init
			catch { file copy -force $dir/$data_dir/$dbfile $init_file } res
			set oflags "-create $method -auto_commit -mode 0644 \
			    -env $env $opts $dbfile sub1"
		}
		default {
			puts "\tBad value $sub for sub"
			return
		}
	}
	#
	# Set our locations to copy and abort
	#
	set ret [eval $env test copy $copy]
	error_check_good test_copy $ret 0
	set ret [eval $env test abort $abort]
	error_check_good test_abort $ret 0

	puts "\t\tExecuting command"
	set ret [catch {eval {berkdb_open} $oflags} db]

	# Sync the mpool so any changes to the file that are
	# in mpool get written to the disk file before the
	# diff.
	$env mpool_sync

	#
	# If we don't abort, then we expect success.
	# If we abort, we expect no file created.
	#
	if {[string first "none" $abort] == -1} {
		#
		# Operation was aborted, verify it does
		# not exist.
		#
		puts "\t\tCommand executed and aborted."
		error_check_bad db_open ret 0

		#
		# Check that the file does not exist.  Final state.
		#
		if { $sub != 2 } {
			error_check_good db_open:exists \
			    [file exists $dir/$data_dir/$dbfile] 0
		} else {
			error_check_good \
			    diff(init,postcreate):diff($init_file,$dir/$data_dir/$dbfile)\
                          [dbdump_diff $dflags $init_file $dir $data_dir/$dbfile] 0
		}
	} else {
		#
		# Operation was committed, verify it exists.
		#
		puts "\t\tCommand executed and committed."
		error_check_good db_open [is_valid_db $db] TRUE
		error_check_good db_close [$db close] 0

		#
		# Check that the file exists.
		#
		error_check_good db_open [file exists $dir/$data_dir/$dbfile] 1
		set init_file $dir/$data_dir/$dbfile.init
		catch { file copy -force $dir/$data_dir/$dbfile $init_file } res

		if { [is_queue $method] == 1  || [is_partitioned $opts] == 1} {
			copy_extent_file $dir/$data_dir $dbfile init
		}
	}
	error_check_good log_flush [$env log_flush] 0
	error_check_good env_close [$env close] 0

	#
	# Run recovery here.  Should be a no-op.  Verify that
	# the file still doesn't exist or change (depending on sub)
	# when we are done.
	#
	berkdb debug_check
	puts -nonewline "\t\tAbout to run recovery ... "
	flush stdout

	set env [eval $env_cmd -recover_fatal]
	error_check_good env_close [$env close] 0
#	set stat [catch {exec $util_path/db_recover -h $dir -c} result]
#	if { $stat == 1 } {
#		error "FAIL: Recovery error: $result."
#		return
#	}
	puts "complete"
	if { $sub != 2 && [string first "none" $abort] == -1} {
		#
		# Operation was aborted, verify it still does
		# not exist.  Only done with file creations.
		#
		error_check_good after_recover1 [file exists $dir/$data_dir/$dbfile] 0
	} else {
		#
		# Operation was committed or just a subdb was aborted.
		# Verify it did not change.
		#
		error_check_good \
		    diff(initial,post-recover1):diff($init_file,$dir/$data_dir/$dbfile) \
                  [dbdump_diff $dflags $init_file $dir $data_dir/$dbfile] 0
		#
		# Need a new copy to get the right LSN into the file.
		#
		catch { file copy -force $dir/$data_dir/$dbfile $init_file } res

		if { [is_queue $method] == 1 || [is_partitioned $opts] == 1 } {
			copy_extent_file $dir/$data_dir $dbfile init
		}
	}

	# If we didn't make a copy, then we are done.
	#
	if {[string first "none" $copy] != -1} {
		return
	}

	#
	# Now move the .afterop file to $dbfile.  Run recovery again.
	#
	copy_afterop $dir

	berkdb debug_check
	puts -nonewline "\t\tAbout to run recovery ... "
	flush stdout

	set env [eval $env_cmd -recover_fatal]
	error_check_good env_close [$env close] 0
#	set stat [catch {exec $util_path/db_recover -h $dir -c} result]
#	if { $stat == 1 } {
#		error "FAIL: Recovery error: $result."
#		return
#	}
	puts "complete"
	if { $sub != 2 && [string first "none" $abort] == -1} {
		#
		# Operation was aborted, verify it still does
		# not exist.  Only done with file creations.
		#
		error_check_good after_recover2 [file exists $dir/$data_dir/$dbfile] 0
	} else {
		#
		# Operation was committed or just a subdb was aborted.
		# Verify it did not change.
		#
		error_check_good \
		    diff(initial,post-recover2):diff($init_file,$dir/$data_dir/$dbfile) \
                  [dbdump_diff $dflags $init_file $dir $data_dir/$dbfile] 0
	}

}

proc file_recover_delete { dir env_cmd method opts dbfile cmd msg op data_dir} {
	#
	# We run this test on each of these scenarios:
	# 1.  Deleting/Renaming just a database
	# 2.  Deleting/Renaming a database with a subdb
	# 3.  Deleting/Renaming a 2nd subdb in a database
	puts "\t$msg $op ($opts) with a database"
	do_file_recover_delete $dir $env_cmd $method $opts $dbfile \
	    0 $cmd $msg $op $data_dir
	if { [is_queue $method] == 1  || [is_partitioned $opts] == 1} {
		puts "\tSkipping subdatabase tests for method $method"
		return
	}
	puts "\t$msg $op ($opts) with a database and subdb"
	do_file_recover_delete $dir $env_cmd $method $opts $dbfile \
	    1 $cmd $msg $op $data_dir
	puts "\t$msg $op ($opts) with a database and 2nd subdb"
	do_file_recover_delete $dir $env_cmd $method $opts $dbfile \
	    2 $cmd $msg $op $data_dir

}

proc do_file_recover_delete { dir env_cmd method opts dbfile sub cmd msg op data_dir} {
	global log_log_record_types
	source ./include.tcl

	# Keep track of the log types we've seen
	if { $log_log_record_types == 1} {
		logtrack_read $dir
	}

	env_cleanup $dir
	# Open the environment and set the copy/abort locations
	set env [eval $env_cmd]
	set copy [lindex $cmd 0]
	set abort [lindex $cmd 1]
	error_check_good copy_location [is_valid_delete_loc $copy] 1
	error_check_good abort_location [is_valid_delete_loc $abort] 1

	if { [is_record_based $method] == 1 } {
		set key1 1
		set key2 2
	} else {
		set key1 recd007_key1
		set key2 recd007_key2
	}
	set data1 recd007_data0
	set data2 recd007_data1
	set data3 NEWrecd007_data2

	#
	# Depending on what sort of subdb we want, if any, our
	# args to the open call will be different (and if we
	# want a 2nd subdb, we create the first here.
	#
	# XXX
	# For dbtruncate, we want oflags to have "$env" in it,
	# not have the value currently in 'env'.  That is why
	# the '$' is protected below.  Later on we use oflags
	# but with a new $env we just opened.
	#
	switch $sub {
		0 {
			set subdb ""
			set new $dbfile.new
			set dflags "-dar"
			set oflags "-create $method -auto_commit -mode 0644 \
			    -env \$env $opts $dbfile"
		}
		1 {
			set subdb sub0
			set new $subdb.new
			set dflags ""
			set oflags "-create $method -auto_commit -mode 0644 \
			    -env \$env $opts $dbfile $subdb"
		}
		2 {
			#
			# If we are aborting here, then we need to
			# create a first subdb, then create a second
			#
			set subdb sub1
			set new $subdb.new
			set dflags ""
			set oflags "-create $method -auto_commit -mode 0644 \
			    -env \$env $opts $dbfile sub0"
			set db [eval {berkdb_open} $oflags]
			error_check_good db_open [is_valid_db $db] TRUE
			set txn [$env txn]
			set ret [$db put -txn $txn $key1 $data1]
			error_check_good db_put $ret 0
			error_check_good commit [$txn commit] 0
			error_check_good db_close [$db close] 0
			set oflags "-create $method -auto_commit -mode 0644 \
			    -env \$env $opts $dbfile $subdb"
		}
		default {
			puts "\tBad value $sub for sub"
			return
		}
	}

	#
	# Set our locations to copy and abort
	#
	set ret [eval $env test copy $copy]
	error_check_good test_copy $ret 0
	set ret [eval $env test abort $abort]
	error_check_good test_abort $ret 0

	#
	# Open our db, add some data, close and copy as our
	# init file.
	#
	set db [eval {berkdb_open} $oflags]
	error_check_good db_open [is_valid_db $db] TRUE
	set txn [$env txn]
	set ret [$db put -txn $txn $key1 $data1]
	error_check_good db_put $ret 0
	set ret [$db put -txn $txn $key2 $data2]
	error_check_good db_put $ret 0
	error_check_good commit [$txn commit] 0
	error_check_good db_close [$db close] 0

	$env mpool_sync

	set init_file $dir/$data_dir/$dbfile.init
	catch { file copy -force $dir/$data_dir/$dbfile $init_file } res

	if { [is_queue $method] == 1 || [is_partitioned $opts] == 1}  {
		copy_extent_file $dir/$data_dir $dbfile init
	}

	#
	# If we don't abort, then we expect success.
	# If we abort, we expect no file removed.
	#
	switch $op {
	"dbrename" {
		set ret [catch { eval {berkdb} $op -env $env -auto_commit \
		    $dbfile $subdb $new } remret]
	}
	"dbremove" {
		set ret [catch { eval {berkdb} $op -env $env -auto_commit \
		    $dbfile $subdb } remret]
	}
	"dbtruncate" {
		set txn [$env txn]
		set db [eval {berkdb_open_noerr -env} \
		    $env -auto_commit $opts $dbfile $subdb]
		error_check_good dbopen [is_valid_db $db] TRUE
		error_check_good txnbegin [is_valid_txn $txn $env] TRUE
		set ret [catch {$db truncate -txn $txn} remret]
	}
	}
	$env mpool_sync
	if { $abort == "none" } {
		if { $op == "dbtruncate" } {
			error_check_good txncommit [$txn commit] 0
			error_check_good dbclose [$db close] 0
		}
		#
		# Operation was committed, verify it.
		#
		puts "\t\tCommand executed and committed."
		error_check_good $op $ret 0
		#
		# If a dbtruncate, check that truncate returned the number
		# of items previously in the database.
		#
		if { [string compare $op "dbtruncate"] == 0 } {
			error_check_good remret $remret 2
		}
		recd007_check $op $sub $dir $dbfile $subdb $new $env $oflags $data_dir
	} else {
		#
		# Operation was aborted, verify it did not change.
		#
		if { $op == "dbtruncate" } {
			error_check_good txnabort [$txn abort] 0
			error_check_good dbclose [$db close] 0
		}
		puts "\t\tCommand executed and aborted."
		error_check_good $op $ret 1

		#
		# Check that the file exists.  Final state.
		# Compare against initial file.
		#
		error_check_good post$op.1 [file exists $dir/$data_dir/$dbfile] 1
		error_check_good \
		    diff(init,post$op.2):diff($init_file,$dir/$data_dir/$dbfile)\
                  [dbdump_diff $dflags $init_file $dir $data_dir/$dbfile] 0
	}
	$env mpool_sync
	error_check_good log_flush [$env log_flush] 0
	error_check_good env_close [$env close] 0
	catch { file copy -force $dir/$data_dir/$dbfile $init_file } res
	if { [is_queue $method] == 1 || [is_partitioned $opts] == 1} {
		copy_extent_file $dir/$data_dir $dbfile init
	}


	#
	# Run recovery here.  Should be a no-op.  Verify that
	# the file still doesn't exist or change (depending on abort)
	# when we are done.
	#
	berkdb debug_check
	puts -nonewline "\t\tAbout to run recovery ... "
	flush stdout

	set env [eval $env_cmd -recover_fatal]
	error_check_good env_close [$env close] 0
#	set stat [catch {exec $util_path/db_recover -h $dir -c} result]
#	if { $stat == 1 } {
#		error "FAIL: Recovery error: $result."
#		return
#	}

	puts "complete"

	if { $abort == "none" } {
		#
		# Operate was committed.
		#
		set env [eval $env_cmd]
		recd007_check $op $sub $dir $dbfile $subdb $new $env $oflags $data_dir
		error_check_good log_flush [$env log_flush] 0
		error_check_good env_close [$env close] 0
	} else {
		#
		# Operation was aborted, verify it did not change.
		#
		berkdb debug_check
		error_check_good \
		    diff(initial,post-recover1):diff($init_file,$dir/$data_dir/$dbfile) \
                  [dbdump_diff $dflags $init_file $dir $data_dir/$dbfile] 0
	}

	#
	# If we didn't make a copy, then we are done.
	#
	if {[string first "none" $copy] != -1} {
		return
	}

	#
	# Now restore the .afterop file(s) to their original name.
	# Run recovery again.
	#
	copy_afterop $dir

	berkdb debug_check
	puts -nonewline "\t\tAbout to run recovery ... "
	flush stdout

	set env [eval $env_cmd -recover_fatal]
	error_check_good env_close [$env close] 0
#	set stat [catch {exec $util_path/db_recover -h $dir -c} result]
#	if { $stat == 1 } {
#		error "FAIL: Recovery error: $result."
#		return
#	}
	puts "complete"

	if { [string first "none" $abort] != -1} {
		set env [eval $env_cmd]
		recd007_check $op $sub $dir $dbfile $subdb $new $env $oflags $data_dir
		error_check_good log_flush [$env log_flush] 0
		error_check_good env_close [$env close] 0
	} else {
		#
		# Operation was aborted, verify it did not change.
		#
		error_check_good \
		    diff(initial,post-recover2):diff($init_file,$dir/$data_dir/$dbfile) \
                  [dbdump_diff $dflags $init_file $dir $data_dir/$dbfile] 0
	}

}

#
# This function tests a specific case of recovering after a db removal.
# This is for SR #2538.  Basically we want to test that:
# - Make an env.
# - Make/close a db.
# - Remove the db.
# - Create another db of same name.
# - Sync db but leave open.
# - Run recovery.
# - Verify no recovery errors and that new db is there.
proc do_file_recover_delmk { dir env_cmd method opts dbfile data_dir} {
	global log_log_record_types
	source ./include.tcl

	# Keep track of the log types we've seen
	if { $log_log_record_types == 1} {
		logtrack_read $dir
	}
	set omethod [convert_method $method]

	puts "\tRecd007.q: Delete and recreate a database"
	env_cleanup $dir
	# Open the environment and set the copy/abort locations
	set env [eval $env_cmd]
	error_check_good env_open [is_valid_env $env] TRUE

	if { [is_record_based $method] == 1 } {
		set key 1
	} else {
		set key recd007_key
	}
	set data1 recd007_data
	set data2 NEWrecd007_data2
	set data3 LASTrecd007_data3

	set oflags \
	    "-create $omethod -auto_commit -mode 0644 $opts $dbfile"

	#
	# Open our db, add some data, close and copy as our
	# init file.
	#
	set db [eval {berkdb_open_noerr} -env $env $oflags]
	error_check_good db_open [is_valid_db $db] TRUE
	set txn [$env txn]
	set ret [$db put -txn $txn $key $data1]
	error_check_good db_put $ret 0
	error_check_good commit [$txn commit] 0
	error_check_good db_close [$db close] 0
	file copy -force $testdir/$data_dir/$dbfile $testdir/$data_dir/${dbfile}.1

	set ret \
	    [catch { berkdb dbremove -env $env -auto_commit $dbfile } remret]

	#
	# Operation was committed, verify it does
	# not exist.
	#
	puts "\t\tCommand executed and committed."
	error_check_good dbremove $ret 0
	error_check_good dbremove.1 [file exists $dir/$data_dir/$dbfile] 0

	#
	# Now create a new db with the same name.
	#
	set db [eval {berkdb_open_noerr} -env $env $oflags]
	error_check_good db_open [is_valid_db $db] TRUE
	set txn [$env txn]
	set ret [$db put -txn $txn $key [chop_data $method $data2]]
	error_check_good db_put $ret 0
	error_check_good commit [$txn commit] 0
	error_check_good db_sync [$db sync] 0

	berkdb debug_check
	puts -nonewline "\t\tAbout to run recovery ... "
	flush stdout

	set envr [eval $env_cmd -recover_fatal]
	error_check_good env_close [$envr close] 0
#	set stat [catch {exec $util_path/db_recover -h $dir -c} result]
#	if { $stat == 1 } {
#		error "FAIL: Recovery error: $result."
#		return
#	}
	puts "complete"
#	error_check_good db_recover $stat 0
	error_check_good file_exist [file exists $dir/$data_dir/$dbfile] 1
	#
	# Since we ran recovery on the open db/env, we need to
	# catch these calls.  Basically they are there to clean
	# up the Tcl widgets.
	#
	set stat [catch {$db close} ret]
	error_check_bad dbclose_after_remove $stat 0
	error_check_good dbclose_after_remove [is_substr $ret recovery] 1
	set stat [catch {$env log_flush} ret]
	set stat [catch {$env close} ret]
	error_check_bad envclose_after_remove $stat 0
	error_check_good envclose_after_remove [is_substr $ret recovery] 1

	#
	# Reopen env and db and verify 2nd database is there.
	#
	set env [eval $env_cmd]
	error_check_good env_open [is_valid_env $env] TRUE
	set db [eval {berkdb_open} -env $env $oflags]
	error_check_good db_open [is_valid_db $db] TRUE
	set ret [$db get $key]
	error_check_good dbget [llength $ret] 1
	set kd [lindex $ret 0]
	error_check_good key [lindex $kd 0] $key
	error_check_good data2 [lindex $kd 1] [pad_data $method $data2]

	error_check_good dbclose [$db close] 0
	error_check_good log_flush [$env log_flush] 0
	error_check_good envclose [$env close] 0

	#
	# Copy back the original database and run recovery again.
	# SR [#13026]
	#
	puts "\t\tRecover from first database"
	file copy -force $testdir/$data_dir/${dbfile}.1 $testdir/$data_dir/$dbfile
	berkdb debug_check
	puts -nonewline "\t\tAbout to run recovery ... "
	flush stdout

	set env [eval $env_cmd -recover_fatal]
	error_check_good env_close [$env close] 0
#	set stat [catch {exec $util_path/db_recover -h $dir -c} result]
#	if { $stat == 1 } {
#		error "FAIL: Recovery error: $result."
#		return
#	}
	puts "complete"
#	error_check_good db_recover $stat 0
	error_check_good db_recover.1 [file exists $dir/$data_dir/$dbfile] 1

	#
	# Reopen env and db and verify 2nd database is there.
	#
	set env [eval $env_cmd]
	error_check_good env_open [is_valid_env $env] TRUE
	set db [eval {berkdb_open_noerr} -env $env $oflags]
	error_check_good db_open [is_valid_db $db] TRUE
	set ret [$db get $key]
	error_check_good dbget [llength $ret] 1
	set kd [lindex $ret 0]
	error_check_good key [lindex $kd 0] $key
	error_check_good data2 [lindex $kd 1] [pad_data $method $data2]

	error_check_good dbclose [$db close] 0

	file copy -force $testdir/$data_dir/$dbfile $testdir/$data_dir/${dbfile}.2

	puts "\t\tRemove second db"
	set ret \
	    [catch { berkdb dbremove -env $env -auto_commit $dbfile } remret]

	#
	# Operation was committed, verify it does
	# not exist.
	#
	puts "\t\tCommand executed and committed."
	error_check_good dbremove $ret 0
	error_check_good dbremove.2 [file exists $dir/$data_dir/$dbfile] 0

	#
	# Now create a new db with the same name.
	#
	puts "\t\tAdd a third version of the database"
	set db [eval {berkdb_open_noerr} -env $env $oflags]
	error_check_good db_open [is_valid_db $db] TRUE
	set txn [$env txn]
	set ret [$db put -txn $txn $key [chop_data $method $data3]]
	error_check_good db_put $ret 0
	error_check_good commit [$txn commit] 0
	error_check_good db_sync [$db sync] 0

	berkdb debug_check
	puts -nonewline "\t\tAbout to run recovery ... "
	flush stdout

	set envr [eval $env_cmd -recover_fatal]
	error_check_good env_close [$envr close] 0
#	set stat [catch {exec $util_path/db_recover -h $dir -c} result]
#	if { $stat == 1 } {
#		error "FAIL: Recovery error: $result."
#		return
#	}
	puts "complete"
#	error_check_good db_recover $stat 0
	error_check_good file_exist [file exists $dir/$data_dir/$dbfile] 1

	#
	# Since we ran recovery on the open db/env, we need to
	# catch these calls to clean up the Tcl widgets.
	#
	set stat [catch {$db close} ret]
	error_check_bad dbclose_after_remove $stat 0
	error_check_good dbclose_after_remove [is_substr $ret recovery] 1
	set stat [catch {$env log_flush} ret]
	set stat [catch {$env close} ret]
	error_check_bad envclose_after_remove $stat 0
	error_check_good envclose_after_remove [is_substr $ret recovery] 1

	#
	# Copy back the second database and run recovery again.
	#
	puts "\t\tRecover from second database"
	file copy -force $testdir/$data_dir/${dbfile}.2 $testdir/$data_dir/$dbfile
	berkdb debug_check
	puts -nonewline "\t\tAbout to run recovery ... "
	flush stdout

	set envr [eval $env_cmd -recover_fatal]
	error_check_good env_close [$envr close] 0
#	set stat [catch {exec $util_path/db_recover -h $dir -c} result]
#	if { $stat == 1 } {
#		error "FAIL: Recovery error: $result."
#		return
#	}
	puts "complete"
#	error_check_good db_recover $stat 0
	error_check_good file_exist.2 [file exists $dir/$data_dir/$dbfile] 1

	#
	# Reopen env and db and verify 3rd database is there.
	#
	set env [eval $env_cmd]
	error_check_good env_open [is_valid_env $env] TRUE
	set db [eval {berkdb_open} -env $env $oflags]
	error_check_good db_open [is_valid_db $db] TRUE
	set ret [$db get $key]
	error_check_good dbget [llength $ret] 1
	set kd [lindex $ret 0]
	error_check_good key [lindex $kd 0] $key
	error_check_good data2 [lindex $kd 1] [pad_data $method $data3]

	error_check_good dbclose [$db close] 0
	error_check_good log_flush [$env log_flush] 0
	error_check_good envclose [$env close] 0
}

proc is_valid_create_loc { loc } {
	switch $loc {
		none		-
		preopen		-
		postopen	-
		postlogmeta	-
		postlog		-
		postsync
			{ return 1 }
		default
			{ return 0 }
	}
}

proc is_valid_delete_loc { loc } {
	switch $loc {
		none		-
		predestroy	-
		postdestroy	-
		postremcall
			{ return 1 }
		default
			{ return 0 }
	}
}

# Do a logical diff on the db dump files.  We expect that either
# the files are identical, or if they differ, that it is exactly
# just a free/invalid page.
# Return 1 if they are different, 0 if logically the same (or identical).
#
proc dbdump_diff { flags initfile dir dbfile } {
	source ./include.tcl

	set initdump $initfile.dump
	set dbdump $dbfile.dump

	set stat [catch {eval {exec $util_path/db_dump} $flags -f $initdump \
	    $initfile} ret]
	error_check_good "dbdump.init $flags $initfile" $stat 0

	# Do a dump without the freelist which should eliminate any
	# recovery differences.
	set stat [catch {eval {exec $util_path/db_dump} $flags -f $dir/$dbdump \
          $dir/$dbfile} ret]
	error_check_good dbdump.db $stat 0

	set stat [filecmp $dir/$dbdump $initdump]

	if {$stat == 0} {
		return 0
	}
	puts "diff: $dbdump $initdump gives:\n$ret"
	return 1
}

proc recd007_check { op sub dir dbfile subdb new env oflags data_dir} {
	#
	# No matter how many subdbs we have, dbtruncate will always
	# have a file, and if we open our particular db, it should
	# have no entries.
	#
	if { $sub == 0 } {
		if { $op == "dbremove" } {
			error_check_good $op:not-exist:$dir/$dbfile \
			    [file exists $dir/$data_dir/$dbfile] 0
		} elseif { $op == "dbrename"} {
			error_check_good $op:exist \
			    [file exists $dir/$data_dir/$dbfile] 0
			error_check_good $op:exist2 \
			    [file exists $dir/$data_dir/$dbfile.new] 1
		} else {
			error_check_good $op:exist \
			    [file exists $dir/$data_dir/$dbfile] 1
			set db [eval {berkdb_open} $oflags]
			error_check_good db_open [is_valid_db $db] TRUE
			set dbc [$db cursor]
			error_check_good dbc_open \
			    [is_valid_cursor $dbc $db] TRUE
			set ret [$dbc get -first]
			error_check_good dbget1 [llength $ret] 0
			error_check_good dbc_close [$dbc close] 0
			error_check_good db_close [$db close] 0
		}
		return
	} else {
		set t1 $dir/t1
		#
		# If we have subdbs, check that all but the last one
		# are there, and the last one is correctly operated on.
		#
		set db [berkdb_open -rdonly -env $env $dbfile]
		error_check_good dbopen [is_valid_db $db] TRUE
		set c [eval {$db cursor}]
		error_check_good db_cursor [is_valid_cursor $c $db] TRUE
		set d [$c get -last]
		if { $op == "dbremove" } {
			if { $sub == 1 } {
				error_check_good subdb:rem [llength $d] 0
			} else {
				error_check_bad subdb:rem [llength $d] 0
				set sdb [lindex [lindex $d 0] 0]
				error_check_bad subdb:rem1 $sdb $subdb
			}
		} elseif { $op == "dbrename"} {
			set sdb [lindex [lindex $d 0] 0]
			error_check_good subdb:ren $sdb $new
			if { $sub != 1 } {
				set d [$c get -prev]
				error_check_bad subdb:ren [llength $d] 0
				set sdb [lindex [lindex $d 0] 0]
				error_check_good subdb:ren1 \
				    [is_substr "new" $sdb] 0
			}
		} else {
			set sdb [lindex [lindex $d 0] 0]
			set dbt [berkdb_open -rdonly -env $env $dbfile $sdb]
			error_check_good db_open [is_valid_db $dbt] TRUE
			set dbc [$dbt cursor]
			error_check_good dbc_open \
			    [is_valid_cursor $dbc $dbt] TRUE
			set ret [$dbc get -first]
			error_check_good dbget2 [llength $ret] 0
			error_check_good dbc_close [$dbc close] 0
			error_check_good db_close [$dbt close] 0
			if { $sub != 1 } {
				set d [$c get -prev]
				error_check_bad subdb:ren [llength $d] 0
				set sdb [lindex [lindex $d 0] 0]
				set dbt [berkdb_open -rdonly -env $env \
				    $dbfile $sdb]
				error_check_good db_open [is_valid_db $dbt] TRUE
				set dbc [$db cursor]
				error_check_good dbc_open \
				    [is_valid_cursor $dbc $db] TRUE
				set ret [$dbc get -first]
				error_check_bad dbget3 [llength $ret] 0
				error_check_good dbc_close [$dbc close] 0
				error_check_good db_close [$dbt close] 0
			}
		}
		error_check_good dbcclose [$c close] 0
		error_check_good db_close [$db close] 0
	}
}

proc copy_afterop { dir } {
	set r [catch { set filecopy [glob $dir/*.afterop] } res]
	if { $r == 1 } {
		return
	}
	foreach f $filecopy {
		set orig [string range $f 0 \
			[expr [string last "." $f] - 1]]
		catch { file rename -force $f $orig} res
	}
}

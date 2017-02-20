# See the file LICENSE for redistribution information.
#
# Copyright (c) 1996-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	recd019
# TEST	Test txn id wrap-around and recovery.
proc recd019 { method {numid 50} args} {
	global fixed_len
	global txn_curid
	global log_log_record_types
	source ./include.tcl

	set orig_fixed_len $fixed_len
	set opts [convert_args $method $args]
	set omethod [convert_method $method]

	puts "Recd019: $method txn id wrap-around test"

	# Create the database and environment.
	env_cleanup $testdir

	set testfile recd019.db

	set flags "-create -txn -home $testdir"

	puts "\tRecd019.a: creating environment"
	set env_cmd "berkdb_env $flags"
	set dbenv [eval $env_cmd]
	error_check_good dbenv [is_valid_env $dbenv] TRUE

	# Test txn wrapping.  Force a txn_recycle msg.
	#
	set new_curid $txn_curid
	set new_maxid [expr $new_curid + $numid]
	error_check_good txn_id_set [$dbenv txn_id_set $new_curid $new_maxid] 0

	#
	# We need to create a database to get the pagesize (either
	# the default or whatever might have been specified).
	# Then remove it so we can compute fixed_len and create the
	# real database.
	set oflags "-create $omethod -mode 0644 \
	    -env $dbenv $opts $testfile"
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
	error_check_good dbremove [berkdb dbremove -env $dbenv $testfile] 0

	# Convert the args again because fixed_len is now real.
	# Create the databases and close the environment.
	# cannot specify db truncate in txn protected env!!!
	set opts [convert_args $method $args]
	set omethod [convert_method $method]
	set oflags "-create $omethod -mode 0644 \
	    -env $dbenv -auto_commit $opts $testfile"
	set db [eval {berkdb_open} $oflags]
	error_check_good db_open [is_valid_db $db] TRUE

	#
	# Force txn ids to wrap twice and then some.
	#
	set nument [expr $numid * 3 - 2]
	puts "\tRecd019.b: Wrapping txn ids after $numid"
	set file $testdir/$testfile.init
	catch { file copy -force $testdir/$testfile $file} res
	copy_extent_file $testdir $testfile init
	for { set i 1 } { $i <= $nument } { incr i } {
		# Use 'i' as key so method doesn't matter
		set key $i
		set data $i

		# Put, in a txn.
		set txn [$dbenv txn]
		error_check_good txn_begin [is_valid_txn $txn $dbenv] TRUE
		error_check_good db_put \
		    [$db put -txn $txn $key [chop_data $method $data]] 0
		error_check_good txn_commit [$txn commit] 0
	}
	error_check_good db_close [$db close] 0
	set file $testdir/$testfile.afterop
	catch { file copy -force $testdir/$testfile $file} res
	copy_extent_file $testdir $testfile afterop
	error_check_good env_close [$dbenv close] 0

        # Keep track of the log types we've seen
	if { $log_log_record_types == 1} {
		logtrack_read $testdir
	}

	# Now, loop through and recover.
	puts "\tRecd019.c: Run recovery (no-op)"
	set ret [catch {exec $util_path/db_recover -h $testdir} r]
	error_check_good db_recover $ret 0

	puts "\tRecd019.d: Run recovery (initial file)"
	set file $testdir/$testfile.init
	catch { file copy -force $file $testdir/$testfile } res
	move_file_extent $testdir $testfile init copy

	set ret [catch {exec $util_path/db_recover -h $testdir} r]
	error_check_good db_recover $ret 0

	puts "\tRecd019.e: Run recovery (after file)"
	set file $testdir/$testfile.afterop
	catch { file copy -force $file $testdir/$testfile } res
	move_file_extent $testdir $testfile afterop copy

	set ret [catch {exec $util_path/db_recover -h $testdir} r]
	error_check_good db_recover $ret 0
	set fixed_len $orig_fixed_len
	return
}

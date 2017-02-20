# See the file LICENSE for redistribution information.
#
# Copyright (c) 2004-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	recd021
# TEST	Test of failed opens in recovery.
# TEST
# TEST	If a file was deleted through the file system (and not
# TEST	within Berkeley DB), an error message should appear.
# TEST	Test for regular files and subdbs.

proc recd021 { method args } {
	source ./include.tcl
	global util_path

	set args [convert_args $method $args]
	set omethod [convert_method $method]
	set nentries 100

	puts "\nRecd021: ($method)\
	    Test failed opens in recovery."

	# The file ops "remove" and "rename" are done within
	# Berkeley DB.  A "delete" is done directly on the file
	# system (as if the user deleted the file).
	#
	# First test regular files.
	#
	foreach op { remove rename delete noop } {
		env_cleanup $testdir
		puts "\tRecd021: Test $op of file in recovery."

		# Create transactional environment.
		set env [berkdb_env -create -home $testdir -txn]
		error_check_good is_valid_env [is_valid_env $env] TRUE

		# Create database
		puts "\t\tRecd021.a.1: Create and populate file."

		if { $op == "rename" } {
			set names {A B}
		} else {
			set names {A}
		}
		set name [lindex $names 0]

		set db [eval {berkdb_open \
		    -create} $omethod $args -env $env -auto_commit $name.db]
		error_check_good dba_open [is_valid_db $db] TRUE

		# Checkpoint.
		error_check_good txn_checkpoint [$env txn_checkpoint] 0
		for { set i 1 } { $i <= $nentries } { incr i } {
			error_check_good dba_put [$db put $i data$i] 0
		}
		error_check_good dba_close [$db close] 0

		# Do operation on file.
		puts "\t\tRecd021.b: Do $op on file."
		set txn [$env txn]
		set ret [do_op $omethod $op $names $txn $env]
		error_check_good do_op $ret 0
		error_check_good txn_commit [$txn commit] 0
		error_check_good env_close [$env close] 0

		# Recover.
		puts "\t\tRecd021.c: Recover."
		set ret [catch {exec $util_path/db_recover -h $testdir} r]
		if { $op == "delete" } {
			error_check_good external_delete \
			    [is_substr $r "Warning: open failed"] 1
		} else {
			error_check_good $op $ret 0
		}

		# Clean up.
		error_check_good \
		    env_remove [berkdb envremove -force -home $testdir] 0
		fileremove -f $testdir/$name.db
	}

	# Test subdbs.
	if { [is_queue $method] == 1 } {
		puts "Recd021: Skipping test of subdbs for method $method."
		return
	}

	# The first subdb test just does the op, and is comparable
	# to the tests for regular files above.
	set trunc 0
	set special {}
	foreach op { remove rename delete noop } {
		recd021_testsubdb $method $op $nentries $special $trunc $args
	}

	# The remainder of the tests are executed first with the log intact,
	# then with the log truncated at the __db_subdb_name record.
	foreach trunc { 0 1 } {
		# Test what happens if subdb2 reuses pages formerly in
		# subdb1, after removing subdb1.
		set special "reuse"
		recd021_testsubdb $method remove $nentries $special $trunc $args

		# Test what happens if a new subdb reuses pages formerly
		# in subdb1, after removing subdb1.
		set special "newdb"
		recd021_testsubdb $method remove $nentries $special $trunc $args

		# Now we test what happens if a new subdb if a different access
		# method reuses pages formerly in subdb1, after removing subdb1.
		set special "newtypedb"
		recd021_testsubdb $method remove $nentries $special $trunc $args
	}
}

proc recd021_testsubdb { method op nentries special trunc largs } {
	source ./include.tcl
	global util_path

	set omethod [convert_method $method]
	env_cleanup $testdir

	puts "\tRecd021: \
	    Test $op of subdb in recovery ($special trunc = $trunc)."

	# Create transactional environment.
	set env [berkdb_env -create -home $testdir -txn]
	error_check_good is_valid_env [is_valid_env $env] TRUE

	# Create database with 2 subdbs
	puts "\t\tRecd021.d: Create and populate subdbs."
	set sname1 S1
	set sname2 S2
	if { $op == "rename" } {
		set names {A S1 NEW_S1}
	} elseif { $op == "delete" } {
		set names {A}
	} else {
		set names {A S1}
	}
	set name [lindex $names 0]

	set sdb1 [eval {berkdb_open -create} $omethod \
	    $largs -env $env -auto_commit $name.db $sname1]
	error_check_good sdb1_open [is_valid_db $sdb1] TRUE
	set sdb2 [eval {berkdb_open -create} $omethod \
	    $largs -env $env -auto_commit $name.db $sname2]
	error_check_good sdb2_open [is_valid_db $sdb2] TRUE

	# Checkpoint.
	error_check_good txn_checkpoint [$env txn_checkpoint] 0
	for { set i 1 } { $i <= $nentries } { incr i } {
		error_check_good sdb1_put [$sdb1 put $i data$i] 0
	}
	set dumpfile dump.s1.$trunc
	set ret [exec $util_path/db_dump -dar -f $dumpfile -h $testdir A.db]
	for { set i 1 } { $i <= $nentries } { incr i } {
		error_check_good sdb2_put [$sdb2 put $i data$i] 0
	}
	error_check_good sdb1_close [$sdb1 close] 0

	# Do operation on subdb.
	puts "\t\tRecd021.e: Do $op on file."
	set txn [$env txn]

	if { $trunc == 1 } {
		# Create a log cursor to mark where we are before
		# doing the op.
		set logc [$env log_cursor]
		set ret [lindex [$logc get -last] 0]
		file copy -force $testdir/log.0000000001 $testdir/log.sav
	}

	set ret [do_subdb_op $omethod $op $names $txn $env]
	error_check_good do_subdb_op $ret 0
	error_check_good txn_commit [$txn commit] 0

	if { $trunc == 1 } {
		# Walk the log and find the __db_subdb_name entry.
		set found 0
		while { $found == 0 } {
			set lsn [lindex [$logc get -next] 0]
			set lfile [lindex $lsn 0]
			set loff [lindex $lsn 1]
			set logrec [exec $util_path/db_printlog -h $testdir \
			    -b $lfile/$loff -e $lfile/$loff]
			if { [is_substr $logrec __db_subdb_name] == 1 } {
				set found 1
			}
		}
		# Create the truncated log, and save it for later.
		catch [exec dd if=$testdir/log.0000000001 \
		    of=$testdir/log.sav count=$loff bs=1 >& /dev/null ] res
	}

	# Here we do the "special" thing, if any.  We always
	# have to close sdb2, but when we do so varies.
	switch -exact -- $special {
		"" {
			error_check_good sdb2_close [$sdb2 close] 0
		}
		reuse {
			for { set i [expr $nentries + 1] } \
			    { $i <= [expr $nentries * 2]} { incr i } {
				error_check_good sdb2_put \
				    [$sdb2 put $i data$i] 0
			}
			error_check_good sdb2_close [$sdb2 close] 0
			set dumpfile dump.s2.$trunc
			set ret [exec $util_path/db_dump -dar \
			    -f $dumpfile -h $testdir A.db]
		}
		newdb {
			error_check_good sdb2_close [$sdb2 close] 0
			set sname3 S3
			set sdb3 [eval {berkdb_open -create} $omethod \
			    $largs -env $env -auto_commit $name.db $sname3]
			error_check_good sdb3_open [is_valid_db $sdb3] TRUE
			for { set i 1 } { $i <= $nentries } { incr i } {
				error_check_good sdb3_put \
				     [$sdb3 put $i data$i] 0
			}
			error_check_good sdb3_close [$sdb3 close] 0
		}
		newtypedb {
			error_check_good sdb2_close [$sdb2 close] 0
			set sname4 S4
			set newmethod [different_method $method]
			set args [convert_args $newmethod]
			set omethod [convert_method $newmethod]
			set sdb4 [eval {berkdb_open -create} $omethod \
			    $args -env $env -auto_commit $name.db $sname4]
			error_check_good sdb4_open [is_valid_db $sdb4] TRUE
			for { set i 1 } { $i <= $nentries } { incr i } {
				error_check_good sdb4_put \
				     [$sdb4 put $i data$i] 0
			}
			error_check_good sdb4_close [$sdb4 close] 0
		}
	}

	# Close the env.
	error_check_good env_close [$env close] 0

	if { $trunc == 1 } {
		# Swap in the truncated log.
		file rename -force $testdir/log.sav $testdir/log.0000000001
	}

	# Recover.
	puts "\t\tRecd021.f: Recover."
	set ret [catch {exec $util_path/db_recover -h $testdir} r]
	if { $op == "delete" || $trunc == 1 && $special != "newdb" } {
		error_check_good expect_warning \
		    [is_substr $r "Warning: open failed"] 1
	} else {
		error_check_good subdb_$op $ret 0
	}

	# Clean up.
	error_check_good env_remove [berkdb envremove -force -home $testdir] 0
	fileremove -f $testdir/$name.db
}

proc different_method { method } {
	# Queue methods are omitted, since this is for subdb testing.
	set methodlist { -btree -rbtree -recno -frecno -rrecno -hash }

	set method [convert_method $method]
        set newmethod $method
        while { $newmethod == $method } {
                set index [berkdb random_int 0 [expr [llength $methodlist] - 1]]
                set newmethod [lindex $methodlist $index]
        }
        return $newmethod
}

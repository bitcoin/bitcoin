# See the file LICENSE for redistribution information.
#
# Copyright (c) 1996-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	recd017
# TEST  Test recovery and security.  This is basically a watered
# TEST  down version of recd001 just to verify that encrypted environments
# TEST  can be recovered.
proc recd017 { method {select 0} args} {
	global fixed_len
	global encrypt
	global passwd
	global has_crypto
	source ./include.tcl

	# Skip test if release does not support encryption.
	if { $has_crypto == 0 } {
		puts "Skipping recd017 for non-crypto release."
		return
	}

	set orig_fixed_len $fixed_len
	set opts [convert_args $method $args]
	set omethod [convert_method $method]

	puts "Recd017: $method operation/transaction tests"

	# Create the database and environment.
	env_cleanup $testdir

	# The recovery tests were originally written to
	# do a command, abort, do it again, commit, and then
	# repeat the sequence with another command.  Each command
	# tends to require that the previous command succeeded and
	# left the database a certain way.  To avoid cluttering up the
	# op_recover interface as well as the test code, we create two
	# databases;  one does abort and then commit for each op, the
	# other does prepare, prepare-abort, and prepare-commit for each
	# op.  If all goes well, this allows each command to depend
	# exactly one successful iteration of the previous command.
	set testfile recd017.db
	set testfile2 recd017-2.db

	set flags "-create -encryptaes $passwd -txn -home $testdir"

	puts "\tRecd017.a.0: creating environment"
	set env_cmd "berkdb_env $flags"
	convert_encrypt $env_cmd
	set dbenv [eval $env_cmd]
	error_check_good dbenv [is_valid_env $dbenv] TRUE

	#
	# We need to create a database to get the pagesize (either
	# the default or whatever might have been specified).
	# Then remove it so we can compute fixed_len and create the
	# real database.
	set oflags "-create $omethod -mode 0644 \
	    -env $dbenv -encrypt $opts $testfile"
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
	convert_encrypt $env_cmd
	set omethod [convert_method $method]
	set oflags "-create $omethod -mode 0644 \
	    -env $dbenv -encrypt $opts $testfile"
	set db [eval {berkdb_open} $oflags]
	error_check_good db_open [is_valid_db $db] TRUE
	error_check_good db_close [$db close] 0

	set oflags "-create $omethod -mode 0644 \
	    -env $dbenv -encrypt $opts $testfile2"
	set db [eval {berkdb_open} $oflags]
	error_check_good db_open [is_valid_db $db] TRUE
	error_check_good db_close [$db close] 0

	error_check_good env_close [$dbenv close] 0

	puts "\tRecd017.a.1: Verify db_printlog can read logfile"
	set tmpfile $testdir/printlog.out
	set stat [catch {exec $util_path/db_printlog -h $testdir -P $passwd \
	    > $tmpfile} ret]
	error_check_good db_printlog $stat 0
	fileremove $tmpfile

	# List of recovery tests: {CMD MSG} pairs.
	set rlist {
	{ {DB put -txn TXNID $key $data}	"Recd017.b: put"}
	{ {DB del -txn TXNID $key}		"Recd017.c: delete"}
	}

	# These are all the data values that we're going to need to read
	# through the operation table and run the recovery tests.

	if { [is_record_based $method] == 1 } {
		set key 1
	} else {
		set key recd017_key
	}
	set data recd017_data
	foreach pair $rlist {
		set cmd [subst [lindex $pair 0]]
		set msg [lindex $pair 1]
		if { $select != 0 } {
			set tag [lindex $msg 0]
			set tail [expr [string length $tag] - 2]
			set tag [string range $tag $tail $tail]
			if { [lsearch $select $tag] == -1 } {
				continue
			}
		}

		if { [is_queue $method] != 1 } {
			if { [string first append $cmd] != -1 } {
				continue
			}
			if { [string first consume $cmd] != -1 } {
				continue
			}
		}

#		if { [is_fixed_length $method] == 1 } {
#			if { [string first partial $cmd] != -1 } {
#				continue
#			}
#		}
		op_recover abort $testdir $env_cmd $testfile $cmd $msg $args
		op_recover commit $testdir $env_cmd $testfile $cmd $msg $args
		#
		# Note that since prepare-discard ultimately aborts
		# the txn, it must come before prepare-commit.
		#
		op_recover prepare-abort $testdir $env_cmd $testfile2 \
		    $cmd $msg $args
		op_recover prepare-discard $testdir $env_cmd $testfile2 \
		    $cmd $msg $args
		op_recover prepare-commit $testdir $env_cmd $testfile2 \
		    $cmd $msg $args
	}
	set fixed_len $orig_fixed_len
	return
}

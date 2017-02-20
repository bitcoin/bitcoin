# See the file LICENSE for redistribution information.
#
# Copyright (c) 1996-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	txn003
# TEST	Test abort/commit/prepare of txns with outstanding child txns.
proc txn003 { {tnum "003"} } {
	source ./include.tcl
	global txn_curid
	global txn_maxid

	puts -nonewline "Txn$tnum: Outstanding child transaction test"

	if { $tnum != "003" } {
		puts " (with ID wrap)"
	} else {
		puts ""
	}
	env_cleanup $testdir
	set testfile txn003.db

	set env_cmd "berkdb_env_noerr -create -txn -home $testdir"
	set env [eval $env_cmd]
	error_check_good dbenv [is_valid_env $env] TRUE
	error_check_good txn_id_set \
	     [$env txn_id_set $txn_curid $txn_maxid] 0

	set oflags {-auto_commit -create -btree -mode 0644 -env $env $testfile}
	set db [eval {berkdb_open} $oflags]
	error_check_good db_open [is_valid_db $db] TRUE

	#
	# Put some data so that we can check commit or abort of child
	#
	set key 1
	set origdata some_data
	set newdata this_is_new_data
	set newdata2 some_other_new_data

	error_check_good db_put [$db put $key $origdata] 0
	error_check_good dbclose [$db close] 0

	set db [eval {berkdb_open} $oflags]
	error_check_good db_open [is_valid_db $db] TRUE

	txn003_check $db $key "Origdata" $origdata

	puts "\tTxn$tnum.a: Parent abort"
	set parent [$env txn]
	error_check_good txn_begin [is_valid_txn $parent $env] TRUE
	set child [$env txn -parent $parent]
	error_check_good txn_begin [is_valid_txn $child $env] TRUE
	error_check_good db_put [$db put -txn $child $key $newdata] 0
	error_check_good parent_abort [$parent abort] 0
	txn003_check $db $key "parent_abort" $origdata
	# Check child handle is invalid
	set stat [catch {$child abort} ret]
	error_check_good child_handle $stat 1
	error_check_good child_h2 [is_substr $ret "invalid command name"] 1

	puts "\tTxn$tnum.b: Parent commit"
	set parent [$env txn]
	error_check_good txn_begin [is_valid_txn $parent $env] TRUE
	set child [$env txn -parent $parent]
	error_check_good txn_begin [is_valid_txn $child $env] TRUE
	error_check_good db_put [$db put -txn $child $key $newdata] 0
	error_check_good parent_commit [$parent commit] 0
	txn003_check $db $key "parent_commit" $newdata
	# Check child handle is invalid
	set stat [catch {$child abort} ret]
	error_check_good child_handle $stat 1
	error_check_good child_h2 [is_substr $ret "invalid command name"] 1
	error_check_good dbclose [$db close] 0
	error_check_good env_close [$env close] 0

	#
	# Since the data check assumes what has come before, the 'commit'
	# operation must be last.
	#
	set hdr "\tTxn$tnum"
	set rlist {
		{begin		".c"}
		{prepare	".d"}
		{abort		".e"}
		{commit		".f"}
	}
	set count 0
	foreach pair $rlist {
		incr count
		set op [lindex $pair 0]
		set msg [lindex $pair 1]
		set msg $hdr$msg
		txn003_body $env_cmd $testfile $testdir $key $newdata2 $msg $op
		set env [eval $env_cmd]
		error_check_good dbenv [is_valid_env $env] TRUE

		berkdb debug_check
		set db [eval {berkdb_open} $oflags]
		error_check_good db_open [is_valid_db $db] TRUE
		#
		# For prepare we'll then just
		# end up aborting after we test what we need to.
		# So set gooddata to the same as abort.
		switch $op {
			abort {
				set gooddata $newdata
			}
			begin {
				set gooddata $newdata
			}
			commit {
				set gooddata $newdata2
			}
			prepare {
				set gooddata $newdata
			}
		}
		txn003_check $db $key "parent_$op" $gooddata
		error_check_good dbclose [$db close] 0
		error_check_good env_close [$env close] 0
	}

	puts "\tTxn$tnum.g: Attempt child prepare"
	set env [eval $env_cmd]
	error_check_good dbenv [is_valid_env $env] TRUE
	berkdb debug_check
	set db [eval {berkdb_open_noerr} $oflags]
	error_check_good db_open [is_valid_db $db] TRUE

	set parent [$env txn]
	error_check_good txn_begin [is_valid_txn $parent $env] TRUE
	set child [$env txn -parent $parent]
	error_check_good txn_begin [is_valid_txn $child $env] TRUE
	error_check_good db_put [$db put -txn $child $key $newdata] 0
	set gid [make_gid child_prepare:$child]
	set stat [catch {$child prepare $gid} ret]
	error_check_good child_prepare $stat 1
	error_check_good child_prep_err [is_substr $ret "txn prepare"] 1

	puts "\tTxn$tnum.h: Attempt child discard"
	set stat [catch {$child discard} ret]
	error_check_good child_discard $stat 1

	# We just panic'd the region, so the next operations will fail.
	# No matter, we still have to clean up all the handles.

	set stat [catch {$parent commit} ret]
	error_check_good parent_commit $stat 1
	error_check_good parent_commit:fail [is_substr $ret "DB_RUNRECOVERY"] 1

	set stat [catch {$db close} ret]
	error_check_good db_close $stat 1
	error_check_good db_close:fail [is_substr $ret "DB_RUNRECOVERY"] 1

	set stat [catch {$env close} ret]
	error_check_good env_close $stat 1
	error_check_good env_close:fail [is_substr $ret "DB_RUNRECOVERY"] 1
}

proc txn003_body { env_cmd testfile dir key newdata2 msg op } {
	source ./include.tcl

	berkdb debug_check
	sentinel_init
	set gidf $dir/gidfile
	fileremove -f $gidf
	set pidlist {}
	puts "$msg.0: Executing child script to prepare txns"
	berkdb debug_check
	set p [exec $tclsh_path $test_path/wrap.tcl txnscript.tcl \
	    $testdir/txnout $env_cmd $testfile $gidf $key $newdata2 &]
	lappend pidlist $p
	watch_procs $pidlist 5
	set f1 [open $testdir/txnout r]
	set r [read $f1]
	puts $r
	close $f1
	fileremove -f $testdir/txnout

	berkdb debug_check
	puts -nonewline "$msg.1: Running recovery ... "
	flush stdout
	berkdb debug_check
	set env [eval $env_cmd "-recover"]
	error_check_good dbenv-recover [is_valid_env $env] TRUE
	puts "complete"

	puts "$msg.2: getting txns from txn_recover"
	set txnlist [$env txn_recover]
	error_check_good txnlist_len [llength $txnlist] 1
	set tpair [lindex $txnlist 0]

	set gfd [open $gidf r]
	set ret [gets $gfd parentgid]
	close $gfd
	set txn [lindex $tpair 0]
	set gid [lindex $tpair 1]
	if { $op == "begin" } {
		puts "$msg.2: $op new txn"
	} else {
		puts "$msg.2: $op parent"
	}
	error_check_good gidcompare $gid $parentgid
	if { $op == "prepare" } {
		set gid [make_gid prepare_recover:$txn]
		set stat [catch {$txn $op $gid} ret]
		error_check_good prep_error $stat 1
		error_check_good prep_err \
		    [is_substr $ret "transaction already prepared"] 1
		error_check_good txn:prep_abort [$txn abort] 0
	} elseif { $op == "begin" } {
		# As of the 4.6 release, we allow new txns to be created
		# while prepared but not committed txns exist, so this
		# should succeed.
		set txn2 [$env txn]
		error_check_good txn:begin_abort [$txn abort] 0
		error_check_good txn2:begin_abort [$txn2 abort] 0
	} else {
		error_check_good txn:$op [$txn $op] 0
	}
	error_check_good envclose [$env close] 0
}

proc txn003_check { db key msg gooddata } {
	set kd [$db get $key]
	set data [lindex [lindex $kd 0] 1]
	error_check_good $msg $data $gooddata
}

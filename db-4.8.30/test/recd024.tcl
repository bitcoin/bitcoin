# See the file LICENSE for redistribution information.
#
# Copyright (c) 1996,2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	recd024
# TEST	Test recovery of streaming partial insert operations. These are 
# TEST	operations that do multiple partial puts that append to an existing
# TEST	data item (as long as the data item is on an overflow page).
# TEST	The interesting cases are:
# TEST	 * Simple streaming operations
# TEST	 * Operations that cause the overflow item to flow onto another page.
# TEST
proc recd024 { method args } {
	source ./include.tcl

	# puts "$args"
	set envargs ""
	set pagesize 512

	set args [convert_args $method $args]
	set omethod [convert_method $method]

	if { [is_record_based $method] == 1 } {
		puts "Recd024 skipping for fixed length access methods."
		return
	}
	set flags "-create -txn -home $testdir $envargs"
	set env_cmd "berkdb_env $flags"

	set testfile recd024.db
	set testfile2 recd024-2.db
	if { [is_record_based $method] == 1 } {
		set key 1
	} else {
		set key recd024_key
	}

	set len 512
	set part_data [replicate "abcdefgh" 64]
	set p [list 0 $len]
	# Insert one 512 byte data item prior to call. To get it off page.
	# Append two more 512 byte data items, to enact the streaming code.
	set cmd [subst \
	    {DBC put -txn TXNID -partial "512 512" -current $part_data \
	    NEW_CMD DBC put -txn TXNID -partial "1024 512" -current $part_data \
	    NEW_CMD DBC put -txn TXNID -partial "1536 512" -current $part_data}]
	set oflags "-create $omethod -mode 0644 $args \
	    -pagesize $pagesize"
	set msg "Recd024.a: partial put prepopulated/expanding"
	foreach op {commit abort prepare-abort prepare-discard prepare-commit} {
		env_cleanup $testdir

		set dbenv [eval $env_cmd]
		error_check_good dbenv [is_valid_env $dbenv] TRUE
		set t [$dbenv txn]
		error_check_good txn_begin [is_valid_txn $t $dbenv] TRUE
		set db [eval {berkdb_open} \
		    $oflags -env $dbenv -txn $t $testfile]
		error_check_good db_open [is_valid_db $db] TRUE
		set db2 [eval {berkdb_open} \
		    $oflags -env $dbenv -txn $t $testfile2]
		error_check_good db_open [is_valid_db $db2] TRUE

		set ret [$db put -txn $t -partial $p $key $part_data]
		error_check_good dbput $ret 0

		set ret [$db2 put -txn $t -partial $p $key $part_data]
		error_check_good dbput $ret 0
		error_check_good txncommit [$t commit] 0
		error_check_good dbclose [$db close] 0
		error_check_good dbclose [$db2 close] 0
		error_check_good dbenvclose [$dbenv close] 0

		op_recover $op $testdir $env_cmd $testfile $cmd $msg \
		    $args
	}
	return
}


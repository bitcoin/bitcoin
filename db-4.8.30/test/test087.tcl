# See the file LICENSE for redistribution information.
#
# Copyright (c) 1999-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	test087
# TEST	Test of cursor stability when converting to and modifying
# TEST	off-page duplicate pages with subtransaction aborts. [#2373]
# TEST
# TEST	Does the following:
# TEST	a. Initialize things by DB->putting ndups dups and
# TEST	   setting a reference cursor to point to each.  Do each put twice,
# TEST	   first aborting, then committing, so we're sure to abort the move
# TEST	   to off-page dups at some point.
# TEST	b. c_put ndups dups (and correspondingly expanding
# TEST	   the set of reference cursors) after the last one, making sure
# TEST	   after each step that all the reference cursors still point to
# TEST	   the right item.
# TEST	c. Ditto, but before the first one.
# TEST	d. Ditto, but after each one in sequence first to last.
# TEST	e. Ditto, but after each one in sequence from last to first.
# TEST	   occur relative to the new datum)
# TEST	f. Ditto for the two sequence tests, only doing a
# TEST	   DBC->c_put(DB_CURRENT) of a larger datum instead of adding a
# TEST	   new one.
proc test087 { method {pagesize 512} {ndups 50} {tnum "087"} args } {
	source ./include.tcl
	global alphabet

	set args [convert_args $method $args]
	set encargs ""
	set args [split_encargs $args encargs]
	set omethod [convert_method $method]

	puts "Test$tnum $omethod ($args): "
	set eindex [lsearch -exact $args "-env"]
	#
	# If we are using an env, then return
	if { $eindex != -1 } {
		puts "Environment specified;  skipping."
		return
	}
	set pgindex [lsearch -exact $args "-pagesize"]
	if { $pgindex != -1 } {
		puts "Test087: skipping for specific pagesizes"
		return
	}
	env_cleanup $testdir
	set testfile test$tnum.db
	set key "the key"
	append args " -pagesize $pagesize -dup"

	if { [is_record_based $method] || [is_rbtree $method] } {
		puts "Skipping for method $method."
		return
	} elseif { [is_compressed $args] == 1 } {
		puts "Test$tnum skipping for btree with compression."
		return
	} else {
		puts "Test$tnum: Cursor stability on dup. pages w/ aborts."
	}

	set env [eval {berkdb_env \
	     -create -home $testdir -txn -pagesize $pagesize} $encargs]
	error_check_good env_create [is_valid_env $env] TRUE

	set db [eval {berkdb_open -auto_commit \
	     -create -env $env -mode 0644} $omethod $args $testfile]
	error_check_good "db open" [is_valid_db $db] TRUE

	# Number of outstanding keys.
	set keys $ndups

	puts "\tTest$tnum.a: put/abort/put/commit loop;\
	    $ndups dups, short data."
	set txn [$env txn]
	error_check_good txn [is_valid_txn $txn $env] TRUE
	for { set i 0 } { $i < $ndups } { incr i } {
		set datum [makedatum_t73 $i 0]

		set ctxn [$env txn -parent $txn]
		error_check_good ctxn(abort,$i) [is_valid_txn $ctxn $env] TRUE
		error_check_good "db put/abort ($i)" \
		    [$db put -txn $ctxn $key $datum] 0
		error_check_good ctxn_abort($i) [$ctxn abort] 0

		verify_t73 is_long dbc [expr $i - 1] $key

		set ctxn [$env txn -parent $txn]
		error_check_good ctxn(commit,$i) [is_valid_txn $ctxn $env] TRUE
		error_check_good "db put/commit ($i)" \
		    [$db put -txn $ctxn $key $datum] 0
		error_check_good ctxn_commit($i) [$ctxn commit] 0

		set is_long($i) 0

		set dbc($i) [$db cursor -txn $txn]
		error_check_good "db cursor ($i)"\
		    [is_valid_cursor $dbc($i) $db] TRUE
		error_check_good "dbc get -get_both ($i)"\
		    [$dbc($i) get -get_both $key $datum]\
		    [list [list $key $datum]]

		verify_t73 is_long dbc $i $key
	}

	puts "\tTest$tnum.b: Cursor put (DB_KEYLAST); $ndups new dups,\
	    short data."

	set ctxn [$env txn -parent $txn]
	error_check_good ctxn($i) [is_valid_txn $ctxn $env] TRUE
	for { set i 0 } { $i < $ndups } { incr i } {
		# !!! keys contains the number of the next dup
		# to be added (since they start from zero)
		set datum [makedatum_t73 $keys 0]
		set curs [$db cursor -txn $ctxn]
		error_check_good "db cursor create" [is_valid_cursor $curs $db]\
		    TRUE
		error_check_good "c_put(DB_KEYLAST, $keys)"\
		    [$curs put -keylast $key $datum] 0

		# We can't do a verification while a child txn is active,
		# or we'll run into trouble when DEBUG_ROP is enabled.
		# If this test has trouble, though, uncommenting this
		# might be illuminating--it makes things a bit more rigorous
		# and works fine when DEBUG_ROP is not enabled.
		# verify_t73 is_long dbc $keys $key
		error_check_good curs_close [$curs close] 0
	}
	error_check_good ctxn_abort [$ctxn abort] 0
	verify_t73 is_long dbc $keys $key

	puts "\tTest$tnum.c: Cursor put (DB_KEYFIRST); $ndups new dups,\
	    short data."

	set ctxn [$env txn -parent $txn]
	error_check_good ctxn($i) [is_valid_txn $ctxn $env] TRUE
	for { set i 0 } { $i < $ndups } { incr i } {
		# !!! keys contains the number of the next dup
		# to be added (since they start from zero)

		set datum [makedatum_t73 $keys 0]
		set curs [$db cursor -txn $ctxn]
		error_check_good "db cursor create" [is_valid_cursor $curs $db]\
		    TRUE
		error_check_good "c_put(DB_KEYFIRST, $keys)"\
		    [$curs put -keyfirst $key $datum] 0

		# verify_t73 is_long dbc $keys $key
		error_check_good curs_close [$curs close] 0
	}
	# verify_t73 is_long dbc $keys $key
	# verify_t73 is_long dbc $keys $key
	error_check_good ctxn_abort [$ctxn abort] 0
	verify_t73 is_long dbc $keys $key

	puts "\tTest$tnum.d: Cursor put (DB_AFTER) first to last;\
	    $keys new dups, short data"
	# We want to add a datum after each key from 0 to the current
	# value of $keys, which we thus need to save.
	set ctxn [$env txn -parent $txn]
	error_check_good ctxn($i) [is_valid_txn $ctxn $env] TRUE
	set keysnow $keys
	for { set i 0 } { $i < $keysnow } { incr i } {
		set datum [makedatum_t73 $keys 0]
		set curs [$db cursor -txn $ctxn]
		error_check_good "db cursor create" [is_valid_cursor $curs $db]\
		    TRUE

		# Which datum to insert this guy after.
		set curdatum [makedatum_t73 $i 0]
		error_check_good "c_get(DB_GET_BOTH, $i)"\
		    [$curs get -get_both $key $curdatum]\
		    [list [list $key $curdatum]]
		error_check_good "c_put(DB_AFTER, $i)"\
		    [$curs put -after $datum] 0

		# verify_t73 is_long dbc $keys $key
		error_check_good curs_close [$curs close] 0
	}
	error_check_good ctxn_abort [$ctxn abort] 0
	verify_t73 is_long dbc $keys $key

	puts "\tTest$tnum.e: Cursor put (DB_BEFORE) last to first;\
	    $keys new dups, short data"
	set ctxn [$env txn -parent $txn]
	error_check_good ctxn($i) [is_valid_txn $ctxn $env] TRUE
	for { set i [expr $keys - 1] } { $i >= 0 } { incr i -1 } {
		set datum [makedatum_t73 $keys 0]
		set curs [$db cursor -txn $ctxn]
		error_check_good "db cursor create" [is_valid_cursor $curs $db]\
		    TRUE

		# Which datum to insert this guy before.
		set curdatum [makedatum_t73 $i 0]
		error_check_good "c_get(DB_GET_BOTH, $i)"\
		    [$curs get -get_both $key $curdatum]\
		    [list [list $key $curdatum]]
		error_check_good "c_put(DB_BEFORE, $i)"\
		    [$curs put -before $datum] 0

		# verify_t73 is_long dbc $keys $key
		error_check_good curs_close [$curs close] 0
	}
	error_check_good ctxn_abort [$ctxn abort] 0
	verify_t73 is_long dbc $keys $key

	puts "\tTest$tnum.f: Cursor put (DB_CURRENT), first to last,\
	    growing $keys data."
	set ctxn [$env txn -parent $txn]
	error_check_good ctxn($i) [is_valid_txn $ctxn $env] TRUE
	for { set i 0 } { $i < $keysnow } { incr i } {
		set olddatum [makedatum_t73 $i 0]
		set newdatum [makedatum_t73 $i 1]
		set curs [$db cursor -txn $ctxn]
		error_check_good "db cursor create" [is_valid_cursor $curs $db]\
		    TRUE

		error_check_good "c_get(DB_GET_BOTH, $i)"\
		    [$curs get -get_both $key $olddatum]\
		    [list [list $key $olddatum]]
		error_check_good "c_put(DB_CURRENT, $i)"\
		    [$curs put -current $newdatum] 0

		set is_long($i) 1

		# verify_t73 is_long dbc $keys $key
		error_check_good curs_close [$curs close] 0
	}
	error_check_good ctxn_abort [$ctxn abort] 0
	for { set i 0 } { $i < $keysnow } { incr i } {
		set is_long($i) 0
	}
	verify_t73 is_long dbc $keys $key

	# Now delete the first item, abort the deletion, and make sure
	# we're still sane.
	puts "\tTest$tnum.g: Cursor delete first item, then abort delete."
	set ctxn [$env txn -parent $txn]
	error_check_good ctxn($i) [is_valid_txn $ctxn $env] TRUE
	set curs [$db cursor -txn $ctxn]
	error_check_good "db cursor create" [is_valid_cursor $curs $db] TRUE
	set datum [makedatum_t73 0 0]
	error_check_good "c_get(DB_GET_BOTH, 0)"\
	    [$curs get -get_both $key $datum] [list [list $key $datum]]
	error_check_good "c_del(0)" [$curs del] 0
	error_check_good curs_close [$curs close] 0
	error_check_good ctxn_abort [$ctxn abort] 0
	verify_t73 is_long dbc $keys $key

	# Ditto, for the last item.
	puts "\tTest$tnum.h: Cursor delete last item, then abort delete."
	set ctxn [$env txn -parent $txn]
	error_check_good ctxn($i) [is_valid_txn $ctxn $env] TRUE
	set curs [$db cursor -txn $ctxn]
	error_check_good "db cursor create" [is_valid_cursor $curs $db] TRUE
	set datum [makedatum_t73 [expr $keys - 1] 0]
	error_check_good "c_get(DB_GET_BOTH, [expr $keys - 1])"\
	    [$curs get -get_both $key $datum] [list [list $key $datum]]
	error_check_good "c_del(0)" [$curs del] 0
	error_check_good curs_close [$curs close] 0
	error_check_good ctxn_abort [$ctxn abort] 0
	verify_t73 is_long dbc $keys $key

	# Ditto, for all the items.
	puts "\tTest$tnum.i: Cursor delete all items, then abort delete."
	set ctxn [$env txn -parent $txn]
	error_check_good ctxn($i) [is_valid_txn $ctxn $env] TRUE
	set curs [$db cursor -txn $ctxn]
	error_check_good "db cursor create" [is_valid_cursor $curs $db] TRUE
	set datum [makedatum_t73 0 0]
	error_check_good "c_get(DB_GET_BOTH, 0)"\
	    [$curs get -get_both $key $datum] [list [list $key $datum]]
	error_check_good "c_del(0)" [$curs del] 0
	for { set i 1 } { $i < $keys } { incr i } {
		error_check_good "c_get(DB_NEXT, $i)"\
		    [$curs get -next] [list [list $key [makedatum_t73 $i 0]]]
		error_check_good "c_del($i)" [$curs del] 0
	}
	error_check_good curs_close [$curs close] 0
	error_check_good ctxn_abort [$ctxn abort] 0
	verify_t73 is_long dbc $keys $key

	# Close cursors.
	puts "\tTest$tnum.j: Closing cursors."
	for { set i 0 } { $i < $keys } { incr i } {
		error_check_good "dbc close ($i)" [$dbc($i) close] 0
	}
	error_check_good txn_commit [$txn commit] 0
	error_check_good "db close" [$db close] 0
	error_check_good "env close" [$env close] 0
}

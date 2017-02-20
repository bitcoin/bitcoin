# See the file LICENSE for redistribution information.
#
# Copyright (c) 2000-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	test079
# TEST	Test of deletes in large trees.  (test006 w/ sm. pagesize).
# TEST
# TEST	Check that delete operations work in large btrees.  10000 entries
# TEST	and a pagesize of 512 push this out to a four-level btree, with a
# TEST	small fraction of the entries going on overflow pages.
proc test079 { method {nentries 10000} {pagesize 512} {tnum "079"} \
    {ndups 20} args} {
	if { [ is_queueext $method ] == 1 } {
		set method  "queue";
		lappend args "-extent" "20"
	}

        set pgindex [lsearch -exact $args "-pagesize"]
        if { $pgindex != -1 } {
                puts "Test$tnum: skipping for specific pagesizes"
                return
        }

	eval {test006 $method $nentries 1 $tnum $ndups -pagesize \
	    $pagesize} $args
}

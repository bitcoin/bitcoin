# See the file LICENSE for redistribution information.
#
# Copyright (c) 1996-2009 Oracle.  All rights reserved.
#
# $Id$
#

# TEST	memp001
# TEST	Randomly updates pages.
proc memp001 { } {
	source ./include.tcl
	memp001_body 1 ""
	memp001_body 3 ""
	memp001_body 1 -private
	memp001_body 3 -private
	if { $is_qnx_test } {
		puts "Skipping remainder of memp001 for\
		    environments in system memory on QNX"
		return
	}
	memp001_body 1 "-system_mem -shm_key 1"
	memp001_body 3 "-system_mem -shm_key 1"
}

proc memp001_body { ncache flags } {
	source ./include.tcl
	global rand_init

	set nfiles 5
	set iter 500
	set psize 512
	set cachearg "-cachesize {0 400000 $ncache}"

	puts \
"Memp001: { $flags } random update $iter iterations on $nfiles files."
	#
	# Check if this platform supports this set of flags
	#
	if { [mem_chk $flags] == 1 } {
		return
	}

	env_cleanup $testdir
	puts "\tMemp001.a: Create env with $ncache caches"
	set env [eval {berkdb_env -create -mode 0644} \
	    $cachearg {-home $testdir} $flags]
	error_check_good env_open [is_valid_env $env] TRUE

	#
	# Do a simple mpool_stat call to verify the number of caches
	# just to exercise the stat code.
	set stat [$env mpool_stat]
	set str "Number of caches"
	set checked 0
	foreach statpair $stat {
		if { $checked == 1 } {
			break
		}
		if { [is_substr [lindex $statpair 0] $str] != 0} {
			set checked 1
			error_check_good ncache [lindex $statpair 1] $ncache
		}
	}
	error_check_good checked $checked 1

	# Open N memp files
	puts "\tMemp001.b: Create $nfiles mpool files"
	for {set i 1} {$i <= $nfiles} {incr i} {
		set fname "data_file.$i"
		file_create $testdir/$fname 50 $psize

		set mpools($i) \
		    [$env mpool -create -pagesize $psize -mode 0644 $fname]
		error_check_good mp_open [is_substr $mpools($i) $env.mp] 1
	}

	# Now, loop, picking files at random
	berkdb srand $rand_init
	puts "\tMemp001.c: Random page replacement loop"
	for {set i 0} {$i < $iter} {incr i} {
		set mpool $mpools([berkdb random_int 1 $nfiles])
		set p(1) [get_range $mpool 10]
		set p(2) [get_range $mpool 10]
		set p(3) [get_range $mpool 10]
		set p(1) [replace $mpool $p(1)]
		set p(3) [replace $mpool $p(3)]
		set p(4) [get_range $mpool 20]
		set p(4) [replace $mpool $p(4)]
		set p(5) [get_range $mpool 10]
		set p(6) [get_range $mpool 20]
		set p(7) [get_range $mpool 10]
		set p(8) [get_range $mpool 20]
		set p(5) [replace $mpool $p(5)]
		set p(6) [replace $mpool $p(6)]
		set p(9) [get_range $mpool 40]
		set p(9) [replace $mpool $p(9)]
		set p(10) [get_range $mpool 40]
		set p(7) [replace $mpool $p(7)]
		set p(8) [replace $mpool $p(8)]
		set p(9) [replace $mpool $p(9) ]
		set p(10) [replace $mpool $p(10)]
		#
		# We now need to put all the pages we have here or
		# else they end up pinned.
		#
		for {set x 1} { $x <= 10} {incr x} {
			error_check_good pgput [$p($x) put] 0
		}
	}

	# Close N memp files, close the environment.
	puts "\tMemp001.d: Close mpools"
	for {set i 1} {$i <= $nfiles} {incr i} {
		error_check_good memp_close:$mpools($i) [$mpools($i) close] 0
	}
	error_check_good envclose [$env close] 0

	for {set i 1} {$i <= $nfiles} {incr i} {
		fileremove -f $testdir/data_file.$i
	}
}

proc file_create { fname nblocks blocksize } {
	set fid [open $fname w]
	for {set i 0} {$i < $nblocks} {incr i} {
		seek $fid [expr $i * $blocksize] start
		puts -nonewline $fid $i
	}
	seek $fid [expr $nblocks * $blocksize - 1]

	# We don't end the file with a newline, because some platforms (like
	# Windows) emit CR/NL.  There does not appear to be a BINARY open flag
	# that prevents this.
	puts -nonewline $fid "Z"
	close $fid

	# Make sure it worked
	if { [file size $fname] != $nblocks * $blocksize } {
		error "FAIL: file_create could not create correct file size"
	}
}

proc get_range { mpool max } {
	set pno [berkdb random_int 0 $max]
	set p [eval $mpool get $pno]
	error_check_good page [is_valid_page $p $mpool] TRUE
	set got [$p pgnum]
	if { $got != $pno } {
		puts "Get_range: Page mismatch page |$pno| val |$got|"
	}
	set ret [$p init "Page is pinned by [pid]"]
	error_check_good page_init $ret 0

	return $p
}

proc replace { mpool p { args "" } } {
	set pgno [$p pgnum]

	set ret [$p init "Page is unpinned by [pid]"]
	error_check_good page_init $ret 0

	set ret [$p put]
	error_check_good page_put $ret 0

	set p2 [eval $mpool get $args $pgno]
	error_check_good page [is_valid_page $p2 $mpool] TRUE

	return $p2
}

proc mem_chk { flags } {
	source ./include.tcl
	global errorCode

	# Open the memp with region init specified
	env_cleanup $testdir

	set cachearg " -cachesize {0 400000 3}"
	set ret [catch {eval {berkdb_env_noerr -create -mode 0644}\
	    $cachearg {-region_init -home $testdir} $flags} env]
	if { $ret != 0 } {
		# If the env open failed, it may be because we're on a platform
		# such as HP-UX 10 that won't support mutexes in shmget memory.
		# Or QNX, which doesn't support system memory at all.
		# Verify that the return value was EINVAL or EOPNOTSUPP
		# and bail gracefully.
		error_check_good is_shm_test [is_substr $flags -system_mem] 1
		error_check_good returned_error [expr \
		    [is_substr $errorCode EINVAL] || \
		    [is_substr $errorCode EOPNOTSUPP]] 1
		puts "Warning:\
		     platform does not support mutexes in shmget memory."
		puts "Skipping shared memory mpool test."
		return 1
	}
	error_check_good env_open [is_valid_env $env] TRUE
	error_check_good env_close [$env close] 0
	env_cleanup $testdir

	return 0
}

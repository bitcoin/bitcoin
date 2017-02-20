# See the file LICENSE for redistribution information.
#
# Copyright (c) 1999-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	env003
# TEST	Test DB_TMP_DIR and env name resolution
# TEST	With an environment path specified using -home, and then again
# TEST	with it specified by the environment variable DB_HOME:
# TEST	1) Make sure that the DB_TMP_DIR config file option is respected
# TEST		a) as a relative pathname.
# TEST		b) as an absolute pathname.
# TEST	2) Make sure that the -tmp_dir config option is respected,
# TEST		again as relative and absolute pathnames.
# TEST	3) Make sure that if -both- -tmp_dir and a file are present,
# TEST		only the file is respected (see doc/env/naming.html).
proc env003 { } {
	#   env003 is essentially just a small driver that runs
	# env003_body twice.  First, it supplies a "home" argument
	# to use with environment opens, and the second time it sets
	# DB_HOME instead.
	#   Note that env003_body itself calls env003_run_test to run
	# the body of the actual test.

	global env
	source ./include.tcl

	puts "Env003: DB_TMP_DIR test."

	puts "\tEnv003: Running with -home argument to berkdb_env."
	env003_body "-home $testdir"

	puts "\tEnv003: Running with environment variable DB_HOME set."
	set env(DB_HOME) $testdir
	env003_body "-use_environ"

	unset env(DB_HOME)

	puts "\tEnv003: Running with both DB_HOME and -home set."
	# Should respect -only- -home, so we give it a bogus
	# environment variable setting.
	set env(DB_HOME) $testdir/bogus_home
	env003_body "-use_environ -home $testdir"
	unset env(DB_HOME)
}

proc env003_body { home_arg } {
	source ./include.tcl

	env_cleanup $testdir
	set tmpdir "tmpfiles_in_here"
	file mkdir $testdir/$tmpdir

	# Set up full path to $tmpdir for when we test absolute paths.
	set curdir [pwd]
	cd $testdir/$tmpdir
	set fulltmpdir [pwd]
	cd $curdir

	# Create DB_CONFIG
	env003_make_config $tmpdir

	# Run the meat of the test.
	env003_run_test a 1 "relative path, config file" $home_arg \
		$testdir/$tmpdir

	env003_make_config $fulltmpdir

	# Run the test again
	env003_run_test a 2 "absolute path, config file" $home_arg \
		$fulltmpdir

	# Now we try without a config file, but instead with db_config
	# relative paths
	env003_run_test b 1 "relative path, db_config" "$home_arg \
		-tmp_dir $tmpdir -data_dir ." \
		$testdir/$tmpdir

	# absolute paths
	env003_run_test b 2 "absolute path, db_config" "$home_arg \
		-tmp_dir $fulltmpdir -data_dir ." \
		$fulltmpdir

	# Now, set db_config -and- have a # DB_CONFIG file, and make
	# sure only the latter is honored.

	file mkdir $testdir/bogus
	env003_make_config $tmpdir

	env003_run_test c 1 "relative path, both db_config and file" \
		"$home_arg -tmp_dir $testdir/bogus -data_dir ." \
		$testdir/$tmpdir

	file mkdir $fulltmpdir/bogus
	env003_make_config $fulltmpdir

	env003_run_test c 2 "absolute path, both db_config and file" \
		"$home_arg -tmp_dir $fulltmpdir/bogus -data_dir ." \
		$fulltmpdir
}

proc env003_run_test { major minor msg env_args tmp_path} {
	global testdir
	global alphabet
	global errorCode

	puts "\t\tEnv003.$major.$minor: $msg"

	# Create an environment and small-cached in-memory database to
	# use.
	set dbenv [eval {berkdb_env -create -home $testdir} $env_args \
	    {-cachesize {0 50000 1}}]
	error_check_good env_open [is_valid_env $dbenv] TRUE

	set db [berkdb_open -env $dbenv -create -btree]
	error_check_good db_open [is_valid_db $db] TRUE

	# Fill the database with more than its cache can fit.
	#
	# When CONFIG_TEST is defined, the tempfile is left linked so
	# we can check for its existence.  Size the data to overfill
	# the cache--the temp file is created lazily, so it is created
	# when the cache overflows.
	#
	set key "key"
	set data [repeat $alphabet 2000]
	error_check_good db_put [$db put $key $data] 0

	# Check for exactly one temp file.
	set ret [glob -nocomplain $tmp_path/BDB*]
	error_check_good temp_file_exists [llength $ret] 1

	# Can't remove temp file until db is closed on Windows.
	error_check_good db_close [$db close] 0
	fileremove -f $ret
	error_check_good env_close [$dbenv close] 0

}

proc env003_make_config { tmpdir } {
	global testdir

	set cid [open $testdir/DB_CONFIG w]
	puts $cid "set_data_dir ."
	puts $cid "set_tmp_dir $tmpdir"
	close $cid
}

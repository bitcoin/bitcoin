# See the file LICENSE for redistribution information.
#
# Copyright (c) 1999-2009 Oracle.  All rights reserved.
#
# $Id$
#
# TEST	env002
# TEST	Test of DB_LOG_DIR and env name resolution.
# TEST	With an environment path specified using -home, and then again
# TEST	with it specified by the environment variable DB_HOME:
# TEST	1) Make sure that the set_lg_dir option is respected
# TEST		a) as a relative pathname.
# TEST		b) as an absolute pathname.
# TEST	2) Make sure that the DB_LOG_DIR db_config argument is respected,
# TEST		again as relative and absolute pathnames.
# TEST	3) Make sure that if -both- db_config and a file are present,
# TEST		only the file is respected (see doc/env/naming.html).
proc env002 { } {
	#   env002 is essentially just a small driver that runs
	# env002_body--formerly the entire test--twice;  once, it
	# supplies a "home" argument to use with environment opens,
	# and the second time it sets DB_HOME instead.
	#   Note that env002_body itself calls env002_run_test to run
	# the body of the actual test and check for the presence
	# of logs.  The nesting, I hope, makes this test's structure simpler.

	global env
	source ./include.tcl

	puts "Env002: set_lg_dir test."

	puts "\tEnv002: Running with -home argument to berkdb_env."
	env002_body "-home $testdir"

	puts "\tEnv002: Running with environment variable DB_HOME set."
	set env(DB_HOME) $testdir
	env002_body "-use_environ"

	unset env(DB_HOME)

	puts "\tEnv002: Running with both DB_HOME and -home set."
	# Should respect -only- -home, so we give it a bogus
	# environment variable setting.
	set env(DB_HOME) $testdir/bogus_home
	env002_body "-use_environ -home $testdir"
	unset env(DB_HOME)

}

proc env002_body { home_arg } {
	source ./include.tcl

	env_cleanup $testdir
	set logdir "logs_in_here"

	file mkdir $testdir/$logdir

	# Set up full path to $logdir for when we test absolute paths.
	set curdir [pwd]
	cd $testdir/$logdir
	set fulllogdir [pwd]
	cd $curdir

	env002_make_config $logdir

	# Run the meat of the test.
	env002_run_test a 1 "relative path, config file" $home_arg \
		$testdir/$logdir

	env_cleanup $testdir

	file mkdir $fulllogdir
	env002_make_config $fulllogdir

	# Run the test again
	env002_run_test a 2 "absolute path, config file" $home_arg \
		$fulllogdir

	env_cleanup $testdir

	# Now we try without a config file, but instead with db_config
	# relative paths
	file mkdir $testdir/$logdir
	env002_run_test b 1 "relative path, db_config" "$home_arg \
		-log_dir $logdir -data_dir ." \
		$testdir/$logdir

	env_cleanup $testdir

	# absolute
	file mkdir $fulllogdir
	env002_run_test b 2 "absolute path, db_config" "$home_arg \
		-log_dir $fulllogdir -data_dir ." \
		$fulllogdir

	env_cleanup $testdir

	# Now, set db_config -and- have a # DB_CONFIG file, and make
	# sure only the latter is honored.

	file mkdir $testdir/$logdir
	env002_make_config $logdir

	# note that we supply a -nonexistent- log dir to db_config
	env002_run_test c 1 "relative path, both db_config and file" \
		"$home_arg -log_dir $testdir/bogus \
		-data_dir ." $testdir/$logdir
	env_cleanup $testdir

	file mkdir $fulllogdir
	env002_make_config $fulllogdir

	# note that we supply a -nonexistent- log dir to db_config
	env002_run_test c 2 "relative path, both db_config and file" \
		"$home_arg -log_dir $fulllogdir/bogus \
		-data_dir ." $fulllogdir
}

proc env002_run_test { major minor msg env_args log_path} {
	global testdir
	set testfile "env002.db"

	puts "\t\tEnv002.$major.$minor: $msg"

	# Create an environment, with logging, and scribble some
	# stuff in a [btree] database in it.
	# puts [concat {berkdb_env -create -log -private} $env_args]
	set dbenv [eval {berkdb_env -create -log -private} $env_args]
	error_check_good env_open [is_valid_env $dbenv] TRUE
	set db [berkdb_open -env $dbenv -create -btree -mode 0644 $testfile]
	error_check_good db_open [is_valid_db $db] TRUE

	set key "some_key"
	set data "some_data"

	error_check_good db_put \
		[$db put $key [chop_data btree $data]] 0

	error_check_good db_close [$db close] 0
	error_check_good env_close [$dbenv close] 0

	# Now make sure the log file is where we want it to be.
	error_check_good db_exists [file exists $testdir/$testfile] 1
	error_check_good log_exists \
		[file exists $log_path/log.0000000001] 1
}

proc env002_make_config { logdir } {
	global testdir

	set cid [open $testdir/DB_CONFIG w]
	puts $cid "set_data_dir ."
	puts $cid "set_lg_dir $logdir"
	close $cid
}

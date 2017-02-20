#!./perl -w

use strict ;


use lib 't' ;

use BerkeleyDB; 
use util ;

use Test::More ;

plan(skip_all => "this needs Berkeley DB 4.7.x or better\n" )
    if $BerkeleyDB::db_version < 4.7;

plan tests => 7;

my $Dfile = "dbhash.tmp";

umask(0);

{
    my $home = "./fred" ;
    ok my $lexD = new LexDir($home) ;
    chdir "./fred" ;
    ok my $env = new BerkeleyDB::Env -Flags => DB_CREATE|DB_INIT_LOG @StdErrFile;

    ok $env->log_get_config( DB_LOG_AUTO_REMOVE, my $on ) == 0, "get config" ;
    ok !$on, "config value" ;

    ok $env->log_set_config( DB_LOG_AUTO_REMOVE, 1 ) == 0;

    ok $env->log_get_config( DB_LOG_AUTO_REMOVE, $on ) == 0;
    ok $on;

    chdir ".." ;
    undef $env ;
}

# test -Verbose
# test -Flags
# db_value_set

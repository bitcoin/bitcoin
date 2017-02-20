#!./perl -w

# Tests for Concurrent Data Store mode

use strict ;
use lib 't' ;

use BerkeleyDB; 
use util ;
use Test::More;



BEGIN {
    plan(skip_all => "this needs BerkeleyDB 2.x or better" )
        if $BerkeleyDB::db_version < 2;

    plan tests => 12;    
}


my $Dfile = "dbhash.tmp";
unlink $Dfile;

umask(0) ;

{
    # Error case -- env not opened in CDS mode

    my $lex = new LexFile $Dfile ;

    my $home = "./fred" ;
    ok my $lexD = new LexDir($home) ;

    ok my $env = new BerkeleyDB::Env -Flags => DB_CREATE|DB_INIT_MPOOL,
    					 -Home => $home, @StdErrFile ;

    ok my $db = new BerkeleyDB::Btree -Filename => $Dfile, 
				    -Env      => $env,
				    -Flags    => DB_CREATE ;

    ok ! $env->cds_enabled() ;
    ok ! $db->cds_enabled() ;

    eval { $db->cds_lock() };
    ok $@ =~ /CDS not enabled for this database/;

    undef $db;
    undef $env ;
}

{
    my $lex = new LexFile $Dfile ;

    my $home = "./fred" ;
    ok my $lexD = new LexDir($home) ;

    ok my $env = new BerkeleyDB::Env -Flags => DB_INIT_CDB|DB_CREATE|DB_INIT_MPOOL,
    					 -Home => $home, @StdErrFile ;

    ok my $db = new BerkeleyDB::Btree -Filename => $Dfile, 
				    -Env      => $env,
				    -Flags    => DB_CREATE ;

    ok $env->cds_enabled() ;
    ok $db->cds_enabled() ;

    my $cds = $db->cds_lock() ;
    ok $cds ;

    undef $db;
    undef $env ;
}

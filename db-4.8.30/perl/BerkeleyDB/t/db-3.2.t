#!./perl -w

# ID: %I%, %G%   

use strict ;

use lib 't' ;
use BerkeleyDB; 
use util ;

use Test::More ;

BEGIN {
    plan(skip_all => "this needs BerkeleyDB 3.2.x or better" )
        if $BerkeleyDB::db_version < 3.2;

    plan tests => 6;    
}

my $Dfile = "dbhash.tmp";
my $Dfile2 = "dbhash2.tmp";
my $Dfile3 = "dbhash3.tmp";
unlink $Dfile;

umask(0) ;



{
    # set_q_extentsize

    ok 1 ;
}

{
    # env->set_flags

    my $home = "./fred" ;
    ok my $lexD = new LexDir($home) ;
    ok my $env = new BerkeleyDB::Env -Home => $home, @StdErrFile,
                                         -Flags => DB_CREATE ,
                                         -SetFlags => DB_NOMMAP ;
 
    undef $env ;                      
}

{
    # env->set_flags

    my $home = "./fred" ;
    ok my $lexD = new LexDir($home) ;
    ok my $env = new BerkeleyDB::Env -Home => $home, @StdErrFile,
                                         -Flags => DB_CREATE ;
    ok ! $env->set_flags(DB_NOMMAP, 1);
 
    undef $env ;                      
}

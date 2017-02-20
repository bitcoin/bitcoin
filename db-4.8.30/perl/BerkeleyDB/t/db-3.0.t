#!./perl -w

# ID: 1.2, 7/17/97

use strict ;

use lib 't';
use BerkeleyDB; 
use util ;

use Test::More ;

BEGIN {
    plan(skip_all => "this needs BerkeleyDB 3.x or better" )
        if $BerkeleyDB::db_version < 3;

    plan tests => 14;    
}

my $Dfile = "dbhash.tmp";

umask(0);

{
    # set_mutexlocks

    my $home = "./fred" ;
    ok my $lexD = new LexDir($home) ;
    chdir "./fred" ;
    ok my $env = new BerkeleyDB::Env -Flags => DB_CREATE, @StdErrFile ;
    ok $env->set_mutexlocks(0) == 0 ;
    chdir ".." ;
    undef $env ;
}

{
    # c_dup


    my $lex = new LexFile $Dfile ;
    my %hash ;
    my ($k, $v) ;
    ok my $db = new BerkeleyDB::Hash -Filename => $Dfile, 
				     -Flags    => DB_CREATE ;

    # create some data
    my @data =  (
		"green"	=> "house",
		"red"	=> 2,
		"blue"	=> "sea",
		) ;

    my $ret = 0 ;
    while (@data)
    {
    	my $k = shift @data ;
	my $v = shift @data ;
        $ret += $db->db_put($k, $v) ;
    }
    ok $ret == 0 ;

    # create a cursor
    ok my $cursor = $db->db_cursor() ;

    # point to a specific k/v pair
    $k = "green" ;
    ok $cursor->c_get($k, $v, DB_SET) == 0 ;
    ok $v eq "house" ;

    # duplicate the cursor
    my $dup_cursor = $cursor->c_dup(DB_POSITION);
    ok $dup_cursor ;

    # move original cursor off green/house
    my $s = $cursor->c_get($k, $v, DB_NEXT) ;
    ok $k ne "green" ;
    ok $v ne "house" ;

    # duplicate cursor should still be on green/house
    ok $dup_cursor->c_get($k, $v, DB_CURRENT) == 0;
    ok $k eq "green" ;
    ok $v eq "house" ;
    
}


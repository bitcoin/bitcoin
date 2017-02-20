#!./perl -w


use strict ;


use lib 't' ;
use BerkeleyDB; 
use util ;

use Test::More ;

BEGIN {
    plan(skip_all => "this needs BerkeleyDB 4.6.x or better" )
        if $BerkeleyDB::db_version < 4.6;

    plan tests => 63;    
}

umask(0);

{
    # db->associate -- secondary keys returning DB_DBT_MULTIPLE

    sub sec_key
    {
        my $pkey = shift ;
        my $pdata = shift ;

        $_[0] = ["a","b", "c"];

        return 0;
    }

    my ($Dfile1, $Dfile2);
    my $lex = new LexFile $Dfile1, $Dfile2 ;
    my %hash ;
    my $status;
    my ($k, $v, $pk) = ('','','');

    # create primary database
    ok my $primary = new BerkeleyDB::Hash -Filename => $Dfile1, 
				     -Flags    => DB_CREATE ;

    # create secondary database
    ok my $secondary = new BerkeleyDB::Hash -Filename => $Dfile2, 
				     -Flags    => DB_CREATE ;

    # associate primary with secondary
    ok $primary->associate($secondary, \&sec_key) ==  0 ;

    # add data to the primary
    ok $primary->db_put("foo", "bar") == 0;

    # check the records in the secondary (there should be three "a", "b", "c")
    is countRecords($secondary), 3 ;

    ok $secondary->db_get("a", $v) == 0;
    is $v, "bar";

    ok $secondary->db_get("b", $v) == 0;
    is $v, "bar";

    ok $secondary->db_get("c", $v) == 0;
    is $v, "bar";
}

{
    # db->associate -- secondary keys returning DB_DBT_MULTIPLE, but with
    # one

    sub sec_key1
    {
        my $pkey = shift ;
        my $pdata = shift ;

        $_[0] = ["a"];

        return 0;
    }

    my ($Dfile1, $Dfile2);
    my $lex = new LexFile $Dfile1, $Dfile2 ;
    my %hash ;
    my $status;
    my ($k, $v, $pk) = ('','','');

    # create primary database
    ok my $primary = new BerkeleyDB::Hash -Filename => $Dfile1, 
				     -Flags    => DB_CREATE ;

    # create secondary database
    ok my $secondary = new BerkeleyDB::Hash -Filename => $Dfile2, 
				     -Flags    => DB_CREATE ;

    # associate primary with secondary
    ok $primary->associate($secondary, \&sec_key1) ==  0 ;

    # add data to the primary
    ok $primary->db_put("foo", "bar") == 0;

    # check the records in the secondary (there should be three "a", "b", "c")
    is countRecords($secondary), 1 ;

    ok $secondary->db_get("a", $v) == 0;
    is $v, "bar";

}

{
    # db->associate -- multiple secondary keys

    sub sec_key_mult
    {
        #print "in sec_key\n";
        my $pkey = shift ;
        my $pdata = shift ;

        $_[0] = [ split ',', $pdata ] ;
        return 0;
    }

    my ($Dfile1, $Dfile2);
    my $lex = new LexFile $Dfile1, $Dfile2 ;
    my %hash ;
    my $status;
    my ($k, $v, $pk) = ('','','');

    # create primary database
    ok my $primary = new BerkeleyDB::Hash -Filename => $Dfile1,
				     -Flags    => DB_CREATE ;

    # create secondary database
    ok my $secondary = new BerkeleyDB::Hash -Filename => $Dfile2,
				     -Flags    => DB_CREATE ;

    # associate primary with secondary
    ok $primary->associate($secondary, \&sec_key_mult) == 0;

    # add data to the primary
    my %data =  (
		"red"	=> "flag",
		"green"	=> "house",
		"blue"	=> "sea",
        "foo"   => "",
        "bar"   => "hello,goodbye",
		) ;

    my $ret = 0 ;
    while (($k, $v) = each %data) {
        my $r = $primary->db_put($k, $v) ;
        $ret += $r;
    }
    ok $ret == 0 ;

    # check the records in the secondary
    is countRecords($secondary), 5 ;

    ok $secondary->db_get("house", $v) == 0;
    ok $v eq "house";

    ok $secondary->db_get("sea", $v) == 0;
    ok $v eq "sea";

    ok $secondary->db_get("flag", $v) == 0;
    ok $v eq "flag";

    ok $secondary->db_get("hello", $v) == 0;
    ok $v eq "hello,goodbye";

    ok $secondary->db_get("goodbye", $v) == 0;
    ok $v eq "hello,goodbye";

    # pget to primary database is illegal
    ok $primary->db_pget('red', $pk, $v) != 0 ;

    # pget to secondary database is ok
    ok $secondary->db_pget('house', $pk, $v) == 0 ;
    ok $pk eq 'green';
    ok $v  eq 'house';

    # pget to secondary database is ok
    ok $secondary->db_pget('hello', $pk, $v) == 0 ;
    ok $pk eq 'bar';
    ok $v  eq 'hello,goodbye';

    ok my $p_cursor = $primary->db_cursor();
    ok my $s_cursor = $secondary->db_cursor();

    # c_get from primary
    $k = 'green';
    ok $p_cursor->c_get($k, $v, DB_SET) == 0;
    ok $k eq 'green';
    ok $v eq 'house';

    # c_get from secondary
    $k = 'sea';
    ok $s_cursor->c_get($k, $v, DB_SET) == 0;
    ok $k eq 'sea';
    ok $v eq 'sea';

    # c_pget from primary database should fail
    $k = 1;
    ok $p_cursor->c_pget($k, $pk, $v, DB_FIRST) != 0;

    # c_pget from secondary database
    $k = 'flag';
    ok $s_cursor->c_pget($k, $pk, $v, DB_SET) == 0;
    ok $k eq 'flag';
    ok $pk eq 'red';
    ok $v eq 'flag';

    # check put to secondary is illegal
    ok $secondary->db_put("tom", "dick") != 0;
    is countRecords($secondary), 5 ;

    # delete from primary
    ok $primary->db_del("green") == 0 ;
    is countRecords($primary), 4 ;

    # check has been deleted in secondary
    ok $secondary->db_get("house", $v) != 0;
    is countRecords($secondary), 4 ;

    # delete from secondary
    ok $secondary->db_del('flag') == 0 ;
    is countRecords($secondary), 3 ;


    # check deleted from primary
    ok $primary->db_get("red", $v) != 0;
    is countRecords($primary), 3 ;
}


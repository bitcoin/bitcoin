#!./perl -w


use strict ;


use lib 't' ;
use BerkeleyDB; 
use util ;
use Test::More;

BEGIN {
    plan(skip_all => "this needs BerkeleyDB 3.3.x or better" )
        if $BerkeleyDB::db_version < 3.3;

    plan tests => 130;    
}

umask(0);

{
    # db->truncate

    my $Dfile;
    my $lex = new LexFile $Dfile ;
    my %hash ;
    my ($k, $v) ;
    ok my $db = new BerkeleyDB::Hash -Filename => $Dfile, 
				     -Flags    => DB_CREATE ;

    # create some data
    my %data =  (
		"red"	=> 2,
		"green"	=> "house",
		"blue"	=> "sea",
		) ;

    my $ret = 0 ;
    while (($k, $v) = each %data) {
        $ret += $db->db_put($k, $v) ;
    }
    ok $ret == 0 ;

    # check there are three records
    is countRecords($db), 3 ;

    # now truncate the database
    my $count = 0;
    ok $db->truncate($count) == 0 ;

    is $count, 3 ;
    ok countRecords($db) == 0 ;

}

{
    # db->associate -- secondary keys

    sub sec_key
    {
        #print "in sec_key\n";
        my $pkey = shift ;
        my $pdata = shift ;

       $_[0] = $pdata ;
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
    ok $primary->associate($secondary, \&sec_key) == 0;

    # add data to the primary
    my %data =  (
		"red"	=> "flag",
		"green"	=> "house",
		"blue"	=> "sea",
		) ;

    my $ret = 0 ;
    while (($k, $v) = each %data) {
        my $r = $primary->db_put($k, $v) ;
	#print "put $r $BerkeleyDB::Error\n";
        $ret += $r;
    }
    ok $ret == 0 ;

    # check the records in the secondary
    is countRecords($secondary), 3 ;

    ok $secondary->db_get("house", $v) == 0;
    is $v, "house";

    ok $secondary->db_get("sea", $v) == 0;
    is $v, "sea";

    ok $secondary->db_get("flag", $v) == 0;
    is $v, "flag";

    # pget to primary database is illegal
    ok $primary->db_pget('red', $pk, $v) != 0 ;

    # pget to secondary database is ok
    ok $secondary->db_pget('house', $pk, $v) == 0 ;
    is $pk, 'green';
    is $v, 'house';

    ok my $p_cursor = $primary->db_cursor();
    ok my $s_cursor = $secondary->db_cursor();

    # c_get from primary 
    $k = 'green';
    ok $p_cursor->c_get($k, $v, DB_SET) == 0;
    is $k, 'green';
    is $v, 'house';

    # c_get from secondary
    $k = 'sea';
    ok $s_cursor->c_get($k, $v, DB_SET) == 0;
    is $k, 'sea';
    is $v, 'sea';

    # c_pget from primary database should fail
    $k = 1;
    ok $p_cursor->c_pget($k, $pk, $v, DB_FIRST) != 0;

    # c_pget from secondary database 
    $k = 'flag';
    ok $s_cursor->c_pget($k, $pk, $v, DB_SET) == 0;
    is $k, 'flag';
    is $pk, 'red';
    is $v, 'flag';

    # check put to secondary is illegal
    ok $secondary->db_put("tom", "dick") != 0;
    is countRecords($secondary), 3 ;

    # delete from primary
    ok $primary->db_del("green") == 0 ;
    is countRecords($primary), 2 ;

    # check has been deleted in secondary
    ok $secondary->db_get("house", $v) != 0;
    is countRecords($secondary), 2 ;

    # delete from secondary
    ok $secondary->db_del('flag') == 0 ;
    is countRecords($secondary), 1 ;


    # check deleted from primary
    ok $primary->db_get("red", $v) != 0;
    is countRecords($primary), 1 ;

}


    # db->associate -- multiple secondary keys


    # db->associate -- same again but when DB_DUP is specified.


{
    # db->associate -- secondary keys, each with a user defined sort

    sub sec_key2
    {
        my $pkey = shift ;
        my $pdata = shift ;
        #print "in sec_key2 [$pkey][$pdata]\n";

        $_[0] = length $pdata ;
        return 0;
    }

    my ($Dfile1, $Dfile2);
    my $lex = new LexFile $Dfile1, $Dfile2 ;
    my %hash ;
    my $status;
    my ($k, $v, $pk) = ('','','');

    # create primary database
    ok my $primary = new BerkeleyDB::Btree -Filename => $Dfile1, 
				     -Compare  => sub { return $_[0] cmp $_[1]},
				     -Flags    => DB_CREATE ;

    # create secondary database
    ok my $secondary = new BerkeleyDB::Btree -Filename => $Dfile2, 
				     -Compare  => sub { return $_[0] <=> $_[1]},
				     -Property => DB_DUP,
				     -Flags    => DB_CREATE ;

    # associate primary with secondary
    ok $primary->associate($secondary, \&sec_key2) == 0;

    # add data to the primary
    my %data =  (
		"red"	=> "flag",
		"orange"=> "custard",
		"green"	=> "house",
		"blue"	=> "sea",
		) ;

    my $ret = 0 ;
    while (($k, $v) = each %data) {
        my $r = $primary->db_put($k, $v) ;
	#print "put [$r] $BerkeleyDB::Error\n";
        $ret += $r;
    }
    ok $ret == 0 ;
    #print "ret $ret\n";

    #print "Primary\n" ; dumpdb($primary) ;
    #print "Secondary\n" ; dumpdb($secondary) ;

    # check the records in the secondary
    is countRecords($secondary), 4 ;

    my $p_data = joinkeys($primary, " ");
    #print "primary [$p_data]\n" ;
    is $p_data, join " ", sort { $a cmp $b } keys %data ;
    my $s_data = joinkeys($secondary, " ");
    #print "secondary [$s_data]\n" ;
    is $s_data, join " ", sort { $a <=> $b } map { length } values %data ;

}

{
    # db->associate -- primary recno, secondary hash

    sub sec_key3
    {
        #print "in sec_key\n";
        my $pkey = shift ;
        my $pdata = shift ;

       $_[0] = $pdata ;
        return 0;
    }

    my ($Dfile1, $Dfile2);
    my $lex = new LexFile $Dfile1, $Dfile2 ;
    my %hash ;
    my $status;
    my ($k, $v, $pk) = ('','','');

    # create primary database
    ok my $primary = new BerkeleyDB::Recno -Filename => $Dfile1, 
				     -Flags    => DB_CREATE ;

    # create secondary database
    ok my $secondary = new BerkeleyDB::Hash -Filename => $Dfile2, 
				     -Flags    => DB_CREATE ;

    # associate primary with secondary
    ok $primary->associate($secondary, \&sec_key3) == 0;

    # add data to the primary
    my %data =  (
		0 => "flag",
		1 => "house",
		2 => "sea",
		) ;

    my $ret = 0 ;
    while (($k, $v) = each %data) {
        my $r = $primary->db_put($k, $v) ;
	#print "put $r $BerkeleyDB::Error\n";
        $ret += $r;
    }
    ok $ret == 0 ;

    # check the records in the secondary
    is countRecords($secondary), 3 ;

    ok $secondary->db_get("flag", $v) == 0;
    is $v, "flag";

    ok $secondary->db_get("house", $v) == 0;
    is $v, "house";

    ok $secondary->db_get("sea", $v) == 0;
    is $v, "sea" ;

    # pget to primary database is illegal
    ok $primary->db_pget(0, $pk, $v) != 0 ;

    # pget to secondary database is ok
    ok $secondary->db_pget('house', $pk, $v) == 0 ;
    is $pk, 1 ;
    is $v, 'house';

    ok my $p_cursor = $primary->db_cursor();
    ok my $s_cursor = $secondary->db_cursor();

    # c_get from primary 
    $k = 1;
    ok $p_cursor->c_get($k, $v, DB_SET) == 0;
    is $k, 1;
    is $v, 'house';

    # c_get from secondary
    $k = 'sea';
    ok $s_cursor->c_get($k, $v, DB_SET) == 0;
    is $k, 'sea' 
        or warn "# key [$k]\n";
    is $v, 'sea';

    # c_pget from primary database should fail
    $k = 1;
    ok $p_cursor->c_pget($k, $pk, $v, DB_FIRST) != 0;

    # c_pget from secondary database 
    $k = 'sea';
    ok $s_cursor->c_pget($k, $pk, $v, DB_SET) == 0;
    is $k, 'sea' ;
    is $pk, 2 ;
    is $v, 'sea';

    # check put to secondary is illegal
    ok $secondary->db_put("tom", "dick") != 0;
    is countRecords($secondary), 3 ;

    # delete from primary
    ok $primary->db_del(2) == 0 ;
    is countRecords($primary), 2 ;

    # check has been deleted in secondary
    ok $secondary->db_get("sea", $v) != 0;
    is countRecords($secondary), 2 ;

    # delete from secondary
    ok $secondary->db_del('flag') == 0 ;
    is countRecords($secondary), 1 ;


    # check deleted from primary
    ok $primary->db_get(0, $v) != 0;
    is countRecords($primary), 1 ;

}

{
    # db->associate -- primary hash, secondary recno

    sub sec_key4
    {
        #print "in sec_key4\n";
        my $pkey = shift ;
        my $pdata = shift ;

       $_[0] = length $pdata ;
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
    ok my $secondary = new BerkeleyDB::Recno -Filename => $Dfile2, 
                     #-Property => DB_DUP,
				     -Flags    => DB_CREATE ;

    # associate primary with secondary
    ok $primary->associate($secondary, \&sec_key4) == 0;

    # add data to the primary
    my %data =  (
		"red"	=> "flag",
		"green"	=> "house",
		"blue"	=> "sea",
		) ;

    my $ret = 0 ;
    while (($k, $v) = each %data) {
        my $r = $primary->db_put($k, $v) ;
	#print "put $r $BerkeleyDB::Error\n";
        $ret += $r;
    }
    ok $ret == 0 ;

    # check the records in the secondary
    is countRecords($secondary), 3 ;

    ok $secondary->db_get(0, $v) != 0;
    ok $secondary->db_get(1, $v) != 0;
    ok $secondary->db_get(2, $v) != 0;
    ok $secondary->db_get(3, $v) == 0;
    ok $v eq "sea";

    ok $secondary->db_get(4, $v) == 0;
    is $v, "flag";

    ok $secondary->db_get(5, $v) == 0;
    is $v, "house";

    # pget to primary database is illegal
    ok $primary->db_pget(0, $pk, $v) != 0 ;

    # pget to secondary database is ok
    ok $secondary->db_pget(4, $pk, $v) == 0 ;
    is $pk, 'red'
        or warn "# $pk\n";;
    is $v, 'flag';

    ok my $p_cursor = $primary->db_cursor();
    ok my $s_cursor = $secondary->db_cursor();

    # c_get from primary 
    $k = 'green';
    ok $p_cursor->c_get($k, $v, DB_SET) == 0;
    is $k, 'green';
    is $v, 'house';

    # c_get from secondary
    $k = 3;
    ok $s_cursor->c_get($k, $v, DB_SET) == 0;
    is $k, 3 ;
    is $v, 'sea';

    # c_pget from primary database should fail
    $k = 1;
    ok $p_cursor->c_pget($k, $pk, $v, DB_SET) != 0;

    # c_pget from secondary database 
    $k = 5;
    ok $s_cursor->c_pget($k, $pk, $v, DB_SET) == 0;
    is $k, 5 ;
    is $pk, 'green';
    is $v, 'house';

    # check put to secondary is illegal
    ok $secondary->db_put(77, "dick") != 0;
    is countRecords($secondary), 3 ;

    # delete from primary
    ok $primary->db_del("green") == 0 ;
    is countRecords($primary), 2 ;

    # check has been deleted in secondary
    ok $secondary->db_get(5, $v) != 0;
    is countRecords($secondary), 2 ;

    # delete from secondary
    ok $secondary->db_del(4) == 0 ;
    is countRecords($secondary), 1 ;


    # check deleted from primary
    ok $primary->db_get("red", $v) != 0;
    is countRecords($primary), 1 ;

}

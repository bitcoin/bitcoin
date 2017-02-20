#!./perl -w

use strict ;


use lib 't' ;

use BerkeleyDB; 
use util ;

use Test::More ;

plan(skip_all => "this needs Berkeley DB 4.8.x or better\n" )
    if $BerkeleyDB::db_version < 4.8;

plan tests => 58;

my $Dfile = "dbhash.tmp";

umask(0);

{
    # db->associate_foreign -- DB_FOREIGN_CASCADE

    sub sec_key
    {
        #print "in sec_key\n";
        my $pkey = shift ;
        my $pdata = shift ;

       $_[0] = $pdata ;
        return 0;
    }

    my ($Dfile1, $Dfile2, $Dfile3);
    my $lex = new LexFile $Dfile1, $Dfile2, $Dfile3 ;
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

    # create secondary database
    ok my $foreign = new BerkeleyDB::Hash -Filename => $Dfile3, 
				     -Flags    => DB_CREATE ;

    # associate primary with secondary
    ok $foreign->associate_foreign($secondary, undef, DB_FOREIGN_CASCADE) == 0;

    # add data to the primary
    my %data =  (
		"red"	=> "flag",
		"green"	=> "house",
		"blue"	=> "sea",
		) ;

    my $ret = 0 ;
    while (($k, $v) = each %data) {
        my $r = $foreign->db_put($v, 1) ;
	#print "put $r $BerkeleyDB::Error\n";
        $ret += $r;
    }
    ok $ret == 0 ;

    while (($k, $v) = each %data) {
        my $r = $primary->db_put($k, $v) ;
	#print "put $r $BerkeleyDB::Error\n";
        $ret += $r;
    }
    ok $ret == 0 ;

    # check the records in the secondary
    is countRecords($primary), 3 ;
    is countRecords($secondary), 3 ;
    is countRecords($foreign), 3 ;

    # deleting from the foreign will cascade
    ok $foreign->db_del("flag") == 0;
    is countRecords($primary), 2 ;
    is countRecords($secondary), 2 ;
    is countRecords($foreign), 2 ;

    cmp_ok  $foreign->db_get("flag", $v), '==', DB_NOTFOUND;
    cmp_ok  $secondary->db_get("flag", $v), '==', DB_NOTFOUND;
    cmp_ok  $primary->db_get("red", $v), '==', DB_NOTFOUND;

    # adding to the primary when no foreign key will fail
    cmp_ok $primary->db_put("hello", "world"), '==', DB_FOREIGN_CONFLICT;

    ok $foreign->db_put("world", "hello") ==  0;

    ok $primary->db_put("hello", "world") == '0';

    is countRecords($primary), 3 ;
    is countRecords($secondary), 3 ;
    is countRecords($foreign), 3 ;
}

{
    # db->associate_foreign -- DB_FOREIGN_ABORT

    sub sec_key2
    {
        #print "in sec_key\n";
        my $pkey = shift ;
        my $pdata = shift ;

       $_[0] = $pdata ;
        return 0;
    }

    my ($Dfile1, $Dfile2, $Dfile3);
    my $lex = new LexFile $Dfile1, $Dfile2, $Dfile3 ;
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
    ok $primary->associate($secondary, \&sec_key2) == 0;

    # create secondary database
    ok my $foreign = new BerkeleyDB::Hash -Filename => $Dfile3, 
				     -Flags    => DB_CREATE ;

    # associate primary with secondary
    ok $foreign->associate_foreign($secondary, undef, DB_FOREIGN_ABORT) == 0;

    # add data to the primary
    my %data =  (
		"red"	=> "flag",
		"green"	=> "house",
		"blue"	=> "sea",
		) ;

    my $ret = 0 ;
    while (($k, $v) = each %data) {
        my $r = $foreign->db_put($v, 1) ;
	#print "put $r $BerkeleyDB::Error\n";
        $ret += $r;
    }
    ok $ret == 0 ;

    while (($k, $v) = each %data) {
        my $r = $primary->db_put($k, $v) ;
	#print "put $r $BerkeleyDB::Error\n";
        $ret += $r;
    }
    ok $ret == 0 ;

    # check the records in the secondary
    is countRecords($primary), 3 ;
    is countRecords($secondary), 3 ;
    is countRecords($foreign), 3 ;

    # deleting from the foreign will fail
    cmp_ok $foreign->db_del("flag"), '==', DB_FOREIGN_CONFLICT;
    is countRecords($primary), 3 ;
    is countRecords($secondary), 3 ;
    is countRecords($foreign), 3 ;

}

{
    # db->associate_foreign -- DB_FOREIGN_NULLIFY

    use constant INVALID => "invalid";

    sub sec_key3
    {
        #print "in sec_key\n";
        my $pkey = shift ;
        my $pdata = shift ;

        if ($pdata eq INVALID)
        {
            #print "BAD\n";
            return DB_DONOTINDEX;
        }

        $_[0] = $pdata ;
        return 0;
    }

    sub nullify_cb
    {
        my $key = \$_[0];
        my $value = \$_[1];
        my $foreignkey = \$_[2];
        my $changed = \$_[3] ;

        #print "key[$$key], value[$$value], foreign[$$foreignkey], changed[$$changed]\n";

        if ($$value eq 'sea')
        {
            #print "SEA\n";
            $$value = INVALID;
            $$changed = 1;
            return 0;
        }

        $$changed = 0;
        return 0;
    }

    my ($Dfile1, $Dfile2, $Dfile3);
    my $lex = new LexFile $Dfile1, $Dfile2, $Dfile3 ;
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
    ok $primary->associate($secondary, \&sec_key3) == 0;

    # create secondary database
    ok my $foreign = new BerkeleyDB::Hash -Filename => $Dfile3, 
				     -Flags    => DB_CREATE ;

    # associate primary with secondary
    cmp_ok $foreign->associate_foreign($secondary, \&nullify_cb, DB_FOREIGN_NULLIFY), '==', 0
        or diag "$BerkeleyDB::Error\n";

    # add data to the primary
    my %data =  (
		"red"	=> "flag",
		"green"	=> "house",
		"blue"	=> "sea",
		) ;

    my $ret = 0 ;
    while (($k, $v) = each %data) {
        my $r = $foreign->db_put($v, 1) ;
	#print "put $r $BerkeleyDB::Error\n";
        $ret += $r;
    }
    ok $ret == 0 ;

    while (($k, $v) = each %data) {
        my $r = $primary->db_put($k, $v) ;
	#print "put $r $BerkeleyDB::Error\n";
        $ret += $r;
    }
    ok $ret == 0 ;

    # check the records in the secondary
    is countRecords($primary), 3 ;
    is countRecords($secondary), 3 ;
    is countRecords($foreign), 3, "count is 3" ;

    # deleting from the foreign will pass, but the other dbs will not be
    # affected
    cmp_ok $foreign->db_del("sea"), '==', 0, "delete"
        or diag "$BerkeleyDB::Error\n";
    is countRecords($primary), 3 ;
    is countRecords($secondary), 2 ;
    is countRecords($foreign), 2 ;


    # deleting from the foreign will pass, but the other dbs will not be
    # affected
    cmp_ok $foreign->db_del("flag"), '==', 0, "delete"
        or diag "$BerkeleyDB::Error\n";
    is countRecords($primary), 3 ;
    is countRecords($secondary), 2 ;
    is countRecords($foreign), 1 ;

}

{
    # db->set_bt_compress

    my ($Dfile1, $Dfile2, $Dfile3);
    my $lex = new LexFile $Dfile1, $Dfile2, $Dfile3 ;
    my %hash ;
    my $status;
    my ($k, $v, $pk) = ('','','');

    # create primary database
    ok my $primary = new BerkeleyDB::Btree -Filename => $Dfile1, 
				     -set_bt_compress    => 1,
				     -Flags    => DB_CREATE ;

    # add data to the primary
    my %data =  (
		"red"	=> "flag",
		"green"	=> "house",
		"blue"	=> "sea",
		) ;

    my $ret = 0 ;
    while (($k, $v) = each %data) {
        my $r = $primary->db_put($k, $v);
	#print "put $r $BerkeleyDB::Error\n";
        $ret += $r;
    }
    ok $ret == 0 ;

    # check the records in the secondary
    is countRecords($primary), 3 ;
}

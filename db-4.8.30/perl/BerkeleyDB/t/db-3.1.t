#!./perl -w

use strict ;

use lib 't';
use util ;

use Test::More ; 

use BerkeleyDB; 

plan(skip_all =>  "1..0 # Skip: this needs Berkeley DB 3.1.x or better\n") 
    if $BerkeleyDB::db_version < 3.1 ;

plan(tests => 48) ;


my $Dfile = "dbhash.tmp";
my $Dfile2 = "dbhash2.tmp";
my $Dfile3 = "dbhash3.tmp";
unlink $Dfile;

umask(0) ;



{
    title "c_count";

    my $lex = new LexFile $Dfile ;
    my %hash ;
    my $db = tie %hash, 'BerkeleyDB::Hash', -Filename => $Dfile,
				                            -Property => DB_DUP,
                                            -Flags    => DB_CREATE ;
    ok $db, "  open database ok";

    $hash{'Wall'} = 'Larry' ;
    $hash{'Wall'} = 'Stone' ;
    $hash{'Smith'} = 'John' ;
    $hash{'Wall'} = 'Brick' ;
    $hash{'Wall'} = 'Brick' ;
    $hash{'mouse'} = 'mickey' ;

    is keys %hash, 6, "  keys == 6" ;

    # create a cursor
    my $cursor = $db->db_cursor() ;
    ok $cursor, "  created cursor";

    my $key = "Wall" ;
    my $value ;
    cmp_ok $cursor->c_get($key, $value, DB_SET), '==', 0, "  c_get ok" ;
    is $key, "Wall", "  key is 'Wall'";
    is $value, "Larry", "  value is 'Larry'"; ;

    my $count ;
    cmp_ok $cursor->c_count($count), '==', 0, "  c_count ok" ;
    is $count, 4, "  count is 4" ;

    $key = "Smith" ;
    cmp_ok $cursor->c_get($key, $value, DB_SET), '==', 0, "  c_get ok" ;
    is $key, "Smith", "  key is 'Smith'";
    is $value, "John", "  value is 'John'"; ;

    cmp_ok $cursor->c_count($count), '==', 0, "  c_count ok" ;
    is $count, 1, "  count is 1" ;


    undef $db ;
    undef $cursor ;
    untie %hash ;

}

{
    title "db_key_range";

    my $lex = new LexFile $Dfile ;
    my %hash ;
    my $db = tie %hash, 'BerkeleyDB::Btree', -Filename => $Dfile,
				      -Property  => DB_DUP,
                                      -Flags    => DB_CREATE ;
    isa_ok $db, 'BerkeleyDB::Btree', "  create database ok";

    $hash{'Wall'} = 'Larry' ;
    $hash{'Wall'} = 'Stone' ;
    $hash{'Smith'} = 'John' ;
    $hash{'Wall'} = 'Brick' ;
    $hash{'Wall'} = 'Brick' ;
    $hash{'mouse'} = 'mickey' ;

    is keys %hash, 6, "  6 keys" ;

    my $key = "Wall" ;
    my ($less, $equal, $greater) ;
    cmp_ok $db->db_key_range($key, $less, $equal, $greater), '==', 0, "  db_key_range ok" ;

    cmp_ok $less, '!=', 0 ;
    cmp_ok $equal, '!=', 0 ;
    cmp_ok $greater, '!=', 0 ;

    $key = "Smith" ;
    cmp_ok $db->db_key_range($key, $less, $equal, $greater), '==', 0, "  db_key_range ok" ;

    cmp_ok $less, '==', 0 ;
    cmp_ok $equal, '!=', 0 ;
    cmp_ok $greater, '!=', 0 ;

    $key = "NotThere" ;
    cmp_ok $db->db_key_range($key, $less, $equal, $greater), '==', 0, "  db_key_range ok" ;

    cmp_ok $less, '==', 0 ;
    cmp_ok $equal, '==', 0 ;
    cmp_ok $greater, '==', 1 ;

    undef $db ;
    untie %hash ;

}

{
    title "rename a subdb";

    my $lex = new LexFile $Dfile ;
  
    my $db1 = new BerkeleyDB::Hash -Filename => $Dfile, 
				        -Subname  => "fred" ,
				        -Flags    => DB_CREATE ;
    isa_ok $db1, 'BerkeleyDB::Hash', "  create database ok";

    my $db2 = new BerkeleyDB::Btree -Filename => $Dfile, 
				        -Subname  => "joe" ,
				        -Flags    => DB_CREATE ;
    isa_ok $db2, 'BerkeleyDB::Btree', "  create database ok";

    # Add a k/v pair
    my %data = qw(
    			red	sky
			blue	sea
			black	heart
			yellow	belley
			green	grass
    		) ;

    ok addData($db1, %data), "  added to db1 ok" ;
    ok addData($db2, %data), "  added to db2 ok" ;

    undef $db1 ;
    undef $db2 ;

    # now rename 
    cmp_ok BerkeleyDB::db_rename(-Filename => $Dfile, 
                              -Subname => "fred",
                              -Newname => "harry"), '==', 0, "  rename ok";
  
    my $db3 = new BerkeleyDB::Hash -Filename => $Dfile, 
				        -Subname  => "harry" ;
    isa_ok $db3, 'BerkeleyDB::Hash', "  verify rename";

}

{
    title "rename a file";

    my $lex = new LexFile $Dfile, $Dfile2 ;
  
    my $db1 = new BerkeleyDB::Hash -Filename => $Dfile, 
				        -Subname  => "fred" ,
				        -Flags    => DB_CREATE;
    isa_ok $db1, 'BerkeleyDB::Hash', "  create database ok";

    my $db2 = new BerkeleyDB::Hash -Filename => $Dfile, 
				        -Subname  => "joe" ,
				        -Flags    => DB_CREATE ;
    isa_ok $db2, 'BerkeleyDB::Hash', "  create database ok";

    # Add a k/v pair
    my %data = qw(
    			red	sky
			blue	sea
			black	heart
			yellow	belley
			green	grass
    		) ;

    ok addData($db1, %data), "  add data to db1" ;
    ok addData($db2, %data), "  add data to db2" ;

    undef $db1 ;
    undef $db2 ;

    # now rename 
    cmp_ok BerkeleyDB::db_rename(-Filename => $Dfile, -Newname => $Dfile2), 
            '==',  0, "  rename file to $Dfile2 ok";
  
    my $db3 = new BerkeleyDB::Hash -Filename => $Dfile2, 
				        -Subname  => "fred" ;
    isa_ok $db3, 'BerkeleyDB::Hash', "  verify rename"
        or diag "$! $BerkeleyDB::Error";


# TODO add rename with no subname & txn
}

{
    title "verify";

    my $lex = new LexFile $Dfile, $Dfile2 ;
  
    my $db1 = new BerkeleyDB::Hash -Filename => $Dfile, 
				        -Subname  => "fred" ,
				        -Flags    => DB_CREATE ;
    isa_ok $db1, 'BerkeleyDB::Hash', "  create database ok";

    # Add a k/v pair
    my %data = qw(
    			red	sky
			blue	sea
			black	heart
			yellow	belley
			green	grass
    		) ;

    ok addData($db1, %data), "  added data ok" ;

    undef $db1 ;

    # now verify 
    cmp_ok BerkeleyDB::db_verify(-Filename => $Dfile, 
                              -Subname => "fred",
                              ), '==', 0, "  verify ok";

    # now verify & dump
    cmp_ok BerkeleyDB::db_verify(-Filename => $Dfile, 
                              -Subname => "fred",
                              -Outfile => $Dfile2,
                              ), '==', 0, "  verify and dump ok";
  
}

# db_remove with env


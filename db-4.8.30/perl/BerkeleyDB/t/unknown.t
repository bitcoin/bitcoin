#!./perl -w

# ID: %I%, %G%   

use strict ;

use lib 't' ;
use BerkeleyDB; 
use util ;
use Test::More;
plan tests =>  41;

my $Dfile = "dbhash.tmp";
unlink $Dfile;

umask(0) ;


# Check for invalid parameters
{
    # Check for invalid parameters
    my $db ;
    eval ' $db = new BerkeleyDB::Unknown  -Stupid => 3 ; ' ;
    ok $@ =~ /unknown key value\(s\) Stupid/  ;

    eval ' $db = new BerkeleyDB::Unknown -Bad => 2, -Mode => 0345, -Stupid => 3; ' ;
    ok $@ =~ /unknown key value\(s\) (Bad,? |Stupid,? ){2}/  ;

    eval ' $db = new BerkeleyDB::Unknown -Env => 2 ' ;
    ok $@ =~ /^Env not of type BerkeleyDB::Env/ ;

    eval ' $db = new BerkeleyDB::Unknown -Txn => "fred" ' ;
    ok $@ =~ /^Txn not of type BerkeleyDB::Txn/ ;

    my $obj = bless [], "main" ;
    eval ' $db = new BerkeleyDB::Unknown -Env => $obj ' ;
    ok $@ =~ /^Env not of type BerkeleyDB::Env/ ;
}

# check the interface to a rubbish database
{
    # first an empty file
    my $lex = new LexFile $Dfile ;
    ok writeFile($Dfile, "") ;

    ok ! (new BerkeleyDB::Unknown -Filename => $Dfile); 

    # now a non-database file
    writeFile($Dfile, "\x2af6") ;
    ok ! (new BerkeleyDB::Unknown -Filename => $Dfile); 
}

# check the interface to a Hash database

{
    my $lex = new LexFile $Dfile ;

    # create a hash database
    ok my $db = new BerkeleyDB::Hash -Filename => $Dfile, 
				    -Flags    => DB_CREATE ;

    # Add a few k/v pairs
    my $value ;
    my $status ;
    ok $db->db_put("some key", "some value") == 0  ;
    ok $db->db_put("key", "value") == 0  ;

    # close the database
    undef $db ;

    # now open it with Unknown
    ok $db = new BerkeleyDB::Unknown -Filename => $Dfile; 

    ok $db->type() == DB_HASH ;
    ok $db->db_get("some key", $value) == 0 ;
    ok $value eq "some value" ;
    ok $db->db_get("key", $value) == 0 ;
    ok $value eq "value" ;

    my @array ;
    eval { $db->Tie(\@array)} ;
    ok $@ =~ /^Tie needs a reference to a hash/ ;

    my %hash ;
    $db->Tie(\%hash) ;
    ok $hash{"some key"} eq "some value" ;

}

# check the interface to a Btree database

{
    my $lex = new LexFile $Dfile ;

    # create a hash database
    ok my $db = new BerkeleyDB::Btree -Filename => $Dfile, 
				    -Flags    => DB_CREATE ;

    # Add a few k/v pairs
    my $value ;
    my $status ;
    ok $db->db_put("some key", "some value") == 0  ;
    ok $db->db_put("key", "value") == 0  ;

    # close the database
    undef $db ;

    # now open it with Unknown
    # create a hash database
    ok $db = new BerkeleyDB::Unknown -Filename => $Dfile; 

    ok $db->type() == DB_BTREE ;
    ok $db->db_get("some key", $value) == 0 ;
    ok $value eq "some value" ;
    ok $db->db_get("key", $value) == 0 ;
    ok $value eq "value" ;


    my @array ;
    eval { $db->Tie(\@array)} ;
    ok $@ =~ /^Tie needs a reference to a hash/ ;

    my %hash ;
    $db->Tie(\%hash) ;
    ok $hash{"some key"} eq "some value" ;


}

# check the interface to a Recno database

{
    my $lex = new LexFile $Dfile ;

    # create a recno database
    ok my $db = new BerkeleyDB::Recno -Filename => $Dfile, 
				    -Flags    => DB_CREATE ;

    # Add a few k/v pairs
    my $value ;
    my $status ;
    ok $db->db_put(0, "some value") == 0  ;
    ok $db->db_put(1, "value") == 0  ;

    # close the database
    undef $db ;

    # now open it with Unknown
    # create a hash database
    ok $db = new BerkeleyDB::Unknown -Filename => $Dfile; 

    ok $db->type() == DB_RECNO ;
    ok $db->db_get(0, $value) == 0 ;
    ok $value eq "some value" ;
    ok $db->db_get(1, $value) == 0 ;
    ok $value eq "value" ;


    my %hash ;
    eval { $db->Tie(\%hash)} ;
    ok $@ =~ /^Tie needs a reference to an array/ ;

    my @array ;
    $db->Tie(\@array) ;
    ok $array[1] eq "value" ;


}

# check i/f to text

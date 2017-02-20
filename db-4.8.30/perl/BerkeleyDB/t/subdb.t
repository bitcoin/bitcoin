#!./perl -w

use strict ;

use lib 't' ;
use BerkeleyDB; 
use Test::More ;
use util ;

plan(skip_all => "this needs Berkeley DB 3.x or better\n" )
    if $BerkeleyDB::db_version < 3;

plan tests => 43;

my $Dfile = "dbhash.tmp";
my $Dfile2 = "dbhash2.tmp";
my $Dfile3 = "dbhash3.tmp";
unlink $Dfile;

umask(0) ;

sub countDatabases
{
    my $file = shift ;

    ok my $db = new BerkeleyDB::Unknown -Filename => $file ,
				         -Flags    => DB_RDONLY ;

    #my $type = $db->type() ; print "type $type\n" ;
    ok my $cursor = $db->db_cursor() ;
    my ($k, $v) = ("", "") ;
    my $status ;
    my @dbnames = () ;
    while (($status = $cursor->c_get($k, $v, DB_NEXT)) == 0) {
        push @dbnames, $k ;
    }

    ok $status == DB_NOTFOUND;

    return wantarray ? sort @dbnames : scalar @dbnames ;
    

}

# Berkeley DB 3.x specific functionality

# Check for invalid parameters
{
    # Check for invalid parameters
    my $db ;
    eval ' BerkeleyDB::db_remove  -Stupid => 3 ; ' ;
    ok $@ =~ /unknown key value\(s\) Stupid/  ;

    eval ' BerkeleyDB::db_remove -Bad => 2, -Filename => "fred", -Stupid => 3; ' ;
    ok $@ =~ /unknown key value\(s\) (Bad,? |Stupid,? ){2}/  ;

    eval ' BerkeleyDB::db_remove -Filename => "a", -Env => 2 ' ;
    ok $@ =~ /^Env not of type BerkeleyDB::Env/ ;

    eval ' BerkeleyDB::db_remove -Subname => "a"' ;
    ok $@ =~ /^Must specify a filename/ ;

    my $obj = bless [], "main" ;
    eval ' BerkeleyDB::db_remove -Filename => "x", -Env => $obj ' ;
    ok $@ =~ /^Env not of type BerkeleyDB::Env/ ;
}

{
    # subdatabases

    # opening a subdatabse in an exsiting database that doesn't have
    # subdatabases at all should fail

    my $lex = new LexFile $Dfile ;

    ok my $db = new BerkeleyDB::Hash -Filename => $Dfile, 
				        -Flags    => DB_CREATE ;

    # Add a k/v pair
    my %data = qw(
    			red	sky
			blue	sea
			black	heart
			yellow	belley
			green	grass
    		) ;

    ok addData($db, %data) ;

    undef $db ;

    $db = new BerkeleyDB::Hash -Filename => $Dfile, 
			       -Subname  => "fred" ;
    ok ! $db ;				    

    ok -e $Dfile ;
    ok ! BerkeleyDB::db_remove(-Filename => $Dfile)  ;
}

{
    # subdatabases

    # opening a subdatabse in an exsiting database that does have
    # subdatabases at all, but not this one

    my $lex = new LexFile $Dfile ;

    ok my $db = new BerkeleyDB::Hash -Filename => $Dfile, 
				         -Subname  => "fred" ,
				         -Flags    => DB_CREATE ;

    # Add a k/v pair
    my %data = qw(
    			red	sky
			blue	sea
			black	heart
			yellow	belley
			green	grass
    		) ;

    ok addData($db, %data) ;

    undef $db ;

    $db = new BerkeleyDB::Hash -Filename => $Dfile, 
				    -Subname  => "joe" ;

    ok !$db ;				    

}

{
    # subdatabases

    my $lex = new LexFile $Dfile ;

    ok my $db = new BerkeleyDB::Hash -Filename => $Dfile, 
				        -Subname  => "fred" ,
				        -Flags    => DB_CREATE ;

    # Add a k/v pair
    my %data = qw(
    			red	sky
			blue	sea
			black	heart
			yellow	belley
			green	grass
    		) ;

    ok addData($db, %data) ;
    undef $db ;

    is join(",", countDatabases($Dfile)), "fred";

}

{
    # subdatabases

    # opening a database with multiple subdatabases - handle should be a list
    # of the subdatabase names

    my $lex = new LexFile $Dfile ;
  
    ok my $db1 = new BerkeleyDB::Hash -Filename => $Dfile, 
				        -Subname  => "fred" ,
				        -Flags    => DB_CREATE ;

    ok my $db2 = new BerkeleyDB::Btree -Filename => $Dfile, 
				        -Subname  => "joe" ,
				        -Flags    => DB_CREATE ;

    # Add a k/v pair
    my %data = qw(
    			red	sky
			blue	sea
			black	heart
			yellow	belley
			green	grass
    		) ;

    ok addData($db1, %data) ;
    ok addData($db2, %data) ;

    undef $db1 ;
    undef $db2 ;
  
    is join(",", countDatabases($Dfile)), "fred,joe";

    ok BerkeleyDB::db_remove(-Filename => $Dfile, -Subname => "harry") != 0;
    ok BerkeleyDB::db_remove(-Filename => $Dfile, -Subname => "fred") == 0 ;
    
    # should only be one subdatabase
    is join(",", countDatabases($Dfile)), "joe";

    # can't delete an already deleted subdatabase
    ok BerkeleyDB::db_remove(-Filename => $Dfile, -Subname => "fred") != 0;
    
    ok BerkeleyDB::db_remove(-Filename => $Dfile, -Subname => "joe") == 0 ;
    
    # should only be one subdatabase
    is countDatabases($Dfile), 0;

    ok -e $Dfile ;
    ok BerkeleyDB::db_remove(-Filename => $Dfile)  == 0 ;
    ok ! -e $Dfile ;
    ok BerkeleyDB::db_remove(-Filename => $Dfile) != 0 ;
}

# db_remove with env

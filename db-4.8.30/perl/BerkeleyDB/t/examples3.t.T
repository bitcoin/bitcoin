#!./perl -w

use strict ; 

BEGIN {
    unless(grep /blib/, @INC) {
        chdir 't' if -d 't';
        @INC = '../lib' if -d '../lib';
    }
}

use lib 't';
use BerkeleyDB; 
use Test::More;
use util ;

#BEGIN 
#{
#    if ($BerkeleyDB::db_version < 3) {
#        print "1..0 # Skipping test, this needs Berkeley DB 3.x or better\n" ;
#        exit 0 ;
#    }
#}

plan(skip_all => "this needs Berkeley DB 3.x or better\n" )
    if $BerkeleyDB::db_version < 3;



plan tests => 2;


my $Dfile = "dbhash.tmp";
my $Dfile2 = "dbhash2.tmp";
my $Dfile3 = "dbhash3.tmp";
unlink $Dfile;

umask(0) ;

my $redirect = "xyzt" ;


{
my $redirect = "xyzt" ;
 {

    my $redirectObj = new Redirect $redirect ;

## BEGIN dupHash
    use strict ;
    use BerkeleyDB ;
    
    my $filename = "fruit" ;
    unlink $filename ;
    my $db = new BerkeleyDB::Hash 
                -Filename => $filename, 
		-Flags    => DB_CREATE,
		-Property  => DB_DUP
        or die "Cannot open file $filename: $! $BerkeleyDB::Error\n" ;

    # Add a few key/value pairs to the file
    $db->db_put("red", "apple") ;
    $db->db_put("orange", "orange") ;
    $db->db_put("green", "banana") ;
    $db->db_put("yellow", "banana") ;
    $db->db_put("red", "tomato") ;
    $db->db_put("green", "apple") ;
    
    # print the contents of the file
    my ($k, $v) = ("", "") ;
    my $cursor = $db->db_cursor() ;
    while ($cursor->c_get($k, $v, DB_NEXT) == 0)
      { print "$k -> $v\n" }
      
    undef $cursor ;
    undef $db ;
## END dupHash
    unlink $filename ;
 }

  #print "[" . docat($redirect) . "]" ;
  is(docat_del_sort($redirect), <<'EOM') ;
green -> apple
green -> banana
orange -> orange
red -> apple
red -> tomato
yellow -> banana
EOM

}

{
my $redirect = "xyzt" ;
 {

    my $redirectObj = new Redirect $redirect ;

## BEGIN dupSortHash
    use strict ;
    use BerkeleyDB ;
    
    my $filename = "fruit" ;
    unlink $filename ;
    my $db = new BerkeleyDB::Hash 
                -Filename => $filename, 
		-Flags    => DB_CREATE,
		-Property  => DB_DUP | DB_DUPSORT
        or die "Cannot open file $filename: $! $BerkeleyDB::Error\n" ;

    # Add a few key/value pairs to the file
    $db->db_put("red", "apple") ;
    $db->db_put("orange", "orange") ;
    $db->db_put("green", "banana") ;
    $db->db_put("yellow", "banana") ;
    $db->db_put("red", "tomato") ;
    $db->db_put("green", "apple") ;
    
    # print the contents of the file
    my ($k, $v) = ("", "") ;
    my $cursor = $db->db_cursor() ;
    while ($cursor->c_get($k, $v, DB_NEXT) == 0)
      { print "$k -> $v\n" }
      
    undef $cursor ;
    undef $db ;
## END dupSortHash
    unlink $filename ;
 }

  #print "[" . docat($redirect) . "]" ;
  is(docat_del_sort($redirect), <<'EOM') ;
green -> apple
green -> banana
orange -> orange
red -> apple
red -> tomato
yellow -> banana
EOM

}



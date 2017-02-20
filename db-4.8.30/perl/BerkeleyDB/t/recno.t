#!./perl -w

# ID: %I%, %G%   

use strict ;

use lib 't' ;
use BerkeleyDB; 
use util ;
use Test::More;

plan tests => 225;

my $Dfile = "dbhash.tmp";
my $Dfile2 = "dbhash2.tmp";
my $Dfile3 = "dbhash3.tmp";
unlink $Dfile;

umask(0) ;

# Check for invalid parameters
{
    # Check for invalid parameters
    my $db ;
    eval ' $db = new BerkeleyDB::Recno  -Stupid => 3 ; ' ;
    ok $@ =~ /unknown key value\(s\) Stupid/  ;

    eval ' $db = new BerkeleyDB::Recno -Bad => 2, -Mode => 0345, -Stupid => 3; ' ;
    ok $@ =~ /unknown key value\(s\) /  ;

    eval ' $db = new BerkeleyDB::Recno -Env => 2 ' ;
    ok $@ =~ /^Env not of type BerkeleyDB::Env/ ;

    eval ' $db = new BerkeleyDB::Recno -Txn => "x" ' ;
    ok $@ =~ /^Txn not of type BerkeleyDB::Txn/ ;

    my $obj = bless [], "main" ;
    eval ' $db = new BerkeleyDB::Recno -Env => $obj ' ;
    ok $@ =~ /^Env not of type BerkeleyDB::Env/ ;
}

# Now check the interface to Recno

{
    my $lex = new LexFile $Dfile ;

    ok my $db = new BerkeleyDB::Recno -Filename => $Dfile, 
				    -Flags    => DB_CREATE ;

    # Add a k/v pair
    my $value ;
    my $status ;
    ok $db->db_put(1, "some value") == 0  ;
    ok $db->status() == 0 ;
    ok $db->db_get(1, $value) == 0 ;
    ok $value eq "some value" ;
    ok $db->db_put(2, "value") == 0  ;
    ok $db->db_get(2, $value) == 0 ;
    ok $value eq "value" ;
    ok $db->db_del(1) == 0 ;
    ok (($status = $db->db_get(1, $value)) == DB_KEYEMPTY) ;
    ok $db->status() == DB_KEYEMPTY ;
    ok $db->status() eq $DB_errors{'DB_KEYEMPTY'} ;

    ok (($status = $db->db_get(7, $value)) == DB_NOTFOUND) ;
    ok $db->status() == DB_NOTFOUND ;
    ok $db->status() eq $DB_errors{'DB_NOTFOUND'} ;

    ok $db->db_sync() == 0 ;

    # Check NOOVERWRITE will make put fail when attempting to overwrite
    # an existing record.

    ok $db->db_put( 2, 'x', DB_NOOVERWRITE) == DB_KEYEXIST ;
    ok $db->status() eq $DB_errors{'DB_KEYEXIST'} ;
    ok $db->status() == DB_KEYEXIST ;


    # check that the value of the key  has not been changed by the
    # previous test
    ok $db->db_get(2, $value) == 0 ;
    ok $value eq "value" ;


}


{
    # Check simple env works with a array.
    my $lex = new LexFile $Dfile ;

    my $home = "./fred" ;
    ok my $lexD = new LexDir($home);

    ok my $env = new BerkeleyDB::Env -Flags => DB_CREATE|DB_INIT_MPOOL,@StdErrFile,
    					 -Home => $home ;

    ok my $db = new BerkeleyDB::Recno -Filename => $Dfile, 
				    -Env      => $env,
				    -Flags    => DB_CREATE ;

    # Add a k/v pair
    my $value ;
    ok $db->db_put(1, "some value") == 0 ;
    ok $db->db_get(1, $value) == 0 ;
    ok $value eq "some value" ;
    undef $db ;
    undef $env ;
}

 
{
    # cursors

    my $lex = new LexFile $Dfile ;
    my @array ;
    my ($k, $v) ;
    ok my $db = new BerkeleyDB::Recno -Filename  => $Dfile, 
				    	  -ArrayBase => 0,
				    	  -Flags     => DB_CREATE ;

    # create some data
    my @data =  (
		"red"	,
		"green"	,
		"blue"	,
		) ;

    my $i ;
    my %data ;
    my $ret = 0 ;
    for ($i = 0 ; $i < @data ; ++$i) {
        $ret += $db->db_put($i, $data[$i]) ;
	$data{$i} = $data[$i] ;
    }
    ok $ret == 0 ;

    # create the cursor
    ok my $cursor = $db->db_cursor() ;

    $k = 0 ; $v = "" ;
    my %copy = %data;
    my $extras = 0 ;
    # sequence forwards
    while ($cursor->c_get($k, $v, DB_NEXT) == 0) 
    {
        if ( $copy{$k} eq $v ) 
            { delete $copy{$k} }
	else
	    { ++ $extras }
    }

    ok $cursor->status() == DB_NOTFOUND ;
    ok $cursor->status() eq $DB_errors{'DB_NOTFOUND'} ;
    ok keys %copy == 0 ;
    ok $extras == 0 ;

    # sequence backwards
    %copy = %data ;
    $extras = 0 ;
    my $status ;
    for ( $status = $cursor->c_get($k, $v, DB_LAST) ;
	  $status == 0 ;
    	  $status = $cursor->c_get($k, $v, DB_PREV)) {
        if ( $copy{$k} eq $v ) 
            { delete $copy{$k} }
	else
	    { ++ $extras }
    }
    ok $status == DB_NOTFOUND ;
    ok $status eq $DB_errors{'DB_NOTFOUND'} ;
    ok $cursor->status() == $status ;
    ok $cursor->status() eq $status ;
    ok keys %copy == 0 ;
    ok $extras == 0 ;
}
 
{
    # Tied Array interface


    my $lex = new LexFile $Dfile ;
    my @array ;
    my $db ;
    ok $db = tie @array, 'BerkeleyDB::Recno', -Filename  => $Dfile,
				    	    -Property => DB_RENUMBER,
				    	    -ArrayBase => 0,
                            -Flags     => DB_CREATE ;

    ok my $cursor = ((tied @array)->db_cursor()) ;
    # check the database is empty
    my $count = 0 ;
    my ($k, $v) = (0,"") ;
    while ($cursor->c_get($k, $v, DB_NEXT) == 0) {
	++ $count ;
    }
    ok $cursor->status() == DB_NOTFOUND ;
    ok $count == 0 ;

    ok @array == 0 ;

    # Add a k/v pair
    my $value ;
    $array[1] = "some value";
    ok ((tied @array)->status() == 0) ;
    ok $array[1] eq "some value";
    ok defined $array[1];
    ok ((tied @array)->status() == 0) ;
    ok !defined $array[3];
    ok ((tied @array)->status() == DB_NOTFOUND) ;

    ok ((tied @array)->db_del(1) == 0) ;
    ok ((tied @array)->status() == 0) ;
    ok ! defined $array[1];
    ok ((tied @array)->status() == DB_NOTFOUND) ;

    $array[1] = 2 ;
    $array[10] = 20 ;
    $array[1000] = 2000 ;

    my ($keys, $values) = (0,0);
    $count = 0 ;
    for ( my $status = $cursor->c_get($k, $v, DB_FIRST) ;
	  $status == 0 ;
    	  $status = $cursor->c_get($k, $v, DB_NEXT)) {
        $keys += $k ;
	$values += $v ;
	++ $count ;
    }
    ok $count == 3 ;
    ok $keys == 1011 ;
    ok $values == 2022 ;

    # unshift
    $FA ? unshift @array, "red", "green", "blue" 
        : $db->unshift("red", "green", "blue" ) ;
    ok $array[1] eq "red" ;
    ok $cursor->c_get($k, $v, DB_FIRST) == 0 ;
    ok $k == 1 ;
    ok $v eq "red" ;
    ok $array[2] eq "green" ;
    ok $cursor->c_get($k, $v, DB_NEXT) == 0 ;
    ok $k == 2 ;
    ok $v eq "green" ;
    ok $array[3] eq "blue" ;
    ok $cursor->c_get($k, $v, DB_NEXT) == 0 ;
    ok $k == 3 ;
    ok $v eq "blue" ;
    ok $array[4] == 2 ;
    ok $cursor->c_get($k, $v, DB_NEXT) == 0 ;
    ok $k == 4 ;
    ok $v == 2 ;

    # shift
    ok (($FA ? shift @array : $db->shift()) eq "red") ;
    ok (($FA ? shift @array : $db->shift()) eq "green") ;
    ok (($FA ? shift @array : $db->shift()) eq "blue") ;
    ok (($FA ? shift @array : $db->shift()) == 2) ;

    # push
    $FA ? push @array, "the", "end" 
        : $db->push("the", "end") ;
    ok $cursor->c_get($k, $v, DB_LAST) == 0 ;
    ok $k == 1001 ;
    ok $v eq "end" ;
    ok $cursor->c_get($k, $v, DB_PREV) == 0 ;
    ok $k == 1000 ;
    ok $v eq "the" ;
    ok $cursor->c_get($k, $v, DB_PREV) == 0 ;
    ok $k == 999 ;
    ok $v == 2000 ;

    # pop
    ok (( $FA ? pop @array : $db->pop ) eq "end") ;
    ok (( $FA ? pop @array : $db->pop ) eq "the") ;
    ok (( $FA ? pop @array : $db->pop ) == 2000)  ;

    # now clear the array 
    $FA ? @array = () 
        : $db->clear() ;
    ok $cursor->c_get($k, $v, DB_FIRST) == DB_NOTFOUND ;

    undef $cursor ;
    undef $db ;
    untie @array ;
}

{
    # in-memory file

    my @array ;
    my $fd ;
    my $value ;
    ok my $db = tie @array, 'BerkeleyDB::Recno' ;

    ok $db->db_put(1, "some value") == 0  ;
    ok $db->db_get(1, $value) == 0 ;
    ok $value eq "some value" ;

}
 
{
    # partial
    # check works via API

    my $lex = new LexFile $Dfile ;
    my $value ;
    ok my $db = new BerkeleyDB::Recno -Filename => $Dfile,
                                        -Flags    => DB_CREATE ;

    # create some data
    my @data =  (
		"",
		"boat",
		"house",
		"sea",
		) ;

    my $ret = 0 ;
    my $i ;
    for ($i = 1 ; $i < @data ; ++$i) {
        $ret += $db->db_put($i, $data[$i]) ;
    }
    ok $ret == 0 ;


    # do a partial get
    my ($pon, $off, $len) = $db->partial_set(0,2) ;
    ok ! $pon && $off == 0 && $len == 0 ;
    ok $db->db_get(1, $value) == 0 && $value eq "bo" ;
    ok $db->db_get(2, $value) == 0 && $value eq "ho" ;
    ok $db->db_get(3, $value) == 0 && $value eq "se" ;

    # do a partial get, off end of data
    ($pon, $off, $len) = $db->partial_set(3,2) ;
    ok $pon ;
    ok $off == 0 ;
    ok $len == 2 ;
    ok $db->db_get(1, $value) == 0 && $value eq "t" ;
    ok $db->db_get(2, $value) == 0 && $value eq "se" ;
    ok $db->db_get(3, $value) == 0 && $value eq "" ;

    # switch of partial mode
    ($pon, $off, $len) = $db->partial_clear() ;
    ok $pon ;
    ok $off == 3 ;
    ok $len == 2 ;
    ok $db->db_get(1, $value) == 0 && $value eq "boat" ;
    ok $db->db_get(2, $value) == 0 && $value eq "house" ;
    ok $db->db_get(3, $value) == 0 && $value eq "sea" ;

    # now partial put
    $db->partial_set(0,2) ;
    ok $db->db_put(1, "") == 0 ;
    ok $db->db_put(2, "AB") == 0 ;
    ok $db->db_put(3, "XYZ") == 0 ;
    ok $db->db_put(4, "KLM") == 0 ;

    ($pon, $off, $len) = $db->partial_clear() ;
    ok $pon ;
    ok $off == 0 ;
    ok $len == 2 ;
    ok $db->db_get(1, $value) == 0 && $value eq "at" ;
    ok $db->db_get(2, $value) == 0 && $value eq "ABuse" ;
    ok $db->db_get(3, $value) == 0 && $value eq "XYZa" ;
    ok $db->db_get(4, $value) == 0 && $value eq "KLM" ;

    # now partial put
    ($pon, $off, $len) = $db->partial_set(3,2) ;
    ok ! $pon ;
    ok $off == 0 ;
    ok $len == 0 ;
    ok $db->db_put(1, "PPP") == 0 ;
    ok $db->db_put(2, "Q") == 0 ;
    ok $db->db_put(3, "XYZ") == 0 ;
    ok $db->db_put(4, "TU") == 0 ;

    $db->partial_clear() ;
    ok $db->db_get(1, $value) == 0 && $value eq "at\0PPP" ;
    ok $db->db_get(2, $value) == 0 && $value eq "ABuQ" ;
    ok $db->db_get(3, $value) == 0 && $value eq "XYZXYZ" ;
    ok $db->db_get(4, $value) == 0 && $value eq "KLMTU" ;
}

{
    # partial
    # check works via tied array 

    my $lex = new LexFile $Dfile ;
    my @array ;
    my $value ;
    ok my $db = tie @array, 'BerkeleyDB::Recno', -Filename => $Dfile,
                                      	        -Flags    => DB_CREATE ;

    # create some data
    my @data =  (
		"",
		"boat",
		"house",
		"sea",
		) ;

    my $i ;
    for ($i = 1 ; $i < @data ; ++$i) {
	$array[$i] = $data[$i] ;
    }


    # do a partial get
    $db->partial_set(0,2) ;
    ok $array[1] eq "bo" ;
    ok $array[2] eq "ho" ;
    ok $array[3]  eq "se" ;

    # do a partial get, off end of data
    $db->partial_set(3,2) ;
    ok $array[1] eq "t" ;
    ok $array[2] eq "se" ;
    ok $array[3] eq "" ;

    # switch of partial mode
    $db->partial_clear() ;
    ok $array[1] eq "boat" ;
    ok $array[2] eq "house" ;
    ok $array[3] eq "sea" ;

    # now partial put
    $db->partial_set(0,2) ;
    ok $array[1] = "" ;
    ok $array[2] = "AB" ;
    ok $array[3] = "XYZ" ;
    ok $array[4] = "KLM" ;

    $db->partial_clear() ;
    ok $array[1] eq "at" ;
    ok $array[2] eq "ABuse" ;
    ok $array[3] eq "XYZa" ;
    ok $array[4] eq "KLM" ;

    # now partial put
    $db->partial_set(3,2) ;
    ok $array[1] = "PPP" ;
    ok $array[2] = "Q" ;
    ok $array[3] = "XYZ" ;
    ok $array[4] = "TU" ;

    $db->partial_clear() ;
    ok $array[1] eq "at\0PPP" ;
    ok $array[2] eq "ABuQ" ;
    ok $array[3] eq "XYZXYZ" ;
    ok $array[4] eq "KLMTU" ;
}

{
    # transaction

    my $lex = new LexFile $Dfile ;
    my @array ;
    my $value ;

    my $home = "./fred" ;
    ok my $lexD = new LexDir($home);
    ok my $env = new BerkeleyDB::Env -Home => $home,@StdErrFile,
				     -Flags => DB_CREATE|DB_INIT_TXN|
					  	DB_INIT_MPOOL|DB_INIT_LOCK ;
    ok my $txn = $env->txn_begin() ;
    ok my $db1 = tie @array, 'BerkeleyDB::Recno', 
				-Filename => $Dfile,
				-ArrayBase => 0,
                      		-Flags    =>  DB_CREATE ,
		        	-Env 	  => $env,
		        	-Txn	  => $txn ;

    
    ok $txn->txn_commit() == 0 ;
    ok $txn = $env->txn_begin() ;
    $db1->Txn($txn);

    # create some data
    my @data =  (
		"boat",
		"house",
		"sea",
		) ;

    my $ret = 0 ;
    my $i ;
    for ($i = 0 ; $i < @data ; ++$i) {
        $ret += $db1->db_put($i, $data[$i]) ;
    }
    ok $ret == 0 ;

    # should be able to see all the records

    ok my $cursor = $db1->db_cursor() ;
    my ($k, $v) = (0, "") ;
    my $count = 0 ;
    # sequence forwards
    while ($cursor->c_get($k, $v, DB_NEXT) == 0) {
        ++ $count ;
    }
    ok $count == 3 ;
    undef $cursor ;

    # now abort the transaction
    ok $txn->txn_abort() == 0 ;

    # there shouldn't be any records in the database
    $count = 0 ;
    # sequence forwards
    ok $cursor = $db1->db_cursor() ;
    while ($cursor->c_get($k, $v, DB_NEXT) == 0) {
        ++ $count ;
    }
    ok $count == 0 ;

    undef $txn ;
    undef $cursor ;
    undef $db1 ;
    undef $env ;
    untie @array ;
}


{
    # db_stat

    my $lex = new LexFile $Dfile ;
    my $recs = ($BerkeleyDB::db_version >= 3.1 ? "bt_ndata" : "bt_nrecs") ;
    my @array ;
    my ($k, $v) ;
    ok my $db = new BerkeleyDB::Recno -Filename 	=> $Dfile, 
				     	   -Flags    	=> DB_CREATE,
					   -Pagesize	=> 4 * 1024,
					;

    my $ref = $db->db_stat() ; 
    ok $ref->{$recs} == 0;
    ok $ref->{'bt_pagesize'} == 4 * 1024;

    # create some data
    my @data =  (
		2,
		"house",
		"sea",
		) ;

    my $ret = 0 ;
    my $i ;
    for ($i = $db->ArrayOffset ; @data ; ++$i) {
        $ret += $db->db_put($i, shift @data) ;
    }
    ok $ret == 0 ;

    $ref = $db->db_stat() ; 
    ok $ref->{$recs} == 3;
}

{
   # sub-class test

   package Another ;

   use strict ;

   open(FILE, ">SubDB.pm") or die "Cannot open SubDB.pm: $!\n" ;
   print FILE <<'EOM' ;

   package SubDB ;

   use strict ;
   use vars qw( @ISA @EXPORT) ;

   require Exporter ;
   use BerkeleyDB;
   @ISA=qw(BerkeleyDB BerkeleyDB::Recno);
   @EXPORT = @BerkeleyDB::EXPORT ;

   sub db_put { 
	my $self = shift ;
        my $key = shift ;
        my $value = shift ;
        $self->SUPER::db_put($key, $value * 3) ;
   }

   sub db_get { 
	my $self = shift ;
        $self->SUPER::db_get($_[0], $_[1]) ;
	$_[1] -= 2 ;
   }

   sub A_new_method
   {
	my $self = shift ;
        my $key = shift ;
        my $value = $self->FETCH($key) ;
	return "[[$value]]" ;
   }

   1 ;
EOM

    close FILE ;

    BEGIN { push @INC, '.'; }    
    use Test::More;
    eval 'use SubDB ; ';
    ok $@ eq "" ;
    my @h ;
    my $X ;
    eval '
	$X = tie(@h, "SubDB", -Filename => "dbrecno.tmp", 
			-Flags => DB_CREATE,
			-Mode => 0640 );
	' ;

    ok $@ eq "" ;

    my $ret = eval '$h[1] = 3 ; return $h[1] ' ;
    ok $@ eq "" ;
    ok $ret == 7 ;

    my $value = 0;
    $ret = eval '$X->db_put(1, 4) ; $X->db_get(1, $value) ; return $value' ;
    ok $@ eq "" ;
    ok $ret == 10 ;

    $ret = eval ' DB_NEXT eq main::DB_NEXT ' ;
    ok $@ eq ""  ;
    ok $ret == 1 ;

    $ret = eval '$X->A_new_method(1) ' ;
    ok $@ eq "" ;
    ok $ret eq "[[10]]" ;

    undef $X;
    untie @h;
    unlink "SubDB.pm", "dbrecno.tmp" ;

}

{
    # variable length records, DB_DELIMETER -- defaults to \n

    my $lex = new LexFile $Dfile, $Dfile2 ;
    touch $Dfile2 ;
    my @array ;
    my $value ;
    ok tie @array, 'BerkeleyDB::Recno', -Filename  => $Dfile,
						-ArrayBase => 0,
                                      	       	-Flags  => DB_CREATE ,
						-Source	=> $Dfile2 ;
    $array[0] = "abc" ;
    $array[1] = "def" ;
    $array[3] = "ghi" ;
    untie @array ;

    my $x = docat($Dfile2) ;
    ok $x eq "abc\ndef\n\nghi\n" ;
}

{
    # variable length records, change DB_DELIMETER

    my $lex = new LexFile $Dfile, $Dfile2 ;
    touch $Dfile2 ;
    my @array ;
    my $value ;
    ok tie @array, 'BerkeleyDB::Recno', -Filename  => $Dfile,
						-ArrayBase => 0,
                                      	       	-Flags  => DB_CREATE ,
						-Source	=> $Dfile2 ,
						-Delim	=> "-";
    $array[0] = "abc" ;
    $array[1] = "def" ;
    $array[3] = "ghi" ;
    untie @array ;

    my $x = docat($Dfile2) ;
    ok $x eq "abc-def--ghi-";
}

{
    # fixed length records, default DB_PAD

    my $lex = new LexFile $Dfile, $Dfile2 ;
    touch $Dfile2 ;
    my @array ;
    my $value ;
    ok tie @array, 'BerkeleyDB::Recno', -Filename  => $Dfile,
						-ArrayBase => 0,
                                      	       	-Flags  => DB_CREATE ,
						-Len 	=> 5,
						-Source	=> $Dfile2 ;
    $array[0] = "abc" ;
    $array[1] = "def" ;
    $array[3] = "ghi" ;
    untie @array ;

    my $x = docat($Dfile2) ;
    ok $x eq "abc  def       ghi  " ;
}

{
    # fixed length records, change Pad

    my $lex = new LexFile $Dfile, $Dfile2 ;
    touch $Dfile2 ;
    my @array ;
    my $value ;
    ok tie @array, 'BerkeleyDB::Recno', -Filename  => $Dfile,
						-ArrayBase => 0,
                                      	       	-Flags  => DB_CREATE ,
						-Len	=> 5,
						-Pad	=> "-",
						-Source	=> $Dfile2 ;
    $array[0] = "abc" ;
    $array[1] = "def" ;
    $array[3] = "ghi" ;
    untie @array ;

    my $x = docat($Dfile2) ;
    ok $x eq "abc--def-------ghi--" ;
}

{
    # DB_RENUMBER

    my $lex = new LexFile $Dfile;
    my @array ;
    my $value ;
    ok my $db = tie @array, 'BerkeleyDB::Recno', -Filename  => $Dfile,
				    	    	-Property => DB_RENUMBER,
						-ArrayBase => 0,
                                      	       	-Flags  => DB_CREATE ;
    # create a few records
    $array[0] = "abc" ;
    $array[1] = "def" ;
    $array[3] = "ghi" ;

    ok my ($length, $joined) = joiner($db, "|") ;
    ok $length == 3 ;
    ok $joined eq "abc|def|ghi";

    ok $db->db_del(1) == 0 ;
    ($length, $joined) = joiner($db, "|") ;
    ok $length == 2 ;
    ok $joined eq "abc|ghi";

    undef $db ;
    untie @array ;

}

{
    # DB_APPEND

    my $lex = new LexFile $Dfile;
    my @array ;
    my $value ;
    ok my $db = tie @array, 'BerkeleyDB::Recno', 
					-Filename  => $Dfile,
                                       	-Flags     => DB_CREATE ;

    # create a few records
    $array[1] = "def" ;
    $array[3] = "ghi" ;

    my $k = 0 ;
    ok $db->db_put($k, "fred", DB_APPEND) == 0 ;
    ok $k == 4 ;

    undef $db ;
    untie @array ;
}

{
    # in-memory Btree with an associated text file

    my $lex = new LexFile $Dfile2 ;
    touch $Dfile2 ;
    my @array ;
    my $value ;
    ok tie @array, 'BerkeleyDB::Recno',    -Source => $Dfile2 ,
						-ArrayBase => 0,
				    	    	-Property => DB_RENUMBER,
                                      	       	-Flags  => DB_CREATE ;
    $array[0] = "abc" ;
    $array[1] = "def" ;
    $array[3] = "ghi" ;
    untie @array ;

    my $x = docat($Dfile2) ;
    ok $x eq "abc\ndef\n\nghi\n" ;
}

{
    # in-memory, variable length records, change DB_DELIMETER

    my $lex = new LexFile $Dfile, $Dfile2 ;
    touch $Dfile2 ;
    my @array ;
    my $value ;
    ok tie @array, 'BerkeleyDB::Recno', 
						-ArrayBase => 0,
                                      	       	-Flags  => DB_CREATE ,
						-Source	=> $Dfile2 ,
				    	    	-Property => DB_RENUMBER,
						-Delim	=> "-";
    $array[0] = "abc" ;
    $array[1] = "def" ;
    $array[3] = "ghi" ;
    untie @array ;

    my $x = docat($Dfile2) ;
    ok $x eq "abc-def--ghi-";
}

{
    # in-memory, fixed length records, default DB_PAD

    my $lex = new LexFile $Dfile, $Dfile2 ;
    touch $Dfile2 ;
    my @array ;
    my $value ;
    ok tie @array, 'BerkeleyDB::Recno', 	-ArrayBase => 0,
                                      	       	-Flags  => DB_CREATE ,
				    	    	-Property => DB_RENUMBER,
						-Len 	=> 5,
						-Source	=> $Dfile2 ;
    $array[0] = "abc" ;
    $array[1] = "def" ;
    $array[3] = "ghi" ;
    untie @array ;

    my $x = docat($Dfile2) ;
    ok $x eq "abc  def       ghi  " ;
}

{
    # in-memory, fixed length records, change Pad

    my $lex = new LexFile $Dfile, $Dfile2 ;
    touch $Dfile2 ;
    my @array ;
    my $value ;
    ok tie @array, 'BerkeleyDB::Recno', 
						-ArrayBase => 0,
                                      	       	-Flags  => DB_CREATE ,
				    	    	-Property => DB_RENUMBER,
						-Len	=> 5,
						-Pad	=> "-",
						-Source	=> $Dfile2 ;
    $array[0] = "abc" ;
    $array[1] = "def" ;
    $array[3] = "ghi" ;
    untie @array ;

    my $x = docat($Dfile2) ;
    ok $x eq "abc--def-------ghi--" ;
}

{
    # 23 Sept 2001 -- push into an empty array
    my $lex = new LexFile $Dfile ;
    my @array ;
    my $db ;
    ok $db = tie @array, 'BerkeleyDB::Recno', 
						-ArrayBase => 0,
                                      	       	-Flags  => DB_CREATE ,
				    	    	-Property => DB_RENUMBER,
						-Filename => $Dfile ;
    $FA ? push @array, "first"
        : $db->push("first") ;

    ok $array[0] eq "first" ;
    ok $FA ? pop @array : $db->pop() eq "first" ;

    undef $db;
    untie @array ;

}

{
    # 23 Sept 2001 -- unshift into an empty array
    my $lex = new LexFile $Dfile ;
    my @array ;
    my $db ;
    ok $db = tie @array, 'BerkeleyDB::Recno', 
						-ArrayBase => 0,
                                      	       	-Flags  => DB_CREATE ,
				    	    	-Property => DB_RENUMBER,
						-Filename => $Dfile ;
    $FA ? unshift @array, "first"
        : $db->unshift("first") ;

    ok $array[0] eq "first" ;
    ok (($FA ? shift @array : $db->shift()) eq "first") ;

    undef $db;
    untie @array ;

}
__END__


# TODO
#
# DB_DELIMETER DB_FIXEDLEN DB_PAD DB_SNAPSHOT with partial records

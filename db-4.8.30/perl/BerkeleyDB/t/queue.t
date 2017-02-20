#!./perl -w

# ID: %I%, %G%   

use strict ;

use lib 't' ;
use BerkeleyDB; 
use Test::More;
use util;

plan(skip_all => "Queue needs Berkeley DB 3.3.x or better\n" )
    if $BerkeleyDB::db_version < 3.3;
    
plan tests => 253;


my $Dfile = "dbhash.tmp";
my $Dfile2 = "dbhash2.tmp";
my $Dfile3 = "dbhash3.tmp";
unlink $Dfile;

umask(0) ;


# Check for invalid parameters
{
    # Check for invalid parameters
    my $db ;
    eval ' $db = new BerkeleyDB::Queue  -Stupid => 3 ; ' ;
    ok $@ =~ /unknown key value\(s\) Stupid/  ;

    eval ' $db = new BerkeleyDB::Queue -Bad => 2, -Mode => 0345, -Stupid => 3; ' ;
    ok $@ =~ /unknown key value\(s\) /  ;

    eval ' $db = new BerkeleyDB::Queue -Env => 2 ' ;
    ok $@ =~ /^Env not of type BerkeleyDB::Env/ ;

    eval ' $db = new BerkeleyDB::Queue -Txn => "x" ' ;
    ok $@ =~ /^Txn not of type BerkeleyDB::Txn/ ;

    my $obj = bless [], "main" ;
    eval ' $db = new BerkeleyDB::Queue -Env => $obj ' ;
    ok $@ =~ /^Env not of type BerkeleyDB::Env/ ;
}

# Now check the interface to Queue

{
    my $lex = new LexFile $Dfile ;
    my $rec_len = 10 ;
    my $pad = "x" ;

    ok my $db = new BerkeleyDB::Queue -Filename => $Dfile, 
				    -Flags    => DB_CREATE,
				    -Len      => $rec_len,
				    -Pad      => $pad;

    # Add a k/v pair
    my $value ;
    my $status ;
    ok $db->db_put(1, "some value") == 0  ;
    ok $db->status() == 0 ;
    ok $db->db_get(1, $value) == 0 ;
    ok $value eq fillout("some value", $rec_len, $pad) ;
    ok $db->db_put(2, "value") == 0  ;
    ok $db->db_get(2, $value) == 0 ;
    ok $value eq fillout("value", $rec_len, $pad) ;
    ok $db->db_put(3, "value") == 0  ;
    ok $db->db_get(3, $value) == 0 ;
    ok $value eq fillout("value", $rec_len, $pad) ;
    ok $db->db_del(2) == 0 ;
    ok $db->db_get(2, $value) == DB_KEYEMPTY ;
    ok $db->status() == DB_KEYEMPTY ;
    ok $db->status() eq $DB_errors{'DB_KEYEMPTY'} ;

    ok $db->db_get(7, $value) == DB_NOTFOUND ;
    ok $db->status() == DB_NOTFOUND ;
    ok $db->status() eq $DB_errors{'DB_NOTFOUND'} ;

    ok $db->db_sync() == 0 ;

    # Check NOOVERWRITE will make put fail when attempting to overwrite
    # an existing record.

    ok $db->db_put( 1, 'x', DB_NOOVERWRITE) == DB_KEYEXIST ;
    ok $db->status() eq $DB_errors{'DB_KEYEXIST'} ;
    ok $db->status() == DB_KEYEXIST ;


    # check that the value of the key  has not been changed by the
    # previous test
    ok $db->db_get(1, $value) == 0 ;
    ok $value eq fillout("some value", $rec_len, $pad) ;


}


{
    # Check simple env works with a array.
    # and pad defaults to space
    my $lex = new LexFile $Dfile ;

    my $home = "./fred" ;
    my $rec_len = 11 ;
    ok my $lexD = new LexDir($home);

    ok my $env = new BerkeleyDB::Env -Flags => DB_CREATE|DB_INIT_MPOOL,@StdErrFile,
    					 -Home => $home ;
    ok my $db = new BerkeleyDB::Queue -Filename => $Dfile, 
				    -Env      => $env,
				    -Flags    => DB_CREATE,
				    -Len      => $rec_len;

    # Add a k/v pair
    my $value ;
    ok $db->db_put(1, "some value") == 0 ;
    ok $db->db_get(1, $value) == 0 ;
    ok $value eq fillout("some value", $rec_len)  ;
    undef $db ;
    undef $env ;
}

 
{
    # cursors

    my $lex = new LexFile $Dfile ;
    my @array ;
    my ($k, $v) ;
    my $rec_len = 5 ;
    ok my $db = new BerkeleyDB::Queue -Filename  => $Dfile, 
				    	  -ArrayBase => 0,
				    	  -Flags     => DB_CREATE ,
				    	  -Len       => $rec_len;

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
        if ( fillout($copy{$k}, $rec_len) eq $v ) 
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
        if ( fillout($copy{$k}, $rec_len) eq $v ) 
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
    my $rec_len = 10 ;
    ok $db = tie @array, 'BerkeleyDB::Queue', -Filename  => $Dfile,
				    	    -ArrayBase => 0,
                                            -Flags     => DB_CREATE ,
				    	    -Len       => $rec_len;

    ok my $cursor = (tied @array)->db_cursor() ;
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
    ok $array[1] eq fillout("some value", $rec_len);
    ok defined $array[1];
    ok ((tied @array)->status() == 0) ;
    ok !defined $array[3];
    ok ((tied @array)->status() == DB_NOTFOUND) ;

    $array[1] = 2 ;
    $array[10] = 20 ;
    $array[100] = 200 ;

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
    ok $keys == 111 ;
    ok $values == 222 ;

    # unshift isn't allowed
#    eval {
#    	$FA ? unshift @array, "red", "green", "blue" 
#        : $db->unshift("red", "green", "blue" ) ;
#	} ;
#    ok $@ =~ /^unshift is unsupported with Queue databases/ ;	
    $array[0] = "red" ;
    $array[1] = "green" ;
    $array[2] = "blue" ;
    $array[4] = 2 ;
    ok $array[0] eq fillout("red", $rec_len) ;
    ok $cursor->c_get($k, $v, DB_FIRST) == 0 ;
    ok $k == 0 ;
    ok $v eq fillout("red", $rec_len) ;
    ok $array[1] eq fillout("green", $rec_len) ;
    ok $cursor->c_get($k, $v, DB_NEXT) == 0 ;
    ok $k == 1 ;
    ok $v eq fillout("green", $rec_len) ;
    ok $array[2] eq fillout("blue", $rec_len) ;
    ok $cursor->c_get($k, $v, DB_NEXT) == 0 ;
    ok $k == 2 ;
    ok $v eq fillout("blue", $rec_len) ;
    ok $array[4] == 2 ;
    ok $cursor->c_get($k, $v, DB_NEXT) == 0 ;
    ok $k == 4 ;
    ok $v == 2 ;

    # shift
    ok (($FA ? shift @array : $db->shift()) eq fillout("red", $rec_len)) ;
    ok (($FA ? shift @array : $db->shift()) eq fillout("green", $rec_len)) ;
    ok (($FA ? shift @array : $db->shift()) eq fillout("blue", $rec_len)) ;
    ok (($FA ? shift @array : $db->shift()) == 2) ;

    # push
    $FA ? push @array, "the", "end" 
        : $db->push("the", "end") ;
    ok $cursor->c_get($k, $v, DB_LAST) == 0 ;
    ok $k == 102 ;
    ok $v eq fillout("end", $rec_len) ;
    ok $cursor->c_get($k, $v, DB_PREV) == 0 ;
    ok $k == 101 ;
    ok $v eq fillout("the", $rec_len) ;
    ok $cursor->c_get($k, $v, DB_PREV) == 0 ;
    ok $k == 100 ;
    ok $v == 200 ;

    # pop
    ok (( $FA ? pop @array : $db->pop ) eq fillout("end", $rec_len)) ;
    ok (( $FA ? pop @array : $db->pop ) eq fillout("the", $rec_len)) ;
    ok (( $FA ? pop @array : $db->pop ) == 200)  ;

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
    my $rec_len = 15 ;
    ok my $db = tie @array, 'BerkeleyDB::Queue',
				    	    -Len       => $rec_len;

    ok $db->db_put(1, "some value") == 0  ;
    ok $db->db_get(1, $value) == 0 ;
    ok $value eq fillout("some value", $rec_len) ;

}
 
{
    # partial
    # check works via API

    my $lex = new LexFile $Dfile ;
    my $value ;
    my $rec_len = 8 ;
    ok my $db = new BerkeleyDB::Queue  -Filename => $Dfile,
                                      	    -Flags    => DB_CREATE ,
				    	    -Len      => $rec_len,
				    	    -Pad      => " " ;

    # create some data
    my @data =  (
		"",
		"boat",
		"house",
		"sea",
		) ;

    my $ret = 0 ;
    my $i ;
    for ($i = 0 ; $i < @data ; ++$i) {
        my $r = $db->db_put($i, $data[$i]) ;
        $ret += $r ;
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
    ok $db->db_get(1, $value) == 0 && $value eq fillout("t", 2) ;
    ok $db->db_get(2, $value) == 0 && $value eq "se" ;
    ok $db->db_get(3, $value) == 0 && $value eq "  " ;

    # switch of partial mode
    ($pon, $off, $len) = $db->partial_clear() ;
    ok $pon ;
    ok $off == 3 ;
    ok $len == 2 ;
    ok $db->db_get(1, $value) == 0 && $value eq fillout("boat", $rec_len) ;
    ok $db->db_get(2, $value) == 0 && $value eq fillout("house", $rec_len) ;
    ok $db->db_get(3, $value) == 0 && $value eq fillout("sea", $rec_len) ;

    # now partial put
    $db->partial_set(0,2) ;
    ok $db->db_put(1, "") != 0 ;
    ok $db->db_put(2, "AB") == 0 ;
    ok $db->db_put(3, "XY") == 0 ;
    ok $db->db_put(4, "KLM") != 0 ;
    ok $db->db_put(4, "KL") == 0 ;

    ($pon, $off, $len) = $db->partial_clear() ;
    ok $pon ;
    ok $off == 0 ;
    ok $len == 2 ;
    ok $db->db_get(1, $value) == 0 && $value eq fillout("boat", $rec_len) ;
    ok $db->db_get(2, $value) == 0 && $value eq fillout("ABuse", $rec_len) ;
    ok $db->db_get(3, $value) == 0 && $value eq fillout("XYa", $rec_len) ;
    ok $db->db_get(4, $value) == 0 && $value eq fillout("KL", $rec_len) ;

    # now partial put
    ($pon, $off, $len) = $db->partial_set(3,2) ;
    ok ! $pon ;
    ok $off == 0 ;
    ok $len == 0 ;
    ok $db->db_put(1, "PP") == 0 ;
    ok $db->db_put(2, "Q") != 0 ;
    ok $db->db_put(3, "XY") == 0 ;
    ok $db->db_put(4, "TU") == 0 ;

    $db->partial_clear() ;
    ok $db->db_get(1, $value) == 0 && $value eq fillout("boaPP", $rec_len) ;
    ok $db->db_get(2, $value) == 0 && $value eq fillout("ABuse",$rec_len) ;
    ok $db->db_get(3, $value) == 0 && $value eq fillout("XYaXY", $rec_len) ;
    ok $db->db_get(4, $value) == 0 && $value eq fillout("KL TU", $rec_len) ;
}

{
    # partial
    # check works via tied array 

    my $lex = new LexFile $Dfile ;
    my @array ;
    my $value ;
    my $rec_len = 8 ;
    ok my $db = tie @array, 'BerkeleyDB::Queue', -Filename => $Dfile,
                                      	        -Flags    => DB_CREATE ,
				    	        -Len       => $rec_len,
				    	        -Pad       => " " ;

    # create some data
    my @data =  (
		"",
		"boat",
		"house",
		"sea",
		) ;

    my $i ;
    my $status = 0 ;
    for ($i = 1 ; $i < @data ; ++$i) {
	$array[$i] = $data[$i] ;
	$status += $db->status() ;
    }

    ok $status == 0 ;

    # do a partial get
    $db->partial_set(0,2) ;
    ok $array[1] eq fillout("bo", 2) ;
    ok $array[2] eq fillout("ho", 2) ;
    ok $array[3]  eq fillout("se", 2) ;

    # do a partial get, off end of data
    $db->partial_set(3,2) ;
    ok $array[1] eq fillout("t", 2) ;
    ok $array[2] eq fillout("se", 2) ;
    ok $array[3] eq fillout("", 2) ;

    # switch of partial mode
    $db->partial_clear() ;
    ok $array[1] eq fillout("boat", $rec_len) ;
    ok $array[2] eq fillout("house", $rec_len) ;
    ok $array[3] eq fillout("sea", $rec_len) ;

    # now partial put
    $db->partial_set(0,2) ;
    $array[1] = "" ;
    ok $db->status() != 0 ;
    $array[2] = "AB" ;
    ok $db->status() == 0 ;
    $array[3] = "XY" ;
    ok $db->status() == 0 ;
    $array[4] = "KL" ;
    ok $db->status() == 0 ;

    $db->partial_clear() ;
    ok $array[1] eq fillout("boat", $rec_len) ;
    ok $array[2] eq fillout("ABuse", $rec_len) ;
    ok $array[3] eq fillout("XYa", $rec_len) ;
    ok $array[4] eq fillout("KL", $rec_len) ;

    # now partial put
    $db->partial_set(3,2) ;
    $array[1] = "PP" ;
    ok $db->status() == 0 ;
    $array[2] = "Q" ;
    ok $db->status() != 0 ;
    $array[3] = "XY" ;
    ok $db->status() == 0 ;
    $array[4] = "TU" ;
    ok $db->status() == 0 ;

    $db->partial_clear() ;
    ok $array[1] eq fillout("boaPP", $rec_len) ;
    ok $array[2] eq fillout("ABuse", $rec_len) ;
    ok $array[3] eq fillout("XYaXY", $rec_len) ;
    ok $array[4] eq fillout("KL TU", $rec_len) ;
}

{
    # transaction

    my $lex = new LexFile $Dfile ;
    my @array ;
    my $value ;

    my $home = "./fred" ;
    ok my $lexD = new LexDir($home);
    my $rec_len = 9 ;
    ok my $env = new BerkeleyDB::Env -Home => $home,@StdErrFile,
				     -Flags => DB_CREATE|DB_INIT_TXN|
					  	DB_INIT_MPOOL|DB_INIT_LOCK ;
    ok my $txn = $env->txn_begin() ;
    ok my $db1 = tie @array, 'BerkeleyDB::Queue', 
				-Filename => $Dfile,
				-ArrayBase => 0,
                      		-Flags    =>  DB_CREATE ,
		        	-Env 	  => $env,
		        	-Txn	  => $txn ,
				-Len      => $rec_len,
				-Pad      => " " ;

    
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
    my $recs = ($BerkeleyDB::db_version >= 3.1 ? "qs_ndata" : "qs_nrecs") ;
    my @array ;
    my ($k, $v) ;
    my $rec_len = 7 ;
    ok my $db = new BerkeleyDB::Queue -Filename 	=> $Dfile, 
				     	   -Flags    	=> DB_CREATE,
					   -Pagesize	=> 4 * 1024,
				           -Len       => $rec_len,
				           -Pad       => " " 
					;

    my $ref = $db->db_stat() ; 
    ok $ref->{$recs} == 0;
    ok $ref->{'qs_pagesize'} == 4 * 1024;

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
   @ISA=qw(BerkeleyDB BerkeleyDB::Queue);
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

    use Test::More;
    BEGIN { push @INC, '.'; }    
    eval 'use SubDB ; ';
    ok $@ eq "" ;
    my @h ;
    my $X ;
    my $rec_len = 34 ;
    eval '
	$X = tie(@h, "SubDB", -Filename => "dbqueue.tmp", 
			-Flags => DB_CREATE,
			-Mode => 0640 ,
	                -Len       => $rec_len,
	                -Pad       => " " 
			);		   
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

    undef $X ;
    untie @h ;
    unlink "SubDB.pm", "dbqueue.tmp" ;

}

{
    # DB_APPEND

    my $lex = new LexFile $Dfile;
    my @array ;
    my $value ;
    my $rec_len = 21 ;
    ok my $db = tie @array, 'BerkeleyDB::Queue', 
					-Filename  => $Dfile,
                                       	-Flags     => DB_CREATE ,
	                		-Len       => $rec_len,
	                		-Pad       => " " ;

    # create a few records
    $array[1] = "def" ;
    $array[3] = "ghi" ;

    my $k = 0 ;
    ok $db->db_put($k, "fred", DB_APPEND) == 0 ;
    ok $k == 4 ;
    ok $array[4] eq fillout("fred", $rec_len) ;

    undef $db ;
    untie @array ;
}

{
    # 23 Sept 2001 -- push into an empty array
    my $lex = new LexFile $Dfile ;
    my @array ;
    my $db ;
    my $rec_len = 21 ;
    ok $db = tie @array, 'BerkeleyDB::Queue', 
                                      	       	-Flags  => DB_CREATE ,
				    	        -ArrayBase => 0,
	                		        -Len       => $rec_len,
	                		        -Pad       => " " ,
						-Filename => $Dfile ;
    $FA ? push @array, "first"
        : $db->push("first") ;

    ok (($FA ? pop @array : $db->pop()) eq fillout("first", $rec_len)) ;

    undef $db;
    untie @array ;

}

{
    # Tied Array interface with transactions

    my $lex = new LexFile $Dfile ;
    my @array ;
    my $db ;
    my $rec_len = 10 ;
    my $home = "./fred" ;
    ok my $lexD = new LexDir($home);
    ok my $env = new BerkeleyDB::Env -Home => $home,@StdErrFile,
				     -Flags => DB_CREATE|DB_INIT_TXN|
					  	DB_INIT_MPOOL|DB_INIT_LOCK ;
    ok my $txn = $env->txn_begin() ;
    ok $db = tie @array, 'BerkeleyDB::Queue', -Filename  => $Dfile,
				    	    -ArrayBase => 0,
                            -Flags     => DB_CREATE ,
                            -Env       => $env ,
                            -Txn       => $txn ,
				    	    -Len       => $rec_len;

    ok $txn->txn_commit() == 0 ;
    ok $txn = $env->txn_begin() ;
    $db->Txn($txn);

    ok my $cursor = (tied @array)->db_cursor() ;
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
    ok $array[1] eq fillout("some value", $rec_len);
    ok defined $array[1];
    ok ((tied @array)->status() == 0) ;
    ok !defined $array[3];
    ok ((tied @array)->status() == DB_NOTFOUND) ;

    $array[1] = 2 ;
    $array[10] = 20 ;
    $array[100] = 200 ;

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
    ok $keys == 111 ;
    ok $values == 222 ;

    # unshift isn't allowed
#    eval {
#    	$FA ? unshift @array, "red", "green", "blue" 
#        : $db->unshift("red", "green", "blue" ) ;
#	} ;
#    ok $@ =~ /^unshift is unsupported with Queue databases/ ;	
    $array[0] = "red" ;
    $array[1] = "green" ;
    $array[2] = "blue" ;
    $array[4] = 2 ;
    ok $array[0] eq fillout("red", $rec_len) ;
    ok $cursor->c_get($k, $v, DB_FIRST) == 0 ;
    ok $k == 0 ;
    ok $v eq fillout("red", $rec_len) ;
    ok $array[1] eq fillout("green", $rec_len) ;
    ok $cursor->c_get($k, $v, DB_NEXT) == 0 ;
    ok $k == 1 ;
    ok $v eq fillout("green", $rec_len) ;
    ok $array[2] eq fillout("blue", $rec_len) ;
    ok $cursor->c_get($k, $v, DB_NEXT) == 0 ;
    ok $k == 2 ;
    ok $v eq fillout("blue", $rec_len) ;
    ok $array[4] == 2 ;
    ok $cursor->c_get($k, $v, DB_NEXT) == 0 ;
    ok $k == 4 ;
    ok $v == 2 ;

    # shift
    ok (($FA ? shift @array : $db->shift()) eq fillout("red", $rec_len)) ;
    ok (($FA ? shift @array : $db->shift()) eq fillout("green", $rec_len)) ;
    ok (($FA ? shift @array : $db->shift()) eq fillout("blue", $rec_len)) ;
    ok (($FA ? shift @array : $db->shift()) == 2) ;

    # push
    $FA ? push @array, "the", "end" 
        : $db->push("the", "end") ;
    ok $cursor->c_get($k, $v, DB_LAST) == 0 ;
    ok $k == 102 ;
    ok $v eq fillout("end", $rec_len) ;
    ok $cursor->c_get($k, $v, DB_PREV) == 0 ;
    ok $k == 101 ;
    ok $v eq fillout("the", $rec_len) ;
    ok $cursor->c_get($k, $v, DB_PREV) == 0 ;
    ok $k == 100 ;
    ok $v == 200 ;

    # pop
    ok (( $FA ? pop @array : $db->pop ) eq fillout("end", $rec_len)) ;
    ok (( $FA ? pop @array : $db->pop ) eq fillout("the", $rec_len)) ;
    ok (( $FA ? pop @array : $db->pop ) == 200 ) ;

    # now clear the array 
    $FA ? @array = () 
        : $db->clear() ;
    ok $cursor->c_get($k, $v, DB_FIRST) == DB_NOTFOUND ;
    undef $cursor ;
    ok $txn->txn_commit() == 0 ;

    undef $db ;
    untie @array ;
}
__END__


# TODO
#
# DB_DELIMETER DB_FIXEDLEN DB_PAD DB_SNAPSHOT with partial records

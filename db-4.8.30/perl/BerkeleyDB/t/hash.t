#!./perl -w

# ID: %I%, %G%   

use strict ;

use lib 't' ;
use BerkeleyDB; 
use util ;
use Test::More;

plan tests =>  212;

my $Dfile = "dbhash.tmp";
my $Dfile2 = "dbhash2.tmp";
my $Dfile3 = "dbhash3.tmp";
unlink $Dfile;

umask(0) ;


# Check for invalid parameters
{
    # Check for invalid parameters
    my $db ;
    eval ' $db = new BerkeleyDB::Hash  -Stupid => 3 ; ' ;
    ok $@ =~ /unknown key value\(s\) Stupid/  ;

    eval ' $db = new BerkeleyDB::Hash -Bad => 2, -Mode => 0345, -Stupid => 3; ' ;
    ok $@ =~ /unknown key value\(s\) (Bad,? |Stupid,? ){2}/  ;

    eval ' $db = new BerkeleyDB::Hash -Env => 2 ' ;
    ok $@ =~ /^Env not of type BerkeleyDB::Env/ ;

    eval ' $db = new BerkeleyDB::Hash -Txn => "fred" ' ;
    ok $@ =~ /^Txn not of type BerkeleyDB::Txn/ ;

    my $obj = bless [], "main" ;
    eval ' $db = new BerkeleyDB::Hash -Env => $obj ' ;
    ok $@ =~ /^Env not of type BerkeleyDB::Env/ ;
}

# Now check the interface to HASH

{
    my $lex = new LexFile $Dfile ;

    ok my $db = new BerkeleyDB::Hash -Filename => $Dfile, 
				    -Flags    => DB_CREATE ;

    # Add a k/v pair
    my $value ;
    my $status ;
    ok $db->db_put("some key", "some value") == 0  ;
    ok $db->status() == 0 ;
    ok $db->db_get("some key", $value) == 0 ;
    ok $value eq "some value" ;
    ok $db->db_put("key", "value") == 0  ;
    ok $db->db_get("key", $value) == 0 ;
    ok $value eq "value" ;
    ok $db->db_del("some key") == 0 ;
    ok (($status = $db->db_get("some key", $value)) == DB_NOTFOUND) ;
    ok $status eq $DB_errors{'DB_NOTFOUND'} ;
    ok $db->status() == DB_NOTFOUND ;
    ok $db->status() eq $DB_errors{'DB_NOTFOUND'};

    ok $db->db_sync() == 0 ;

    # Check NOOVERWRITE will make put fail when attempting to overwrite
    # an existing record.

    ok $db->db_put( 'key', 'x', DB_NOOVERWRITE) == DB_KEYEXIST ;
    ok $db->status() eq $DB_errors{'DB_KEYEXIST'};
    ok $db->status() == DB_KEYEXIST ;

    # check that the value of the key  has not been changed by the
    # previous test
    ok $db->db_get("key", $value) == 0 ;
    ok $value eq "value" ;

    # test DB_GET_BOTH
    my ($k, $v) = ("key", "value") ;
    ok $db->db_get($k, $v, DB_GET_BOTH) == 0 ;

    ($k, $v) = ("key", "fred") ;
    ok $db->db_get($k, $v, DB_GET_BOTH) == DB_NOTFOUND ;

    ($k, $v) = ("another", "value") ;
    ok $db->db_get($k, $v, DB_GET_BOTH) == DB_NOTFOUND ;


}

{
    # Check simple env works with a hash.
    my $lex = new LexFile $Dfile ;

    my $home = "./fred" ;
    ok my $lexD = new LexDir($home);

    ok my $env = new BerkeleyDB::Env -Flags => DB_CREATE| DB_INIT_MPOOL,@StdErrFile,
    					 -Home  => $home ;
    ok my $db = new BerkeleyDB::Hash -Filename => $Dfile, 
				    -Env      => $env,
				    -Flags    => DB_CREATE ;

    # Add a k/v pair
    my $value ;
    ok $db->db_put("some key", "some value") == 0 ;
    ok $db->db_get("some key", $value) == 0 ;
    ok $value eq "some value" ;
    undef $db ;
    undef $env ;
}


{
    # override default hash
    my $lex = new LexFile $Dfile ;
    my $value ;
    $::count = 0 ;
    ok my $db = new BerkeleyDB::Hash -Filename => $Dfile, 
				     -Hash     => sub {  ++$::count ; length $_[0] },
				     -Flags    => DB_CREATE ;

    ok $db->db_put("some key", "some value") == 0 ;
    ok $db->db_get("some key", $value) == 0 ;
    ok $value eq "some value" ;
    ok $::count > 0 ;

}
 
{
    # cursors

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

    # create the cursor
    ok my $cursor = $db->db_cursor() ;

    $k = $v = "" ;
    my %copy = %data ;
    my $extras = 0 ;
    # sequence forwards
    while ($cursor->c_get($k, $v, DB_NEXT) == 0) {
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

    ($k, $v) = ("green", "house") ;
    ok $cursor->c_get($k, $v, DB_GET_BOTH) == 0 ;

    ($k, $v) = ("green", "door") ;
    ok $cursor->c_get($k, $v, DB_GET_BOTH) == DB_NOTFOUND ;

    ($k, $v) = ("black", "house") ;
    ok $cursor->c_get($k, $v, DB_GET_BOTH) == DB_NOTFOUND ;
    
}
 
{
    # Tied Hash interface

    my $lex = new LexFile $Dfile ;
    my %hash ;
    ok tie %hash, 'BerkeleyDB::Hash', -Filename => $Dfile,
                                      -Flags    => DB_CREATE ;

    # check "each" with an empty database
    my $count = 0 ;
    while (my ($k, $v) = each %hash) {
	++ $count ;
    }
    ok ((tied %hash)->status() == DB_NOTFOUND) ;
    ok $count == 0 ;

    # Add a k/v pair
    my $value ;
    $hash{"some key"} = "some value";
    ok ((tied %hash)->status() == 0) ;
    ok $hash{"some key"} eq "some value";
    ok defined $hash{"some key"} ;
    ok ((tied %hash)->status() == 0) ;
    ok exists $hash{"some key"} ;
    ok !defined $hash{"jimmy"} ;
    ok ((tied %hash)->status() == DB_NOTFOUND) ;
    ok !exists $hash{"jimmy"} ;
    ok ((tied %hash)->status() == DB_NOTFOUND) ;

    delete $hash{"some key"} ;
    ok ((tied %hash)->status() == 0) ;
    ok ! defined $hash{"some key"} ;
    ok ((tied %hash)->status() == DB_NOTFOUND) ;
    ok ! exists $hash{"some key"} ;
    ok ((tied %hash)->status() == DB_NOTFOUND) ;

    $hash{1} = 2 ;
    $hash{10} = 20 ;
    $hash{1000} = 2000 ;

    my ($keys, $values) = (0,0);
    $count = 0 ;
    while (my ($k, $v) = each %hash) {
        $keys += $k ;
	$values += $v ;
	++ $count ;
    }
    ok $count == 3 ;
    ok $keys == 1011 ;
    ok $values == 2022 ;

    # now clear the hash
    %hash = () ;
    ok keys %hash == 0 ;

    untie %hash ;
}

{
    # in-memory file

    my $lex = new LexFile $Dfile ;
    my %hash ;
    my $fd ;
    my $value ;
    ok my $db = tie %hash, 'BerkeleyDB::Hash'
        or die $BerkeleyDB::Error;

    ok $db->db_put("some key", "some value") == 0  ;
    ok $db->db_get("some key", $value) == 0 ;
    ok $value eq "some value" ;

    undef $db ;
    untie %hash ;
}
 
{
    # partial
    # check works via API

    my $lex = new LexFile $Dfile ;
    my %hash ;
    my $value ;
    ok my $db = tie %hash, 'BerkeleyDB::Hash', -Filename => $Dfile,
                                      	       -Flags    => DB_CREATE ;

    # create some data
    my %data =  (
		"red"	=> "boat",
		"green"	=> "house",
		"blue"	=> "sea",
		) ;

    my $ret = 0 ;
    while (my ($k, $v) = each %data) {
        $ret += $db->db_put($k, $v) ;
    }
    ok $ret == 0 ;


    # do a partial get
    my($pon, $off, $len) = $db->partial_set(0,2) ;
    ok $pon == 0 && $off == 0 && $len == 0 ;
    ok (( $db->db_get("red", $value) == 0) && $value eq "bo") ;
    ok (( $db->db_get("green", $value) == 0) && $value eq "ho") ;
    ok (( $db->db_get("blue", $value) == 0) && $value eq "se") ;

    # do a partial get, off end of data
    ($pon, $off, $len) = $db->partial_set(3,2) ;
    ok $pon ;
    ok $off == 0 ;
    ok $len == 2 ;
    ok $db->db_get("red", $value) == 0 && $value eq "t" ;
    ok $db->db_get("green", $value) == 0 && $value eq "se" ;
    ok $db->db_get("blue", $value) == 0 && $value eq "" ;

    # switch of partial mode
    ($pon, $off, $len) = $db->partial_clear() ;
    ok $pon ;
    ok $off == 3 ;
    ok $len == 2 ;
    ok $db->db_get("red", $value) == 0 && $value eq "boat" ;
    ok $db->db_get("green", $value) == 0 && $value eq "house" ;
    ok $db->db_get("blue", $value) == 0 && $value eq "sea" ;

    # now partial put
    ($pon, $off, $len) = $db->partial_set(0,2) ;
    ok ! $pon ;
    ok $off == 0 ;
    ok $len == 0 ;
    ok $db->db_put("red", "") == 0 ;
    ok $db->db_put("green", "AB") == 0 ;
    ok $db->db_put("blue", "XYZ") == 0 ;
    ok $db->db_put("new", "KLM") == 0 ;

    $db->partial_clear() ;
    ok $db->db_get("red", $value) == 0 && $value eq "at" ;
    ok $db->db_get("green", $value) == 0 && $value eq "ABuse" ;
    ok $db->db_get("blue", $value) == 0 && $value eq "XYZa" ;
    ok $db->db_get("new", $value) == 0 && $value eq "KLM" ;

    # now partial put
    $db->partial_set(3,2) ;
    ok $db->db_put("red", "PPP") == 0 ;
    ok $db->db_put("green", "Q") == 0 ;
    ok $db->db_put("blue", "XYZ") == 0 ;
    ok $db->db_put("new", "--") == 0 ;

    ($pon, $off, $len) = $db->partial_clear() ;
    ok $pon ;
    ok $off == 3 ;
    ok $len == 2 ;
    ok $db->db_get("red", $value) == 0 && $value eq "at\0PPP" ;
    ok $db->db_get("green", $value) == 0 && $value eq "ABuQ" ;
    ok $db->db_get("blue", $value) == 0 && $value eq "XYZXYZ" ;
    ok $db->db_get("new", $value) == 0 && $value eq "KLM--" ;
}

{
    # partial
    # check works via tied hash 

    my $lex = new LexFile $Dfile ;
    my %hash ;
    my $value ;
    ok my $db = tie %hash, 'BerkeleyDB::Hash', -Filename => $Dfile,
                                      	       -Flags    => DB_CREATE ;

    # create some data
    my %data =  (
		"red"	=> "boat",
		"green"	=> "house",
		"blue"	=> "sea",
		) ;

    while (my ($k, $v) = each %data) {
	$hash{$k} = $v ;
    }


    # do a partial get
    $db->partial_set(0,2) ;
    ok $hash{"red"} eq "bo" ;
    ok $hash{"green"} eq "ho" ;
    ok $hash{"blue"}  eq "se" ;

    # do a partial get, off end of data
    $db->partial_set(3,2) ;
    ok $hash{"red"} eq "t" ;
    ok $hash{"green"} eq "se" ;
    ok $hash{"blue"} eq "" ;

    # switch of partial mode
    $db->partial_clear() ;
    ok $hash{"red"} eq "boat" ;
    ok $hash{"green"} eq "house" ;
    ok $hash{"blue"} eq "sea" ;

    # now partial put
    $db->partial_set(0,2) ;
    ok $hash{"red"} = "" ;
    ok $hash{"green"} = "AB" ;
    ok $hash{"blue"} = "XYZ" ;
    ok $hash{"new"} = "KLM" ;

    $db->partial_clear() ;
    ok $hash{"red"} eq "at" ;
    ok $hash{"green"} eq "ABuse" ;
    ok $hash{"blue"} eq "XYZa" ;
    ok $hash{"new"} eq "KLM" ;

    # now partial put
    $db->partial_set(3,2) ;
    ok $hash{"red"} = "PPP" ;
    ok $hash{"green"} = "Q" ;
    ok $hash{"blue"} = "XYZ" ;
    ok $hash{"new"} = "TU" ;

    $db->partial_clear() ;
    ok $hash{"red"} eq "at\0PPP" ;
    ok $hash{"green"} eq "ABuQ" ;
    ok $hash{"blue"} eq "XYZXYZ" ;
    ok $hash{"new"} eq "KLMTU" ;
}

{
    # transaction

    my $lex = new LexFile $Dfile ;
    my %hash ;
    my $value ;

    my $home = "./fred" ;
    ok my $lexD = new LexDir($home);
    ok my $env = new BerkeleyDB::Env -Home => $home,@StdErrFile,
				     -Flags => DB_CREATE|DB_INIT_TXN|
					  	DB_INIT_MPOOL|DB_INIT_LOCK ;
    ok my $txn = $env->txn_begin() ;
    ok my $db1 = tie %hash, 'BerkeleyDB::Hash', -Filename => $Dfile,
                                      	       	-Flags     => DB_CREATE ,
					       	-Env 	   => $env,
					    	-Txn	   => $txn  ;

    
    ok $txn->txn_commit() == 0 ;
    ok $txn = $env->txn_begin() ;
    $db1->Txn($txn);
    # create some data
    my %data =  (
		"red"	=> "boat",
		"green"	=> "house",
		"blue"	=> "sea",
		) ;

    my $ret = 0 ;
    while (my ($k, $v) = each %data) {
        $ret += $db1->db_put($k, $v) ;
    }
    ok $ret == 0 ;

    # should be able to see all the records

    ok my $cursor = $db1->db_cursor() ;
    my ($k, $v) = ("", "") ;
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
    untie %hash ;
}


{
    # DB_DUP

    my $lex = new LexFile $Dfile ;
    my %hash ;
    ok my $db = tie %hash, 'BerkeleyDB::Hash', -Filename => $Dfile,
				      -Property  => DB_DUP,
                                      -Flags    => DB_CREATE ;

    $hash{'Wall'} = 'Larry' ;
    $hash{'Wall'} = 'Stone' ;
    $hash{'Smith'} = 'John' ;
    $hash{'Wall'} = 'Brick' ;
    $hash{'Wall'} = 'Brick' ;
    $hash{'mouse'} = 'mickey' ;

    ok keys %hash == 6 ;

    # create a cursor
    ok my $cursor = $db->db_cursor() ;

    my $key = "Wall" ;
    my $value ;
    ok $cursor->c_get($key, $value, DB_SET) == 0 ;
    ok $key eq "Wall" && $value eq "Larry" ;
    ok $cursor->c_get($key, $value, DB_NEXT) == 0 ;
    ok $key eq "Wall" && $value eq "Stone" ;
    ok $cursor->c_get($key, $value, DB_NEXT) == 0 ;
    ok $key eq "Wall" && $value eq "Brick" ;
    ok $cursor->c_get($key, $value, DB_NEXT) == 0 ;
    ok $key eq "Wall" && $value eq "Brick" ;

    #my $ref = $db->db_stat() ; 
    #ok $ref->{bt_flags} | DB_DUP ;

    # test DB_DUP_NEXT
    my ($k, $v) = ("Wall", "") ;
    ok $cursor->c_get($k, $v, DB_SET) == 0 ;
    ok $k eq "Wall" && $v eq "Larry" ;
    ok $cursor->c_get($k, $v, DB_NEXT_DUP) == 0 ;
    ok $k eq "Wall" && $v eq "Stone" ;
    ok $cursor->c_get($k, $v, DB_NEXT_DUP) == 0 ;
    ok $k eq "Wall" && $v eq "Brick" ;
    ok $cursor->c_get($k, $v, DB_NEXT_DUP) == 0 ;
    ok $k eq "Wall" && $v eq "Brick" ;
    ok $cursor->c_get($k, $v, DB_NEXT_DUP) == DB_NOTFOUND ;
    

    undef $db ;
    undef $cursor ;
    untie %hash ;

}

{
    # DB_DUP & DupCompare
    my $lex = new LexFile $Dfile, $Dfile2;
    my ($key, $value) ;
    my (%h, %g) ;
    my @Keys   = qw( 0123 9 12 -1234 9 987654321 9 def  ) ; 
    my @Values = qw( 1    11 3   dd   x abc      2 0    ) ; 

    ok tie %h, "BerkeleyDB::Hash", -Filename => $Dfile, 
				     -DupCompare   => sub { $_[0] cmp $_[1] },
				     -Property  => DB_DUP|DB_DUPSORT,
				     -Flags    => DB_CREATE ;

    ok tie %g, 'BerkeleyDB::Hash', -Filename => $Dfile2, 
				     -DupCompare   => sub { $_[0] <=> $_[1] },
				     -Property  => DB_DUP|DB_DUPSORT,
				     -Flags    => DB_CREATE ;

    foreach (@Keys) {
        local $^W = 0 ;
	my $value = shift @Values ;
        $h{$_} = $value ; 
        $g{$_} = $value ;
    }

    ok my $cursor = (tied %h)->db_cursor() ;
    $key = 9 ; $value = "";
    ok $cursor->c_get($key, $value, DB_SET) == 0 ;
    ok $key == 9 && $value eq 11 ;
    ok $cursor->c_get($key, $value, DB_NEXT) == 0 ;
    ok $key == 9 && $value == 2 ;
    ok $cursor->c_get($key, $value, DB_NEXT) == 0 ;
    ok $key == 9 && $value eq "x" ;

    $cursor = (tied %g)->db_cursor() ;
    $key = 9 ;
    ok $cursor->c_get($key, $value, DB_SET) == 0 ;
    ok $key == 9 && $value eq "x" ;
    ok $cursor->c_get($key, $value, DB_NEXT) == 0 ;
    ok $key == 9 && $value == 2 ;
    ok $cursor->c_get($key, $value, DB_NEXT) == 0 ;
    ok $key == 9 && $value  == 11 ;


}

{
    # get_dup etc
    my $lex = new LexFile $Dfile;
    my %hh ;

    ok my $YY = tie %hh, "BerkeleyDB::Hash", -Filename => $Dfile, 
				     -DupCompare   => sub { $_[0] cmp $_[1] },
				     -Property  => DB_DUP,
				     -Flags    => DB_CREATE ;

    $hh{'Wall'} = 'Larry' ;
    $hh{'Wall'} = 'Stone' ; # Note the duplicate key
    $hh{'Wall'} = 'Brick' ; # Note the duplicate key
    $hh{'Smith'} = 'John' ;
    $hh{'mouse'} = 'mickey' ;
    
    # first work in scalar context
    ok scalar $YY->get_dup('Unknown') == 0 ;
    ok scalar $YY->get_dup('Smith') == 1 ;
    ok scalar $YY->get_dup('Wall') == 3 ;
    
    # now in list context
    my @unknown = $YY->get_dup('Unknown') ;
    ok "@unknown" eq "" ;
    
    my @smith = $YY->get_dup('Smith') ;
    ok "@smith" eq "John" ;
    
    {
        my @wall = $YY->get_dup('Wall') ;
        my %wall ;
        @wall{@wall} = @wall ;
        ok (@wall == 3 && $wall{'Larry'} 
			&& $wall{'Stone'} && $wall{'Brick'});
    }
    
    # hash
    my %unknown = $YY->get_dup('Unknown', 1) ;
    ok keys %unknown == 0 ;
    
    my %smith = $YY->get_dup('Smith', 1) ;
    ok keys %smith == 1 && $smith{'John'} ;
    
    my %wall = $YY->get_dup('Wall', 1) ;
    ok keys %wall == 3 && $wall{'Larry'} == 1 && $wall{'Stone'} == 1 
    		&& $wall{'Brick'} == 1 ;
    
    undef $YY ;
    untie %hh ;

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
   @ISA=qw(BerkeleyDB BerkeleyDB::Hash);
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
    my %h ;
    my $X ;
    eval '
	$X = tie(%h, "SubDB", -Filename => "dbhash.tmp", 
			-Flags => DB_CREATE,
			-Mode => 0640 );
	' ;

    ok $@ eq "" ;

    my $ret = eval '$h{"fred"} = 3 ; return $h{"fred"} ' ;
    ok $@ eq "" ;
    ok $ret == 7 ;

    my $value = 0;
    $ret = eval '$X->db_put("joe", 4) ; $X->db_get("joe", $value) ; return $value' ;
    ok $@ eq "" ;
    ok $ret == 10 ;

    $ret = eval ' DB_NEXT eq main::DB_NEXT ' ;
    ok $@ eq ""  ;
    ok $ret == 1 ;

    $ret = eval '$X->A_new_method("joe") ' ;
    ok $@ eq "" ;
    ok $ret eq "[[10]]" ;

    unlink "SubDB.pm", "dbhash.tmp" ;

}

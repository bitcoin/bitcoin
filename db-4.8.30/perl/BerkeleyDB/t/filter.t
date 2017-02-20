#!./perl -w

# ID: %I%, %G%   

use strict ;

use lib 't' ;
use BerkeleyDB; 
use util ;
use Test::More;

plan tests => 52;

my $Dfile = "dbhash.tmp";
unlink $Dfile;

umask(0) ;


{
   # DBM Filter tests
   use strict ;
   my (%h, $db) ;
   my ($fetch_key, $store_key, $fetch_value, $store_value) = ("") x 4 ;
   unlink $Dfile;

   sub checkOutput
   {
       my($fk, $sk, $fv, $sv) = @_ ;
       return
           $fetch_key eq $fk && $store_key eq $sk && 
	   $fetch_value eq $fv && $store_value eq $sv &&
	   $_ eq 'original' ;
   }
   
    ok $db = tie %h, 'BerkeleyDB::Hash', 
    		-Filename   => $Dfile, 
	        -Flags      => DB_CREATE; 

   $db->filter_fetch_key   (sub { $fetch_key = $_ }) ;
   $db->filter_store_key   (sub { $store_key = $_ }) ;
   $db->filter_fetch_value (sub { $fetch_value = $_}) ;
   $db->filter_store_value (sub { $store_value = $_ }) ;

   $_ = "original" ;

   $h{"fred"} = "joe" ;
   #                   fk   sk     fv   sv
   ok checkOutput( "", "fred", "", "joe") ;

   ($fetch_key, $store_key, $fetch_value, $store_value) = ("") x 4 ;
   ok $h{"fred"} eq "joe";
   #                   fk    sk     fv    sv
   ok checkOutput( "", "fred", "joe", "") ;

   ($fetch_key, $store_key, $fetch_value, $store_value) = ("") x 4 ;
   ok $db->FIRSTKEY() eq "fred" ;
   #                    fk     sk  fv  sv
   ok checkOutput( "fred", "", "", "") ;

   # replace the filters, but remember the previous set
   my ($old_fk) = $db->filter_fetch_key   
   			(sub { $_ = uc $_ ; $fetch_key = $_ }) ;
   my ($old_sk) = $db->filter_store_key   
   			(sub { $_ = lc $_ ; $store_key = $_ }) ;
   my ($old_fv) = $db->filter_fetch_value 
   			(sub { $_ = "[$_]"; $fetch_value = $_ }) ;
   my ($old_sv) = $db->filter_store_value 
   			(sub { s/o/x/g; $store_value = $_ }) ;
   
   ($fetch_key, $store_key, $fetch_value, $store_value) = ("") x 4 ;
   $h{"Fred"} = "Joe" ;
   #                   fk   sk     fv    sv
   ok checkOutput( "", "fred", "", "Jxe") ;

   ($fetch_key, $store_key, $fetch_value, $store_value) = ("") x 4 ;
   ok $h{"Fred"} eq "[Jxe]";
   print "$h{'Fred'}\n";
   #                   fk   sk     fv    sv
   ok checkOutput( "", "fred", "[Jxe]", "") ;

   ($fetch_key, $store_key, $fetch_value, $store_value) = ("") x 4 ;
   ok $db->FIRSTKEY() eq "FRED" ;
   #                   fk   sk     fv    sv
   ok checkOutput( "FRED", "", "", "") ;

   # put the original filters back
   $db->filter_fetch_key   ($old_fk);
   $db->filter_store_key   ($old_sk);
   $db->filter_fetch_value ($old_fv);
   $db->filter_store_value ($old_sv);

   ($fetch_key, $store_key, $fetch_value, $store_value) = ("") x 4 ;
   $h{"fred"} = "joe" ;
   ok checkOutput( "", "fred", "", "joe") ;

   ($fetch_key, $store_key, $fetch_value, $store_value) = ("") x 4 ;
   ok $h{"fred"} eq "joe";
   ok checkOutput( "", "fred", "joe", "") ;

   ($fetch_key, $store_key, $fetch_value, $store_value) = ("") x 4 ;
   ok $db->FIRSTKEY() eq "fred" ;
   ok checkOutput( "fred", "", "", "") ;

   # delete the filters
   $db->filter_fetch_key   (undef);
   $db->filter_store_key   (undef);
   $db->filter_fetch_value (undef);
   $db->filter_store_value (undef);

   ($fetch_key, $store_key, $fetch_value, $store_value) = ("") x 4 ;
   $h{"fred"} = "joe" ;
   ok checkOutput( "", "", "", "") ;

   ($fetch_key, $store_key, $fetch_value, $store_value) = ("") x 4 ;
   ok $h{"fred"} eq "joe";
   ok checkOutput( "", "", "", "") ;

   ($fetch_key, $store_key, $fetch_value, $store_value) = ("") x 4 ;
   ok $db->FIRSTKEY() eq "fred" ;
   ok checkOutput( "", "", "", "") ;

   undef $db ;
   untie %h;
   unlink $Dfile;
}

{    
    # DBM Filter with a closure

    use strict ;
    my (%h, $db) ;

    unlink $Dfile;
    ok $db = tie %h, 'BerkeleyDB::Hash', 
    		-Filename   => $Dfile, 
	        -Flags      => DB_CREATE; 

    my %result = () ;

    sub Closure
    {
        my ($name) = @_ ;
	my $count = 0 ;
	my @kept = () ;

	return sub { ++$count ; 
		     push @kept, $_ ; 
		     $result{$name} = "$name - $count: [@kept]" ;
		   }
    }

    $db->filter_store_key(Closure("store key"))  ;
    $db->filter_store_value(Closure("store value")) ;
    $db->filter_fetch_key(Closure("fetch key")) ;
    $db->filter_fetch_value(Closure("fetch value")) ;

    $_ = "original" ;

    $h{"fred"} = "joe" ;
    ok $result{"store key"} eq "store key - 1: [fred]" ;
    ok $result{"store value"} eq "store value - 1: [joe]" ;
    ok ! defined $result{"fetch key"}  ;
    ok ! defined $result{"fetch value"}  ;
    ok $_ eq "original"  ;

    ok $db->FIRSTKEY() eq "fred"  ;
    ok $result{"store key"} eq "store key - 1: [fred]" ;
    ok $result{"store value"} eq "store value - 1: [joe]" ;
    ok $result{"fetch key"} eq "fetch key - 1: [fred]" ;
    ok ! defined $result{"fetch value"}  ;
    ok $_ eq "original"  ;

    $h{"jim"}  = "john" ;
    ok $result{"store key"} eq "store key - 2: [fred jim]" ;
    ok $result{"store value"} eq "store value - 2: [joe john]" ;
    ok $result{"fetch key"} eq "fetch key - 1: [fred]" ;
    ok ! defined $result{"fetch value"}  ;
    ok $_ eq "original"  ;

    ok $h{"fred"} eq "joe" ;
    ok $result{"store key"} eq "store key - 3: [fred jim fred]" ;
    ok $result{"store value"} eq "store value - 2: [joe john]" ;
    ok $result{"fetch key"} eq "fetch key - 1: [fred]" ;
    ok $result{"fetch value"} eq "fetch value - 1: [joe]" ;
    ok $_ eq "original" ;

    undef $db ;
    untie %h;
    unlink $Dfile;
}		

{
   # DBM Filter recursion detection
   use strict ;
   my (%h, $db) ;
   unlink $Dfile;

    ok $db = tie %h, 'BerkeleyDB::Hash', 
    		-Filename   => $Dfile, 
	        -Flags      => DB_CREATE; 

   $db->filter_store_key (sub { $_ = $h{$_} }) ;

   eval '$h{1} = 1234' ;
   ok $@ =~ /^recursion detected in filter_store_key at/ ;
   
   undef $db ;
   untie %h;
   unlink $Dfile;
}

{
   # Check that DBM Filter can cope with read-only $_

   #use warnings ;
   use strict ;
   my (%h, $db) ;
   unlink $Dfile;

   ok $db = tie %h, 'BerkeleyDB::Hash', 
    		-Filename   => $Dfile, 
	        -Flags      => DB_CREATE; 

   $db->filter_fetch_key   (sub { }) ;
   $db->filter_store_key   (sub { }) ;
   $db->filter_fetch_value (sub { }) ;
   $db->filter_store_value (sub { }) ;

   $_ = "original" ;

   $h{"fred"} = "joe" ;
   ok($h{"fred"} eq "joe");

   eval { grep { $h{$_} } (1, 2, 3) };
   ok (! $@);


   # delete the filters
   $db->filter_fetch_key   (undef);
   $db->filter_store_key   (undef);
   $db->filter_fetch_value (undef);
   $db->filter_store_value (undef);

   $h{"fred"} = "joe" ;

   ok($h{"fred"} eq "joe");

   ok($db->FIRSTKEY() eq "fred") ;
   
   eval { grep { $h{$_} } (1, 2, 3) };
   ok (! $@);

   undef $db ;
   untie %h;
   unlink $Dfile;
}

if(0)
{
    # Filter without tie
    use strict ;
    my (%h, $db) ;

    unlink $Dfile;
    ok $db = tie %h, 'BerkeleyDB::Hash', 
    		-Filename   => $Dfile, 
	        -Flags      => DB_CREATE; 

    my %result = () ;

    sub INC { return ++ $_[0] }
    sub DEC { return -- $_[0] }
    #$db->filter_fetch_key   (sub { warn "FFK $_\n"; $_ = INC($_); warn "XX\n" }) ;
    #$db->filter_store_key   (sub { warn "FSK $_\n"; $_ = DEC($_); warn "XX\n" }) ;
    #$db->filter_fetch_value (sub { warn "FFV $_\n"; $_ = INC($_); warn "XX\n"}) ;
    #$db->filter_store_value (sub { warn "FSV $_\n"; $_ = DEC($_); warn "XX\n" }) ;

    $db->filter_fetch_key   (sub { warn "FFK $_\n"; $_ = pack("i", $_); warn "XX\n" }) ;
    $db->filter_store_key   (sub { warn "FSK $_\n"; $_ = unpack("i", $_); warn "XX\n" }) ;
    $db->filter_fetch_value (sub { warn "FFV $_\n"; $_ = pack("i", $_); warn "XX\n"}) ;
    #$db->filter_store_value (sub { warn "FSV $_\n"; $_ = unpack("i", $_); warn "XX\n" }) ;

    #$db->filter_fetch_key   (sub { ++ $_ }) ;
    #$db->filter_store_key   (sub { -- $_ }) ;
    #$db->filter_fetch_value (sub { ++ $_ }) ;
    #$db->filter_store_value (sub { -- $_ }) ;

    my ($k, $v) = (0,0);
    ok ! $db->db_put(3,5);
    exit;
    ok ! $db->db_get(3, $v);
    ok $v == 5 ;

    $h{4} = 7 ;
    ok $h{4} == 7;

    $k = 10;
    $v = 30;
    $h{$k} = $v ;
    ok $k == 10;
    ok $v == 30;
    ok $h{$k} == 30;

    $k = 3;
    ok ! $db->db_get($k, $v, DB_GET_BOTH);
    ok $k == 3 ;
    ok $v == 5 ;

    my $cursor = $db->db_cursor();

    my %tmp = ();
    while ($cursor->c_get($k, $v, DB_NEXT) == 0)
    {
	$tmp{$k} = $v;
    }

    ok keys %tmp == 3 ;
    ok $tmp{3} == 5;

    undef $cursor ;
    undef $db ;
    untie %h;
    unlink $Dfile;
}


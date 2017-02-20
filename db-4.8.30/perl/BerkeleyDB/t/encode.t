#!./perl -w

use strict ;

use lib 't' ;
use BerkeleyDB; 
use util ;
use Test::More ;

BEGIN 
{
    eval { require Encode; };
    
    plan skip_all => "Encode is not available"
        if $@;

    plan tests => 8;

    use_ok('charnames', qw{greek});
}


use charnames qw{greek};


my $Dfile = "dbhash.tmp";
unlink $Dfile;

umask(0) ;

{
    # UTF8
    #

   #use warnings ;
   use strict ;
   my (%h, $db) ;
   unlink $Dfile;

   ok $db = tie %h, 'BerkeleyDB::Hash', 
    		-Filename   => $Dfile, 
	        -Flags      => DB_CREATE; 

   $db->filter_fetch_key   (sub { $_ = Encode::decode_utf8($_) if defined $_ });
   $db->filter_store_key   (sub { $_ = Encode::encode_utf8($_) if defined $_ });
   $db->filter_fetch_value (sub { $_ = Encode::decode_utf8($_) if defined $_ });
   $db->filter_store_value (sub { $_ = Encode::encode_utf8($_) if defined $_ });

   $h{"\N{alpha}"} = "alpha";
   $h{"gamma"} = "\N{gamma}";

   is $h{"\N{alpha}"}, "alpha";
   is $h{"gamma"}, "\N{gamma}";

   undef $db ;
   untie %h;

   my %newH;
   ok $db = tie %newH, 'BerkeleyDB::Hash', 
    		-Filename   => $Dfile, 
	        -Flags      => DB_CREATE; 

   $newH{"fred"} = "joe" ;
   is $newH{"fred"}, "joe";

   is $newH{"gamma"}, "\xCE\xB3";
   is $newH{"\xCE\xB1"}, "alpha";

   undef $db ;
   untie %newH;
   unlink $Dfile;
}

#!./perl -w

use strict ;

use lib 't' ;
use BerkeleyDB; 
use Test::More ;
use util ;

plan(skip_all => "this needs Berkeley DB 4.4.x or better\n" )
    if $BerkeleyDB::db_version < 4.4;

plan tests => 5;

{
    title "Testing compact";

    # db->db_compact

    my $Dfile;
    my $lex = new LexFile $Dfile ;
    my ($k, $v) ;
    ok my $db = new BerkeleyDB::Btree -Filename => $Dfile, 
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
    ok $ret == 0, "  Created some data" ;

    my $key;
    my $end;
    my %hash;
    $hash{compact_filepercent} = 20;

    ok $db->compact("red", "green", \%hash, 0, $end) == 0, "  Compacted ok";

    if (0)
    {
        diag "end at $end";
        for my $key (sort keys %hash)
        {
            diag "[$key][$hash{$key}]\n";
        }
    }

    ok $db->compact() == 0, "  Compacted ok";
}


#!./perl -w

use strict ;
use lib 't';
use BerkeleyDB; 
use Test::More;
use util ;

plan(skip_all => "this needs Berkeley DB 4.x.x or better\n" )
    if $BerkeleyDB::db_version < 4;


plan tests => 9;

my $Dfile = "dbhash.tmp";
unlink $Dfile;

umask(0) ;

my $db = BerkeleyDB::Btree->new(
	-Filename => $Dfile,
	-Flags    => DB_CREATE,
	-Property => DB_DUP | DB_DUPSORT
) || die "Cannot open file $Dfile: $! $BerkeleyDB::Error\n" ;

my $cursor = $db->db_cursor();

my @pairs = qw(
	Alabama/Athens
	Alabama/Florence
	Alaska/Anchorage
	Alaska/Fairbanks
	Arizona/Avondale
	Arizona/Florence
);

for (@pairs) {
	$db->db_put(split '/');
}

my @tests = (
	["Alaska", "Fa", "Alaska", "Fairbanks"],
	["Arizona", "Fl", "Arizona", "Florence"],
	["Alaska", "An", "Alaska", "Anchorage"],
);

#my $i;
while (my $test = shift @tests) {
	my ($k1, $v1, $k2, $v2) = @$test;
	ok $cursor->c_get($k1, $v1, DB_GET_BOTH_RANGE) ==  0;
	is $k1, $k2;
	is $v1, $v2;
}

undef $db;
unlink $Dfile;

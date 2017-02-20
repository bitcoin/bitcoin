#!/usr/bin/perl -w

use strict ;

use lib 't';
use Test::More ;

BEGIN 
{
	plan skip_all => "this is Perl $], skipping test\n"
        if $] < 5.005 ;

    eval { require Data::Dumper ; };
    if ($@) {
        plan skip_all =>  "Data::Dumper is not installed on this system.\n";
    }
    {
        local ($^W) = 0 ;
        if ($Data::Dumper::VERSION < 2.08) {
            plan skip_all =>  "Data::Dumper 2.08 or better required (found $Data::Dumper::VERSION).\n";
    }
    }
    eval { require MLDBM ; };
    if ($@) {
        plan skip_all =>  "MLDBM is not installed on this system.\n";
    }

    plan tests => 12;    
}

use lib 't' ;
use util ;

{
    package BTREE ;
    
    use BerkeleyDB ;
    use MLDBM qw(BerkeleyDB::Btree) ; 
    use Data::Dumper;
    use Test::More;
    
    my $filename = "";
    my $lex = new LexFile $filename;
    
    $MLDBM::UseDB = "BerkeleyDB::Btree" ;
    my %o ;
    my $db = tie %o, 'MLDBM', -Filename => $filename,
    		     -Flags    => DB_CREATE
    		or die $!;
    ok $db ;
    ok $db->type() == DB_BTREE ;
    
    my $c = [\'c'];
    my $b = {};
    my $a = [1, $b, $c];
    $b->{a} = $a;
    $b->{b} = $a->[1];
    $b->{c} = $a->[2];
    @o{qw(a b c)} = ($a, $b, $c);
    $o{d} = "{once upon a time}";
    $o{e} = 1024;
    $o{f} = 1024.1024;
    
    my $struct = [@o{qw(a b c)}];
    ok ::_compare([$a, $b, $c], $struct);
    ok $o{d} eq "{once upon a time}" ;
    ok $o{e} == 1024 ;
    ok $o{f} eq 1024.1024 ;
    
}

{

    package HASH ;

    use BerkeleyDB ;
    use MLDBM qw(BerkeleyDB::Hash) ; 
    use Data::Dumper;

    my $filename = "";
    my $lex = new LexFile $filename;

    unlink $filename ;
    $MLDBM::UseDB = "BerkeleyDB::Hash" ;
    my %o ;
    my $db = tie %o, 'MLDBM', -Filename => $filename,
		         -Flags    => DB_CREATE
		    or die $!;
    ::ok $db ;
    ::ok $db->type() == DB_HASH ;


    my $c = [\'c'];
    my $b = {};
    my $a = [1, $b, $c];
    $b->{a} = $a;
    $b->{b} = $a->[1];
    $b->{c} = $a->[2];
    @o{qw(a b c)} = ($a, $b, $c);
    $o{d} = "{once upon a time}";
    $o{e} = 1024;
    $o{f} = 1024.1024;

    my $struct = [@o{qw(a b c)}];
    ::ok ::_compare([$a, $b, $c], $struct);
    ::ok $o{d} eq "{once upon a time}" ;
    ::ok $o{e} == 1024 ;
    ::ok $o{f} eq 1024.1024 ;

}

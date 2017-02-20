#!./perl -w

# ID: %I%, %G%   

use strict ;

use lib 't';
use BerkeleyDB; 
use util ;
use Test::More;

BEGIN {
    plan(skip_all => "this needs BerkeleyDB 2.5.2 or better" )
        if $BerkeleyDB::db_ver < 2.005002;

    plan tests => 42;    
}

my $Dfile1 = "dbhash1.tmp";
my $Dfile2 = "dbhash2.tmp";
my $Dfile3 = "dbhash3.tmp";
unlink $Dfile1, $Dfile2, $Dfile3 ;

umask(0) ;

{
    # error cases
    my $lex = new LexFile $Dfile1, $Dfile2, $Dfile3 ;
    my %hash1 ;
    my $value ;
    my $status ;
    my $cursor ;

    ok my $db1 = tie %hash1, 'BerkeleyDB::Hash', 
				-Filename => $Dfile1,
                               	-Flags     => DB_CREATE,
                                -DupCompare   => sub { $_[0] lt $_[1] },
                                -Property  => DB_DUP|DB_DUPSORT ;

    # no cursors supplied
    eval '$cursor = $db1->db_join() ;' ;
    ok $@ =~ /Usage: \$db->BerkeleyDB::db_join\Q([cursors], flags=0)/;

    # empty list
    eval '$cursor = $db1->db_join([]) ;' ;
    ok $@ =~ /db_join: No cursors in parameter list/;

    # cursor list, isn not a []
    eval '$cursor = $db1->db_join({}) ;' ;
    ok $@ =~ /db_join: first parameter is not an array reference/;

    eval '$cursor = $db1->db_join(\1) ;' ;
    ok $@ =~ /db_join: first parameter is not an array reference/;

    my ($a, $b) = ("a", "b");
    $a = bless [], "fred";
    $b = bless [], "fred";
    eval '$cursor = $db1->db_join($a, $b) ;' ;
    ok $@ =~ /db_join: first parameter is not an array reference/;

}

{
    # test a 2-way & 3-way join

    my $lex = new LexFile $Dfile1, $Dfile2, $Dfile3 ;
    my %hash1 ;
    my %hash2 ;
    my %hash3 ;
    my $value ;
    my $status ;

    my $home = "./fred7" ;
    rmtree $home;
    ok ! -d $home;
    ok my $lexD = new LexDir($home);
    ok my $env = new BerkeleyDB::Env -Home => $home, @StdErrFile,
				     -Flags => DB_CREATE|DB_INIT_TXN
					  	|DB_INIT_MPOOL;
					  	#|DB_INIT_MPOOL| DB_INIT_LOCK;
    ok my $txn = $env->txn_begin() ;
    ok my $db1 = tie %hash1, 'BerkeleyDB::Hash', 
				-Filename => $Dfile1,
                               	-Flags     => DB_CREATE,
                                -DupCompare   => sub { $_[0] cmp $_[1] },
                                -Property  => DB_DUP|DB_DUPSORT,
			       	-Env 	   => $env,
			    	-Txn	   => $txn  ;
				;

    ok my $db2 = tie %hash2, 'BerkeleyDB::Hash', 
				-Filename => $Dfile2,
                               	-Flags     => DB_CREATE,
                                -DupCompare   => sub { $_[0] cmp $_[1] },
                                -Property  => DB_DUP|DB_DUPSORT,
			       	-Env 	   => $env,
			    	-Txn	   => $txn  ;

    ok my $db3 = tie %hash3, 'BerkeleyDB::Btree', 
				-Filename => $Dfile3,
                               	-Flags     => DB_CREATE,
                                -DupCompare   => sub { $_[0] cmp $_[1] },
                                -Property  => DB_DUP|DB_DUPSORT,
			       	-Env 	   => $env,
			    	-Txn	   => $txn  ;

    
    ok addData($db1, qw( 	apple		Convenience
    				peach		Shopway
				pear		Farmer
				raspberry	Shopway
				strawberry	Shopway
				gooseberry	Farmer
				blueberry	Farmer
    			));

    ok addData($db2, qw( 	red	apple
    				red	raspberry
    				red	strawberry
				yellow	peach
				yellow	pear
				green	gooseberry
				blue	blueberry)) ;

    ok addData($db3, qw( 	expensive	apple
    				reasonable	raspberry
    				expensive	strawberry
				reasonable	peach
				reasonable	pear
				expensive	gooseberry
				reasonable	blueberry)) ;

    ok my $cursor2 = $db2->db_cursor() ;
    my $k = "red" ;
    my $v = "" ;
    ok $cursor2->c_get($k, $v, DB_SET) == 0 ;

    # Two way Join
    ok my $cursor1 = $db1->db_join([$cursor2]) ;

    my %expected = qw( apple Convenience
			raspberry Shopway
			strawberry Shopway
		) ;

    # sequence forwards
    while ($cursor1->c_get($k, $v) == 0) {
	delete $expected{$k} 
	    if defined $expected{$k} && $expected{$k} eq $v ;
	#print "[$k] [$v]\n" ;
    }
    is keys %expected, 0 ;
    ok $cursor1->status() == DB_NOTFOUND ;

    # Three way Join
    ok $cursor2 = $db2->db_cursor() ;
    $k = "red" ;
    $v = "" ;
    ok $cursor2->c_get($k, $v, DB_SET) == 0 ;

    ok my $cursor3 = $db3->db_cursor() ;
    $k = "expensive" ;
    $v = "" ;
    ok $cursor3->c_get($k, $v, DB_SET) == 0 ;
    ok $cursor1 = $db1->db_join([$cursor2, $cursor3]) ;

    %expected = qw( apple Convenience
			strawberry Shopway
		) ;

    # sequence forwards
    while ($cursor1->c_get($k, $v) == 0) {
	delete $expected{$k} 
	    if defined $expected{$k} && $expected{$k} eq $v ;
	#print "[$k] [$v]\n" ;
    }
    is keys %expected, 0 ;
    ok $cursor1->status() == DB_NOTFOUND ;

    # test DB_JOIN_ITEM
    # #################
    ok $cursor2 = $db2->db_cursor() ;
    $k = "red" ;
    $v = "" ;
    ok $cursor2->c_get($k, $v, DB_SET) == 0 ;
 
    ok $cursor3 = $db3->db_cursor() ;
    $k = "expensive" ;
    $v = "" ;
    ok $cursor3->c_get($k, $v, DB_SET) == 0 ;
    ok $cursor1 = $db1->db_join([$cursor2, $cursor3]) ;
 
    %expected = qw( apple 1
                        strawberry 1
                ) ;
 
    # sequence forwards
    $k = "" ;
    $v = "" ;
    while ($cursor1->c_get($k, $v, DB_JOIN_ITEM) == 0) {
        delete $expected{$k}
            if defined $expected{$k} ;
        #print "[$k]\n" ;
    }
    is keys %expected, 0 ;
    ok $cursor1->status() == DB_NOTFOUND ;

    ok $cursor1->c_close() == 0 ;
    ok $cursor2->c_close() == 0 ;
    ok $cursor3->c_close() == 0 ;

    ok (($status = $txn->txn_commit()) == 0);

    undef $txn ;

    ok my $cursor1a = $db1->db_cursor() ;
    eval { $cursor1 = $db1->db_join([$cursor1a]) };
    ok $@ =~ /BerkeleyDB Aborting: attempted to do a self-join at/;
    eval { $cursor1 = $db1->db_join([$cursor1]) } ;
    ok $@ =~ /BerkeleyDB Aborting: attempted to do a self-join at/;

    undef $cursor1a;
    #undef $cursor1;
    #undef $cursor2;
    #undef $cursor3;
    undef $db1 ;
    undef $db2 ;
    undef $db3 ;
    undef $env ;
    untie %hash1 ;
    untie %hash2 ;
    untie %hash3 ;
}

print "# at the end\n";

#!./perl -w

use strict ;

use lib 't' ;
use BerkeleyDB; 
use util ;

use Test::More ;

plan tests => 44;    

my $Dfile = "dbhash.tmp";
my $home = "./fred" ;

umask(0);

{
    # closing a database & an environment in the correct order.
    my $lex = new LexFile $Dfile ;
    my %hash ;
    my $status ;

    ok my $lexD = new LexDir($home);
    ok my $env = new BerkeleyDB::Env -Home => $home,@StdErrFile,
                                     -Flags => DB_CREATE|DB_INIT_TXN|
                                                DB_INIT_MPOOL|DB_INIT_LOCK ;
					  	
    ok my $db1 = tie %hash, 'BerkeleyDB::Hash', -Filename => $Dfile,
                                      	       	-Flags     => DB_CREATE ,
					       	-Env 	   => $env;

    ok $db1->db_close() == 0 ; 

    eval { $status = $env->db_appexit() ; } ;
    ok $status == 0 ;
    ok $@ eq "" ;
    #print "[$@]\n" ;

}

{
    # closing an environment with an open database
    my $lex = new LexFile $Dfile ;
    my %hash ;

    ok my $lexD = new LexDir($home);
    ok my $env = new BerkeleyDB::Env -Home => $home,@StdErrFile,
                                     -Flags => DB_CREATE|DB_INIT_TXN|
                                                DB_INIT_MPOOL|DB_INIT_LOCK ;
					  	
    ok my $db1 = tie %hash, 'BerkeleyDB::Hash', -Filename => $Dfile,
                                      	       	-Flags     => DB_CREATE ,
					       	-Env 	   => $env;

    eval { $env->db_appexit() ; } ;
    ok $@ =~ /BerkeleyDB Aborting: attempted to close an environment with 1 open database/ ;
    #print "[$@]\n" ;

    undef $db1 ;
    untie %hash ;
    undef $env ;
}

{
    # closing a transaction & a database 
    my $lex = new LexFile $Dfile ;
    my %hash ;
    my $status ;

    ok my $lexD = new LexDir($home);
    ok my $env = new BerkeleyDB::Env -Home => $home,@StdErrFile,
                                     -Flags => DB_CREATE|DB_INIT_TXN|
                                                DB_INIT_MPOOL|DB_INIT_LOCK ;

    ok my $txn = $env->txn_begin() ;
    ok my $db = tie %hash, 'BerkeleyDB::Hash', -Filename => $Dfile,
                                                -Flags     => DB_CREATE ,
					       	-Env 	   => $env,
                                                -Txn       => $txn  ;

    ok $txn->txn_commit()  == 0 ;
    eval { $status = $db->db_close() ; } ;
    ok $status == 0 ;
    ok $@ eq "" ;
    #print "[$@]\n" ;
    eval { $status = $env->db_appexit() ; } ;
    ok $status == 0 ;
    ok $@ eq "" ;
    #print "[$@]\n" ;
}

{
    # closing a database with an open transaction
    my $lex = new LexFile $Dfile ;
    my %hash ;

    ok my $lexD = new LexDir($home);
    ok my $env = new BerkeleyDB::Env -Home => $home,@StdErrFile,
                                     -Flags => DB_CREATE|DB_INIT_TXN|
                                                DB_INIT_MPOOL|DB_INIT_LOCK ;

    ok my $txn = $env->txn_begin() ;
    ok my $db = tie %hash, 'BerkeleyDB::Hash', -Filename => $Dfile,
                                                -Flags     => DB_CREATE ,
					       	-Env 	   => $env,
                                                -Txn       => $txn  ;

    eval { $db->db_close() ; } ;
    ok $@ =~ /BerkeleyDB Aborting: attempted to close a database while a transaction was still open at/ ;
    #print "[$@]\n" ;
    $txn->txn_abort();
    $db->db_close();
}

{
    # closing a cursor & a database 
    my $lex = new LexFile $Dfile ;
    my %hash ;
    my $status ;
    ok my $db = tie %hash, 'BerkeleyDB::Hash', -Filename => $Dfile,
                                                -Flags     => DB_CREATE ;
    ok my $cursor = $db->db_cursor() ;
    ok $cursor->c_close() == 0 ;
    eval { $status = $db->db_close() ; } ;
    ok $status == 0 ;
    ok $@ eq "" ;
    #print "[$@]\n" ;
}

{
    # closing a database with an open cursor
    my $lex = new LexFile $Dfile ;
    my %hash ;
    ok my $db = tie %hash, 'BerkeleyDB::Hash', -Filename => $Dfile,
                                                -Flags     => DB_CREATE ;
    ok my $cursor = $db->db_cursor() ;
    eval { $db->db_close() ; } ;
    ok $@ =~ /\QBerkeleyDB Aborting: attempted to close a database with 1 open cursor(s) at/;
    #print "[$@]\n" ;
}

{
    # closing a transaction & a cursor 
    my $lex = new LexFile $Dfile ;
    my %hash ;
    my $status ;
    my $home = 'fred1';

    ok my $lexD = new LexDir($home);
    ok my $env = new BerkeleyDB::Env -Home => $home,@StdErrFile,
                                     -Flags => DB_CREATE|DB_INIT_TXN|
                                                DB_INIT_MPOOL|DB_INIT_LOCK ;
    ok my $txn = $env->txn_begin() ;
    ok my $db = tie %hash, 'BerkeleyDB::Hash', -Filename => $Dfile,
                                                -Flags     => DB_CREATE ,
					       	-Env 	   => $env,
                                                -Txn       => $txn  ;
    ok my $cursor = $db->db_cursor() ;
    eval { $status = $cursor->c_close() ; } ;
    ok $status == 0 ;
    ok $txn->txn_commit() == 0 ;
    ok $@ eq "" ;
    eval { $status = $db->db_close() ; } ;
    ok $status == 0 ;
    ok $@ eq "" ;
    #print "[$@]\n" ;
    eval { $status = $env->db_appexit() ; } ;
    ok $status == 0 ;
    ok $@ eq "" ;
    #print "[$@]\n" ;
}


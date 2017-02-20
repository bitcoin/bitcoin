#!./perl -w

use strict ;

use lib 't' ;
use BerkeleyDB; 
use Test::More ;
use util ;

plan(skip_all => "this needs Berkeley DB 4.3.x or better\n" )
    if $BerkeleyDB::db_version < 4.3;

plan tests => 16;


if (1)
{
    # -MsgFile with a filename
    my $msgfile = "./msgfile" ;
    my $home = "./fred" ;
    ok my $lexD = new LexDir($home) ;
    my $lex = new LexFile $msgfile ;
    ok my $env = new BerkeleyDB::Env( -MsgFile => $msgfile, 
    				  -Flags => DB_CREATE,
				  -Home   => $home) ;
    $env->stat_print();
    ok length readFile($msgfile) > 0;

    undef $env ;
}


{
    # -MsgFile with a filehandle
    use IO::File ;
    my $msgfile = "./msgfile" ;
    my $home = "./fred" ;
    ok my $lexD = new LexDir($home) ;
    my $lex = new LexFile $msgfile ;
    my $fh = new IO::File ">$msgfile" ;
    ok my $env = new BerkeleyDB::Env( -MsgFile => $fh, 
    					  -Flags => DB_CREATE,
					  -Home   => $home) ;
    is $env->stat_print(), 0;
    close $fh;
    ok length readFile($msgfile) > 0;

    undef $env ;
}

{
    # -MsgFile with a filehandle
    use IO::File ;
    my $msgfile = "./msgfile" ;
    my $home = "./fred" ;
    ok my $lexD = new LexDir($home) ;
    my $lex = new LexFile $msgfile ;
    my $Dfile = "db.db";
    my $lex1 = new LexFile $Dfile ;
    my $fh = new IO::File ">$msgfile" ;
    ok my $env = new BerkeleyDB::Env( -MsgFile => $fh, 
    					  -Flags => DB_CREATE|DB_INIT_MPOOL,
					  -Home   => $home) ;
    ok my $db = new BerkeleyDB::Btree -Filename => $Dfile, 
				    -Env      => $env,
				    -Flags    => DB_CREATE ;
    is $db->stat_print(), 0;
    close $fh;
    ok length readFile($msgfile) > 0;

    undef $db;
    undef $env ;
}

{
    # txn_stat_print
    use IO::File ;
    my $msgfile = "./msgfile" ;
    my $home = "./fred" ;
    ok my $lexD = new LexDir($home) ;
    my $lex = new LexFile $msgfile ;
    my $fh = new IO::File ">$msgfile" ;
    ok my $env = new BerkeleyDB::Env( -MsgFile => $fh, 
    					  -Flags => DB_CREATE|DB_INIT_TXN,
					  -Home   => $home) ;
    is $env->txn_stat_print(), 0
        or diag "$BerkeleyDB::Error";
    close $fh;
    ok length readFile($msgfile) > 0;

    undef $env ;
}


use strict ;

use lib 't' ;
use Test::More;
use BerkeleyDB; 
use util;

plan(skip_all => "Sequence needs Berkeley DB 4.3.x or better\n" )
    if $BerkeleyDB::db_version < 4.3;
    
plan tests => 13;

{
my $home = "./fred7" ;
ok my $lexD = new LexDir($home) ;
my $Dfile = "$home/f" ;
my $lex = new LexFile $Dfile;

umask(0) ;

my $env = new BerkeleyDB::Env -Home => $home, @StdErrFile,
                              -Flags => DB_CREATE|DB_INIT_MPOOL;
isa_ok($env, "BerkeleyDB::Env");

my $db = BerkeleyDB::Btree->new(
    Env => $env,
    -Filename => $Dfile, 
	-Flags    => DB_CREATE
);

my $seq = $db->db_create_sequence();
isa_ok($seq, "BerkeleyDB::Sequence");

is int $seq->set_cachesize(42), 0, "set_cachesize";

my $key = "test sequence";
is int $seq->open($key),  DB_NOTFOUND, "opened with no CREATE";
is int $seq->open($key, DB_CREATE), 0, "opened";

my $gotcs;
is int $seq->get_cachesize($gotcs), 0;
is $gotcs, 42;

# First sequence should be 0
my $val;
is int $seq->get($val), 0, "get";
is length($val), 8, "64 bts == 8 bytes";

my $gotkey ='';
is int $seq->get_key($gotkey), 0, "get_key";
is $gotkey, $key;

is int $seq->close(),  0, "close";
}

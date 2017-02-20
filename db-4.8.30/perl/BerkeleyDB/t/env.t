#!./perl -w

use strict ;


use lib 't' ;

BEGIN {
    $ENV{LC_ALL} = 'de_DE@euro';
}

use BerkeleyDB; 
use util ;

use Test::More ;

plan tests => 53;

my $Dfile = "dbhash.tmp";

umask(0);

my $version_major  = 0;

{
    # db version stuff
    my ($major, $minor, $patch) = (0, 0, 0) ;

    ok my $VER = BerkeleyDB::DB_VERSION_STRING ;
    ok my $ver = BerkeleyDB::db_version($version_major, $minor, $patch) ;
    ok $VER eq $ver ;
    ok $version_major > 1 ;
    ok defined $minor ;
    ok defined $patch ;
}

{
    # Check for invalid parameters
    my $env ;
    eval ' $env = new BerkeleyDB::Env( -Stupid => 3) ; ' ;
    ok $@ =~ /unknown key value\(s\) Stupid/, "Unknown key"  ;

    eval ' $env = new BerkeleyDB::Env( -Bad => 2, -Home => "/tmp", -Stupid => 3) ; ' ;
    ok $@ =~ /unknown key value\(s\) (Bad,? |Stupid,? ){2}/  ;

    eval ' $env = new BerkeleyDB::Env (-Config => {"fred" => " "} ) ; ' ;
    ok !$env ;
    ok $BerkeleyDB::Error =~ /^(illegal name-value pair|Invalid argument)/ ;
    #print " $BerkeleyDB::Error\n";
}

{
    # create a very simple environment
    my $home = "./fred" ;
    ok my $lexD = new LexDir($home) ;
    chdir "./fred" ;
    ok my $env = new BerkeleyDB::Env -Flags => DB_CREATE,
					@StdErrFile;
    chdir ".." ;
    undef $env ;
}

{
    # create an environment with a Home
    my $home = "./fred" ;
    ok my $lexD = new LexDir($home) ;
    ok my $env = new BerkeleyDB::Env -Home => $home,
    					 -Flags => DB_CREATE ;

    undef $env ;
}

{
    # make new fail.
    my $home = "./not_there" ;
    rmtree $home ;
    ok ! -d $home ;
    my $env = new BerkeleyDB::Env -Home => $home, @StdErrFile,
			          -Flags => DB_INIT_LOCK ;
    ok ! $env ;
    ok $! != 0 || $^E != 0, "got error" ;

    rmtree $home ;
}

{
    # Config
    use Cwd ;
    my $cwd = cwd() ;
    my $home = "$cwd/fred" ;
    my $data_dir = "$home/data_dir" ;
    my $log_dir = "$home/log_dir" ;
    my $data_file = "data.db" ;
    ok my $lexD = new LexDir($home) ;
    ok -d $data_dir ? chmod 0777, $data_dir : mkdir($data_dir, 0777) ;
    ok -d $log_dir ? chmod 0777, $log_dir : mkdir($log_dir, 0777) ;
    my $env = new BerkeleyDB::Env -Home   => $home, @StdErrFile,
			      -Config => { DB_DATA_DIR => $data_dir,
					   DB_LOG_DIR  => $log_dir
					 },
			      -Flags  => DB_CREATE|DB_INIT_TXN|DB_INIT_LOG|
					 DB_INIT_MPOOL|DB_INIT_LOCK ;
    ok $env ;

    ok my $txn = $env->txn_begin() ;

    my %hash ;
    ok tie %hash, 'BerkeleyDB::Hash', -Filename => $data_file,
                                       -Flags     => DB_CREATE ,
                                       -Env       => $env,
                                       -Txn       => $txn  ;

    $hash{"abc"} = 123 ;
    $hash{"def"} = 456 ;

    $txn->txn_commit() ;

    untie %hash ;

    undef $txn ;
    undef $env ;
}

sub chkMsg
{
    my $prefix = shift || '';

    $prefix = "$prefix: " if $prefix;

    my $ErrMsg = join "|", map { "$prefix$_" }
                        'illegal flag specified to (db_open|DB->open)',
                       'DB_AUTO_COMMIT may not be specified in non-transactional environment';
    
    return 1 if $BerkeleyDB::Error =~ /^$ErrMsg/ ;
    warn "# $BerkeleyDB::Error\n" ;
    return 0;
}

{
    # -ErrFile with a filename
    my $errfile = "./errfile" ;
    my $home = "./fred" ;
    ok my $lexD = new LexDir($home) ;
    my $lex = new LexFile $errfile ;
    ok my $env = new BerkeleyDB::Env( -ErrFile => $errfile, 
    					  -Flags => DB_CREATE,
					  -Home   => $home) ;
    my $db = new BerkeleyDB::Hash -Filename => $Dfile,
			     -Env      => $env,
			     -Flags    => -1;
    ok !$db ;

    my $ErrMsg = join "'", 'illegal flag specified to (db_open|DB->open)',
                           'DB_AUTO_COMMIT may not be specified in non-transactional environment';
    
    ok chkMsg();
    ok -e $errfile ;
    my $contents = docat($errfile) ;
    chomp $contents ;
    ok $BerkeleyDB::Error eq $contents ;

    undef $env ;
}

{
    # -ErrFile with a filehandle
    use IO::File ;
    my $errfile = "./errfile" ;
    my $home = "./fred" ;
    ok my $lexD = new LexDir($home) ;
    my $lex = new LexFile $errfile ;
    my $fh = new IO::File ">$errfile" ;
    ok my $env = new BerkeleyDB::Env( -ErrFile => $fh, 
    					  -Flags => DB_CREATE,
					  -Home   => $home) ;
    my $db = new BerkeleyDB::Hash -Filename => $Dfile,
			     -Env      => $env,
			     -Flags    => -1;
    ok !$db ;

    ok chkMsg();
    ok -e $errfile ;
    my $contents = docat($errfile) ;
    chomp $contents ;
    ok $BerkeleyDB::Error eq $contents ;

    undef $env ;
}

{
    # -ErrPrefix
    my $home = "./fred" ;
    ok my $lexD = new LexDir($home) ;
    my $errfile = "./errfile" ;
    my $lex = new LexFile $errfile ;
    ok my $env = new BerkeleyDB::Env( -ErrFile => $errfile,
					-ErrPrefix => "PREFIX",
    					  -Flags => DB_CREATE,
					  -Home   => $home) ;
    my $db = new BerkeleyDB::Hash -Filename => $Dfile,
			     -Env      => $env,
			     -Flags    => -1;
    ok !$db ;

    ok chkMsg('PREFIX');
    ok -e $errfile ;
    my $contents = docat($errfile) ;
    chomp $contents ;
    ok $BerkeleyDB::Error eq $contents ;

    # change the prefix on the fly
    my $old = $env->errPrefix("NEW ONE") ;
    ok $old eq "PREFIX" ;

    $db = new BerkeleyDB::Hash -Filename => $Dfile,
			     -Env      => $env,
			     -Flags    => -1;
    ok !$db ;
    ok chkMsg('NEW ONE');
    $contents = docat($errfile) ;
    chomp $contents ;
    ok $contents =~ /$BerkeleyDB::Error$/ ;
    undef $env ;
}

{
    # test db_appexit
    use Cwd ;
    my $cwd = cwd() ;
    my $home = "$cwd/fred" ;
    my $data_dir = "$home/data_dir" ;
    my $log_dir = "$home/log_dir" ;
    my $data_file = "data.db" ;
    ok my $lexD = new LexDir($home);
    ok -d $data_dir ? chmod 0777, $data_dir : mkdir($data_dir, 0777) ;
    ok -d $log_dir ? chmod 0777, $log_dir : mkdir($log_dir, 0777) ;
    my $env = new BerkeleyDB::Env -Home   => $home, @StdErrFile,
			      -Config => { DB_DATA_DIR => $data_dir,
					   DB_LOG_DIR  => $log_dir
					 },
			      -Flags  => DB_CREATE|DB_INIT_TXN|DB_INIT_LOG|
					 DB_INIT_MPOOL|DB_INIT_LOCK ;
    ok $env ;

    ok my $txn_mgr = $env->TxnMgr() ;

    ok $env->db_appexit() == 0 ;

}

{
    # attempt to open a new environment without DB_CREATE
    # should fail with Berkeley DB 3.x or better.

    my $home = "./fred" ;
    ok my $lexD = new LexDir($home) ;
    chdir "./fred" ;
    my $env = new BerkeleyDB::Env -Home => $home, -Flags => DB_CREATE ;
    ok $version_major == 2 ? $env : ! $env ;

    # The test below is not portable -- the error message returned by
    # $BerkeleyDB::Error is locale dependant.

    #ok $version_major == 2 ? 1 
    #                           : $BerkeleyDB::Error =~ /No such file or directory/ ;
    #    or print "# BerkeleyDB::Error is $BerkeleyDB::Error\n";
    chdir ".." ;
    undef $env ;
}

# test -Verbose
# test -Flags
# db_value_set

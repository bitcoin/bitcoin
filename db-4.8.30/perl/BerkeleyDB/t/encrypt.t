#!./perl -w

# ID: %I%, %G%   

use strict ;

use lib 't' ;
use BerkeleyDB; 
use util ;
use Test::More;

BEGIN {
    plan(skip_all => "this needs BerkeleyDB 4.1.x or better" )
        if $BerkeleyDB::db_version < 4.1;

    # Is encryption available?
    my $env = new BerkeleyDB::Env @StdErrFile,
             -Encrypt => {Password => "abc",
	                  Flags    => DB_ENCRYPT_AES
	                 };

    plan skip_all => "encryption support not present"
        if $BerkeleyDB::Error =~ /Operation not supported/;

    plan tests => 80;
}


umask(0);

{    
    eval
    {
        my $env = new BerkeleyDB::Env @StdErrFile,
             -Encrypt => 1,
             -Flags => DB_CREATE ;
     };
     ok $@ =~ /^Encrypt parameter must be a hash reference at/;

    eval
    {
        my $env = new BerkeleyDB::Env @StdErrFile,
             -Encrypt => {},
             -Flags => DB_CREATE ;
     };
     ok $@ =~ /^Must specify Password and Flags with Encrypt parameter at/;

    eval
    {
        my $env = new BerkeleyDB::Env @StdErrFile,
             -Encrypt => {Password => "fred"},
             -Flags => DB_CREATE ;
     };
     ok $@ =~ /^Must specify Password and Flags with Encrypt parameter at/;

    eval
    {
        my $env = new BerkeleyDB::Env @StdErrFile,
             -Encrypt => {Flags => 1},
             -Flags => DB_CREATE ;
     };
     ok $@ =~ /^Must specify Password and Flags with Encrypt parameter at/;

    eval
    {
        my $env = new BerkeleyDB::Env @StdErrFile,
             -Encrypt => {Fred => 1},
             -Flags => DB_CREATE ;
     };
     ok $@ =~ /^\Qunknown key value(s) Fred at/;

}

{
    # new BerkeleyDB::Env -Encrypt =>

    # create an environment with a Home
    my $home = "./fred" ;
    #mkdir $home;
    ok my $lexD = new LexDir($home) ;
    ok my $env = new BerkeleyDB::Env @StdErrFile,
             -Home => $home,
             -Encrypt => {Password => "abc",
	                  Flags    => DB_ENCRYPT_AES
	                 },
             -Flags => DB_CREATE | DB_INIT_MPOOL ;



    my $Dfile = "abc.enc";
    my $lex = new LexFile $Dfile ;
    my %hash ;
    my ($k, $v) ;
    ok my $db = new BerkeleyDB::Hash -Filename => $Dfile, 
	                             -Env         => $env,
				     -Flags       => DB_CREATE, 
				     -Property    => DB_ENCRYPT ;

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
    ok $ret == 0 ;

    # check there are three records
    ok countRecords($db) == 3 ;

    undef $db;

    # once the database is created, do not need to specify DB_ENCRYPT
    ok my $db1 = new BerkeleyDB::Hash -Filename => $Dfile, 
	                              -Env      => $env,
				      -Flags    => DB_CREATE ;
    $v = '';				      
    ok ! $db1->db_get("red", $v) ;
    ok $v eq $data{"red"},
    undef $db1;
    undef $env;

    # open a database without specifying encryption
    ok ! new BerkeleyDB::Hash -Filename => "$home/$Dfile"; 

    ok ! new BerkeleyDB::Env 
             -Home => $home,
             -Encrypt => {Password => "def",
	                  Flags    => DB_ENCRYPT_AES
	                 },
             -Flags => DB_CREATE | DB_INIT_MPOOL ;
}

{    
    eval
    {
        my $env = new BerkeleyDB::Hash 
             -Encrypt => 1,
             -Flags => DB_CREATE ;
     };
     ok $@ =~ /^Encrypt parameter must be a hash reference at/;

    eval
    {
        my $env = new BerkeleyDB::Hash 
             -Encrypt => {},
             -Flags => DB_CREATE ;
     };
     ok $@ =~ /^Must specify Password and Flags with Encrypt parameter at/;

    eval
    {
        my $env = new BerkeleyDB::Hash 
             -Encrypt => {Password => "fred"},
             -Flags => DB_CREATE ;
     };
     ok $@ =~ /^Must specify Password and Flags with Encrypt parameter at/;

    eval
    {
        my $env = new BerkeleyDB::Hash 
             -Encrypt => {Flags => 1},
             -Flags => DB_CREATE ;
     };
     ok $@ =~ /^Must specify Password and Flags with Encrypt parameter at/;

    eval
    {
        my $env = new BerkeleyDB::Hash 
             -Encrypt => {Fred => 1},
             -Flags => DB_CREATE ;
     };
     ok $@ =~ /^\Qunknown key value(s) Fred at/;

}

{    
    eval
    {
        my $env = new BerkeleyDB::Btree 
             -Encrypt => 1,
             -Flags => DB_CREATE ;
     };
     ok $@ =~ /^Encrypt parameter must be a hash reference at/;

    eval
    {
        my $env = new BerkeleyDB::Btree 
             -Encrypt => {},
             -Flags => DB_CREATE ;
     };
     ok $@ =~ /^Must specify Password and Flags with Encrypt parameter at/;

    eval
    {
        my $env = new BerkeleyDB::Btree 
             -Encrypt => {Password => "fred"},
             -Flags => DB_CREATE ;
     };
     ok $@ =~ /^Must specify Password and Flags with Encrypt parameter at/;

    eval
    {
        my $env = new BerkeleyDB::Btree 
             -Encrypt => {Flags => 1},
             -Flags => DB_CREATE ;
     };
     ok $@ =~ /^Must specify Password and Flags with Encrypt parameter at/;

    eval
    {
        my $env = new BerkeleyDB::Btree 
             -Encrypt => {Fred => 1},
             -Flags => DB_CREATE ;
     };
     ok $@ =~ /^\Qunknown key value(s) Fred at/;

}

{    
    eval
    {
        my $env = new BerkeleyDB::Queue 
             -Encrypt => 1,
             -Flags => DB_CREATE ;
     };
     ok $@ =~ /^Encrypt parameter must be a hash reference at/;

    eval
    {
        my $env = new BerkeleyDB::Queue 
             -Encrypt => {},
             -Flags => DB_CREATE ;
     };
     ok $@ =~ /^Must specify Password and Flags with Encrypt parameter at/;

    eval
    {
        my $env = new BerkeleyDB::Queue 
             -Encrypt => {Password => "fred"},
             -Flags => DB_CREATE ;
     };
     ok $@ =~ /^Must specify Password and Flags with Encrypt parameter at/;

    eval
    {
        my $env = new BerkeleyDB::Queue 
             -Encrypt => {Flags => 1},
             -Flags => DB_CREATE ;
     };
     ok $@ =~ /^Must specify Password and Flags with Encrypt parameter at/;

    eval
    {
        my $env = new BerkeleyDB::Queue 
             -Encrypt => {Fred => 1},
             -Flags => DB_CREATE ;
     };
     ok $@ =~ /^\Qunknown key value(s) Fred at/;

}

{    
    eval
    {
        my $env = new BerkeleyDB::Recno 
             -Encrypt => 1,
             -Flags => DB_CREATE ;
     };
     ok $@ =~ /^Encrypt parameter must be a hash reference at/;

    eval
    {
        my $env = new BerkeleyDB::Recno 
             -Encrypt => {},
             -Flags => DB_CREATE ;
     };
     ok $@ =~ /^Must specify Password and Flags with Encrypt parameter at/;

    eval
    {
        my $env = new BerkeleyDB::Recno 
             -Encrypt => {Password => "fred"},
             -Flags => DB_CREATE ;
     };
     ok $@ =~ /^Must specify Password and Flags with Encrypt parameter at/;

    eval
    {
        my $env = new BerkeleyDB::Recno 
             -Encrypt => {Flags => 1},
             -Flags => DB_CREATE ;
     };
     ok $@ =~ /^Must specify Password and Flags with Encrypt parameter at/;

    eval
    {
        my $env = new BerkeleyDB::Recno 
             -Encrypt => {Fred => 1},
             -Flags => DB_CREATE ;
     };
     ok $@ =~ /^\Qunknown key value(s) Fred at/;

}


{
    # new BerkeleyDB::Hash -Encrypt =>

    my $Dfile = "abcd.enc";
    my $lex = new LexFile $Dfile ;
    my %hash ;
    my ($k, $v) ;
    ok my $db = new BerkeleyDB::Hash 
                           -Filename => $Dfile, 
		           -Flags    => DB_CREATE, 
                           -Encrypt  => {Password => "beta",
	                                 Flags    => DB_ENCRYPT_AES
	                                },
		           -Property => DB_ENCRYPT ;

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
    ok $ret == 0 ;

    # check there are three records
    ok countRecords($db) == 3 ;

    undef $db;

    # attempt to open a database without specifying encryption
    ok ! new BerkeleyDB::Hash -Filename => $Dfile, 
				      -Flags    => DB_CREATE ;


    # try opening with the wrong password				      
    ok ! new BerkeleyDB::Hash -Filename => $Dfile, 
                           -Filename => $Dfile, 
                           -Encrypt => {Password => "def",
	                                Flags    => DB_ENCRYPT_AES
	                               },
		           -Property    => DB_ENCRYPT ;


    # read the encrypted data				      
    ok my $db1 = new BerkeleyDB::Hash -Filename => $Dfile, 
                           -Filename => $Dfile, 
                           -Encrypt => {Password => "beta",
	                                Flags    => DB_ENCRYPT_AES
	                               },
		           -Property    => DB_ENCRYPT ;


    $v = '';				      
    ok ! $db1->db_get("red", $v) ;
    ok $v eq $data{"red"};
    # check there are three records
    ok countRecords($db1) == 3 ;
    undef $db1;
}

{
    # new BerkeleyDB::Btree -Encrypt =>

    my $Dfile = "abcd.enc";
    my $lex = new LexFile $Dfile ;
    my %hash ;
    my ($k, $v) ;
    ok my $db = new BerkeleyDB::Btree 
                           -Filename => $Dfile, 
		           -Flags    => DB_CREATE, 
                           -Encrypt  => {Password => "beta",
	                                 Flags    => DB_ENCRYPT_AES
	                                },
		           -Property => DB_ENCRYPT ;

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
    ok $ret == 0 ;

    # check there are three records
    ok countRecords($db) == 3 ;

    undef $db;

    # attempt to open a database without specifying encryption
    ok ! new BerkeleyDB::Btree -Filename => $Dfile, 
				      -Flags    => DB_CREATE ;


    # try opening with the wrong password				      
    ok ! new BerkeleyDB::Btree -Filename => $Dfile, 
                           -Filename => $Dfile, 
                           -Encrypt => {Password => "def",
	                                Flags    => DB_ENCRYPT_AES
	                               },
		           -Property    => DB_ENCRYPT ;


    # read the encrypted data				      
    ok my $db1 = new BerkeleyDB::Btree -Filename => $Dfile, 
                           -Filename => $Dfile, 
                           -Encrypt => {Password => "beta",
	                                Flags    => DB_ENCRYPT_AES
	                               },
		           -Property    => DB_ENCRYPT ;


    $v = '';				      
    ok ! $db1->db_get("red", $v) ;
    ok $v eq $data{"red"};
    # check there are three records
    ok countRecords($db1) == 3 ;
    undef $db1;
}

{
    # new BerkeleyDB::Queue -Encrypt =>

    my $Dfile = "abcd.enc";
    my $lex = new LexFile $Dfile ;
    my %hash ;
    my ($k, $v) ;
    ok my $db = new BerkeleyDB::Queue 
                           -Filename => $Dfile, 
                           -Len      => 5,
                           -Pad      => "x",
		           -Flags    => DB_CREATE, 
                           -Encrypt  => {Password => "beta",
	                                 Flags    => DB_ENCRYPT_AES
	                                },
		           -Property => DB_ENCRYPT ;

    # create some data
    my %data =  (
		1	=> 2,
		2	=> "house",
		3	=> "sea",
		) ;

    my $ret = 0 ;
    while (($k, $v) = each %data) {
        $ret += $db->db_put($k, $v) ;
    }
    ok $ret == 0 ;

    # check there are three records
    ok countRecords($db) == 3 ;

    undef $db;

    # attempt to open a database without specifying encryption
    ok ! new BerkeleyDB::Queue -Filename => $Dfile, 
                                   -Len      => 5,
                                   -Pad      => "x",
				   -Flags    => DB_CREATE ;


    # try opening with the wrong password				      
    ok ! new BerkeleyDB::Queue -Filename => $Dfile, 
                                   -Len      => 5,
                                   -Pad      => "x",
                                   -Encrypt => {Password => "def",
	                                        Flags    => DB_ENCRYPT_AES
	                                       },
		                   -Property    => DB_ENCRYPT ;


    # read the encrypted data				      
    ok my $db1 = new BerkeleyDB::Queue -Filename => $Dfile, 
                                           -Len      => 5,
                                           -Pad      => "x",
                                           -Encrypt => {Password => "beta",
	                                        Flags    => DB_ENCRYPT_AES
	                                       },
		                           -Property    => DB_ENCRYPT ;


    $v = '';				      
    ok ! $db1->db_get(3, $v) ;
    ok $v eq fillout($data{3}, 5, 'x');
    # check there are three records
    ok countRecords($db1) == 3 ;
    undef $db1;
}

{
    # new BerkeleyDB::Recno -Encrypt =>

    my $Dfile = "abcd.enc";
    my $lex = new LexFile $Dfile ;
    my %hash ;
    my ($k, $v) ;
    ok my $db = new BerkeleyDB::Recno 
                           -Filename => $Dfile, 
		           -Flags    => DB_CREATE, 
                           -Encrypt  => {Password => "beta",
	                                 Flags    => DB_ENCRYPT_AES
	                                },
		           -Property => DB_ENCRYPT ;

    # create some data
    my %data =  (
		1	=> 2,
		2	=> "house",
		3	=> "sea",
		) ;

    my $ret = 0 ;
    while (($k, $v) = each %data) {
        $ret += $db->db_put($k, $v) ;
    }
    ok $ret == 0 ;

    # check there are three records
    ok countRecords($db) == 3 ;

    undef $db;

    # attempt to open a database without specifying encryption
    ok ! new BerkeleyDB::Recno -Filename => $Dfile, 
				      -Flags    => DB_CREATE ;


    # try opening with the wrong password				      
    ok ! new BerkeleyDB::Recno -Filename => $Dfile, 
                           -Filename => $Dfile, 
                           -Encrypt => {Password => "def",
	                                Flags    => DB_ENCRYPT_AES
	                               },
		           -Property    => DB_ENCRYPT ;


    # read the encrypted data				      
    ok my $db1 = new BerkeleyDB::Recno -Filename => $Dfile, 
                           -Filename => $Dfile, 
                           -Encrypt => {Password => "beta",
	                                Flags    => DB_ENCRYPT_AES
	                               },
		           -Property    => DB_ENCRYPT ;


    $v = '';				      
    ok ! $db1->db_get(3, $v) ;
    ok $v eq $data{3};
    # check there are three records
    ok countRecords($db1) == 3 ;
    undef $db1;
}

{
    # new BerkeleyDB::Unknown -Encrypt =>

    my $Dfile = "abcd.enc";
    my $lex = new LexFile $Dfile ;
    my %hash ;
    my ($k, $v) ;
    ok my $db = new BerkeleyDB::Hash 
                           -Filename => $Dfile, 
		           -Flags    => DB_CREATE, 
                           -Encrypt  => {Password => "beta",
	                                 Flags    => DB_ENCRYPT_AES
	                                },
		           -Property => DB_ENCRYPT ;

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
    ok $ret == 0 ;

    # check there are three records
    ok countRecords($db) == 3 ;

    undef $db;

    # attempt to open a database without specifying encryption
    ok ! new BerkeleyDB::Unknown -Filename => $Dfile, 
				      -Flags    => DB_CREATE ;


    # try opening with the wrong password				      
    ok ! new BerkeleyDB::Unknown -Filename => $Dfile, 
                           -Filename => $Dfile, 
                           -Encrypt => {Password => "def",
	                                Flags    => DB_ENCRYPT_AES
	                               },
		           -Property    => DB_ENCRYPT ;


    # read the encrypted data				      
    ok my $db1 = new BerkeleyDB::Unknown -Filename => $Dfile, 
                           -Filename => $Dfile, 
                           -Encrypt => {Password => "beta",
	                                Flags    => DB_ENCRYPT_AES
	                               },
		           -Property    => DB_ENCRYPT ;


    $v = '';				      
    ok ! $db1->db_get("red", $v) ;
    ok $v eq $data{"red"};
    # check there are three records
    ok countRecords($db1) == 3 ;
    undef $db1;
}


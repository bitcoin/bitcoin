#! perl -w

use strict ;
use ExtUtils::MakeMaker 5.16 ;
use Config ;

die "DB_File needs Perl 5.004_05 or better. This is $]\n" 
    if $] <= 5.00404; 

my $VER_INFO ;
my $LIB_DIR ;
my $INC_DIR ;
my $DB_NAME ;
my $LIBS ;
my $COMPAT185 = "" ;

ParseCONFIG() ;

my @files = ('DB_File.pm', glob "t/*.t") ;
UpDowngrade(@files);

if (defined $DB_NAME)
  { $LIBS = $DB_NAME }
else {
    if ($^O eq 'MSWin32')
      { $LIBS = $Config{cc} =~ /gcc/ ? '-ldb' : '-llibdb' }
    else
      { $LIBS = '-ldb' }
}

# Solaris is special.
#$LIBS .= " -lthread" if $^O eq 'solaris' ;

# AIX is special.
$LIBS .= " -lpthread" if $^O eq 'aix' ;

# OS2 is a special case, so check for it now.
my $OS2 = "" ;
$OS2 = "-DOS2" if $Config{'osname'} eq 'os2' ;

my $WALL = '' ;
#$WALL = ' -Wall ';

WriteMakefile(
	NAME 		=> 'DB_File',
	LIBS 		=> ["-L${LIB_DIR} $LIBS"],
	INC		=> "-I$INC_DIR",
    VERSION_FROM	=> 'DB_File.pm',
	XS_VERSION      => eval MM->parse_version('DB_File.pm'),
	XSPROTOARG	=> '-noprototypes',
	DEFINE		=> "-D_NOT_CORE $OS2 $VER_INFO $COMPAT185 $WALL",
	OBJECT		=> 'version$(OBJ_EXT) DB_File$(OBJ_EXT)',
    ((ExtUtils::MakeMaker->VERSION() gt '6.30') 
        ?  ('LICENSE'  => 'perl')         
        : ()
    ),
    (
    $] >= 5.005
        ? (ABSTRACT_FROM => 'DB_File.pm',
            AUTHOR       => 'Paul Marquess <pmqs@cpan.org>')
        : ()
    ),
    

	#OPTIMIZE	=> '-g',
	'depend'	=> { 'Makefile'          => 'config.in',
                             'version$(OBJ_EXT)' => 'version.c'},
	'clean'		=> { FILES => 'constants.h constants.xs' },
	'macro'		=> { INSTALLDIRS => 'perl', my_files => "@files" },
        'dist'          => { COMPRESS => 'gzip', SUFFIX => 'gz',
			     DIST_DEFAULT => 'MyDoubleCheck tardist'},    
	);


my @names = qw(
	BTREEMAGIC
	BTREEVERSION
	DB_LOCK
	DB_SHMEM
	DB_TXN
	HASHMAGIC
	HASHVERSION
	MAX_PAGE_NUMBER
	MAX_PAGE_OFFSET
	MAX_REC_NUMBER
	RET_ERROR
	RET_SPECIAL
	RET_SUCCESS
	R_CURSOR
	R_DUP
	R_FIRST
	R_FIXEDLEN
	R_IAFTER
	R_IBEFORE
	R_LAST
	R_NEXT
	R_NOKEY
	R_NOOVERWRITE
	R_PREV
	R_RECNOSYNC
	R_SETCURSOR
	R_SNAPSHOT
	__R_UNUSED
	);

if (eval {require ExtUtils::Constant; 1}) {
    # Check the constants above all appear in @EXPORT in DB_File.pm
    my %names = map { $_, 1} @names;
    open F, "<DB_File.pm" or die "Cannot open DB_File.pm: $!\n";
    while (<F>)
    {
        last if /^\s*\@EXPORT\s+=\s+qw\(/ ;
    }

    while (<F>)
    {
        last if /^\s*\)/ ;
        /(\S+)/ ;
        delete $names{$1} if defined $1 ;
    }
    close F ;

    if ( keys %names )
    {
        my $missing = join ("\n\t", sort keys %names) ;
        die "The following names are missing from \@EXPORT in DB_File.pm\n" .
            "\t$missing\n" ;
    }
    

    ExtUtils::Constant::WriteConstants(
                                     NAME => 'DB_File',
                                     NAMES => \@names,
                                     C_FILE  => 'constants.h',
                                     XS_FILE  => 'constants.xs',
                                                                       
                                    );
} 
else {
    use File::Copy;
    copy ('fallback.h', 'constants.h')
      or die "Can't copy fallback.h to constants.h: $!";
    copy ('fallback.xs', 'constants.xs')
      or die "Can't copy fallback.xs to constants.xs: $!";
}

exit;


sub MY::libscan
{
    my $self = shift ;
    my $path = shift ;

    return undef
        if $path =~ /(~|\.bak)$/ || 
           $path =~ /^\..*\.swp$/ ;

    return $path;    
}


sub MY::postamble { <<'EOM' } ;

MyDoubleCheck:
	@echo Checking config.in is setup for a release
	@(grep "^LIB.*/usr/local/BerkeleyDB" config.in && 	\
	grep "^INCLUDE.*/usr/local/BerkeleyDB" config.in &&	\
	grep "^#DBNAME.*" config.in) >/dev/null ||		\
	    (echo config.in needs fixing ; exit 1)
	@echo config.in is ok
	@echo 
	@echo Checking DB_File.xs is ok for a release.
	@(perl -ne ' exit 1 if /^\s*#\s*define\s+TRACE/ ; ' DB_File.xs || \
	    (echo DB_File.xs needs fixing ; exit 1))
	@echo DB_File.xs is ok
	@echo 
	@echo Checking for $$^W in files: $(my_files)
	@perl -ne '						\
	    exit 1 if /^\s*local\s*\(\s*\$$\^W\s*\)/;' $(my_files) ||	\
	  (echo found unexpected $$^W ; exit 1)
	@echo No $$^W found.
	@echo 
	@echo Checking for 'use vars' in files: $(my_files)
	@perl -ne '						\
	    exit 0 if /^__(DATA|END)__/;               	\
	    exit 1 if /^\s*use\s+vars/;' $(my_files) ||	\
	  (echo found unexpected "use vars"; exit 1)
	@echo No 'use vars' found.
	@echo 
	@echo All files are OK for a release.
	@echo 

EOM



sub ParseCONFIG
{
    my ($k, $v) ;
    my @badkey = () ;
    my %Info = () ;
    my @Options = qw( INCLUDE LIB PREFIX HASH DBNAME COMPAT185 ) ;
    my %ValidOption = map {$_, 1} @Options ;
    my %Parsed = %ValidOption ;
    my $CONFIG = 'config.in' ;

    print "Parsing $CONFIG...\n" ;

    # DBNAME & COMPAT185 are optional, so pretend they  have 
    # been parsed.
    delete $Parsed{'DBNAME'} ;
    delete $Parsed{'COMPAT185'} ;
    $Info{COMPAT185} = "No" ;


    open(F, "$CONFIG") or die "Cannot open file $CONFIG: $!\n" ;
    while (<F>) {
	s/^\s*|\s*$//g ;
	next if /^\s*$/ or /^\s*#/ ;
	s/\s*#\s*$// ;

	($k, $v) = split(/\s+=\s+/, $_, 2) ;
	$k = uc $k ;
	if ($ValidOption{$k}) {
	    delete $Parsed{$k} ;
	    $Info{$k} = $v ;
	}
	else {
	    push(@badkey, $k) ;
	}
    }
    close F ;

    print "Unknown keys in $CONFIG ignored [@badkey]\n"
	if @badkey ;

    # check parsed values
    my @missing = () ;
    die "The following keys are missing from $CONFIG file: [@missing]\n" 
        if @missing = keys %Parsed ;

    $INC_DIR = $ENV{'DB_FILE_INCLUDE'} || $Info{'INCLUDE'} ;
    $LIB_DIR = $ENV{'DB_FILE_LIB'} || $Info{'LIB'} ;
    $DB_NAME = $ENV{'DB_FILE_NAME'} || $Info{'DBNAME'} ;
    $COMPAT185 = "-DCOMPAT185 -DDB_LIBRARY_COMPATIBILITY_API" 
        if (defined $ENV{'DB_FILE_COMPAT185'} && 
		$ENV{'DB_FILE_COMPAT185'} =~ /^\s*(on|true|1)\s*$/i) ||
		$Info{'COMPAT185'} =~ /^\s*(on|true|1)\s*$/i ; 
    my $PREFIX  = $Info{'PREFIX'} ;
    my $HASH    = $Info{'HASH'} ;

    $VER_INFO = "-DmDB_Prefix_t=${PREFIX} -DmDB_Hash_t=${HASH}" ;

    print <<EOM if 0 ;
    INCLUDE	[$INC_DIR]
    LIB		[$LIB_DIR]
    HASH	[$HASH]
    PREFIX	[$PREFIX]
    DBNAME      [$DB_NAME]

EOM

    print "Looks Good.\n" ;

}

sub UpDowngrade
{
    my @files = @_ ;

    # our      is stable from 5.6.0 onward
    # warnings is stable from 5.6.1 onward

    # Note: this code assumes that each statement it modifies is not
    #       split across multiple lines.


    my $warn_sub ;
    my $our_sub ;

    if ($] < 5.006001) {
        # From: use|no warnings "blah"
        # To:   local ($^W) = 1; # use|no warnings "blah"
        #
        # and
        #
        # From: warnings::warnif(x,y);
        # To:   $^W && carp(y); # warnif -- x
        $warn_sub = sub {
	    s/^(\s*)(no\s+warnings)/${1}local (\$^W) = 0; #$2/ ;
	    s/^(\s*)(use\s+warnings)/${1}local (\$^W) = 1; #$2/ ;

	    s/^(\s*)warnings::warnif\s*\((.*?)\s*,\s*(.*?)\)\s*;/${1}\$^W && carp($3); # warnif - $2/ ;
          };
    }
    else {
        # From: local ($^W) = 1; # use|no warnings "blah"
        # To:   use|no warnings "blah"
        #
        # and
        #
        # From: $^W && carp(y); # warnif -- x
        # To:   warnings::warnif(x,y);
        $warn_sub = sub {
	    s/^(\s*)local\s*\(\$\^W\)\s*=\s*\d+\s*;\s*#\s*((no|use)\s+warnings.*)/$1$2/ ;
	    s/^(\s*)\$\^W\s+\&\&\s*carp\s*\((.*?)\)\s*;\s*#\s*warnif\s*-\s*(.*)/${1}warnings::warnif($3, $2);/ ;
          };
    }

    if ($] < 5.006000) {
        $our_sub = sub {
	    if ( /^(\s*)our\s+\(\s*([^)]+\s*)\)/ ) {
                my $indent = $1;
                my $vars = join ' ', split /\s*,\s*/, $2;
                $_ = "${indent}use vars qw($vars);\n";
            }
          };
    }
    else {
        $our_sub = sub {
	    if ( /^(\s*)use\s+vars\s+qw\((.*?)\)/ ) {
                my $indent = $1;
                my $vars = join ', ', split ' ', $2;
                $_ = "${indent}our ($vars);\n";
            }
          };
    }

    foreach (@files)
      { doUpDown($our_sub, $warn_sub, $_) }
}


sub doUpDown
{
    my $our_sub = shift;
    my $warn_sub = shift;

    local ($^I) = ".bak" ;
    local (@ARGV) = shift;

    while (<>)
    {
	print, last if /^__(END|DATA)__/ ;

	&{ $our_sub }();
	&{ $warn_sub }();
	print ;
    }

    return if eof ;

    while (<>)
      { print }
}

# end of file Makefile.PL

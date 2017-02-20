#! perl -w

# It should not be necessary to edit this file. The configuration for
# BerkeleyDB is controlled from the file config.in


BEGIN { die "BerkeleyDB needs Perl 5.004_04 or greater" if $] < 5.004_04 ; }

use strict ;
use ExtUtils::MakeMaker ;
use Config ;

# Check for the presence of sfio
if ($Config{'d_sfio'}) {
   print <<EOM;

WARNING: Perl seems to have been built with SFIO support enabled.
         Please read the SFIO Notes in the README file.

EOM
}

my $LIB_DIR ;
my $INC_DIR ;
my $DB_NAME ;
my $LIBS ;

ParseCONFIG() ;

if (defined $DB_NAME)
  { $LIBS = $DB_NAME }
else {
    if ($^O eq 'MSWin32')
      { $LIBS = '-llibdb' }
    elsif ($^O =~ /aix/i )  {
      $LIBS .= '-ldb -lpthread ';
      if ($Config{'cc'} eq 'gcc' && $Config{'osvers'} eq '5.1')
        { $LIBS .= '-lgcc_s' }
    }
    else
      { $LIBS = '-ldb' }
}

# OS2 is a special case, so check for it now.
my $OS2 = "" ;
$OS2 = "-DOS2" if $^O eq 'os2' ;

my $WALL = '';
#$WALL = ' -Wall ' if $Config{'cc'} =~ /gcc/ ;


WriteMakefile(
	NAME 		=> 'BerkeleyDB',
	LIBS 		=> ["-L${LIB_DIR} $LIBS"],
	#MAN3PODS        => {},         # Pods will be built by installman. 
	INC		=> "-I$INC_DIR",
	VERSION_FROM	=> 'BerkeleyDB.pm',
	XSPROTOARG	=> '-noprototypes',
	DEFINE		=> "$OS2 $WALL",
	#'macro'		=> { INSTALLDIRS => 'perl' },
        'dist'          => {COMPRESS=>'gzip', SUFFIX=>'gz'},    
	($] >= 5.005
	    ? (ABSTRACT_FROM	=> 'BerkeleyDB.pod',
	       AUTHOR  	=> 'Paul Marquess <pmqs@cpan.org>')
	    : ()
	),
    ((ExtUtils::MakeMaker->VERSION() gt '6.30') 
        ?  ('LICENSE'  => 'perl')         
        : ()
    ),

	);


sub MY::libscan
{
    my $self = shift ;
    my $path = shift ;

    return undef
        if $path =~ /(~|\.bak)$/ || 
           $path =~ /^\..*\.swp$/ ;

    return $path;    
}

    
sub MY::postamble {
	'
$(NAME).pod:	$(NAME).pod.P t/examples.t.T t/examples3.t.T mkpod
	perl ./mkpod

$(NAME).xs:	typemap
	$(TOUCH) $(NAME).xs

Makefile:	config.in 


' ;
}

sub ParseCONFIG
{
    my ($k, $v) ;
    my @badkey = () ;
    my %Info = () ;
    my @Options = qw( INCLUDE LIB DBNAME ) ;
    my %ValidOption = map {$_, 1} @Options ;
    my %Parsed = %ValidOption ;
    my $CONFIG = 'config.in' ;

    print "Parsing $CONFIG...\n" ;

    # DBNAME is optional, so pretend it has been parsed.
    delete $Parsed{'DBNAME'} ;

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

    $INC_DIR =  $ENV{'BERKELEYDB_INCLUDE'} || $Info{'INCLUDE'} ;
    $LIB_DIR =  $ENV{'BERKELEYDB_LIB'} || $Info{'LIB'} ;
    $DB_NAME = $ENV{BERKELEYDB_NAME} || $Info{'DBNAME'} ;
    #$DB_NAME =  $ENV{} || $Info{'DBNAME'} if defined $Info{'DBNAME'} ;

    print "Looks Good.\n" ;

}

# end of file Makefile.PL

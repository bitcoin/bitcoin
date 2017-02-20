#!./perl -w

BEGIN {
    unless(grep /blib/, @INC) {
        chdir 't' if -d 't';
        @INC = '../lib' if -d '../lib';
    }
}
 
use warnings;
use strict;
use Config;
 
BEGIN {
    if(-d "lib" && -f "TEST") {
        if ($Config{'extensions'} !~ /\bDB_File\b/ ) {
            print "1..0 # Skip: DB_File was not built\n";
            exit 0;
        }
    }
}

use DB_File; 
use Fcntl;
our ($dbh, $Dfile, $bad_ones, $FA);

# full tied array support started in Perl 5.004_57
# Double check to see if it is available.

{
    sub try::TIEARRAY { bless [], "try" }
    sub try::FETCHSIZE { $FA = 1 }
    $FA = 0 ;
    my @a ; 
    tie @a, 'try' ;
    my $a = @a ;
}


sub ok
{
    my $no = shift ;
    my $result = shift ;

    print "not " unless $result ;
    print "ok $no\n" ;

    return $result ;
}

{
    package Redirect ;
    use Symbol ;

    sub new
    {
        my $class = shift ;
        my $filename = shift ;
	my $fh = gensym ;
	open ($fh, ">$filename") || die "Cannot open $filename: $!" ;
	my $real_stdout = select($fh) ;
	return bless [$fh, $real_stdout ] ;

    }
    sub DESTROY
    {
        my $self = shift ;
	close $self->[0] ;
	select($self->[1]) ;
    }
}

sub docat
{
    my $file = shift;
    local $/ = undef;
    open(CAT,$file) || die "Cannot open $file:$!";
    my $result = <CAT>;
    close(CAT);
    normalise($result) ;
    return $result;
}

sub docat_del
{ 
    my $file = shift;
    my $result = docat($file);
    unlink $file ;
    return $result;
}   

sub safeUntie
{
    my $hashref = shift ;
    my $no_inner = 1;
    local $SIG{__WARN__} = sub {-- $no_inner } ;
    untie @$hashref;
    return $no_inner;
}

sub bad_one
{
    unless ($bad_ones++) {
	print STDERR <<EOM ;
#
# Some older versions of Berkeley DB version 1 will fail db-recno
# tests 61, 63, 64 and 65.
EOM
        if ($^O eq 'darwin'
	    && $Config{db_version_major} == 1
	    && $Config{db_version_minor} == 0
	    && $Config{db_version_patch} == 0) {
	    print STDERR <<EOM ;
#
# For example Mac OS X 10.2 (or earlier) has such an old
# version of Berkeley DB.
EOM
	}

	print STDERR <<EOM ;
#
# You can safely ignore the errors if you're never going to use the
# broken functionality (recno databases with a modified bval). 
# Otherwise you'll have to upgrade your DB library.
#
# If you want to use Berkeley DB version 1, then 1.85 and 1.86 are the
# last versions that were released. Berkeley DB version 2 is continually
# being updated -- Check out http://www.sleepycat.com/ for more details.
#
EOM
    }
}

sub normalise
{
    return unless $^O eq 'cygwin' ;
    foreach (@_)
      { s#\r\n#\n#g }     
}

BEGIN 
{ 
    { 
        local $SIG{__DIE__} ; 
        eval { require Data::Dumper ; import Data::Dumper } ; 
    }
 
    if ($@) {
        *Dumper = sub { my $a = shift; return "[ @{ $a } ]" } ;
    }          
}

my $splice_tests = 10 + 12 + 1; # ten regressions, plus the randoms
my $total_tests = 181 ;
$total_tests += $splice_tests if $FA ;
print "1..$total_tests\n";   

$Dfile = "recno.tmp";
unlink $Dfile ;

umask(0);

# Check the interface to RECNOINFO

$dbh = new DB_File::RECNOINFO ;
ok(1, ! defined $dbh->{bval}) ;
ok(2, ! defined $dbh->{cachesize}) ;
ok(3, ! defined $dbh->{psize}) ;
ok(4, ! defined $dbh->{flags}) ;
ok(5, ! defined $dbh->{lorder}) ;
ok(6, ! defined $dbh->{reclen}) ;
ok(7, ! defined $dbh->{bfname}) ;

$dbh->{bval} = 3000 ;
ok(8, $dbh->{bval} == 3000 );

$dbh->{cachesize} = 9000 ;
ok(9, $dbh->{cachesize} == 9000 );

$dbh->{psize} = 400 ;
ok(10, $dbh->{psize} == 400 );

$dbh->{flags} = 65 ;
ok(11, $dbh->{flags} == 65 );

$dbh->{lorder} = 123 ;
ok(12, $dbh->{lorder} == 123 );

$dbh->{reclen} = 1234 ;
ok(13, $dbh->{reclen} == 1234 );

$dbh->{bfname} = 1234 ;
ok(14, $dbh->{bfname} == 1234 );


# Check that an invalid entry is caught both for store & fetch
eval '$dbh->{fred} = 1234' ;
ok(15, $@ =~ /^DB_File::RECNOINFO::STORE - Unknown element 'fred' at/ );
eval 'my $q = $dbh->{fred}' ;
ok(16, $@ =~ /^DB_File::RECNOINFO::FETCH - Unknown element 'fred' at/ );

# Now check the interface to RECNOINFO

my $X  ;
my @h ;
ok(17, $X = tie @h, 'DB_File', $Dfile, O_RDWR|O_CREAT, 0640, $DB_RECNO ) ;

my %noMode = map { $_, 1} qw( amigaos MSWin32 NetWare cygwin ) ;

ok(18, ((stat($Dfile))[2] & 0777) == (($^O eq 'os2' || $^O eq 'MacOS') ? 0666 : 0640)
	||  $noMode{$^O} );

#my $l = @h ;
my $l = $X->length ;
ok(19, ($FA ? @h == 0 : !$l) );

my @data = qw( a b c d ever f g h  i j k longername m n o p) ;

$h[0] = shift @data ;
ok(20, $h[0] eq 'a' );

my $ i;
foreach (@data)
  { $h[++$i] = $_ }

unshift (@data, 'a') ;

ok(21, defined $h[1] );
ok(22, ! defined $h[16] );
ok(23, $FA ? @h == @data : $X->length == @data );


# Overwrite an entry & check fetch it
$h[3] = 'replaced' ;
$data[3] = 'replaced' ;
ok(24, $h[3] eq 'replaced' );

#PUSH
my @push_data = qw(added to the end) ;
($FA ? push(@h, @push_data) : $X->push(@push_data)) ;
push (@data, @push_data) ;
ok(25, $h[++$i] eq 'added' );
ok(26, $h[++$i] eq 'to' );
ok(27, $h[++$i] eq 'the' );
ok(28, $h[++$i] eq 'end' );

# POP
my $popped = pop (@data) ;
my $value = ($FA ? pop @h : $X->pop) ;
ok(29, $value eq $popped) ;

# SHIFT
$value = ($FA ? shift @h : $X->shift) ;
my $shifted = shift @data ;
ok(30, $value eq $shifted );

# UNSHIFT

# empty list
($FA ? unshift @h,() : $X->unshift) ;
ok(31, ($FA ? @h == @data : $X->length == @data ));

my @new_data = qw(add this to the start of the array) ;
$FA ? unshift (@h, @new_data) : $X->unshift (@new_data) ;
unshift (@data, @new_data) ;
ok(32, $FA ? @h == @data : $X->length == @data );
ok(33, $h[0] eq "add") ;
ok(34, $h[1] eq "this") ;
ok(35, $h[2] eq "to") ;
ok(36, $h[3] eq "the") ;
ok(37, $h[4] eq "start") ;
ok(38, $h[5] eq "of") ;
ok(39, $h[6] eq "the") ;
ok(40, $h[7] eq "array") ;
ok(41, $h[8] eq $data[8]) ;

# Brief test for SPLICE - more thorough 'soak test' is later.
my @old;
if ($FA) {
    @old = splice(@h, 1, 2, qw(bananas just before));
}
else {
    @old = $X->splice(1, 2, qw(bananas just before));
}
ok(42, $h[0] eq "add") ;
ok(43, $h[1] eq "bananas") ;
ok(44, $h[2] eq "just") ;
ok(45, $h[3] eq "before") ;
ok(46, $h[4] eq "the") ;
ok(47, $h[5] eq "start") ;
ok(48, $h[6] eq "of") ;
ok(49, $h[7] eq "the") ;
ok(50, $h[8] eq "array") ;
ok(51, $h[9] eq $data[8]) ;
$FA ? splice(@h, 1, 3, @old) : $X->splice(1, 3, @old);

# Now both arrays should be identical

my $ok = 1 ;
my $j = 0 ;
foreach (@data)
{
   $ok = 0, last if $_ ne $h[$j ++] ; 
}
ok(52, $ok );

# Neagtive subscripts

# get the last element of the array
ok(53, $h[-1] eq $data[-1] );
ok(54, $h[-1] eq $h[ ($FA ? @h : $X->length) -1] );

# get the first element using a negative subscript
eval '$h[ - ( $FA ? @h : $X->length)] = "abcd"' ;
ok(55, $@ eq "" );
ok(56, $h[0] eq "abcd" );

# now try to read before the start of the array
eval '$h[ - (1 + ($FA ? @h : $X->length))] = 1234' ;
ok(57, $@ =~ '^Modification of non-creatable array value attempted' );

# IMPORTANT - $X must be undefined before the untie otherwise the
#             underlying DB close routine will not get called.
undef $X ;
ok(58, safeUntie \@h);

unlink $Dfile;


{
    # Check bval defaults to \n

    my @h = () ;
    my $dbh = new DB_File::RECNOINFO ;
    ok(59, tie @h, 'DB_File', $Dfile, O_RDWR|O_CREAT, 0640, $dbh ) ;
    $h[0] = "abc" ;
    $h[1] = "def" ;
    $h[3] = "ghi" ;
    ok(60, safeUntie \@h);
    my $x = docat($Dfile) ;
    unlink $Dfile;
    ok(61, $x eq "abc\ndef\n\nghi\n") ;
}

{
    # Change bval

    my @h = () ;
    my $dbh = new DB_File::RECNOINFO ;
    $dbh->{bval} = "-" ;
    ok(62, tie @h, 'DB_File', $Dfile, O_RDWR|O_CREAT, 0640, $dbh ) ;
    $h[0] = "abc" ;
    $h[1] = "def" ;
    $h[3] = "ghi" ;
    ok(63, safeUntie \@h);
    my $x = docat($Dfile) ;
    unlink $Dfile;
    my $ok = ($x eq "abc-def--ghi-") ;
    bad_one() unless $ok ;
    ok(64, $ok) ;
}

{
    # Check R_FIXEDLEN with default bval (space)

    my @h = () ;
    my $dbh = new DB_File::RECNOINFO ;
    $dbh->{flags} = R_FIXEDLEN ;
    $dbh->{reclen} = 5 ;
    ok(65, tie @h, 'DB_File', $Dfile, O_RDWR|O_CREAT, 0640, $dbh ) ;
    $h[0] = "abc" ;
    $h[1] = "def" ;
    $h[3] = "ghi" ;
    ok(66, safeUntie \@h);
    my $x = docat($Dfile) ;
    unlink $Dfile;
    my $ok = ($x eq "abc  def       ghi  ") ;
    bad_one() unless $ok ;
    ok(67, $ok) ;
}

{
    # Check R_FIXEDLEN with user-defined bval

    my @h = () ;
    my $dbh = new DB_File::RECNOINFO ;
    $dbh->{flags} = R_FIXEDLEN ;
    $dbh->{bval} = "-" ;
    $dbh->{reclen} = 5 ;
    ok(68, tie @h, 'DB_File', $Dfile, O_RDWR|O_CREAT, 0640, $dbh ) ;
    $h[0] = "abc" ;
    $h[1] = "def" ;
    $h[3] = "ghi" ;
    ok(69, safeUntie \@h);
    my $x = docat($Dfile) ;
    unlink $Dfile;
    my $ok = ($x eq "abc--def-------ghi--") ;
    bad_one() unless $ok ;
    ok(70, $ok) ;
}

{
    # check that attempting to tie an associative array to a DB_RECNO will fail

    my $filename = "xyz" ;
    my %x ;
    eval { tie %x, 'DB_File', $filename, O_RDWR|O_CREAT, 0640, $DB_RECNO ; } ;
    ok(71, $@ =~ /^DB_File can only tie an array to a DB_RECNO database/) ;
    unlink $filename ;
}

{
   # sub-class test

   package Another ;

   use warnings ;
   use strict ;

   open(FILE, ">SubDB.pm") or die "Cannot open SubDB.pm: $!\n" ;
   print FILE <<'EOM' ;

   package SubDB ;

   use warnings ;
   use strict ;
   our (@ISA, @EXPORT);

   require Exporter ;
   use DB_File;
   @ISA=qw(DB_File);
   @EXPORT = @DB_File::EXPORT ;

   sub STORE { 
	my $self = shift ;
        my $key = shift ;
        my $value = shift ;
        $self->SUPER::STORE($key, $value * 2) ;
   }

   sub FETCH { 
	my $self = shift ;
        my $key = shift ;
        $self->SUPER::FETCH($key) - 1 ;
   }

   sub put { 
	my $self = shift ;
        my $key = shift ;
        my $value = shift ;
        $self->SUPER::put($key, $value * 3) ;
   }

   sub get { 
	my $self = shift ;
        $self->SUPER::get($_[0], $_[1]) ;
	$_[1] -= 2 ;
   }

   sub A_new_method
   {
	my $self = shift ;
        my $key = shift ;
        my $value = $self->FETCH($key) ;
	return "[[$value]]" ;
   }

   1 ;
EOM

    close FILE  or die "Could not close: $!";

    BEGIN { push @INC, '.'; } 
    eval 'use SubDB ; ';
    main::ok(72, $@ eq "") ;
    my @h ;
    my $X ;
    eval '
	$X = tie(@h, "SubDB","recno.tmp", O_RDWR|O_CREAT, 0640, $DB_RECNO );
	' ;
    die "Could not tie: $!" unless $X;

    main::ok(73, $@ eq "") ;

    my $ret = eval '$h[3] = 3 ; return $h[3] ' ;
    main::ok(74, $@ eq "") ;
    main::ok(75, $ret == 5) ;

    my $value = 0;
    $ret = eval '$X->put(1, 4) ; $X->get(1, $value) ; return $value' ;
    main::ok(76, $@ eq "") ;
    main::ok(77, $ret == 10) ;

    $ret = eval ' R_NEXT eq main::R_NEXT ' ;
    main::ok(78, $@ eq "" ) ;
    main::ok(79, $ret == 1) ;

    $ret = eval '$X->A_new_method(1) ' ;
    main::ok(80, $@ eq "") ;
    main::ok(81, $ret eq "[[11]]") ;

    undef $X;
    main::ok(82, main::safeUntie \@h);
    unlink "SubDB.pm", "recno.tmp" ;

}

{

    # test $#
    my $self ;
    unlink $Dfile;
    ok(83, $self = tie @h, 'DB_File', $Dfile, O_RDWR|O_CREAT, 0640, $DB_RECNO ) ;
    $h[0] = "abc" ;
    $h[1] = "def" ;
    $h[2] = "ghi" ;
    $h[3] = "jkl" ;
    ok(84, $FA ? $#h == 3 : $self->length() == 4) ;
    undef $self ;
    ok(85, safeUntie \@h);
    my $x = docat($Dfile) ;
    ok(86, $x eq "abc\ndef\nghi\njkl\n") ;

    # $# sets array to same length
    $self = tie @h, 'DB_File', $Dfile, O_RDWR, 0640, $DB_RECNO ;
    ok(87, $self) 
        or warn "# $DB_File::Error\n";
    if ($FA)
      { $#h = 3 }
    else 
      { $self->STORESIZE(4) }
    ok(88, $FA ? $#h == 3 : $self->length() == 4) ;
    undef $self ;
    ok(89, safeUntie \@h);
    $x = docat($Dfile) ;
    ok(90, $x eq "abc\ndef\nghi\njkl\n") ;

    # $# sets array to bigger
    ok(91, $self = tie @h, 'DB_File', $Dfile, O_RDWR, 0640, $DB_RECNO ) ;
    if ($FA)
      { $#h = 6 }
    else 
      { $self->STORESIZE(7) }
    ok(92, $FA ? $#h == 6 : $self->length() == 7) ;
    undef $self ;
    ok(93, safeUntie \@h);
    $x = docat($Dfile) ;
    ok(94, $x eq "abc\ndef\nghi\njkl\n\n\n\n") ;

    # $# sets array smaller
    ok(95, $self = tie @h, 'DB_File', $Dfile, O_RDWR, 0640, $DB_RECNO ) ;
    if ($FA)
      { $#h = 2 }
    else 
      { $self->STORESIZE(3) }
    ok(96, $FA ? $#h == 2 : $self->length() == 3) ;
    undef $self ;
    ok(97, safeUntie \@h);
    $x = docat($Dfile) ;
    ok(98, $x eq "abc\ndef\nghi\n") ;

    unlink $Dfile;


}

{
   # DBM Filter tests
   use warnings ;
   use strict ;
   my (@h, $db) ;
   my ($fetch_key, $store_key, $fetch_value, $store_value) = ("") x 4 ;
   unlink $Dfile;

   sub checkOutput
   {
       my($fk, $sk, $fv, $sv) = @_ ;

       print "# Fetch Key   : expected '$fk' got '$fetch_key'\n" 
           if $fetch_key ne $fk ;
       print "# Fetch Value : expected '$fv' got '$fetch_value'\n" 
           if $fetch_value ne $fv ;
       print "# Store Key   : expected '$sk' got '$store_key'\n" 
           if $store_key ne $sk ;
       print "# Store Value : expected '$sv' got '$store_value'\n" 
           if $store_value ne $sv ;
       print "# \$_          : expected 'original' got '$_'\n" 
           if $_ ne 'original' ;

       return
           $fetch_key   eq $fk && $store_key   eq $sk && 
	   $fetch_value eq $fv && $store_value eq $sv &&
	   $_ eq 'original' ;
   }
   
   ok(99, $db = tie(@h, 'DB_File', $Dfile, O_RDWR|O_CREAT, 0640, $DB_RECNO ) );

   $db->filter_fetch_key   (sub { $fetch_key = $_ }) ;
   $db->filter_store_key   (sub { $store_key = $_ }) ;
   $db->filter_fetch_value (sub { $fetch_value = $_}) ;
   $db->filter_store_value (sub { $store_value = $_ }) ;

   $_ = "original" ;

   $h[0] = "joe" ;
   #                   fk   sk     fv   sv
   ok(100, checkOutput( "", 0, "", "joe")) ;

   ($fetch_key, $store_key, $fetch_value, $store_value) = ("") x 4 ;
   ok(101, $h[0] eq "joe");
   #                   fk  sk  fv    sv
   ok(102, checkOutput( "", 0, "joe", "")) ;

   ($fetch_key, $store_key, $fetch_value, $store_value) = ("") x 4 ;
   ok(103, $db->FIRSTKEY() == 0) ;
   #                    fk     sk  fv  sv
   ok(104, checkOutput( 0, "", "", "")) ;

   # replace the filters, but remember the previous set
   my ($old_fk) = $db->filter_fetch_key   
   			(sub { ++ $_ ; $fetch_key = $_ }) ;
   my ($old_sk) = $db->filter_store_key   
   			(sub { $_ *= 2 ; $store_key = $_ }) ;
   my ($old_fv) = $db->filter_fetch_value 
   			(sub { $_ = "[$_]"; $fetch_value = $_ }) ;
   my ($old_sv) = $db->filter_store_value 
   			(sub { s/o/x/g; $store_value = $_ }) ;
   
   ($fetch_key, $store_key, $fetch_value, $store_value) = ("") x 4 ;
   $h[1] = "Joe" ;
   #                   fk   sk     fv    sv
   ok(105, checkOutput( "", 2, "", "Jxe")) ;

   ($fetch_key, $store_key, $fetch_value, $store_value) = ("") x 4 ;
   ok(106, $h[1] eq "[Jxe]");
   #                   fk   sk     fv    sv
   ok(107, checkOutput( "", 2, "[Jxe]", "")) ;

   ($fetch_key, $store_key, $fetch_value, $store_value) = ("") x 4 ;
   ok(108, $db->FIRSTKEY() == 1) ;
   #                   fk   sk     fv    sv
   ok(109, checkOutput( 1, "", "", "")) ;
   
   # put the original filters back
   $db->filter_fetch_key   ($old_fk);
   $db->filter_store_key   ($old_sk);
   $db->filter_fetch_value ($old_fv);
   $db->filter_store_value ($old_sv);

   ($fetch_key, $store_key, $fetch_value, $store_value) = ("") x 4 ;
   $h[0] = "joe" ;
   ok(110, checkOutput( "", 0, "", "joe")) ;

   ($fetch_key, $store_key, $fetch_value, $store_value) = ("") x 4 ;
   ok(111, $h[0] eq "joe");
   ok(112, checkOutput( "", 0, "joe", "")) ;

   ($fetch_key, $store_key, $fetch_value, $store_value) = ("") x 4 ;
   ok(113, $db->FIRSTKEY() == 0) ;
   ok(114, checkOutput( 0, "", "", "")) ;

   # delete the filters
   $db->filter_fetch_key   (undef);
   $db->filter_store_key   (undef);
   $db->filter_fetch_value (undef);
   $db->filter_store_value (undef);

   ($fetch_key, $store_key, $fetch_value, $store_value) = ("") x 4 ;
   $h[0] = "joe" ;
   ok(115, checkOutput( "", "", "", "")) ;

   ($fetch_key, $store_key, $fetch_value, $store_value) = ("") x 4 ;
   ok(116, $h[0] eq "joe");
   ok(117, checkOutput( "", "", "", "")) ;

   ($fetch_key, $store_key, $fetch_value, $store_value) = ("") x 4 ;
   ok(118, $db->FIRSTKEY() == 0) ;
   ok(119, checkOutput( "", "", "", "")) ;

   undef $db ;
   ok(120, safeUntie \@h);
   unlink $Dfile;
}

{    
    # DBM Filter with a closure

    use warnings ;
    use strict ;
    my (@h, $db) ;

    unlink $Dfile;
    ok(121, $db = tie(@h, 'DB_File', $Dfile, O_RDWR|O_CREAT, 0640, $DB_RECNO ) );

    my %result = () ;

    sub Closure
    {
        my ($name) = @_ ;
	my $count = 0 ;
	my @kept = () ;

	return sub { ++$count ; 
		     push @kept, $_ ; 
		     $result{$name} = "$name - $count: [@kept]" ;
		   }
    }

    $db->filter_store_key(Closure("store key")) ;
    $db->filter_store_value(Closure("store value")) ;
    $db->filter_fetch_key(Closure("fetch key")) ;
    $db->filter_fetch_value(Closure("fetch value")) ;

    $_ = "original" ;

    $h[0] = "joe" ;
    ok(122, $result{"store key"} eq "store key - 1: [0]");
    ok(123, $result{"store value"} eq "store value - 1: [joe]");
    ok(124, ! defined $result{"fetch key"} );
    ok(125, ! defined $result{"fetch value"} );
    ok(126, $_ eq "original") ;

    ok(127, $db->FIRSTKEY() == 0 ) ;
    ok(128, $result{"store key"} eq "store key - 1: [0]");
    ok(129, $result{"store value"} eq "store value - 1: [joe]");
    ok(130, $result{"fetch key"} eq "fetch key - 1: [0]");
    ok(131, ! defined $result{"fetch value"} );
    ok(132, $_ eq "original") ;

    $h[7]  = "john" ;
    ok(133, $result{"store key"} eq "store key - 2: [0 7]");
    ok(134, $result{"store value"} eq "store value - 2: [joe john]");
    ok(135, $result{"fetch key"} eq "fetch key - 1: [0]");
    ok(136, ! defined $result{"fetch value"} );
    ok(137, $_ eq "original") ;

    ok(138, $h[0] eq "joe");
    ok(139, $result{"store key"} eq "store key - 3: [0 7 0]");
    ok(140, $result{"store value"} eq "store value - 2: [joe john]");
    ok(141, $result{"fetch key"} eq "fetch key - 1: [0]");
    ok(142, $result{"fetch value"} eq "fetch value - 1: [joe]");
    ok(143, $_ eq "original") ;

    undef $db ;
    ok(144, safeUntie \@h);
    unlink $Dfile;
}		

{
   # DBM Filter recursion detection
   use warnings ;
   use strict ;
   my (@h, $db) ;
   unlink $Dfile;

   ok(145, $db = tie(@h, 'DB_File', $Dfile, O_RDWR|O_CREAT, 0640, $DB_RECNO ) );

   $db->filter_store_key (sub { $_ = $h[0] }) ;

   eval '$h[1] = 1234' ;
   ok(146, $@ =~ /^recursion detected in filter_store_key at/ );
   
   undef $db ;
   ok(147, safeUntie \@h);
   unlink $Dfile;
}


{
   # Examples from the POD

  my $file = "xyzt" ;
  {
    my $redirect = new Redirect $file ;

    use warnings FATAL => qw(all);
    use strict ;
    use DB_File ;

    my $filename = "text" ;
    unlink $filename ;

    my @h ;
    my $x = tie @h, "DB_File", $filename, O_RDWR|O_CREAT, 0640, $DB_RECNO 
        or die "Cannot open file 'text': $!\n" ;

    # Add a few key/value pairs to the file
    $h[0] = "orange" ;
    $h[1] = "blue" ;
    $h[2] = "yellow" ;

    $FA ? push @h, "green", "black" 
        : $x->push("green", "black") ;

    my $elements = $FA ? scalar @h : $x->length ;
    print "The array contains $elements entries\n" ;

    my $last = $FA ? pop @h : $x->pop ;
    print "popped $last\n" ;

    $FA ? unshift @h, "white" 
        : $x->unshift("white") ;
    my $first = $FA ? shift @h : $x->shift ;
    print "shifted $first\n" ;

    # Check for existence of a key
    print "Element 1 Exists with value $h[1]\n" if $h[1] ;

    # use a negative index
    print "The last element is $h[-1]\n" ;
    print "The 2nd last element is $h[-2]\n" ;

    undef $x ;
    untie @h ;

    unlink $filename ;
  }  

  ok(148, docat_del($file) eq <<'EOM') ;
The array contains 5 entries
popped black
shifted white
Element 1 Exists with value blue
The last element is green
The 2nd last element is yellow
EOM

  my $save_output = "xyzt" ;
  {
    my $redirect = new Redirect $save_output ;

    use warnings FATAL => qw(all);
    use strict ;
    our (@h, $H, $file, $i);
    use DB_File ;
    use Fcntl ;
    
    $file = "text" ;

    unlink $file ;

    $H = tie @h, "DB_File", $file, O_RDWR|O_CREAT, 0640, $DB_RECNO 
        or die "Cannot open file $file: $!\n" ;
    
    # first create a text file to play with
    $h[0] = "zero" ;
    $h[1] = "one" ;
    $h[2] = "two" ;
    $h[3] = "three" ;
    $h[4] = "four" ;

    
    # Print the records in order.
    #
    # The length method is needed here because evaluating a tied
    # array in a scalar context does not return the number of
    # elements in the array.  

    print "\nORIGINAL\n" ;
    foreach $i (0 .. $H->length - 1) {
        print "$i: $h[$i]\n" ;
    }

    # use the push & pop methods
    $a = $H->pop ;
    $H->push("last") ;
    print "\nThe last record was [$a]\n" ;

    # and the shift & unshift methods
    $a = $H->shift ;
    $H->unshift("first") ;
    print "The first record was [$a]\n" ;

    # Use the API to add a new record after record 2.
    $i = 2 ;
    $H->put($i, "Newbie", R_IAFTER) ;

    # and a new record before record 1.
    $i = 1 ;
    $H->put($i, "New One", R_IBEFORE) ;

    # delete record 3
    $H->del(3) ;

    # now print the records in reverse order
    print "\nREVERSE\n" ;
    for ($i = $H->length - 1 ; $i >= 0 ; -- $i)
      { print "$i: $h[$i]\n" }

    # same again, but use the API functions instead
    print "\nREVERSE again\n" ;
    my ($s, $k, $v)  = (0, 0, 0) ;
    for ($s = $H->seq($k, $v, R_LAST) ; 
             $s == 0 ; 
             $s = $H->seq($k, $v, R_PREV))
      { print "$k: $v\n" }

    undef $H ;
    untie @h ;    

    unlink $file ;
  }  

  ok(149, docat_del($save_output) eq <<'EOM') ;

ORIGINAL
0: zero
1: one
2: two
3: three
4: four

The last record was [four]
The first record was [zero]

REVERSE
5: last
4: three
3: Newbie
2: one
1: New One
0: first

REVERSE again
5: last
4: three
3: Newbie
2: one
1: New One
0: first
EOM
   
}

{
    # Bug ID 20001013.009
    #
    # test that $hash{KEY} = undef doesn't produce the warning
    #     Use of uninitialized value in null operation 
    use warnings ;
    use strict ;
    use DB_File ;

    unlink $Dfile;
    my @h ;
    my $a = "";
    local $SIG{__WARN__} = sub {$a = $_[0]} ;
    
    tie @h, 'DB_File', $Dfile, O_RDWR|O_CREAT, 0664, $DB_RECNO 
	or die "Can't open file: $!\n" ;
    $h[0] = undef;
    ok(150, $a eq "") ;
    ok(151, safeUntie \@h);
    unlink $Dfile;
}

{
    # test that %hash = () doesn't produce the warning
    #     Argument "" isn't numeric in entersub
    use warnings ;
    use strict ;
    use DB_File ;
    my $a = "";
    local $SIG{__WARN__} = sub {$a = $_[0]} ;

    unlink $Dfile;
    my @h ;
    
    tie @h, 'DB_File', $Dfile, O_RDWR|O_CREAT, 0664, $DB_RECNO 
	or die "Can't open file: $!\n" ;
    @h = (); ;
    ok(152, $a eq "") ;
    ok(153, safeUntie \@h);
    unlink $Dfile;
}

{
   # Check that DBM Filter can cope with read-only $_

   use warnings ;
   use strict ;
   my (@h, $db) ;
   unlink $Dfile;

   ok(154, $db = tie(@h, 'DB_File', $Dfile, O_RDWR|O_CREAT, 0640, $DB_RECNO ) );

   $db->filter_fetch_key   (sub { }) ;
   $db->filter_store_key   (sub { }) ;
   $db->filter_fetch_value (sub { }) ;
   $db->filter_store_value (sub { }) ;

   $_ = "original" ;

   $h[0] = "joe" ;
   ok(155, $h[0] eq "joe");

   eval { my @r= grep { $h[$_] } (1, 2, 3) };
   ok (156, ! $@);


   # delete the filters
   $db->filter_fetch_key   (undef);
   $db->filter_store_key   (undef);
   $db->filter_fetch_value (undef);
   $db->filter_store_value (undef);

   $h[1] = "joe" ;

   ok(157, $h[1] eq "joe");

   eval { my @r= grep { $h[$_] } (1, 2, 3) };
   ok (158, ! $@);

   undef $db ;
   untie @h;
   unlink $Dfile;
}

{
   # Check low-level API works with filter

   use warnings ;
   use strict ;
   my (@h, $db) ;
   my $Dfile = "xxy.db";
   unlink $Dfile;

   ok(159, $db = tie(@h, 'DB_File', $Dfile, O_RDWR|O_CREAT, 0640, $DB_RECNO ) );


   $db->filter_fetch_key   (sub { ++ $_ } );
   $db->filter_store_key   (sub { -- $_ } );
   $db->filter_fetch_value (sub { $_ = unpack("i", $_) } );
   $db->filter_store_value (sub { $_ = pack("i", $_) } );

   $_ = 'fred';

   my $key = 22 ;
   my $value = 34 ;

   $db->put($key, $value) ;
   ok 160, $key == 22;
   ok 161, $value == 34 ;
   ok 162, $_ eq 'fred';
   #print "k [$key][$value]\n" ;

   my $val ;
   $db->get($key, $val) ;
   ok 163, $key == 22;
   ok 164, $val == 34 ;
   ok 165, $_ eq 'fred';

   $key = 51 ;
   $value = 454;
   $h[$key] = $value ;
   ok 166, $key == 51;
   ok 167, $value == 454 ;
   ok 168, $_ eq 'fred';

   undef $db ;
   untie @h;
   unlink $Dfile;
}


{
    # Regression Test for bug 30237
    # Check that substr can be used in the key to db_put
    # and that db_put does not trigger the warning
    # 
    #     Use of uninitialized value in subroutine entry


    use warnings ;
    use strict ;
    my (@h, $db) ;
    my $status ;
    my $Dfile = "xxy.db";
    unlink $Dfile;

    ok(169, $db = tie(@h, 'DB_File', $Dfile, O_RDWR|O_CREAT, 0640, $DB_RECNO) );

    my $warned = '';
    local $SIG{__WARN__} = sub {$warned = $_[0]} ;

    # db-put with substr of key
    my %remember = () ;
    for my $ix ( 0 .. 2 )
    {
        my $key = $ix . "data" ;
        my $value = "value$ix" ;
        $remember{substr($key,0, 1)} = $value ;
        $db->put(substr($key,0, 1), $value) ;
    }

    ok 170, $warned eq '' 
      or print "# Caught warning [$warned]\n" ;

    # db-put with substr of value
    $warned = '';
    for my $ix ( 3 .. 5 )
    {
        my $key = $ix . "data" ;
        my $value = "value$ix" ;
        $remember{$ix} = $value ;
        $db->put($ix, substr($value,0)) ;
    }

    ok 171, $warned eq '' 
      or print "# Caught warning [$warned]\n" ;

    # via the tied array is not a problem, but check anyway
    # substr of key
    $warned = '';
    for my $ix ( 6 .. 8 )
    {
        my $key = $ix . "data" ;
        my $value = "value$ix" ;
        $remember{substr($key,0,1)} = $value ;
        $h[substr($key,0,1)] = $value ;
    }

    ok 172, $warned eq '' 
      or print "# Caught warning [$warned]\n" ;

    # via the tied array is not a problem, but check anyway
    # substr of value
    $warned = '';
    for my $ix ( 9 .. 10 )
    {
        my $key = $ix . "data" ;
        my $value = "value$ix" ;
        $remember{$ix} = $value ;
        $h[$ix] = substr($value,0) ;
    }

    ok 173, $warned eq '' 
      or print "# Caught warning [$warned]\n" ;

    my %bad = () ;
    my $key = '';
    for (my $status = $db->seq($key, $value, R_FIRST ) ;
         $status == 0 ;
         $status = $db->seq($key, $value, R_NEXT ) ) {

        #print "# key [$key] value [$value]\n" ;
        if (defined $remember{$key} && defined $value && 
             $remember{$key} eq $value) {
            delete $remember{$key} ;
        }
        else {
            $bad{$key} = $value ;
        }
    }
    
    ok 174, keys %bad == 0 ;
    ok 175, keys %remember == 0 ;

    print "# missing -- $key $value\n" while ($key, $value) = each %remember;
    print "# bad     -- $key $value\n" while ($key, $value) = each %bad;

    # Make sure this fix does not break code to handle an undef key
    my $value = 'fred';
    $warned = '';
    $status = $db->put(undef, $value) ;
    ok 176, $status == 0
      or print "# put failed - status $status\n";
    ok 177, $warned eq '' 
      or print "# Caught warning [$warned]\n" ;
    $warned = '';

    print "# db_ver $DB_File::db_ver\n";
    $value = '' ;
    $status = $db->get(undef, $value) ;
    ok 178, $status == 0
	or print "# get failed - status $status\n" ;
    ok(179, $db->get(undef, $value) == 0) or print "# get failed\n" ;
    ok 180, $value eq 'fred' or print "# got [$value]\n" ;
    ok 181, $warned eq '' 
      or print "# Caught warning [$warned]\n" ;
    $warned = '';

    undef $db ;
    untie @h;
    unlink $Dfile;
}

# Only test splice if this is a newish version of Perl
exit unless $FA ;

# Test SPLICE

{
    # check that the splice warnings are under the same lexical control
    # as their non-tied counterparts.

    use warnings;
    use strict;

    my $a = '';
    my @a = (1);
    local $SIG{__WARN__} = sub {$a = $_[0]} ;

    unlink $Dfile;
    my @tied ;
    
    tie @tied, 'DB_File', $Dfile, O_RDWR|O_CREAT, 0664, $DB_RECNO 
	or die "Can't open file: $!\n" ;

    # uninitialized offset
    use warnings;
    my $offset ;
    $a = '';
    splice(@a, $offset);
    ok(182, $a =~ /^Use of uninitialized value /);
    $a = '';
    splice(@tied, $offset);
    ok(183, $a =~ /^Use of uninitialized value in splice/);

    no warnings 'uninitialized';
    $a = '';
    splice(@a, $offset);
    ok(184, $a eq '');
    $a = '';
    splice(@tied, $offset);
    ok(185, $a eq '');

    # uninitialized length
    use warnings;
    my $length ;
    $a = '';
    splice(@a, 0, $length);
    ok(186, $a =~ /^Use of uninitialized value /);
    $a = '';
    splice(@tied, 0, $length);
    ok(187, $a =~ /^Use of uninitialized value in splice/);

    no warnings 'uninitialized';
    $a = '';
    splice(@a, 0, $length);
    ok(188, $a eq '');
    $a = '';
    splice(@tied, 0, $length);
    ok(189, $a eq '');

    # offset past end of array
    use warnings;
    $a = '';
    splice(@a, 3);
    my $splice_end_array = ($a =~ /^splice\(\) offset past end of array/);
    $a = '';
    splice(@tied, 3);
    ok(190, !$splice_end_array || $a =~ /^splice\(\) offset past end of array/);

    no warnings 'misc';
    $a = '';
    splice(@a, 3);
    ok(191, $a eq '');
    $a = '';
    splice(@tied, 3);
    ok(192, $a eq '');

    ok(193, safeUntie \@tied);
    unlink $Dfile;
}

# 
# These are a few regression tests: bundles of five arguments to pass
# to test_splice().  The first four arguments correspond to those
# given to splice(), and the last says which context to call it in
# (scalar, list or void).
# 
# The expected result is not needed because we get that by running
# Perl's built-in splice().
# 
my @tests = ([ [ 'falsely', 'dinosaur', 'remedy', 'commotion',
		 'rarely', 'paleness' ],
	       -4, -2,
	       [ 'redoubled', 'Taylorize', 'Zoe', 'halogen' ],
	       'void' ],

	     [ [ 'a' ], -2, 1, [ 'B' ], 'void' ],

	     [ [ 'Hartley', 'Islandia', 'assents', 'wishful' ],
	       0, -4,
	       [ 'maids' ],
	       'void' ],

	     [ [ 'visibility', 'pocketful', 'rectangles' ],
	       -10, 0,
	       [ 'garbages' ],
	       'void' ],

	     [ [ 'sleeplessly' ],
	       8, -4,
	       [ 'Margery', 'clearing', 'repercussion', 'clubs',
		 'arise' ],
	       'void' ],

	     [ [ 'chastises', 'recalculates' ],
	       0, 0,
	       [ 'momentariness', 'mediates', 'accents', 'toils',
		 'regaled' ],
	       'void' ],

	     [ [ 'b', '' ],
	       9, 8,
	       [ 'otrb', 'stje', 'ixrpw', 'vxfx', 'lhhf' ],
	       'scalar' ],

	     [ [ 'b', '' ],
	       undef, undef,
	       [ 'otrb', 'stje', 'ixrpw', 'vxfx', 'lhhf' ],
	       'scalar' ],
	     
	     [ [ 'riheb' ], -8, undef, [], 'void' ],

	     [ [ 'uft', 'qnxs', '' ],
	       6, -2,
	       [ 'znp', 'mhnkh', 'bn' ],
	       'void' ],
	    );

my $testnum = 194;
my $failed = 0;
my $tmp = "dbr$$";
foreach my $test (@tests) {
    my $err = test_splice(@$test);
    if (defined $err) {
	print STDERR "# failed: ", Dumper($test);
	print STDERR "# error: $err\n";
	$failed = 1;
	ok($testnum++, 0);
    }
    else { ok($testnum++, 1) }
}

if ($failed) {
    # Not worth running the random ones
    print STDERR '# skipping ', $testnum++, "\n";
}
else {
    # A thousand randomly-generated tests
    $failed = 0;
    srand(0);
    foreach (0 .. 1000 - 1) {
	my $test = rand_test();
	my $err = test_splice(@$test);
	if (defined $err) {
	    print STDERR "# failed: ", Dumper($test);
	    print STDERR "# error: $err\n";
	    $failed = 1;
	    print STDERR "# skipping any remaining random tests\n";
	    last;
	}
    }

    ok($testnum++, not $failed);
}

die "testnum ($testnum) != total_tests ($total_tests) + 1" 
    if $testnum != $total_tests + 1;

exit ;

# Subroutines for SPLICE testing

# test_splice()
# 
# Test the new splice() against Perl's built-in one.  The first four
# parameters are those passed to splice(), except that the lists must
# be (explicitly) passed by reference, and are not actually modified.
# (It's just a test!)  The last argument specifies the context in
# which to call the functions: 'list', 'scalar', or 'void'.
# 
# Returns:
#   undef, if the two splices give the same results for the given
#     arguments and context;
# 
#   an error message showing the difference, otherwise.
# 
# Reads global variable $tmp.
# 
sub test_splice {
    die 'usage: test_splice(array, offset, length, list, context)' if @_ != 5;
    my ($array, $offset, $length, $list, $context) = @_;
    my @array = @$array;
    my @list = @$list;

    unlink $tmp;
    
    my @h;
    my $H = tie @h, 'DB_File', $tmp, O_CREAT|O_RDWR, 0644, $DB_RECNO
      or die "cannot open $tmp: $!";

    my $i = 0;
    foreach ( @array ) { $h[$i++] = $_ }
    
    return "basic DB_File sanity check failed"
      if list_diff(\@array, \@h);

    # Output from splice():
    # Returned value (munged a bit), error msg, warnings
    # 
    my ($s_r, $s_error, @s_warnings);

    my $gather_warning = sub { push @s_warnings, $_[0] };
    if ($context eq 'list') {
	my @r;
	eval {
	    local $SIG{__WARN__} = $gather_warning;
	    @r = splice @array, $offset, $length, @list;
	};
	$s_error = $@;
	$s_r = \@r;
    }
    elsif ($context eq 'scalar') {
	my $r;
	eval {
	    local $SIG{__WARN__} = $gather_warning;
	    $r = splice @array, $offset, $length, @list;
	};
	$s_error = $@;
	$s_r = [ $r ];
    }
    elsif ($context eq 'void') {
	eval {
	    local $SIG{__WARN__} = $gather_warning;
	    splice @array, $offset, $length, @list;
	};
	$s_error = $@;
	$s_r = [];
    }
    else {
	die "bad context $context";
    }

    foreach ($s_error, @s_warnings) {
	chomp;
	s/ at \S+ line \d+\.$//;
	# only built-in splice identifies name of uninit value
	s/(uninitialized value) \$\w+/$1/;
    }

    # Now do the same for DB_File's version of splice
    my ($ms_r, $ms_error, @ms_warnings);
    $gather_warning = sub { push @ms_warnings, $_[0] };
    if ($context eq 'list') {
	my @r;
	eval {
	    local $SIG{__WARN__} = $gather_warning;
	    @r = splice @h, $offset, $length, @list;
	};
	$ms_error = $@;
	$ms_r = \@r;
    }
    elsif ($context eq 'scalar') {
	my $r;
	eval {
	    local $SIG{__WARN__} = $gather_warning;
	    $r = splice @h, $offset, $length, @list;
	};
	$ms_error = $@;
	$ms_r = [ $r ];
    }
    elsif ($context eq 'void') {
	eval {
	    local $SIG{__WARN__} = $gather_warning;
	    splice @h, $offset, $length, @list;
	};
	$ms_error = $@;
	$ms_r = [];
    }
    else {
	die "bad context $context";
    }

    foreach ($ms_error, @ms_warnings) {
	chomp;
    s/ at \S+(\s+\S+)*? line \d+\.?.*//s;
    }

    return "different errors: '$s_error' vs '$ms_error'"
      if $s_error ne $ms_error;
    return('different return values: ' . Dumper($s_r) . ' vs ' . Dumper($ms_r))
      if list_diff($s_r, $ms_r);
    return('different changed list: ' . Dumper(\@array) . ' vs ' . Dumper(\@h))
      if list_diff(\@array, \@h);

    if ((scalar @s_warnings) != (scalar @ms_warnings)) {
	return 'different number of warnings';
    }

    while (@s_warnings) {
	my $sw  = shift @s_warnings;
	my $msw = shift @ms_warnings;
	
	if (defined $sw and defined $msw) {
	    $msw =~ s/ \(.+\)$//;
	    $msw =~ s/ in splice$// if $] < 5.006;
	    if ($sw ne $msw) {
		return "different warning: '$sw' vs '$msw'";
	    }
	}
	elsif (not defined $sw and not defined $msw) {
	    # Okay.
	}
	else {
	    return "one warning defined, another undef";
	}
    }
    
    undef $H;
    untie @h;
    
    open(TEXT, $tmp) or die "cannot open $tmp: $!";
    @h = <TEXT>; normalise @h; chomp @h;
    close TEXT or die "cannot close $tmp: $!";
    return('list is different when re-read from disk: '
	   . Dumper(\@array) . ' vs ' . Dumper(\@h))
      if list_diff(\@array, \@h);

    unlink $tmp;

    return undef; # success
}


# list_diff()
#
# Do two lists differ?
#
# Parameters:
#   reference to first list
#   reference to second list
#
# Returns true iff they differ.  Only works for lists of (string or
# undef). 
# 
# Surely there is a better way to do this?
# 
sub list_diff {
    die 'usage: list_diff(ref to first list, ref to second list)'
      if @_ != 2;
    my ($a, $b) = @_;
    my @a = @$a; my @b = @$b;
    return 1 if (scalar @a) != (scalar @b);
    for (my $i = 0; $i < @a; $i++) {
	my ($ae, $be) = ($a[$i], $b[$i]);
	if (defined $ae and defined $be) {
	    return 1 if $ae ne $be;
	}
	elsif (not defined $ae and not defined $be) {
	    # Two undefined values are 'equal'
	}
	else {
	    return 1;
	}
    }
    return 0;
} 


# rand_test()
# 
# Think up a random ARRAY, OFFSET, LENGTH, LIST, and context.
# ARRAY or LIST might be empty, and OFFSET or LENGTH might be
# undefined.  Return a 'test' - a listref of these five things.
# 
sub rand_test {
    die 'usage: rand_test()' if @_;
    my @contexts = qw<list scalar void>;
    my $context = $contexts[int(rand @contexts)];
    return [ rand_list(),
	     (rand() < 0.5) ? (int(rand(20)) - 10) : undef,
	     (rand() < 0.5) ? (int(rand(20)) - 10) : undef,
	     rand_list(),
	     $context ];
}


sub rand_list {
    die 'usage: rand_list()' if @_;
    my @r;

    while (rand() > 0.1 * (scalar @r + 1)) {
	push @r, rand_word();
    }
    return \@r;
}


sub rand_word {
    die 'usage: rand_word()' if @_;
    my $r = '';
    my @chars = qw<a b c d e f g h i j k l m n o p q r s t u v w x y z>;
    while (rand() > 0.1 * (length($r) + 1)) {
	$r .= $chars[int(rand(scalar @chars))];
    }
    return $r;
}



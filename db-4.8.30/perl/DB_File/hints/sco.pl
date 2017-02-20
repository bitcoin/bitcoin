# osr5 needs to explicitly link against libc to pull in some static symbols
$self->{LIBS} = ['-ldb -lc'] if $Config{'osvers'} =~ '3\.2v5\.0\..' ;

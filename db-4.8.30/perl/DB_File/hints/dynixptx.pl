# Need to add an extra '-lc' to the end to work around a DYNIX/ptx bug

$self->{LIBS} = ['-lm -lc'];

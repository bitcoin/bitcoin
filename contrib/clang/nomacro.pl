#!/usr/bin/perl
# Copyright 2012 pooler@litecoinpool.org
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the Free
# Software Foundation; either version 2 of the License, or (at your option)
# any later version.  See COPYING for more details.
#
# nomacro.pl - convert assembler macros to C preprocessor macros.

use strict;

foreach my $f (<*.S>) {
	rename $f, "$f.orig";
	open FIN, "$f.orig";
	open FOUT, ">$f";
	my $inmacro = 0;
	my %macros = ();
	while (<FIN>) {
		if (m/^\.macro\s+([_0-9A-Z]+)(?:\s*)(.*)$/i) {
			print FOUT "#define $1($2) \\\n";
			$macros{$1} = 1;
			$inmacro = 1;
			next;
		}
		if (m/^\.endm/) {
			print FOUT "\n";
			$inmacro = 0;
			next;
		}
		for my $m (keys %macros) {
			s/^([ \t]*)($m)(?:[ \t]+([^#\n]*))?([;\n])/\1\2(\3)\4/;
		}
		if ($inmacro) {
			if (m/^\s*#if/) {
				$_ = <FIN> while (!m/^\s*#endif/);
				next;
			}
			next if (m/^\s*$/);
			s/\\//g;
			s/$/; \\/;
		}
		print FOUT;
	}
	close FOUT;
	close FIN;
}

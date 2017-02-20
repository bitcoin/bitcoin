# $Id$
#
# Print out a range of the log.
# Command line should set RECTYPE to a comma separated list
# of the rectypes (or partial strings of rectypes) sought.
NR == 1 {
	ntypes = 0
	while ((ndx = index(RECTYPE, ",")) != 0) {
		types[ntypes] = substr(RECTYPE, 1, ndx - 1);
		RECTYPE = substr(RECTYPE, ndx + 1, length(RECTYPE) - ndx);
		ntypes++
	}
	types[ntypes] = RECTYPE;
}

/^\[/{
	printme = 0
	for (i = 0; i <= ntypes; i++)
		if (index($1, types[i]) != 0) {
			printme = 1
			break;
		}
}
{
	if (printme == 1)
		print $0
}

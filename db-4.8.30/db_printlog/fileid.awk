# $Id$
#
# Take a comma-separated list of file numbers and spit out all the
# log records that affect those file numbers.

NR == 1 {
	nfiles = 0
	while ((ndx = index(FILEID, ",")) != 0) {
		files[nfiles] = substr(FILEID, 1, ndx - 1);
		FILEID = substr(FILEID, ndx + 1, length(FILEID) - ndx);
		nfiles++
	}
	files[nfiles] = FILEID;
}

/^\[/{
	if (printme == 1) {
		printf("%s\n", rec);
		printme = 0
	}
	rec = "";

	rec = $0
}
/^	/{
	rec = sprintf("%s\n%s", rec, $0);
}
/fileid/{
	for (i = 0; i <= nfiles; i++)
		if ($2 == files[i])
			printme = 1
}

END {
	if (printme == 1)
		printf("%s\n", rec);
}

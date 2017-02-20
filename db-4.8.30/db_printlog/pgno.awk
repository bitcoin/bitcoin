# $Id$
#
# Take a comma-separated list of page numbers and spit out all the
# log records that affect those page numbers.

NR == 1 {
	npages = 0
	while ((ndx = index(PGNO, ",")) != 0) {
		pgno[npages] = substr(PGNO, 1, ndx - 1);
		PGNO = substr(PGNO, ndx + 1, length(PGNO) - ndx);
		npages++
	}
	pgno[npages] = PGNO;
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
/pgno/{
	for (i = 0; i <= npages; i++)
		if ($2 == pgno[i])
			printme = 1
}
/right/{
	for (i = 0; i <= npages; i++)
		if ($2 == pgno[i])
			printme = 1
}
/left/{
	for (i = 0; i <= npages; i++)
		if ($2 == pgno[i])
			printme = 1
}

END {
	if (printme == 1)
		printf("%s\n", rec);
}

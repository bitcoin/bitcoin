# $Id$
#
# Print out all the records for a comma-separated list of transaction ids.
NR == 1 {
	ntxns = 0
	while ((ndx = index(TXN, ",")) != 0) {
		txn[ntxns] = substr(TXN, 1, ndx - 1);
		TXN = substr(TXN, ndx + 1, length(TXN) - ndx);
		ntxns++
	}
	txn[ntxns] = TXN;
}

/^\[/{
	if (printme == 1) {
		printf("%s\n", rec);
		printme = 0
	}
	rec = "";

	for (i = 0; i <= ntxns; i++)
		if (txn[i] == $5) {
			rec = $0
			printme = 1
		}
}
/^	/{
	rec = sprintf("%s\n%s", rec, $0);
}

END {
	if (printme == 1)
		printf("%s\n", rec);
}

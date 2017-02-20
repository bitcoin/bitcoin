# $Id$
#
# Read through db_printlog output and list all the transactions encountered
# and whether they committed or aborted.
#
# 1 = started
# 2 = committed
# 3 = explicitly aborted
# 4 = other
BEGIN {
	cur_txn = 0
}
/^\[.*]\[/{
	in_regop = 0
	if (status[$5] == 0) {
		status[$5] = 1;
		txns[cur_txn] = $5;
		cur_txn++;
	}
}
/	child:/ {
	txnid = substr($2, 3);
	status[txnid] = 2;
}
/txn_regop/ {
	txnid = $5
	in_regop = 1
}
/opcode:/ {
	if (in_regop == 1) {
		if ($2 == 1)
			status[txnid] = 2
		else if ($2 == 3)
			status[txnid] = 3
		else
			status[txnid] = 4
	}
}
END {
	for (i = 0; i < cur_txn; i++) {
		if (status[txns[i]] == 1)
			printf("%s\tABORT\n", txns[i]);
		else if (status[txns[i]] == 2)
			printf("%s\tCOMMIT\n", txns[i]);
		else if (status[txns[i]] == 3)
			printf("%s\tABORT\n", txns[i]);
		else if (status[txns[i]] == 4)
			printf("%s\tOTHER\n", txns[i]);
	}
}

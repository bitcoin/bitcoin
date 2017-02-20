# $Id$
#
# Output accumulated log record count/size statistics.
BEGIN {
	l_file = 0;
	l_offset = 0;
}

/^\[/{
	gsub("[][:	 ]", " ", $1)
	split($1, a)

	if (a[1] == l_file) {
		l[a[3]] += a[2] - l_offset
		++n[a[3]]
	} else
		++s[a[3]]

	l_file = a[1]
	l_offset = a[2]
}

END {
	# We can't figure out the size of the first record in each log file,
	# use the average for other records we found as an estimate.
	for (i in s)
		if (s[i] != 0 && n[i] != 0) {
			l[i] += s[i] * (l[i]/n[i])
			n[i] += s[i]
			delete s[i]
		}
	for (i in l)
		printf "%s: %d (n: %d, avg: %.2f)\n", i, l[i], n[i], l[i]/n[i]
	for (i in s)
		printf "%s: unknown (n: %d, unknown)\n", i, s[i]
}

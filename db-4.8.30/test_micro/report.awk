# $Id$

/^[^#]/ {
	total[$1] += $2
	sum[$1] += $2 * $2
	++count[$1];
}
END {
	# Compute the average, find the maximum.
	for (i in total) {
		avg[i] = total[i] / count[i];
		if (max < avg[i])
			max = avg[i]
	}

	for (i in total) {
		# Calculate variance by raw score method.
		var = (sum[i] - ((total[i] * total[i]) / count[i])) / count[i];

		# The standard deviation is the square root of the variance.
		stdv = sqrt(var);

		# Display the release value, the average score, and run count.
		printf("%s:%.2f:%d:", i, avg[i],  count[i]);

		# If this run wasn't the fastest, display the percent by which
		# this run was slower.
		if (max != avg[i])
			printf("%.0f%%", ((max - avg[i]) / max) * 100);

		printf(":");

		# If there was more than a single run, display the relative
		# standard deviation.
		if (count[i] >  1)
			printf("%.0f%%", stdv * 100 / avg[i]);

		printf("\n");
	}
}

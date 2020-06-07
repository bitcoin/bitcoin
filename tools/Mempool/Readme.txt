This data logs the mempool as the Bitcoin Core node runs.

Then "CompareMempoolLogs.py" post processes this data and finds the differences in datasets, for if the mempool is logged in two different locations in the world, then this script will find the differences in connectivity to the rest of the network, by analyzing the rate at which transactions flow in.
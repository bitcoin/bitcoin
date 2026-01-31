# Embedded ASMap data

## Background

The ASMap feature (available via `-asmap`) makes it possible to use a peer's AS Number (ASN), an ISP/hoster identifier,
in netgroup bucketing in order to ensure a higher diversity in the peer
set. When not using this, the default behavior is to have the buckets formed
based on IP prefixes but this does not
prevent having connections dominated by peers at the same large-scale hoster,
for example, since such companies usually control many diverse IP ranges.
In order to use ASMap, the mapping between IP prefixes and AS Numbers needs
to be available. This mapping data can be provided through an external file
but Bitcoin Core also embeds a default map in its builds to make the feature
available to users when they are unable to provide a file.

## Data sourcing and tools

ASMap is a mapping of IP prefix to ASN, essentially a snapshot of the
internet routing table at some point in time. Due to the high volatility
of parts of this routing table and the known vulnerabilities in the BGP
protocol it is challenging to collect this data and prove its consistency.
Sourcing the data from a single trusted source is problematic as well.

The [Kartograf](https://github.com/asmap/kartograf) tool was created to
deal with these uncertainties as good as possible. The mapping data is sourced from RPKI, IRR and
Routeviews. The former two are themselves used as security mechanisms to
protect against BGP security issues, which is why they are considered more secure and
their data takes precedence. The latter is a trusted collector of BGP traffic
and only used for IP space that is not covered by RPKI and IRR.

The process in which the Kartograf project parses, processes and merges these
data sources is deterministic. Given the raw download files from these
different sources, anyone can build their own map file and verify the content
matches with other users' results. Before the map is usable by Bitcoin Core
it needs to be encoded as well. This is done using `asmap-tool.py` in `contrib/asmap`
and this step is deterministic as well.

When it comes to obtaining the initial input data, the high volatility remains
a challenge if users don't want to trust a single creator of the used ASMap file.
To overcome this, multiple users can start the download process at the exact
same time which leads to a high likelihood that their downloaded data will be
similar enough that they receive the same output at the end of the process.
This process is regularly coordinated at the [asmap-data](https://github.com/asmap/asmap-data)
project. If enough participants have joined the effort (5 or more is recommended) and a majority of the
participants have received the same result, the resulting ASMap file is added
to the repository for public use. Files will not be merged to the repository
without at least two additional reviewers confirming that the process described
above was followed as expected and that the encoding step yielded the same
file hash. New files are created on an ongoing basis but without any central planning
or an explicit schedule.

## Release process

As an upcoming release approaches the embedded ASMap data should be updated
by replacing the `ip_asn.dat` with a newer ASMap file from the asmap-data
repository so that its data is embedded in the release. Ideally, there may be a file
already created recently that can be selected for an upcoming release. Alternatively,
a new creation process can be initiated with the goal of obtaining a fresh map
for use in the upcoming release.

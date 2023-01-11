# Maintainers

This document contains list of present maintainers, their role and process to add new maintainers.

## Current Maintainers

|   Username    |
| ------------- |
|  MarcoFalke   |
|  fanquake     |
|  hebasto      |
|  achow101     |
|  glozow       |

## Responsibilities and roles

Project maintainers have commit access and are responsible for merging patches
from contributors. They perform a janitorial role of implementing the rough
consensus of reviewers by merging patches. Maintainers make sure that patches
are safe and align with the project goals. However, their decision is not the
last step, as patches must also go through post-merge review and testing before
being implemented. The maintainers’ role is by rough consensus of project contributors.

## Self Merges

In general, it is considered best practice for maintainers to avoid merging their
own changes directly into the codebase. This is because merging your own code can
create a potential conflict of interest, where a maintainer's personal interests
may override the best interests of the project.

However, may be acceptable for maintainers to occasionally merge their own changes
if the pull request has enough reviews and no other maintainer is available to merge.

## Nomination and Addition of New Maintainers

Contributors can nominate a long-term contributor in the
[IRC channel](https://web.libera.chat/#bitcoin-core-dev) to take on the
role of a maintainer. Contributors are free to self-nominate as well.

Maintainers are added by a new maintainer opening a pull request to add their
key to the Trusted Keys file. For example, see [this pull
request](https://github.com/bitcoin/bitcoin/pull/23798). The adding of the key
is reviewed by the community and merged by one of the maintainers based on rough
consensus of the reviewing contributors. Before opening such a pull request, it is
typical that the adding of the maintainer might be discussed in the regular Bitcoin
Core IRC meeting, but the actual decision-making process occurs in the pull request.

## Maintainers "Moving On"

Maintainers may choose to cease being a maintainer at any time without notice.
Where possible, maintainers should try to gracefully transition out of their
role, until a new maintainer has been selected to replace them (if needed).

## Maintainer Removal

If it is deemed necessary to remove a maintainer, the decision should be made by open
discussion and rough consensus of contributors and the community.
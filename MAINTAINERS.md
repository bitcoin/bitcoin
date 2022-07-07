# Maintainers

This document serves to clarify the responsibilities of a maintainer as well as
serve as a wiki for maintainers to link to / include resources.


## Current Maintainers

The current maintainers are:

| Name  | Focus Areas |
| ------------- | ------------- |
| @laanwj | General  |
| @MarcoFalke   | General, tests, QA  |
| @fanquake   | General, tooling, build system  |
| @hebasto    | UI/UX  |
| @achow101    | Wallet, PSBTs  |
| @glozow     | General, transaction relay and mempool policy |



## Regular Contributors, Scoped Maintainers, General Maintainers, and Lead Maintainers

The title of Maintainer in Bitcoin is more of a "janitorial responsibility"
than a position of privilege. However, the role does include permissions on the
repository, and connotes a general sense of stewardship over the project.


Largely, the role of maintainers is to decide to merge Pull Requests.

Whether a pull request is merged into Bitcoin Core rests with the project merge
maintainers.

Maintainers will take into consideration if a patch is in line with the general
principles of the project, meets the minimum standards for inclusion, and will
judge the general consensus of contributors. The review process that
establishes general consensus is detailed in
[CONTRIBUTING.mg](/CONTRIBUTING.md).

Thus maintainers role is not directly elevated above that of regular
contributors, as maintainers serve mostly as arbiters of if changes are
acceptable to the contributors to the project in aggregate.

The role of the Maintainer is currently evolving with respect to "Scoped
Maintainers". The concept of a scoped maintainer is that it is helpful to add
maintainers who are subject matter experts in a particular area of the code,
but perhaps not in other areas. As such, maintainers with an explicit scope
typically will avoid merging PRs in areas outside their scope, but may do so on
occasion. As an individual maintainer's experience levels in the project
increase, they may change their focus from scoped to general, but this is not
currently an explicit designation (all maintainers should be considered to be
general at this time). As a rough measure, scoped maintainers should avoid
"stepping on the toes" of work that is being guided by another maintainer or
contributor.

Historically, one Bitcoin's maintainers was a designated "Lead Maintainer",
which was most recently @laanwj. However, [in a blog
post](https://laanwj.github.io/2021/01/21/decentralize.html), @laanwj stated
that they wanted to take more of a background role as a maintainer and have not
"passed the baton" to a successor as a Lead Maintainer, and instead focused on
decentralizing the maintainership ecosystem. As such, while the unofficial
title of Lead Maintainer still is held by @laanwj, it should be considered a
bug-not-feature if it confers any additional responsibility or privilege over
other General Maintainers, and may be explicitly deprecated in the future.

## Self Merges

Typically, a PR authored by a maintainer will be merged by a different
maintainer. However, given that there may not be another maintainer who
regularly works on the section of code or is familiar, self-merge is not
forbidden. However, as a guard, self merges should usually only happen if the
following criteria has been met:

1. The PR has been reviewed and ACK'd by other regular contributors familiar
   with that section of the code.
1. There are no NACKs being overruled on that PR.
1. The PR is at least 1 week old, to permit time for review.
1. The maintainer should communicate intent to self-merge 48 hours before doing
   so. (Regular contributors may also indicate if they think a PR can be merged     by commenting "Ready for Merge").


It is preferred that maintainers self-merge as opposed to privately allying
with another maintainer to avoid the self-merge decorum. This promotes
transparency and minimizes bias in the review process. Tracking self-merges
also helps the project demonstrate need for additional maintainers in areas where they might be frequent, which can help solve the issues around self-merges.


These rules aren't hard and fast, but are considered a best practice to avoid
self-merge as a convenience.



## Nomination and Selection of Maintainers

Being a Maintainer is not an honor (but it may be honorable to serve as one).
Many regular contributors who are capable or experienced enough choose not to
be a Maintainer. Similarly, just because a regular contributor has reached a
certain level of experience does not mean that they "deserve" to be a maintainer.

Maintainers are added when there is a demonstrated need. For there to be a new
maintainer, first an issue should be created with a "Call for Maintainer" (C4M)
by one of the existing maintainers or regular contributors. C4Ms should specify
if the call is for a General Maintainer, or for a Scoped Maintainer. If the
role is for a Scoped Maintainer, the C4M should describe the need for a new
maintainer, and link to some Pull Requests (or other material) that would be
relevant for such a maintainer to be responsible for.

Following the C4M, anyone may respond to the issue with a nomination for the
position and a description of their qualifications. Self-nomination is
encouraged. ACKs/co-signs of other nominations are also encouraged so that the
maintainers may get a sense of developer sentiment towards nominees.
Nominations must be accepted by the nominee. After 2 weeks from the date of
issue creation, assuming there is at least 1 nomination, the maintainers may
select one or more candidate from the nominee pool at their discretion.


The selected candidate should create a PR for adding their key to the
repository and to the above table (see
https://github.com/bitcoin/bitcoin/pull/23798 for example).  During / Before
the PR, regular contributors are encouraged to correspond with candidate(s)
publicly about any concerns or questions.  Regular contributors are also
encouraged to raise any concerns about the maintainers selection of candidate
against other nominated candidates not selected.

At the discretion of the maintainers, and no sooner than 2 weeks after the PR
is opened, the candidate(s) PR may be merged.


## Maintainer Prioritizations

While the maintainers are ultimately not accountable to any particular roadmap,
nor can make regular contributors focus on one area or another, practically the
views and prioritizations of maintainers have an impact on what gets reviewed,
merged, and the general direction of the project.


However, to discourage people from looking to maintainers for any roadmap,
maintainers are not required to share what they prioritize. Publicizing or
listing in the project documentation maintainer's personal prioritizations
might serve to discourage other contributors from working on activities outside
the maintainer's areas of interests, reducing the diversity of thought in the
project. To avoid this centralization effect around the maintainers, it is best
that the maintainers project neutrality with respect to any particular personal
goal for the project.

Maintainers are not forbidden from producing a document detailing their
prioritizations, e.g., on their personal blog. But it will not be considered to
be a post in their capacity as a maintainer.

## Maintainers "Moving On"

Maintainers may choose to cease being a maintainer at any time without notice.
Where possible, maintainers should try to gracefully transition out of their
role, until a new maintainer has been selected to replace them (if needed).

## Maintainer Removal

There is no public or defined process to remove a maintainer.

If a maintainer should be removed, individuals should either remark publicly or
confer with other maintainers privately to suggest their removal.

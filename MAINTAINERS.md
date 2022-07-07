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

There is no prohibition or process that differentiates self-merges from normal
merges.


## Nomination and Selection of Maintainers

Being a Maintainer is not an honor (but it may be honorable to serve as one).
Many regular contributors who are capable or experienced enough choose not to
be a Maintainer. Similarly, just because a regular contributor has reached a
certain level of experience does not mean that they "deserve" to be a maintainer.

Maintainers are added by a new maintainer opening a pull-request to add their
key to the Trusted Keys file. For example, see [this pull
request](https://github.com/bitcoin/bitcoin/pull/23798). The adding of the key
is reviewed by the community and merged at the discretion of the existing
maintainers. Before opening such a pull request, it is typical that the adding
of the maintainer might be discussed in the regular Bitcoin Core IRC meeting,
but the actual decision making process occurs in the pull request.


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

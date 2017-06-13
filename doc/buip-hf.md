BUIP-HF Development Notes
===========================

Specification
----------------

There is a draft specification currently located at

https://github.com/BitcoinUnlimited/BUIP/blob/master/BUIP-HF/buip-hf-technical-spec.md

This is still work in progress - please check the `BUIP` repo for active
PRs on the spec before you implement something.
Try to get clarity first, if necessary comment / ask on the relevant PRs.


Development repository, branches etc.
----------------------------------------

The development is currently taking place against the `buip-hf` branch at

https://github.com/ftrader-bitcoinunlimited/BitcoinUnlimited

`buip-hf` has been made the default branch until this development is
completed.


Development guidelines
------------------------

The regular BU and Bitcoin development guidelines apply, with following
additions or emphasis:

1. Do not commit sensitive information i.e. HF critical consensus
   parameters without first consulting BU. Use dummy values until
   the fork parameters are ready for official release.

2. No mixing of formatting and functional changes.
   I will be quite firm in requiring amendment to PRs which do this.

3. No formatting changes unless strictly needed and in consultation with
   BU developer or myself. Keep the diffs minimal - we can do formatting
   as final step before we do PR's back to BU's main repository.


Project workflow, admin
---------------------------

Issues can be raised on 'freetraders' BitcoinUnlimited repo (URL above).

'freetrader' together with BU Developer and others will create issues
labeled 'buip-hf task' which represent tasks that need to be completed.
These can be development of features, tests, review etc.
These tasks should give us an indication of open work packages.

Where possible these tasks will be assigned to keep it clear who is working
on what, to avoid collisions and allow easy identification of contacts.


Creating PRs
---------------

Due to a Github issue, it seems not possible to create a PR to the
'ftrader-bitcoinunlimited' organization (it does not appear in the
drop down lists).

While this is being sorted out with Github support, please use the following
method to create a PR on the website:

1. Enter a compare page URL from the base 'buip-hf' to your branch.
To do this, you need to adapt the following URL by replacing 'youraccount'
and 'yourbranch' with your Github account name and the branch from which
you want to create the PR:

https://github.com/ftrader-bitcoinunlimited/bitcoinunlimited/compare/buip-hf...yourname:yourbranch

2. On the Compare page, click on 'Create pull request' button


Merge Policy
--------------

Upstream: 'freetrader' will merge from upstream BU `dev` branch on a
semi-regular basis, especially changes that are deemed critical for the
fork or look like they might conflict with fork work and should be resolved
speedily.

PRs that conflict with requirements shall not be merged.

PR authors might be required to rebase their PRs in some cases. Please
consider this an incentive to create small PRs.


Software Design
-----------------

Ad hoc via discussion on BU slack channels and in the Github issues
relating to tasks around this project.

Where something is necessary to document, we could create a software
design document for buip-hf .


END

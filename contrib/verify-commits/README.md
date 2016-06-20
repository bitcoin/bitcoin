Tooling for verification of PGP signed commits
----------------------------------------------

This is an incomplete work in progress, but currently includes a pre-push hook
script (`pre-push-hook.sh`) for maintainers to ensure that their own commits
are PGP signed (nearly always merge commits), as well as a script to verify
commits against a trusted keys list.


Using verify-commits.sh safely
------------------------------

Remember that you can't use an untrusted script to verify itself. This means
that checking out code, then running `verify-commits.sh` against `HEAD` is
_not_ safe, because the version of `verify-commits.sh` that you just ran could
be backdoored. Instead, you need to use a trusted version of verify-commits
prior to checkout to make sure you're checking out only code signed by trusted
keys:

    git fetch origin && \
      ./contrib/verify-commits/verify-commits.sh origin/master && \
      git checkout origin/master

Note that the above isn't a good UI/UX yet, and needs significant improvements
to make it more convenient and reduce the chance of errors; pull-reqs
improving this process would be much appreciated.

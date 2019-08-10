## ci scripts

This directory contains scripts for each build step in each build stage.

Currently three stages `lint`, `extended_lint` and `test` are defined. Each stage has its own lifecycle, similar to the
[Travis CI lifecycle](https://docs.travis-ci.com/user/job-lifecycle#the-job-lifecycle).  Every script in here is named
and numbered according to which stage and lifecycle step it belongs to.


## travis build scripts

The `.travis` directory contains scripts for each build step in each build stage.
Currently the travis build defines two stages `lint` and `test`. Each stage has
it's own [lifecycle](https://docs.travis-ci.com/user/customizing-the-build/#the-build-lifecycle).
Every script in here is named and numbered according to which stage and lifecycle
step it belongs to.


## ci scripts

This directory contains scripts for each build step in each build stage.

Currently three stages `lint`, `extended_lint` and `test` are defined. Each stage has its own lifecycle, similar to the
[Travis CI lifecycle](https://docs.travis-ci.com/user/job-lifecycle#the-job-lifecycle). Every script in here is named
and numbered according to which stage and lifecycle step it belongs to.

### Running a stage locally

Be aware that the tests will be built and run in-place, so please run at your own risk.
If the repository is not a fresh git clone, you might have to clean files from previous builds or test runs first.

The ci needs to perform various sysadmin tasks such as installing packages or writing to the user's home directory.
While most of the actions are done inside a docker container, this is not possible for all. Thus, cache directories,
such as the depends cache, previous release binaries, or ccache, are mounted as read-write into the docker container. While it should be fine to run
the ci system locally on you development box, the ci scripts can generally be assumed to have received less review and
testing compared to other parts of the codebase. If you want to keep the work tree clean, you might want to run the ci
system in a virtual machine with a Linux operating system of your choice.

To allow for a wide range of tested environments, but also ensure reproducibility to some extent, the test stage
requires `docker` to be installed. To install all requirements on Ubuntu, run

```
sudo apt install docker.io bash
```

To run the default test stage,

```
./ci/test_run_all.sh
```

To run the test stage with a specific configuration,

```
FILE_ENV="./ci/test/00_setup_env_arm.sh" ./ci/test_run_all.sh
```

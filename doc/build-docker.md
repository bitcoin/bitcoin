Prerequisites
---------------------

* Install docker
* clone this repo

Build version
---------------------

* checkout the tag, etc you want to build

That will also change the build-docker/ code. If you want to build an older version you would need a strategy such as copying the build-docker directory out of the repo and then moving it back in after the checkout.

Build
---------------------

Currently the Berkeley DB code isn't getting included properly

    CONFIGURE_OPTS="--disable-wallet" ./build-docker/build.sh

`./build-docker/docker-env` runs a command in the docker build environment.
To rebuild without re-configuring, run

    ./build-docker/docker-env make



Run
---------------------

The build output defaults to dynamically linked. The output files can be run from the docker-env:

    ./build-docker/docker-env ./src/bitcoind -disablewallet


Build environment
-----------------

The docker build is equivalent to what is documented in doc/build-unix.md but it is performed in a docker container with supporting libraries pre-installed.

The build is performed by mounting the bitcoin source into the container. Thus build artifacts are produced in the local source tree rather than committing them into a docker image. This is ideal for a development workload but not a ci/release workflow.

The image currently uses an alpine image, which might be ideal if we can eventually produce a static alpine build. Otherwise switching to a debian based build to closedly follow doc/build-unix-md would make sense.


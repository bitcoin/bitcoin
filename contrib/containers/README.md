## Containers

This directory contains configuration files for containerization utilities.

Currently two Docker containers exist, `ci` defines how Dash's GitLab CI container is built and the `dev` builds on top of the `ci` to provide a containerized development environment that is as close as possible to CI for contributors! See also [Dash on Docker Hub](https://hub.docker.com/u/dashpay) i.e. for the [dashd container](https://hub.docker.com/r/dashpay/dashd).

### Usage Guide

We utilise edrevo's [dockerfile-plus](https://github.com/edrevo/dockerfile-plus), a syntax extension that
leverages Docker [BuildKit](https://docs.docker.com/develop/develop-images/build_enhancements/) to reduce
the amount of repetitive code.

As BuildKit is opt-in within many currently supported versions of Docker (as of this writing), you need to
set the following environment variables before continuing. While not needed after the initial `docker-compose build`
(barring updates to the `Dockerfile`), we recommend placing this in your `~/.bash_profile`/`~/.zshrc` or equivalent

```bash
export DOCKER_BUILDKIT=1
export COMPOSE_DOCKER_CLI_BUILD=1
```

After that, it's simply a matter of building and running your own development container. You can use extensions
for your IDE like Visual Studio Code's [Remote Containers](https://code.visualstudio.com/docs/remote/containers)
to run terminal commands from inside the terminal and build Dash Core.

```bash
cd contrib/containers/develop
docker-compose build
docker-compose run container
```

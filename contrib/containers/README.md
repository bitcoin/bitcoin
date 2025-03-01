## Containers

This directory contains configuration files for containers. Containers that depends on other containers require BuildKit to
be enabled in order for syntax extensions to work correctly.

| Name      | Depends On | Purpose                                                                    |
| --------- | -----------| -------------------------------------------------------------------------- |
| `ci-slim` | None       | Slimmed down container used to run functional tests and (some) linters     |
| `ci`      | `ci-slim`  | Full container used to (cross) compile                                     |
| `develop` | `ci`       | Interactive environment to allow debugging in an environment that's 1:1 CI |
| `deploy`  | None       | Packaging of builds for release on Docker Hub                              |
| `guix`    | None       | Interactive environment for building (and packaging) with Guix             |

### Usage Guide

We utilise edrevo's [devthefuture/dockerfile-x](https://codeberg.org/devthefuture/dockerfile-x), a syntax extension that
leverages Docker [BuildKit](https://docs.docker.com/build/buildkit/) to reduce
the amount of repetitive code.

As BuildKit is opt-in within some versions of Docker, you may need to set the following environment variables before
continuing. While not needed after the initial `docker compose build` (barring updates to the `Dockerfile`), we
recommend placing this in your `~/.bash_profile`/`~/.zshrc` or equivalent

```bash
export DOCKER_BUILDKIT=1
export COMPOSE_DOCKER_CLI_BUILD=1
```

You can use extensions for your IDE like Visual Studio Code's [Remote Containers](https://code.visualstudio.com/docs/remote/containers)
to run terminal commands from inside the terminal and work with Dash Core.

```bash
cd contrib/containers/develop
docker compose build
docker compose run container
```

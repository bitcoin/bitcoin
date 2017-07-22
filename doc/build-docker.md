# Docker Build Instructions and Notes

Bitcoind can be easily built on any platform which hosts a 
[Docker](https://en.wikipedia.org/wiki/Docker_(software)) daemon. This is
likely the lowest-effort way to get a bitcoin development environment on your
machine.

The Docker development environment does not facilitate development of the
GUI.

## Preparation

1. [Install Docker](https://www.docker.com/community-edition#/download).
2. If on Linux, install Docker Compose with `pip install docker-compose`.
3. Change directory to the root of this repository.
4. Run `docker-compose run bitcoind` to compile bitcoin.

Bitcoin's source tree will be
[mounted](https://docs.docker.com/compose/compose-file/compose-file-v2/#volumes-volume_driver) 
into the Docker container, so any changes you make to the source tree on
your host computer will be immediately reflected within the Docker container.

## Commands

### Run all tests

```sh
docker-compose run --rm bitcoind test
```

### Run C++ tests

```sh
docker-compose run --rm bitcoind test-cpp [args ...]
```

### Run Python tests

```sh
docker-compose run --rm bitcoind test-python [args ...]
```
  
### Run bitcoind

In the foreground

```sh
docker-compose up bitcoind
```

or in the background

```sh
docker-compose up -d bitcoind
```
 
### Other commands

Any command can be run within the container image by passing it as additional
arguments:
 
```sh
docker-compose run --rm bitcoind [cmd ...]
```             

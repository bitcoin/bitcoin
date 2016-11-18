# Dockerfile for building 42 binaries.

Now, you can build your own 42 files on all systems with docker and do it easy without installing depends on your system.

## How:

### Build docker image

```
sudo docker build .
```

### Run docker container

Builder will return HASH of image
Example:
Successfully built 9bbff825d50f

```
sudo docker run -it -v ~/path/to/42/folder:/42 9bbff825d50f
```

If your system uses SELINUX you may use --privileged=true key

```
sudo docker run --privileged=true -it -v ~/development/42:/42 9bbff825d50f
```

See 42-qt file in used 42 folder and 42d file in src subfolder.
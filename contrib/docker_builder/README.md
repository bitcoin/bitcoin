# Dockerfile for building novacoin binaries.

Now, you can build your own novacoin files on all systems with docker and do it easy without installing depends on your system.

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
sudo docker run -it -v ~/path/to/novacoin/folder:/novacoin 9bbff825d50f
```

If your system uses SELINUX you may use --privileged=true key

```
sudo docker run --privileged=true -it -v ~/development/novacoin:/novacoin 9bbff825d50f
```

See novacoin-qt file in used novacoin folder and novacoind file in src subfolder.
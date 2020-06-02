# Bootstrappable Bitcoin Core Builds

This directory contains the files necessary to perform bootstrappable Bitcoin
Core builds.

[Bootstrappability][b17e] furthers our binary security guarantees by allowing us
to _audit and reproduce_ our toolchain instead of blindly _trusting_ binary
downloads.

We achieve bootstrappability by using Guix as a functional package manager.

## Requirements

Conservatively, a x86_64 machine with:

- 4GB of free disk space on the partition that /gnu/store will reside in
- 24GB of free disk space on the partition that the Bitcoin Core git repository
  resides in

> Note: these requirements are slightly less onerous than those of Gitian builds

## Setup

### Installing Guix

If you're just testing this out, you can use the
[Dockerfile][fanquake/guix-docker] for convenience. It automatically speeds up
your builds by [using substitutes](#speeding-up-builds-with-substitute-servers).
If you don't want this behaviour, refer to the [next
section](#choosing-your-security-model).

Otherwise, follow the [Guix installation guide][guix/bin-install].

> Note: For those who like to keep their filesystems clean, Guix is designed to
> be very standalone and _will not_ conflict with your system's package
> manager/existing setup. It _only_ touches `/var/guix`, `/gnu`, and
> `~/.config/guix`.

### Choosing your security model

Guix allows us to achieve better binary security by using our CPU time to build
everything from scratch. However, it doesn't sacrifice user choice in pursuit of
this: users can decide whether or not to bootstrap and to use substitutes.

After installation, you may want to consider [adding substitute
servers](#speeding-up-builds-with-substitute-servers) to speed up your build if
that fits your security model (say, if you're just testing that this works).
This is skippable if you're using the [Dockerfile][fanquake/guix-docker].

If you prefer not to use any substitutes, make sure to set
`ADDITIONAL_GUIX_ENVIRONMENT_FLAGS` like the following snippet. The first build
will take a while, but the resulting packages will be cached for future builds.

```sh
export ADDITIONAL_GUIX_ENVIRONMENT_FLAGS='--no-substitutes'
```

Likewise, to perform a bootstrapped build (takes even longer):

```sh
export ADDITIONAL_GUIX_ENVIRONMENT_FLAGS='--bootstrap --no-substitutes'
```

### Using a version of Guix with `guix time-machine` capabilities

> Note: This entire section can be skipped if you are already using a version of
> Guix that has [the `guix time-machine` command][guix/time-machine].

Once Guix is installed, if it doesn't have the `guix time-machine` command, pull
the latest `guix`.

```sh
guix pull --max-jobs=4 # change number of jobs accordingly
```

Make sure that you are using your current profile. (You are prompted to do this
at the end of the `guix pull`)

```bash
export PATH="${HOME}/.config/guix/current/bin${PATH:+:}$PATH"
```

## Usage

### As a Development Environment

For a Bitcoin Core depends development environment, simply invoke

```sh
guix environment --manifest=contrib/guix/manifest.scm
```

And you'll land back in your shell with all the build dependencies required for
a `depends` build injected into your environment.

### As a Tool for Deterministic Builds

From the top of a clean Bitcoin Core repository:

```sh
./contrib/guix/guix-build.sh
```

After the build finishes successfully (check the status code please), compare
hashes:

```sh
find output/ -type f -print0 | sort -z | xargs -r0 sha256sum
```

#### Recognized environment variables

* _**HOSTS**_

  Override the space-separated list of platform triples for which to perform a
  bootstrappable build. _(defaults to "x86\_64-linux-gnu
  arm-linux-gnueabihf aarch64-linux-gnu riscv64-linux-gnu")_

  > Windows and OS X platform triplet support are WIP.

* _**SOURCES_PATH**_

  Set the depends tree download cache for sources. This is passed through to the
  depends tree. Setting this to the same directory across multiple builds of the
  depends tree can eliminate unnecessary redownloading of package sources.

* _**MAX_JOBS**_

  Override the maximum number of jobs to run simultaneously, you might want to
  do so on a memory-limited machine. This may be passed to `make` as in `make
  --jobs="$MAX_JOBS"` or `xargs` as in `xargs -P"$MAX_JOBS"`. _(defaults to the
  value of `nproc` outside the container)_

* _**SOURCE_DATE_EPOCH**_

  Override the reference UNIX timestamp used for bit-for-bit reproducibility,
  the variable name conforms to [standard][r12e/source-date-epoch]. _(defaults
  to the output of `$(git log --format=%at -1)`)_

* _**V**_

  If non-empty, will pass `V=1` to all `make` invocations, making `make` output
  verbose.

  Note that any given value is ignored. The variable is only checked for
  emptiness. More concretely, this means that `V=` (setting `V` to the empty
  string) is interpreted the same way as not setting `V` at all, and that `V=0`
  has the same effect as `V=1`.

* _**ADDITIONAL_GUIX_ENVIRONMENT_FLAGS**_

  Additional flags to be passed to `guix environment`. For a fully-bootstrapped
  build, set this to `--bootstrap --no-substitutes` (refer to the [security
  model section](#choosing-your-security-model) for more details). Note that a
  fully-bootstrapped build will take quite a long time on the first run.

## Tips and Tricks

### Speeding up builds with substitute servers

_This whole section is automatically done in the convenience
[Dockerfiles][fanquake/guix-docker]_

For those who are used to life in the fast _(and trustful)_ lane, you can use
[substitute servers][guix/substitutes] to enable binary downloads of packages.

> For those who only want to use substitutes from the official Guix build farm
> and have authorized the build farm's signing key during Guix's installation,
> you don't need to do anything.

#### Authorize the signing keys

For the official Guix build farm at https://ci.guix.gnu.org, run as root:

```
guix archive --authorize < ~root/.config/guix/current/share/guix/ci.guix.gnu.org.pub
```

For dongcarl's substitute server at https://guix.carldong.io, run as root:

```sh
wget -qO- 'https://guix.carldong.io/signing-key.pub' | guix archive --authorize
```

#### Use the substitute servers

The official Guix build farm at https://ci.guix.gnu.org is automatically used
unless the `--no-substitutes` flag is supplied.

This can be overridden for all `guix` invocations by passing the
`--substitute-urls` option to your invocation of `guix-daemon`. This can also be
overridden on a call-by-call basis by passing the same `--substitute-urls`
option to client tools such at `guix environment`.

To use dongcarl's substitute server for Bitcoin Core builds after having
[authorized his signing key](#authorize-the-signing-keys):

```
export ADDITIONAL_GUIX_ENVIRONMENT_FLAGS='--substitute-urls="https://guix.carldong.io https://ci.guix.gnu.org"'
```

## FAQ

### How can I trust the binary installation?

As mentioned at the bottom of [this manual page][guix/bin-install]:

> The binary installation tarballs can be (re)produced and verified simply by
> running the following command in the Guix source tree:
>
>     make guix-binary.x86_64-linux.tar.xz

### When will Guix be packaged in debian?

Vagrant Cascadian has been making good progress on this
[here][debian/guix-package]. We have all the pieces needed to put up an APT
repository and will likely put one up soon.

[b17e]: http://bootstrappable.org/
[r12e/source-date-epoch]: https://reproducible-builds.org/docs/source-date-epoch/

[guix/install.sh]: https://git.savannah.gnu.org/cgit/guix.git/plain/etc/guix-install.sh
[guix/bin-install]: https://www.gnu.org/software/guix/manual/en/html_node/Binary-Installation.html
[guix/env-setup]: https://www.gnu.org/software/guix/manual/en/html_node/Build-Environment-Setup.html
[guix/substitutes]: https://www.gnu.org/software/guix/manual/en/html_node/Substitutes.html
[guix/substitute-server-auth]: https://www.gnu.org/software/guix/manual/en/html_node/Substitute-Server-Authorization.html
[guix/time-machine]: https://guix.gnu.org/manual/en/html_node/Invoking-guix-time_002dmachine.html

[debian/guix-package]: https://bugs.debian.org/cgi-bin/bugreport.cgi?bug=850644
[fanquake/guix-docker]: https://github.com/fanquake/core-review/tree/master/guix

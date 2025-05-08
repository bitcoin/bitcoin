# See test/lint/README.md for usage.
#
# This container basically has to live in this directory in order to pull in the CI
# install scripts. If it lived in the root directory, it would have to pull in the
# entire repo as docker context during build; if it lived elsewhere, it wouldn't be
# able to make back-references to pull in the install scripts. So here it lives.

FROM python:3.7-buster

ENV DEBIAN_FRONTEND=noninteractive
ENV LC_ALL=C.UTF-8

# This is used by the 04_install.sh script; we can't read the Python version from
# .python-version for the same reasons as above, and it's more efficient to pull a
# preexisting Python image than it is to build from source.
ENV SKIP_PYTHON_INSTALL=1

# Must be built from ./ci/lint/ for these paths to work.
COPY ./docker-entrypoint.sh /entrypoint.sh
COPY ./04_install.sh /install.sh

RUN /install.sh && \
  echo 'alias lint="./ci/lint/06_script.sh"' >> ~/.bashrc && \
  chmod 755 /entrypoint.sh && \
  rm -rf /var/lib/apt/lists/*


WORKDIR /bitcoin
ENTRYPOINT ["/entrypoint.sh"]

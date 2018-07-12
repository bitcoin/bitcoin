FROM ubuntu:bionic

RUN apt-get update && apt-get install -y \
  ruby curl make libltdl7 git apache2 apt-cacher-ng python-vm-builder ruby qemu-utils \
  && rm -rf /var/lib/apt/lists

ARG USER_ID=1000
ARG GROUP_ID=1000

# add user with specified (or default) user/group ids
ENV USER_ID ${USER_ID}
ENV GROUP_ID ${GROUP_ID}
RUN groupadd -g ${GROUP_ID} dash
RUN useradd -u ${USER_ID} -g dash -s /bin/bash -m -d /dash dash

WORKDIR /dash
USER dash
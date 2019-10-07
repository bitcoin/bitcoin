FROM ubuntu:19.04

RUN apt-get update && apt-get install -y --no-install-recommends \
        curl ca-certificates \
        gcc libc6-dev \
        gcc-s390x-linux-gnu libc6-dev-s390x-cross \
        qemu-system-s390x \
        cpio

COPY linux-s390x.sh /
RUN bash /linux-s390x.sh

COPY test-runner-linux /

ENV CARGO_TARGET_S390X_UNKNOWN_LINUX_GNU_LINKER=s390x-linux-gnu-gcc \
    CARGO_TARGET_S390X_UNKNOWN_LINUX_GNU_RUNNER="/test-runner-linux s390x" \
    CC_s390x_unknown_linux_gnu=s390x-linux-gnu-gcc \
    PATH=$PATH:/rust/bin

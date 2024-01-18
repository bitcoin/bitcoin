# Copyright (c) The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit/.

# See ci/README.md for usage.

ARG CI_IMAGE_NAME_TAG
FROM ${CI_IMAGE_NAME_TAG}

ARG FILE_ENV
ENV FILE_ENV=${FILE_ENV}

COPY ./ci/retry/retry /usr/bin/retry
COPY ./ci/test/00_setup_env.sh ./${FILE_ENV} ./ci/test/01_base_install.sh /ci_container_base/ci/test/

RUN ["bash", "-c", "cd /ci_container_base/ && set -o errexit && source ./ci/test/00_setup_env.sh && ./ci/test/01_base_install.sh"]

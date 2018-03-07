#!/bin/bash -eux

echo "==> Disabling UseDNS from sshd configuration"
echo "UseDNS no" >> /etc/ssh/sshd_config

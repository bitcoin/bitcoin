#!/bin/bash

date > /etc/vagrant_box_build_time

SSH_USER=${SSH_USERNAME:-vagrant}
SSH_PASS=${SSH_PASSWORD:-vagrant}
SSH_USER_HOME=${SSH_USER_HOME:-/home/${SSH_USER}}
VAGRANT_INSECURE_KEY="ssh-rsa AAAAB3NzaC1yc2EAAAABIwAAAQEA6NF8iallvQVp22WDkTkyrtvp9eWW6A8YVr+kz4TjGYe7gHzIw+niNltGEFHzD8+v1I2YJ6oXevct1YeS0o9HZyN1Q9qgCgzUFtdOKLv6IedplqoPkcmF0aYet2PkEDo3MlTBckFXPITAMzF8dJSIFo9D8HfdOV0IAdx4O7PtixWKn5y2hMNG0zQPyUecp4pzC6kivAIhyfHilFR61RGL+GPXQ2MWZWFYbAGjyiYJnAmCP3NOTd0jMZEnDkbUvxhMmBYSdETk1rRgm+R4LOzFUGaHqHDLKLX+FIPKcF96hrucXzcWyLbIbEgE98OHlnVYCzRdK8jlqm8tehUc9c9WhQ== vagrant insecure public key"

# Packer passes boolean user variables through as '1', but this might change in
# the future, so also check for 'true'.
if [ "$INSTALL_VAGRANT_KEY" = "true" ] || [ "$INSTALL_VAGRANT_KEY" = "1" ]; then
    # Create Vagrant user (if not already present)
    if ! id -u $SSH_USER >/dev/null 2>&1; then
        echo "==> Creating $SSH_USER user"
        /usr/sbin/groupadd $SSH_USER
        /usr/sbin/useradd $SSH_USER -g $SSH_USER -G sudo -d $SSH_USER_HOME --create-home
        echo "${SSH_USER}:${SSH_PASS}" | chpasswd
    fi

    # Set up sudo
    echo "==> Giving ${SSH_USER} sudo powers"
    echo "${SSH_USER}        ALL=(ALL)       NOPASSWD: ALL" >> /etc/sudoers.d/vagrant
    chmod 440 /etc/sudoers.d/vagrant

    # Fix stdin not being a tty
    if grep -q -E "^mesg n$" /root/.profile && sed -i "s/^mesg n$/tty -s \\&\\& mesg n/g" /root/.profile; then
      echo "==> Fixed stdin not being a tty."
    fi

    echo "==> Installing vagrant key"
    mkdir $SSH_USER_HOME/.ssh
    chmod 700 $SSH_USER_HOME/.ssh
    cd $SSH_USER_HOME/.ssh

    # https://raw.githubusercontent.com/hashicorp/vagrant/master/keys/vagrant.pub
    echo "${VAGRANT_INSECURE_KEY}" > $SSH_USER_HOME/.ssh/authorized_keys
    chmod 600 $SSH_USER_HOME/.ssh/authorized_keys
    chown -R $SSH_USER:$SSH_USER $SSH_USER_HOME/.ssh
fi

# Grub updates have been known to trash vagrant boxen.
echo "==> Holding packages known to disrupt vagrant"
echo 'grub-common hold' | dpkg --set-selections

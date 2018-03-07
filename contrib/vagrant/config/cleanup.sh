#!/bin/bash -eux

SSH_USER=${SSH_USERNAME:-vagrant}

# Make sure udev does not block our network - http://6.ptmc.org/?p=164
echo "==> Cleaning up udev rules"
rm -rf /dev/.udev/
rm /lib/udev/rules.d/75-persistent-net-generator.rules

echo "==> Cleaning up leftover dhcp leases"
# Ubuntu 10.04
if [ -d "/var/lib/dhcp3" ]; then
    rm /var/lib/dhcp3/*
fi
# Ubuntu 12.04 & 14.04
if [ -d "/var/lib/dhcp" ]; then
    rm /var/lib/dhcp/*
fi 

UBUNTU_VERSION=$(lsb_release -sr)
if [[ ${UBUNTU_VERSION} == 16.04 ]] || [[ ${UBUNTU_VERSION} == 16.10 ]]; then
    # Modified version of
    # https://github.com/cbednarski/packer-ubuntu/blob/master/scripts-1604/vm_cleanup.sh#L9-L15
    # Instead of eth0 the interface is now called ens5 to mach the PCI
    # slot, so we need to change the networking scripts to enable the
    # correct interface.
    #
    # NOTE: After the machine is rebooted Packer will not be able to reconnect
    # (Vagrant will be able to) so make sure this is done in your final
    # provisioner.
    sed -i "s/ens3/ens5/g" /etc/network/interfaces
fi

# Add delay to prevent "vagrant reload" from failing
echo "pre-up sleep 2" >> /etc/network/interfaces

echo "==> Cleaning up tmp"
rm -rf /tmp/*

# Cleanup apt cache
apt-get -y autoremove --purge
apt-get -y clean
apt-get -y autoclean

echo "==> Installed packages"
dpkg --get-selections | grep -v deinstall

DISK_USAGE_BEFORE_CLEANUP=$(df -h)

# Remove Bash history
unset HISTFILE
rm -f /root/.bash_history
rm -f /home/${SSH_USER}/.bash_history

# Clean up log files
find /var/log -type f | while read f; do echo -ne '' > "${f}"; done;

echo "==> Clearing last login information"
>/var/log/lastlog
>/var/log/wtmp
>/var/log/btmp

# NOTE: Shrinking is not part of the build process
# so this will only grow the image...

# # Whiteout root
# count=$(df --sync -kP / | tail -n1  | awk -F ' ' '{print $4}')
# let count--
# dd if=/dev/zero of=/tmp/whitespace bs=1024 count=$count
# rm /tmp/whitespace

# # Whiteout /boot
# count=$(df --sync -kP /boot | tail -n1 | awk -F ' ' '{print $4}')
# let count--
# dd if=/dev/zero of=/boot/whitespace bs=1024 count=$count
# rm /boot/whitespace

# echo '==> Clear out swap and disable until reboot'
# set +e
# swapuuid=$(/sbin/blkid -o value -l -s UUID -t TYPE=swap)
# case "$?" in
#     2|0) ;;
#     *) exit 1 ;;
# esac
# set -e
# if [ "x${swapuuid}" != "x" ]; then
#     # Whiteout the swap partition to reduce box size
#     # Swap is disabled till reboot
#     swappart=$(readlink -f /dev/disk/by-uuid/$swapuuid)
#     /sbin/swapoff "${swappart}"
#     dd if=/dev/zero of="${swappart}" bs=1M || echo "dd exit code $? is suppressed"
#     /sbin/mkswap -U "${swapuuid}" "${swappart}"
# fi

# # Zero out the free space to save space in the final image
# dd if=/dev/zero of=/EMPTY bs=1M  || echo "dd exit code $? is suppressed"
# rm -f /EMPTY

# # Make sure we wait until all the data is written to disk, otherwise
# # Packer might quite too early before the large files are deleted
# sync

# echo "==> Disk usage before cleanup"
# echo ${DISK_USAGE_BEFORE_CLEANUP}

# echo "==> Disk usage after cleanup"
# df -h

ESSENTIAL_PACKAGES="
ntp
nfs-common
"

GITIAN_PACKAGES="
git
apache2
apt-cacher-ng
bridge-utils
python-vm-builder
ruby
qemu-utils
lxc
"

export DEBIAN_FRONTEND=noninteractive

# Perform ALL security updates for the guest VM distribution.
echo "==> Updating distribution-provided package lists"
apt-get -y update
echo "==> Applying security updates & upgrading default packages"
apt-get -y dist-upgrade

# Essential packages are necessary for virtualbox/vagrant integration.
echo "==> Installing packages necessary for virtualbox/vagrant integration"
apt-get -y install $ESSENTIAL_PACKAGES

# Gitian packages are necessary for gitian-builder.
echo "==> Installing packages necessary for gitian-builder"
apt-get -y install $GITIAN_PACKAGES

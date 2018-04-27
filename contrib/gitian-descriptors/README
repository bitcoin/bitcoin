Gavin's notes on getting gitian builds up and running:

You need the right hardware: you need a 64-bit-capable CPU with hardware virtualization support (Intel VT-x or AMD-V). Not all modern CPUs support hardware virtualization.

You probably need to enable hardware virtualization in your machine's BIOS.

You need to be running a recent version of 64-bit-Ubuntu, and you need to install several prerequisites:
  sudo apt-get install apache2 git apt-cacher-ng python-vm-builder qemu-kvm

Sanity checks:
  sudo service apt-cacher-ng status   # Should return apt-cacher-ng is running
  ls -l /dev/kvm   # Should show a /dev/kvm device

Once you've got the right hardware and software:

    git clone git://github.com/bitcoin/bitcoin.git
    git clone git://github.com/devrandom/gitian-builder.git
    mkdir gitian-builder/inputs
    wget 'http://miniupnp.tuxfamily.org/files/download.php?file=miniupnpc-1.6.tar.gz' -O gitian-builder/inputs/miniupnpc-1.6.tar.gz

    cd gitian-builder
    bin/make-base-vm --arch i386
    bin/make-base-vm --arch amd64
    cd ..

    # To build
    cd bitcoin
    git pull
    cd ../gitian-builder
    git pull
    ./bin/gbuild --commit bitcoin=HEAD ../bitcoin/contrib/gitian.yml

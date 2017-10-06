sudo checkinstall \
    --install=no \
    --fstrans=yes \
    --nodoc \
    --maintainer="IoP Ventures LLC \<info@iop-ventures.com\>" \
    --pkgsource="https://github.com/internet-of-people/iop-hd" \
    --pkglicense=MIT \
    --pkggroup=net \
    --pkgname=iop-hd \
    --pkgversion=6.0.0b2 \
    --pkgarch=$(dpkg \
    --print-architecture) \
    $@

# Package Description: IoP is a digital currency for the Internet of People that enables instant payments to anyone, anywhere in the world.
#
# -- If library versions has to be restricted, this escaping format works:
#--requires="libssl1.0.0 \(\>= 1.0.0\),libevent-2.0-5 \(\>= 2.0.0\),libevent-pthreads-2.0-5 \(\>= 2.0.0\),libdb5.3++ \(\>= 5.3.0\),libqrencode3 \(\>= 3.0.0\),libprotobuf9v5 \(\>= 2.6.0\),libqt5gui5 \(\>= 5.0.0\),libboost-program-options1.58.0 \(\>= 1.58.0\),libboost-thread1.58.0 \(\>= 1.58.0\),libboost-chrono1.58.0 \(\>= 1.58.0\)" \

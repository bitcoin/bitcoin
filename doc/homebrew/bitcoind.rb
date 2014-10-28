require "formula"

class Bitcoind < Formula
  homepage "https://bitcoin.org/en/"
  url "https://github.com/bitcoin/bitcoin/archive/v0.9.3.tar.gz"
  sha256 "3ed92e8323cb4187cae015457c7c5920a5c658438f01c6c45f0ce3aabf9bd428"
  head "https://github.com/bitcoin/bitcoin.git"

  depends_on "pkg-config" => :build
  depends_on "autoconf" => :build
  depends_on "automake" => :build
  depends_on "libtool" => :build
  depends_on "openssl"
  depends_on "boost"
  depends_on "protobuf"
  depends_on "qt" => :recommended
  depends_on "miniupnpc" => :recommended
  depends_on "qrencode" => :recommended

  resource "berkeleydb4" do
    url "http://download.oracle.com/berkeley-db/db-4.8.30.tar.gz"
    sha256 "e0491a07cdb21fb9aa82773bbbedaeb7639cbd0e7f96147ab46141e0045db72a"
  end

  def install
    resource("berkeleydb4").stage do
      ENV.deparallelize

    inreplace "dbinc/atomic.h", "__atomic_compare_exchange((p), (o), (n))", "__atomic_compare_exchange_db((p), (o), (n))"
    inreplace "dbinc/atomic.h", "static inline int __atomic_compare_exchange(", "static inline int __atomic_compare_exchange_db("

    args = ["--disable-debug",
            "--prefix=#{prefix}/berkeley-db4/4.8.30",
            "--mandir=#{man}/berkeley-db4/4.8.30",
            "--disable-shared",
            "--disable-replication",
            "--enable-cxx"]

      cd 'build_unix' do
        system "../dist/configure", *args
        system "make"
        system "make", "install"
      end
    end

    ENV.prepend_path "PATH", "#{prefix}/berkeley-db4/4.8.30/bin"
    system "./autogen.sh"

    inreplace "configure", "bdb_prefix=`$BREW --prefix berkeley-db4`", "bdb_prefix='$#{prefix}/berkeley-db4/4.8.30'" if build.stable?
    inreplace "configure", "bdb_prefix=`$BREW --prefix berkeley-db4 2>/dev/null`", "bdb_prefix=`$#{prefix}/berkeley-db4/4.8.30 2>/dev/null`" if build.head?

    args = ["--prefix=#{prefix}",
            "--disable-dependency-tracking",
            "--with-incompatible-bdb"]

    args << "--with-qrencode" if build.with? "qrencode"
    args << "--with-gui" if build.with? "qt"
    args << "--with-miniupnpc" if build.with? "miniupnpc"

    system "./configure", *args
    system "make"
    system "make", "check"
    system "make", "install"
  end

  def caveats; <<-EOS.undent
    Bitcoind will need to set up bitcoin.conf on first-run. You can manually do this with:
      echo -e "rpcuser=bitcoinrpc\\nrpcpassword=$(xxd -l 16 -p /dev/urandom)" > ~/Library/Application\\ Support/Bitcoin/bitcoin.conf
      chmod 600 ~/Library/Application\\ Support/Bitcoin/bitcoin.conf

      Or automatically by running 'bitcoind' in your Terminal application, which will provide you instructions.
    EOS
  end
end

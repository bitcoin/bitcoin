{ nixpkgs ? <nixpkgs>}:

with import nixpkgs {};

rec {
  c_rrb = stdenv.mkDerivation rec {
    name = "c-rrb-${version}";
    version = "git-${commit}";
    commit = "d908617ff84515af90c454ff4d0f98675ae6b456";
    src = fetchFromGitHub {
      owner = "hyPiRion";
      repo = "c-rrb";
      rev = commit;
      sha256 = "0zmha3xi80vgdcwzb4vwdllf97dvggjpjfgahrpsb5f5qi3yshxa";
    };
    nativeBuildInputs = [ autoreconfHook ];
    propagatedBuildInputs = [ boehmgc ];
    meta = with lib; {
      homepage = "http://hypirion.com/thesis";
      description = "RRB-tree implemented as a library in C. ";
      license = licenses.mit;
    };
  };

  steady = stdenv.mkDerivation rec {
    name = "steady-${version}";
    version = "git-${commit}";
    commit = "aac65955f18d0de856e3421d8593378d5e1832c4";
    src = fetchFromGitHub {
      owner = "marcusz";
      repo = "steady";
      rev = commit;
      sha256 = "0zaxr9aj1q5sr99h133fzxch8lw9daw1pxs2g972s02gdq8r8fzh";
    };
    dontBuild = true;
    installPhase = "mkdir -vp $out/include; cp -vr $src/steady $out/include/";
    meta = with lib; {
      homepage = "https://github.com/marcusz/steady";
      description = "This is a fast and reliable persistent (immutable) vector class for C++";
      license = licenses.asl20;
    };
  };

  chunkedseq = stdenv.mkDerivation rec {
    name = "chunkedseq-${version}";
    version = "git-${commit}";
    commit = "f1c4f9470edcb59996fdc4aad34311767c46b25e";
    src = fetchFromGitHub {
      owner = "deepsea-inria";
      repo = "chunkedseq";
      rev = commit;
      sha256 = "19bps5r3dy55jlvpwq8x2s4d0mwwx0djb6g2r2vvbf628bg6cx8p";
    };
    dontBuild = true;
    installPhase = "mkdir -vp $out/include/chunkedseq; cp -vr $src/include/* $out/include/chunkedseq/";
    meta = with lib; {
      homepage = "http://deepsea.inria.fr/chunkedseq";
      description = "Container data structure for representing sequences by many fixed-capacity heap-allocated buffers--chunks";
      license = licenses.mit;
    };
  };

  immutable_cpp = stdenv.mkDerivation rec {
    name = "immutable-cpp-${version}";
    version = "git-${commit}";
    commit = "a4a32022d895dd0d3c03547a2b2a2b03face01eb";
    src = fetchFromGitHub {
      owner = "rsms";
      repo = "immutable-cpp";
      rev = commit;
      sha256 = "11zn67avqcc8baddf43fsd29bpv08p0ri6zqng12105k725fq2p7";
    };
    dontBuild = true;
    installPhase = "mkdir -vp $out/include; cp -vr $src/immutable $out/include/";
    meta = with lib; {
      homepage = "https://github.com/rsms/immutable-cpp";
      description = "Persistent immutable data structures for C++";
      license = licenses.mit;
    };
  };

  hash_trie = stdenv.mkDerivation rec {
    name = "hash_trie-${version}";
    version = "git-${commit}";
    commit = "1ce346c74923ba16329332103dc0f425b658c8be";
    src = fetchFromGitHub {
      owner = "philsquared";
      repo = "hash_trie";
      rev = commit;
      sha256 = "0rpa1682vm0fvvrs1jr7jdkskcrgkm3qrawrcr7xrmknz5jf0v4l";
    };
    dontBuild = true;
    installPhase = "mkdir -vp $out/include; cp -vr $src/hash_trie.hpp $out/include/";
    meta = with lib; {
      homepage = "https://github.com/rsms/immutable-cpp";
      description = "Persistent immutable data structures for C++";
      license = licenses.mit;
    };
  };
}

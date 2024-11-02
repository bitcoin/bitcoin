{ pkgs ? import <nixpkgs> {} }:

with pkgs;

let
  inherit (import (pkgs.fetchFromGitHub {
    owner = "hercules-ci";
    repo = "gitignore.nix";
    rev = "80463148cd97eebacf80ba68cf0043598f0d7438";
    sha256 = "1l34rmh4lf4w8a1r8vsvkmg32l1chl0p593fl12r28xx83vn150v";
  }) {}) gitignoreSource;

  nixFilter = name: type: !(lib.hasSuffix ".nix" name);
  srcFilter = src: lib.cleanSourceWith {
    filter = nixFilter;
    src = gitignoreSource src;
  };

in
stdenv.mkDerivation rec {
  name = "immer-git";
  version = "git";
  src = srcFilter ./.;
  nativeBuildInputs = [ cmake ];
  dontBuild = true;
  dontUseCmakeBuildDir = true;
  cmakeFlags = [
    "-Dimmer_BUILD_TESTS=OFF"
    "-Dimmer_BUILD_EXAMPLES=OFF"
  ];
  meta = {
    homepage    = "https://github.com/arximboldi/immer";
    description = "Postmodern immutable data structures for C++";
    license     = lib.licenses.boost;
  };
}

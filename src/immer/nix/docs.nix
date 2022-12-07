{ nixpkgs ? <nixpkgs>}:

with import nixpkgs {};

rec {
  breathe = with python27Packages; buildPythonPackage rec {
    version = "git-arximboldi-${commit}";
    pname = "breathe";
    name = "${pname}-${version}";
    commit = "5074aecb5ad37bb70f50216eaa01d03a375801ec";
    src = fetchFromGitHub {
      owner = "arximboldi";
      repo = "breathe";
      rev = commit;
      sha256 = "10kkh3wb0ggyxx1a7x50aklhhw0cq269g3jddf2gb3pv9gpbj7sa";
    };
    propagatedBuildInputs = [ docutils sphinx ];
    meta = with stdenv.lib; {
      homepage = https://github.com/michaeljones/breathe;
      license = licenses.bsd3;
      description = "Sphinx Doxygen renderer";
      inherit (sphinx.meta) platforms;
    };
  };

  recommonmark = python27Packages.recommonmark;
}

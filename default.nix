{ stdenv, flex, bison, version }:
stdenv.mkDerivation {
  name = "pjass";
  src = ./.;
  doCheck = true;
  checkTarget = "test";
  makeFlags = [ "CFLAGS=-O" "PREFIX=$(out)" "VERSION=${version}" ];
  nativeBuildInputs = [ flex bison ];
}

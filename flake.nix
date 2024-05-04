{
  inputs = { systems.url = "github:nix-systems/default"; };

  outputs = { self, nixpkgs, systems }:
    let eachSystem = nixpkgs.lib.genAttrs (import systems);
    in {
      devShells = eachSystem (system:
        let pkgs = import nixpkgs { inherit system; };
        in {
          default =
            pkgs.mkShell { packages = [ pkgs.gnumake pkgs.flex pkgs.bison ]; };
        });
      packages = eachSystem (system:
        let
          pkgs = import nixpkgs { inherit system; };
          mingw = pkgs.pkgsCross.mingw32;
          drv-args = {
            src = self;
            nativeBuildInputs = [ pkgs.flex pkgs.bison ];
            name = "pjass";
            buildPhase = "make pjass";
            installPhase = "install -Dt $out/bin pjass";
          };
        in {
          pjass = pkgs.stdenv.mkDerivation drv-args;
          pjass-mingw = mingw.stdenv.mkDerivation (drv-args // {
            CFLAGS = "-O2";
            installPhase = "install -Dt $out/bin pjass.exe";
          });
        });
    };

}


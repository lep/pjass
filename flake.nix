{
  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = { self, nixpkgs, flake-utils }:
    flake-utils.lib.eachDefaultSystem (system:
      let
        pkgs = import nixpkgs { inherit system; };
        packageName = "pjass";
        buildInputs = [ pkgs.gnumake pkgs.flex pkgs.bison ];

        drv-args = {
          name = packageName;
          src = self;
          inherit buildInputs;

          buildPhase = "make pjass";
          installPhase = ''
            install -Dt $out/bin pjass
          '';
        };
        drv = pkgs.stdenv.mkDerivation drv-args;

        mingw = pkgs.pkgsCross.mingw32;
      in rec {
        apps.default = {
          type = "app";
          program = "${drv}/bin/pjass";
        };
        apps.pjass = apps.default;
        packages = {
          default = drv;
          ${packageName} = drv;
          "${packageName}-mingw32" = mingw.stdenv.mkDerivation (drv-args // {
            CFLAGS = "-O2";
            installPhase = ''
              install -Dt $out/bin pjass.exe
            '';
          });
        };

        defaultPackage = packages.${packageName};
      });
}


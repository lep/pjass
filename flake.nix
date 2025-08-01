{
  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixpkgs-unstable";
    systems.url = "github:nix-systems/default";
  };

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
          version = if self ? shortRev then self.shortRev else "nix-dirty";
        in {
          default = pkgs.callPackage ./default.nix { inherit version; };
          mingw = pkgs.pkgsCross.mingw32.callPackage ./default.nix {
            inherit version;
          };
        });
    };

}


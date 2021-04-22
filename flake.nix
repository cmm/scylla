{
  description = "Monstrously Fast + Scalable NoSQL";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = { self, nixpkgs, flake-utils } @ inputs:
    flake-utils.lib.eachDefaultSystem (system: let
      pkgs = nixpkgs.legacyPackages.${system};
      packageName = "scylla";

      repl = pkgs.writeText "repl" ''
        let
          self = builtins.getFlake (toString ${inputs.self.outPath});
          pkgs = import self.inputs.nixpkgs { system = "${system}"; };
        in {
          inherit self pkgs;
        }
      '';

      args = {
        flake = true;
        srcPath = "${self}";
        inherit nixpkgs system repl;
      };
    in {
      packages.${packageName} = import ./default.nix args;

      defaultPackage = self.packages.${system}.${packageName};

      devShell = import ./shell.nix args;
    });
}

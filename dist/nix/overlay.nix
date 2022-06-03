nixpkgs:
final: prev:

let
  patched = pkg: patches:
    pkg.overrideAttrs (old: {
      patches = (old.patches or []) ++ (map final.fetchurl patches);
    });
in {
  gdbWithGreenThreadSupport = patched prev.gdb [{
    url = "https://github.com/cmm/gnu-binutils/commit/eb9148f3b9b377c5c4bae8cb4e29e5fdccd45ab0.patch";
    sha256 = "1qhc8a6lssnvpjvfc6sidfv9hmz96gxz4czcpannd7z5dd82mv1l";
  }];

  zstdStatic = final.callPackage "${nixpkgs}/pkgs/tools/compression/zstd" {
    static = true;
    buildContrib = false;
    doCheck = false;
  };

  cxxbridge = final.callPackage ./pkg/upstreamable/cxxbridge { };
  wasmtime = final.callPackage ./pkg/upstreamable/wasmtime { };

  scylla-driver = final.callPackage ./pkg/upstreamable/python-driver { };
}

nixpkgs:
final: prev:

let
  lib = final.lib;
  stdenv = final.stdenv;

  patched = pkg: patches: pkg.overrideAttrs (old: {
    patches = (old.patches or []) ++ (map final.fetchurl patches);
  });
in {
  gdbWithGreenThreadSupport = patched prev.gdb [{
    url = "https://github.com/cmm/gnu-binutils/commit/1c52ca4b27e93e1684c68eeaee44ca3e36648410.patch";
    sha256 = "sha256-3s3KvN70dHMdr7Sx1dtzbZ8S+MynPTN7yCocoGlea2Y=";
  }];

  zstdStatic = final.callPackage "${nixpkgs}/pkgs/tools/compression/zstd" {
    static = true;
    buildContrib = false;
    doCheck = false;
  };

  # wrap mold so it find libraries (
  mold = prev.mold.overrideAttrs (old: {
    suffixSalt = lib.replaceStrings ["-" "."] ["_" "_"] stdenv.targetPlatform.config;
    wrapperName = "MOLD_WRAPPER";
    coreutils_bin = lib.getBin final.coreutils;
    postInstall = let
      targetPrefix = lib.optionalString (stdenv.targetPlatform != stdenv.hostPlatform)
        (stdenv.targetPlatform.config + "-");
    in ''
      mkdir -p $out/nix-support

      mv $out/bin/mold $out/bin/.mold

      export prog=$out/bin/.mold
      substituteAll \
        ${nixpkgs}/pkgs/build-support/bintools-wrapper/ld-wrapper.sh \
        "$out/bin/${targetPrefix}mold"
      chmod +x "$out/bin/${targetPrefix}mold"
      ln -s $out/bin/${targetPrefix}mold $out/bin/${targetPrefix}ld

      substituteAll \
        ${nixpkgs}/pkgs/build-support/bintools-wrapper/add-flags.sh \
        $out/nix-support/add-flags.sh
      substituteAll \
        ${nixpkgs}/pkgs/build-support/bintools-wrapper/add-hardening.sh \
        $out/nix-support/add-hardening.sh
      substituteAll \
        ${nixpkgs}/pkgs/build-support/wrapper-common/utils.bash \
        $out/nix-support/utils.bash
    '';
  });

  cxxbridge = final.callPackage ./pkg/upstreamable/cxxbridge { };
  wasmtime = final.callPackage ./pkg/upstreamable/wasmtime { };

  scylla-driver = final.callPackage ./pkg/upstreamable/python-driver { };
}

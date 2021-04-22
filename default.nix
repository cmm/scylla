# Copyright (C) 2021-present ScyllaDB
#

#
# SPDX-License-Identifier: AGPL-3.0-or-later
#

#
# * At present this is not very useful for nix build, just for nix develop
#
# * IMPORTANT: to avoid using up ungodly amounts of disk space under
#   /nix/store/ when you are not using flakes, make sure to move the
#   actual build directory outside this tree and make ./build a
#   symlink to it.  Or use flakes (please use flakes!).
#

{ flake ? false
, shell ? false
, nixpkgs ? <nixpkgs>
, system ? null
, srcPath ? builtins.path { path = ./.; name = "scylla"; }
, repl ? null
, mode ? "release"
, verbose ? false

# shell env will want to add stuff to the environment, and the way
# for it to do so is to pass us a function with this signatire:
, devInputs ? ({ pkgs, llvm, withPatches }: [])
} @ args:

let
  pkgs = import nixpkgs {
    system = if system != null then system else builtins.currentSystem;
  };

  inherit (import (builtins.fetchTarball {
    url = "https://github.com/hercules-ci/gitignore/archive/5b9e0ff9d3b551234b4f3eb3983744fa354b17f1.tar.gz";
    sha256 = "01l4phiqgw9xgaxr6jr456qmww6kzghqrnbc7aiiww3h6db5vw53";
  }) { inherit (pkgs) lib; })
    gitignoreSource;

  inherit (builtins)
    baseNameOf
    concatStringsSep
    fetchurl
    head
    map
    match
    split
    trace
    typeOf
  ;

  withPatches = pkg: patches:
    pkg.overrideAttrs (old: {
      patches = (old.patches or []) ++ (map (patch: if (typeOf patch) == "path"
                                                    then patch
                                                    else pkgs.fetchpatch patch)
        patches);
    });

in with pkgs; let
  zstdStatic = callPackage "${nixpkgs}/pkgs/tools/compression/zstd" {
    static = true;
    buildContrib = false;
    doCheck = false;
  };

  # tests don't like boost17x (which is boost177 at the time of writing)
  boost = boost175;

  # current clang13 cannot compile Scylla with sanitizers:
  llvm = llvmPackages_12;
  # llvm = llvmPackages_latest;

  clang = llvm.clang;
  cc = llvm.stdenv.cc;

  # define custom ccache- and distcc-aware wrappers for all relevant
  # compile drivers (used only in shell env)
  wrappers =
    let wrap = pkg: driver: ''
      #! ${stdenv.shell} -e
      driver=${pkg}/bin/${driver}
      dist_driver="$driver${if ((match "clang.*" driver) != null) then " -Wno-error=unused-command-line-argument" else ""}"
      distcc=${distcc}/bin/distcc
      ccache=${ccache}/bin/ccache

      export DISTCC_IO_TIMEOUT=1200  # hello, repair/row_level.cc

      wrap=
      if [[ -z "$NODISTCC" ]]; then
        wrap=d
      fi
      if [[ -n "$CCACHE_DIR" ]]; then
        wrap+=c
      fi

      if [[ -z "$wrap" ]]; then
        exec $driver "$@"
      elif [[ "$wrap" == d ]]; then
        exec $distcc $dist_driver "$@"
      elif [[ "$wrap" == c ]]; then
        exec $ccache $driver "$@"
      elif [[ "$wrap" == dc ]]; then
        export CCACHE_PREFIX=$distcc
        exec $ccache $dist_driver "$@"
      else
        echo wrapper bug 1>&2
        exit 1
      fi
    ''; in runCommand "distcc-ccache-wrap" { } ''
      ${coreutils}/bin/mkdir -p $out/bin
      ${lib.concatStrings (map ({pkg, driver}: ''
        ${coreutils}/bin/echo ${lib.escapeShellArg (wrap pkg driver)} > $out/bin/${driver}
        ${coreutils}/bin/chmod +x $out/bin/${driver}
      '') [
        { pkg = gcc; driver = "gcc"; }
        { pkg = gcc; driver = "g++"; }
        { pkg = clang; driver = "clang"; }
        { pkg = clang; driver = "clang++"; }
        { pkg = cc; driver = "cc"; }
        { pkg = cc; driver = "c++"; }
      ])}
    '';

  stdenv = if shell
           then overrideCC llvm.stdenv wrappers
           else llvm.stdenv;

  # filter out anything .gitignored (if not flake -- for flakes nix
  # does it itself, and much more efficiently) _and_ any .nix files
  noNix = path: type: type != "regular" || (match ".*\.nix" path) == null;
  src = builtins.filterSource noNix
    (if flake
     then srcPath
     else gitignoreSource srcPath);

  derive = if shell
           then pkgs.mkShell.override { inherit stdenv; }
           else stdenv.mkDerivation;

in derive ({
  name = "scylla";
  inherit src;

  # since Scylla build, as it exists, is not cross-capable, the
  # nativeBuildInputs/buildInputs distinction below ranges, depending
  # on how charitable one feels, from "pedantic" through
  # "aspirational" all the way to "cargo cult ritual" -- i.e. not
  # expected to be actually correct or verifiable.  but it's the
  # thought that counts!
  nativeBuildInputs = [
    ant
    antlr3
    boost
    cmake
    gcc
    # openjdk8_headless
    openjdk11_headless
    libtool
    llvm.bintools
    maven
    ninja
    pkg-config
    python2
    (python3.withPackages (ps: with ps; [
      boto3
      cassandra-driver
      colorama
      distro
      magic
      psutil
      pyparsing
      pytest
      pyudev
      pyyaml
      requests
      setuptools
      urwid
    ]))
    ragel
    stow
  ] ++ (devInputs { inherit pkgs llvm withPatches; });

  buildInputs = [
    antlr3
    boost
    c-ares
    cryptopp
    fmt
    gmp
    gnutls
    hwloc
    icu
    jsoncpp
    libidn2
    libp11
    libsystemtap
    libtasn1
    libunistring
    libxfs
    libxml2
    libyamlcpp
    llvm.compiler-rt
    lksctp-tools
    lua53Packages.lua
    lz4
    nettle
    numactl
    openssl
    p11-kit
    protobuf
    rapidjson
    snappy
    systemd
    thrift
    valgrind
    xorg.libpciaccess
    xxHash
    zlib
    zstdStatic
  ];

  JAVA8_HOME = "${openjdk8_headless}/lib/openjdk";
  JAVA_HOME = "${openjdk11_headless}/lib/openjdk";

}
// (if shell then {

  configurePhase = "./configure.py${if verbose then " --verbose" else ""} --disable-dpdk";

} else {

  # sha256 of the filtered source tree:
  SCYLLA_RELEASE = head (split "-" (baseNameOf src));

  postPatch = ''
    patchShebangs ./configure.py
    patchShebangs ./seastar/scripts/seastar-json2code.py
    patchShebangs ./seastar/cooking.sh
  '';

  configurePhase = "./configure.py${if verbose then " --verbose" else ""} --mode=${mode}";

  buildPhase = ''
    ${ninja}/bin/ninja \
      build/${mode}/scylla \
      build/${mode}/iotune \

  '';
   #   build/${mode}/dist/tar/scylla-tools-package.tar.gz \
   #   build/${mode}/dist/tar/scylla-jmx-package.tar.gz \

  installPhase = ''
    echo not implemented 1>&2
    exit 1
  '';

})
// (if !shell || repl == null then {} else {

  REPL = repl;

})
)

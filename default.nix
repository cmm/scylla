# Copyright (C) 2021-present ScyllaDB
#

#
# SPDX-License-Identifier: AGPL-3.0-or-later
#

#
# * At present this is not very useful for "nix build", just for
#   "nix develop"
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
  inherit (builtins)
    baseNameOf
    fetchurl
    head
    map
    match
    split
    trace
    typeOf
  ;

  pkgs = import nixpkgs {
    system = if system != null then system else builtins.currentSystem;
    overlays = [
      (final: _: {
        cxxbridge = final.callPackage ./dist/nix/pkg/upstreamable/cxxbridge { };
        wasmtime = final.callPackage ./dist/nix/pkg/upstreamable/wasmtime { };
        scylla-driver = final.callPackage ./dist/nix/pkg/upstreamable/python-driver { };

        # build zstd statically, since the default dynamic-built zstd
        # lacks some things Scylla needs
        zstdStatic = final.callPackage "${nixpkgs}/pkgs/tools/compression/zstd" {
          static = true;
          buildContrib = false;
          doCheck = false;
        };
      })
    ];
  };

  inherit (import (builtins.fetchTarball {
    url = "https://github.com/hercules-ci/gitignore/archive/5b9e0ff9d3b551234b4f3eb3983744fa354b17f1.tar.gz";
    sha256 = "01l4phiqgw9xgaxr6jr456qmww6kzghqrnbc7aiiww3h6db5vw53";
  }) { inherit (pkgs) lib; })
    gitignoreSource;

  withPatches = pkg: patches:
    pkg.overrideAttrs (old: {
      patches = (old.patches or []) ++ (map (patch: if (typeOf patch) == "path"
                                                    then patch
                                                    else pkgs.fetchpatch patch)
        patches);
    });

  # tests don't like boost17x (which is boost177 at the time of writing)
  boost = pkgs.boost175;

  # current clang13 cannot compile Scylla with sanitizers:
  llvm = pkgs.llvmPackages_12;
  # llvm = pkgs.llvmPackages_latest;

  stdenvUnwrapped = llvm.stdenv;

  # define custom ccache- and distcc-aware wrappers for all relevant
  # compile drivers (used only in shell env)
  cc-wrappers = pkgs.callPackage ./dist/nix/pkg/custom/ccache-distcc-wrap {
    cc = stdenvUnwrapped.cc;
    clang = llvm.clang;
    inherit (pkgs) gcc;
  };

  stdenv = if shell then pkgs.overrideCC stdenvUnwrapped cc-wrappers
           else stdenvUnwrapped;

  noNix = path: type: type != "regular" || (match ".*\.nix" path) == null;
  src = builtins.filterSource noNix (if flake then srcPath
                                     else gitignoreSource srcPath);

  derive = if shell then pkgs.mkShell.override { inherit stdenv; }
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
  nativeBuildInputs = with pkgs; [
    ant
    antlr3
    boost
    cargo
    cmake
    cxxbridge
    gcc
    openjdk11_headless
    libtool
    llvm.bintools
    maven
    ninja
    pkg-config
    python2
    (python3.withPackages (ps: with ps; [
      aiohttp
      boto3
      colorama
      distro
      magic
      psutil
      pyparsing
      pytest
      pyudev
      pyyaml
      requests
      scylla-driver
      setuptools
      tabulate
      urwid
    ]))
    ragel
    stow
  ] ++ (devInputs { inherit pkgs llvm withPatches; });

  buildInputs = with pkgs; [
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
    wasmtime
    xorg.libpciaccess
    xxHash
    zlib
    zstdStatic
  ];

  JAVA8_HOME = "${pkgs.openjdk8_headless}/lib/openjdk";
  JAVA_HOME = "${pkgs.openjdk11_headless}/lib/openjdk";

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
    ${pkgs.ninja}/bin/ninja \
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

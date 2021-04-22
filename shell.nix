# Copyright (C) 2021-present ScyllaDB
#

#
# SPDX-License-Identifier: AGPL-3.0-or-later

args:

import ./default.nix (args // {
  shell = true;

  devInputs = { pkgs, llvm, withPatches }: with pkgs; [
    # for impure building
    ccache
    distcc

    # for debugging
    binutils  # addr2line etc.
    elfutils
    (withPatches gdb [
      {
        url = "https://github.com/cmm/gnu-binutils/commit/eb9148f3b9b377c5c4bae8cb4e29e5fdccd45ab0.patch";
        sha256 = "1qhc8a6lssnvpjvfc6sidfv9hmz96gxz4czcpannd7z5dd82mv1l";
      }
    ])
    llvm.llvm
    lz4       # coredumps on modern Systemd installations are lz4-compressed

    # etc
    diffutils
    colordiff
  ];
})

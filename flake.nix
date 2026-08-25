{
  inputs = {
    nixpkgs-unstable.url = "github:NixOS/nixpkgs/nixos-unstable";
    nixpkgs-22.url = "github:NixOS/nixpkgs/nixos-22.05";
    flake-utils.url = "github:numtide/flake-utils";

    treefmt-nix = {
      url = "github:numtide/treefmt-nix";
      inputs.nixpkgs.follows = "nixpkgs-unstable";
    };

    dpf = {
      url = "github:DISTRHO/DPF/4238e1c7f0351bbe488d79f0899c540543ac7583";
      flake = false;
    };

    gcc9 = {
      url = "https://ftp.gnu.org/gnu/gcc/gcc-9.5.0/gcc-9.5.0.tar.xz";
      flake = false;
    };

    libmodla = {
      url = "https://download.mod.audio/shared/libmodla-v1.3.1-pablito.tar.gz";
      flake = false;
    };

    darkglass-lv2-extensions = {
      url = "github:Darkglass-Electronics/LV2-Extensions/6d34a7111d49a260d2ad2e2fa83d48d3db2c670b";
      flake = false;
    };

    mod-lv2-extensions = {
      url = "github:mod-audio/mod-lv2-extensions/f4341a6c2b2f50e2eb405b06ce19f9f0b4b1a62b";
      flake = false;
    };

    kxstudio-lv2-extensions = {
      url = "github:KXStudio/LV2-Extensions/8b5f6cb9cd75e300958c9aacac253d44c964e80b";
      flake = false;
    };

    range-v3 = {
      url = "github:ericniebler/range-v3/108f93c279c8f9cec175dac361084983d0176e99";
      flake = false;
    };
  };

  outputs =
    {
      nixpkgs-unstable,
      nixpkgs-22,
      flake-utils,
      treefmt-nix,
      dpf,
      gcc9,
      libmodla,
      darkglass-lv2-extensions,
      mod-lv2-extensions,
      kxstudio-lv2-extensions,
      range-v3,
      ...
    }:
    flake-utils.lib.eachSystem
      [
        "aarch64-linux"
        "x86_64-linux"
      ]
      (
        system:
        let
          nativePkgs = nixpkgs-22.legacyPackages.${system}.extend deviceToolchain;
          toolPkgs = nixpkgs-unstable.legacyPackages.${system};
          anagramToolPkgs = toolPkgs.pkgsCross.aarch64-multiplatform;

          deviceToolchain = _: prev: {
            gcc9 = prev.callPackage ./nix/gcc9.nix {
              gcc9 = prev.gcc9;
              src = gcc9;
            };
          };

          anagramNativePkgs =
            if nativePkgs.stdenv.hostPlatform.isAarch64 then
              nativePkgs
            else
              nativePkgs.pkgsCross.aarch64-multiplatform;

          anagramEmulator = anagramNativePkgs.stdenv.hostPlatform.emulator anagramNativePkgs.buildPackages;

          dpfAnagram = nativePkgs.callPackage ./nix/dpf-anagram.nix { src = dpf; };

          mkRangeV3 = pkgs: pkgs.callPackage ./nix/range-v3.nix { src = range-v3; };

          modla = anagramNativePkgs.callPackage ./nix/modla.nix {
            src = libmodla;
            lv2 = anagramToolPkgs.lv2;
          };

          lv2Bundles = toolPkgs.callPackage ./nix/lv2.nix {
            inherit
              darkglass-lv2-extensions
              mod-lv2-extensions
              kxstudio-lv2-extensions
              ;
          };

          lv2Tools = toolPkgs.callPackage ./nix/lv2-tools.nix { bundles = lv2Bundles; };

          anagramLv2Tools = toolPkgs.callPackage ./nix/lv2-tools.nix {
            inherit (anagramToolPkgs) lilv lv2lint;
            bundles = lv2Bundles;
            emulator = anagramEmulator;
          };

          anagramToolchain = toolPkgs.writeText "anagram-toolchain.cmake" ''
            set(CMAKE_SYSTEM_NAME Linux)
            set(CMAKE_SYSTEM_PROCESSOR aarch64)

            set(CMAKE_CROSSCOMPILING_EMULATOR "${anagramEmulator}")
          '';
        in
        {
          devShells = {
            default = (nativePkgs.mkShell.override { stdenv = nativePkgs.gcc9Stdenv; }) {
              buildInputs = [
                dpfAnagram
                lv2Tools
                (mkRangeV3 nativePkgs)
              ];

              packages = [
                nativePkgs.cmake
                nativePkgs.ninja
              ];

              BUILD_DIR = "build";
            };

            anagram = (anagramNativePkgs.mkShell.override { stdenv = anagramNativePkgs.gcc9Stdenv; }) {
              buildInputs = [
                dpfAnagram
                modla
                anagramLv2Tools
                (mkRangeV3 anagramNativePkgs)
              ];

              packages = [
                anagramNativePkgs.buildPackages.cmake
                anagramNativePkgs.buildPackages.ninja
                anagramNativePkgs.buildPackages.patchelf
              ];

              hardeningDisable = [ "all" ];

              BUILD_DIR = "build-anagram";
              CMAKE_TOOLCHAIN_FILE = "${anagramToolchain}";
            };
          };

          formatter = toolPkgs.callPackage ./nix/formatter.nix { inherit treefmt-nix; };
        }
      );
}

{
  pkgs,
  treefmt-nix,
}:
(treefmt-nix.lib.evalModule pkgs {
  projectRootFile = null;

  settings = {
    tree-root-cmd = "${pkgs.git}/bin/git rev-parse --show-toplevel";
    formatter.clang-tidy.priority = 1;
  };

  programs = {
    clang-format = {
      enable = true;
      package = pkgs.llvmPackages_23.clang-tools;
    };

    clang-tidy = {
      enable = true;
      package = pkgs.llvmPackages_23.clang-tools;
      excludes = [ "config/*" ];
    };

    cmake-format.enable = true;
    nixfmt.enable = true;
  };
}).config.build.wrapper

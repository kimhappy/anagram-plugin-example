{
  gcc9,
  src,
}:
gcc9.override {
  cc = gcc9.cc.overrideAttrs (old: {
    inherit src;
    version = "9.5.0";

    passthru = old.passthru // {
      version = "9.5.0";
    };

    patches = builtins.filter (
      patch: builtins.match ".*no-sys-dirs.*" (toString patch) != null
    ) old.patches;

    EXTRA_FLAGS_FOR_TARGET = old.EXTRA_FLAGS_FOR_TARGET ++ [ "-Os" ];
  });
}

{
  applyPatches,
  runCommand,
  src,
}:
let
  source = applyPatches {
    inherit src;
    name = "dpf-anagram-source";
    postPatch = "find . -name '*.orig' -delete";

    patches = [
      ../patch/dpf-anagram-quick-pot.patch
      ../patch/dpf-maintainer-email.patch
      ../patch/dpf-lv2-project-shortdesc.patch
    ];
  };
in
runCommand "dpf-anagram" { } ''
  mkdir -p $out/share/cmake/DPF
  ln -s ${source} $out/share/dpf

  cat > $out/share/cmake/DPF/DPFConfig.cmake <<EOF
  set(DPF_ROOT_DIR "$out/share/dpf" CACHE INTERNAL
      "Root directory of the DISTRHO Plugin Framework."
  )

  list(APPEND CMAKE_MODULE_PATH "$out/share/dpf/cmake")

  include(DPF-plugin)
  EOF
''

{
  runCommand,
  sord,
  lilv,
  lv2lint,
  bundles,
  emulator ? null,
}:
let
  prefix = if emulator == null then "" else ''"${emulator}" '';
in
runCommand "lv2-tools" { } ''
  mkdir -p $out/share/cmake/LV2Tools

  cat > $out/share/cmake/LV2Tools/LV2ToolsConfig.cmake <<EOF
  set(LV2_BUNDLES "${bundles}")

  set(SORD_VALIDATE_TOOL "${sord}/bin/sord_validate")
  set(LV2LS_TOOL ${prefix}"${lilv}/bin/lv2ls")
  set(LV2LINT_TOOL ${prefix}"${lv2lint}/bin/lv2lint")
  EOF
''

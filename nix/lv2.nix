{
  runCommand,
  lv2,
  darkglass-lv2-extensions,
  mod-lv2-extensions,
  kxstudio-lv2-extensions,
}:
runCommand "lv2-bundles" { } ''
  mkdir -p $out
  cp -r ${lv2}/lib/lv2/* $out/
  cp -r ${darkglass-lv2-extensions}/dg-* $out/
  cp -r ${mod-lv2-extensions}/mod*.lv2 $out/
  cp -r ${kxstudio-lv2-extensions}/kx-* $out/
''

{
  stdenvNoCC,
  lv2,
  src,
}:
stdenvNoCC.mkDerivation {
  inherit src;
  pname = "modla";
  version = "1.3.1";

  propagatedBuildInputs = [ lv2.dev ];

  installPhase = ''
    runHook preInstall

    install -Dm444 libmodla.a -t $out/lib
    install -Dm444 libmodla.h mod-license.h -t $out/include

    mkdir -p $out/lib/cmake/modla

    cat > $out/lib/cmake/modla/modla-config.cmake <<EOF
    add_library(modla::modla STATIC IMPORTED)

    set_target_properties(
        modla::modla
        PROPERTIES IMPORTED_LOCATION "$out/lib/libmodla.a"
                   INTERFACE_INCLUDE_DIRECTORIES "$out/include"
    )
    EOF

    runHook postInstall
  '';
}

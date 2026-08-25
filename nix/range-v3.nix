{
  stdenv,
  cmake,
  src,
}:
stdenv.mkDerivation {
  inherit src;
  pname = "range-v3";
  version = "unstable";
  nativeBuildInputs = [ cmake ];

  cmakeFlags = [
    "-DRANGE_V3_TESTS=OFF"
    "-DRANGE_V3_EXAMPLES=OFF"
    "-DRANGE_V3_DOCS=OFF"
  ];
}

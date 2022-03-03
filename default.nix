let
  expat-version =
    let
      cmakeFile = builtins.readFile ./expat/CMakeLists.txt;
      captures = builtins.match ".*VERSION[\r\n] *([0-9\.]*).*" cmakeFile;
    in
    builtins.elemAt captures 0;

  pkgs = import <nixpkgs> {
    overlays = [
      (final: prev: {
        # normally, a release is consumed
        expat = prev.expat.overrideAttrs (oldAttrs: {
          name = "expat-${expat-version}";
          src = ./.;

          nativeBuildInputs = [ prev.autoreconfHook ];
          prePatch = ''cd expat'';
          postFixup = ''
            substituteInPlace $dev/lib/cmake/expat-${expat-version}/expat-noconfig.cmake \
              --replace "$"'{_IMPORT_PREFIX}' $out
          '';
        });
      })
    ];
  };
in
pkgs



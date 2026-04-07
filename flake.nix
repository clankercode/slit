{
  description = "slit — streaming terminal viewer";

  outputs = inputs @ {
    self,
    flake-parts,
    ...
  }:
    flake-parts.lib.mkFlake {inherit inputs;} {
      systems = ["x86_64-linux" "aarch64-linux" "x86_64-darwin" "aarch64-darwin"];

      perSystem = {
        pkgs,
        system,
        ...
      }: let
        slit-c = pkgs.stdenv.mkDerivation {
          pname = "slit";
          version = self.shortRev or "dev";

          src = self;

          buildPhase = ''
            runHook preBuild
            ${pkgs.gcc}/bin/gcc -O2 -Wall -Wextra -std=c11 -pedantic \
              -D_POSIX_C_SOURCE=200809L \
              -o slit c/main.c c/buffer.c c/terminal.c c/render.c \
              c/layout.c c/spinner.c c/status.c c/tee.c c/debug.c c/completion.c
            runHook postBuild
          '';

          installPhase = ''
            runHook preInstall
            install -Dm755 slit $out/bin/slit
            runHook postInstall
          '';
        };

        slit-go = pkgs.buildGoModule {
          pname = "slit";
          version = self.shortRev or "dev";

          src = self;
          modRoot = "go";

          vendorHash = ""; # set to actual hash after first build attempt
        };
      in {
        packages = {
          default = slit-c;
          slit-c = slit-c;
          slit-go = slit-go;
        };

        devShells.default = pkgs.mkShell {
          packages = with pkgs; [gcc go cargo just];
        };
      };
    };

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixpkgs-unstable";
    flake-parts.url = "github:hercules-ci/flake-parts";
  };
}

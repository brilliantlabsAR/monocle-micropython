{
  inputs.nixpkgs.url = "github:nixos/nixpkgs/nixos-22.11";
  inputs.flake-utils.url = "github:numtide/flake-utils";
  inputs.flake-compat = {
    url = "github:edolstra/flake-compat";
    flake = false;
  };

  outputs = inputs@{ self, nixpkgs, flake-utils, ... }:
    flake-utils.lib.eachDefaultSystem (system:
      with import nixpkgs
        {
          inherit system;
          config.allowUnfree = true;
          config.segger-jlink.acceptLicense = true;
        };
      {
        devShell = with pkgs; mkShell {
          nativeBuildInputs = [
            gcc-arm-embedded
            git
            nrf-command-line-tools
            nrfutil
            python3
          ];
        };
      }
    );
}

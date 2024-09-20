let
  nixpkgs = fetchTarball "https://github.com/NixOS/nixpkgs/tarball/nixos-24.05";
  pkgs = import nixpkgs { config = {}; overlays = []; };
in
{
  glslang-shared = pkgs.callPackage ./glslang-shared.nix { };
  open4x = pkgs.callPackage ./open4x.nix { };
}

{
  description = "Open4X nixos flake";

  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs?ref=nixos-24.05";
  };

  outputs = { self , nixpkgs ,... }: let
    system = "x86_64-linux";
  in {
    devShells."${system}".default = let
      pkgs = import nixpkgs {
        inherit system;
      };
      PROJECT_ROOT = builtins.toString ./.;
      glslang-shared = pkgs.callPackage "${PROJECT_ROOT}/glslang-shared.nix" {};
    in pkgs.mkShell {
      packages = with pkgs; [
        glm
        vulkan-headers
        vulkan-loader
        vulkan-validation-layers
        glfw
        spirv-cross
        spirv-tools
        glslang-shared
        vulkan-tools
      ];
      shellHook = ''
        exec zsh
      '';

    };
  };
}

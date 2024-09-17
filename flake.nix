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
    in pkgs.mkShell {
      nativeBuildInputs = with pkgs; [
	glm
	vulkan-headers
	vulkan-loader
	vulkan-validation-layers
	glfw
	glslang
	spirv-cross
	shaderc
      ];
      shellHook = ''
        exec zsh
      '';

    };
  };
}

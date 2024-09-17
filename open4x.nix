with import <nixpkgs> {};
{
    stdenv, 
    lib, 

    cmake, 
    ninja,

    glm, 
	vulkan-headers,
	vulkan-loader,
	vulkan-validation-layers,
	glfw,
	spirv-cross,
    spirv-tools,
    glslang
}:
let
  fs = lib.fileset;
  sourceFiles = fs.difference ./. ./build;
  glslang-shared = pkgs.callPackage ./glslang-shared.nix {};
in

fs.trace sourceFiles

stdenv.mkDerivation {
    name = "fileset";
    src = fs.toSource {
     root = ./.;
     fileset = sourceFiles;
    };

  nativeBuildInputs = [ cmake ninja ];

  buildInputs = [ 
    glm 
	vulkan-headers
	vulkan-loader
	vulkan-validation-layers
	glfw
	spirv-cross
    spirv-tools
	glslang-shared
#    glslang
];
  installPhase = ''
    mkdir -p $out/bin
    cp Open4X $out/bin
'';
}

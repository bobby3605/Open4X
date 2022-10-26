# Makefile used to control cmake and ninja for building
.PHONY: open4x run clean shaders cleanCache

open4x:
	mkdir -p build
	cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=1 -S . -B build -GNinja
	ninja -v -C build

run:
	build/Open4X

shaders:
	mkdir -p build/assets/shaders
	rm -f build/assets/shaders/*
	cmake -S . -B build
	cmake --build build/ --target shaders

clean:
	rm -r build

cleanCache:
	rm -r assets/cache/*

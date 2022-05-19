# Makefile used to control cmake and ninja for building
.PHONY: open4x run clean

open4x:
	mkdir -p build
	cmake -S . -B build -GNinja
	ninja -v -C build

run:
	build/Open4X

clean:
	rm -r build

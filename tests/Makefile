.PHONY: tests

tests:
	cmake -S . -B build -GNinja && ninja -v -C build

clean:
	rm -r build

run:
	build/Open4X-Tests

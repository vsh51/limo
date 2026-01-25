.PHONY: build build-debug build-release build-docs test clean

BUILD_TYPE ?= Debug

build:
	cmake -S . -B build -DCMAKE_BUILD_TYPE=$(BUILD_TYPE)
	cmake --build build

build-debug:
	$(MAKE) build BUILD_TYPE=Debug

build-release:
	$(MAKE) build BUILD_TYPE=Release

build-docs:
	cmake -S . -B build -DCMAKE_BUILD_TYPE=$(BUILD_TYPE) -DLIMO_BUILD_DOCS=ON
	cmake --build build --target docs

test:
	cmake --build build --target tests
	ctest --test-dir build

clean:
	rm -rf build

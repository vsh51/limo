.PHONY: build build-debug build-release build-docs test coverage clean

BUILD_TYPE ?= Debug
CMAKE_FLAGS ?=

build:
	cmake -S . -B build -DCMAKE_BUILD_TYPE=$(BUILD_TYPE) $(CMAKE_FLAGS)
	cmake --build build

build-debug:
	$(MAKE) build BUILD_TYPE=Debug

build-release:
	$(MAKE) build BUILD_TYPE=Release

build-docs:
	cmake -S . -B build -DCMAKE_BUILD_TYPE=$(BUILD_TYPE) -DLIMO_BUILD_DOCS=ON $(CMAKE_FLAGS)
	cmake --build build --target docs

test:
	cmake --build build --target tests

coverage:
	cmake -S . -B build -DCMAKE_BUILD_TYPE=$(BUILD_TYPE) -DLIMO_BUILD_TESTS=ON -DLIMO_ENABLE_COVERAGE=ON $(CMAKE_FLAGS)
	cmake --build build --target tests
	mkdir -p build/coverage-report
	gcovr -r . build \
		--exclude ".*/tests/.*" \
		--exclude ".*/_deps/.*" \
		--exclude ".*/build/.*" \
		--exclude-branches-by-pattern ".*" \
		--print-summary \
		--html-details -o build/coverage-report/index.html \
		--fail-under-line 90 --fail-under-function 90

clean:
	rm -rf build

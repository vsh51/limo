.PHONY: build build-debug build-release build-docs test coverage integration-test \
       clean docker-up docker-down docker-build lint help

BUILD_TYPE ?= Debug
CMAKE_FLAGS ?=

# ── C++ build targets ─────────────────────────────────────────────────────────

build: ## Build the project (default: Debug)
	cmake -S . -B build -DCMAKE_BUILD_TYPE=$(BUILD_TYPE) $(CMAKE_FLAGS)
	cmake --build build

build-debug: ## Build in Debug mode
	$(MAKE) build BUILD_TYPE=Debug

build-release: ## Build in Release mode
	$(MAKE) build BUILD_TYPE=Release

build-docs: ## Generate Doxygen documentation
	cmake -S . -B build -DCMAKE_BUILD_TYPE=$(BUILD_TYPE) -DLIMO_BUILD_DOCS=ON $(CMAKE_FLAGS)
	cmake --build build --target docs

# ── Test targets ──────────────────────────────────────────────────────────────

test: ## Run unit tests
	cmake --build build --target tests

coverage: ## Generate coverage report (90% threshold)
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

integration-test: ## Run Python integration tests
	python3 -m pytest tests/integration/ -v

# ── Docker targets ────────────────────────────────────────────────────────────

docker-build: ## Build all Docker images
	docker compose build

docker-up: ## Start all services
	docker compose up -d

docker-up-monitoring: ## Start all services including Flower monitoring
	docker compose --profile monitoring up -d

docker-down: ## Stop all services
	docker compose down

docker-logs: ## Tail logs from all services
	docker compose logs -f

# ── Utility targets ───────────────────────────────────────────────────────────

clean: ## Remove build directory
	rm -rf build

help: ## Show this help message
	@grep -E '^[a-zA-Z_-]+:.*?## .*$$' $(MAKEFILE_LIST) | \
		awk 'BEGIN {FS = ":.*?## "}; {printf "  \033[36m%-22s\033[0m %s\n", $$1, $$2}'

SRC_DIR=$(PWD)/../src
BUILD_DIR=$(PWD)/../build

GO="go"
COVERAGE_OUTPUT ?= coverage.out

.PHONY: default vet test clean

default: prepare vet test clean

MAKEFILE_PATH := $(abspath $(lastword $(MAKEFILE_LIST)))
CURR_DIR := $(dir $(MAKEFILE_PATH))

CGO_ENABLED := 1

CGO_LDFLAGS ?= "\
-L$(CURR_DIR)../build/depends/mimalloc \
-L$(CURR_DIR)../build/depends/relic/lib \
-L$(CURR_DIR)../build/src \
-ldashbls -lrelic_s -lmimalloc-secure -lgmp"

CGO_CXXFLAGS ?= "\
-I$(CURR_DIR)../build/depends/relic/include \
-I$(CURR_DIR)../depends/mimalloc/include \
-I$(CURR_DIR)../depends/relic/include \
-I$(CURR_DIR)../include"

prepare:
	@mkdir -p ../build/src/dashbls
	cp -rv ../src/* ../build/src/dashbls

fmt:  ## Run go fmt to format Go files
	$(GO) fmt

test: ## Run a basic test suite
	CGO_CXXFLAGS=$(CGO_CXXFLAGS) CGO_LDFLAGS=$(CGO_LDFLAGS) $(GO) test

cover:  ## Run tests and generate test coverage file, output coverage results and HTML coverage file.
	CGO_CXXFLAGS=$(CGO_CXXFLAGS) CGO_LDFLAGS=$(CGO_LDFLAGS) $(GO) test -coverprofile $(COVERAGE_OUTPUT)
	CGO_CXXFLAGS=$(CGO_CXXFLAGS) CGO_LDFLAGS=$(CGO_LDFLAGS) $(GO) tool cover -func=$(COVERAGE_OUTPUT)
	CGO_CXXFLAGS=$(CGO_CXXFLAGS) CGO_LDFLAGS=$(CGO_LDFLAGS) $(GO) tool cover -html=$(COVERAGE_OUTPUT)
	rm -f $(COVERAGE_OUTPUT)

vet:  ## Go vet all project code
	CGO_CXXFLAGS=$(CGO_CXXFLAGS) CGO_LDFLAGS=$(CGO_LDFLAGS) go vet ./...

help: ## Show This Help
	@for line in $$(cat Makefile | grep "##" | grep -v "grep" | sed  "s/:.*##/:/g" | sed "s/\ /!/g"); do verb=$$(echo $$line | cut -d ":" -f 1); desc=$$(echo $$line | cut -d ":" -f 2 | sed "s/!/\ /g"); printf "%-30s--%s\n" "$$verb" "$$desc"; done

clean: ## Clean up transient (generated) files
	$(GO) clean
	rm -f $(COVERAGE_OUTPUT)

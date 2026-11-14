CXX ?= g++
CPPFLAGS := -Iinclude
CXXFLAGS ?= -std=c++17 -Wall -Wextra -Wpedantic -Werror -O2
CORE := src/inventory.cpp src/logistics.cpp src/hub.cpp
ASAN_OPTIONS ?= detect_leaks=1
HEADERS := $(wildcard include/erl/*.hpp)
.PHONY: all test sanitize clean
all: build/erl-hub
build:
	mkdir -p build
build/erl-hub: $(CORE) src/main.cpp $(HEADERS) | build
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(CORE) src/main.cpp -o $@
build/tests: $(CORE) tests/tests.cpp $(HEADERS) | build
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(CORE) tests/tests.cpp -o $@
test: build/tests
	./build/tests
sanitize: | build
	$(CXX) $(CPPFLAGS) -std=c++17 -g -Wall -Wextra -Wpedantic -fsanitize=address,undefined -fno-omit-frame-pointer $(CORE) tests/tests.cpp -o build/tests-sanitized
	ASAN_OPTIONS=$(ASAN_OPTIONS) ./build/tests-sanitized
clean:
	rm -rf build

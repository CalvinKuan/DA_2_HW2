CXX      ?= g++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra

TARGET := eol_drc
SRCS   := $(wildcard src/*.cpp)
OBJS   := $(SRCS:src/%.cpp=build/%.o)

.PHONY: all test run figures clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

build/%.o: src/%.cpp | build
	$(CXX) $(CXXFLAGS) -MMD -MP -c $< -o $@

build:
	mkdir -p build

# Run all testcases and compare with testcases/expected/
test: $(TARGET)
	./scripts/run_tests.sh

# Run the sample from the spec
run: $(TARGET)
	mkdir -p output
	./$(TARGET) testcases/inputs/tc00_sample.txt output/tc00_sample.out
	cat output/tc00_sample.out

# Draw testcases/figures/*.png (and output/<name>.png for results in output/)
figures:
	./scripts/make_figures.sh

clean:
	rm -rf build $(TARGET) output

-include $(OBJS:.o=.d)

CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -pedantic -O2 -pthread

BUILD_DIR = build
COMMON_SRC = src/common/utils.cpp
COMMON_OBJ = $(BUILD_DIR)/common/utils.o

all: $(COMMON_OBJ)

$(BUILD_DIR)/common/%.o: src/common/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR) *.o

.PHONY: all clean
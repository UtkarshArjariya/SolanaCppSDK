BUILD_DIR := build
.PHONY: all clean test debug
all:
cmake -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=Release
cmake --build $(BUILD_DIR)
debug:
cmake -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=Debug
cmake --build $(BUILD_DIR)
clean:
rm -rf $(BUILD_DIR)
test: all
cd $(BUILD_DIR) && ctest --output-on-failure

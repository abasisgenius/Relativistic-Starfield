BUILD_DIR ?= build
TARGET := $(BUILD_DIR)/relativistic-starfield

.PHONY: all configure run clean web

all: $(TARGET)

configure:
	cmake -S . -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=Release

$(TARGET): configure
	cmake --build $(BUILD_DIR) --config Release -j

run: all
	./$(TARGET)

web:
	emcmake cmake -S . -B build-web -DCMAKE_BUILD_TYPE=Release
	cmake --build build-web --config Release -j

clean:
	rm -rf build build-web

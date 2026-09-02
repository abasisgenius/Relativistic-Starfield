BUILD_DIR ?= build
BUILD_WEB_DIR ?= build-web
DOCS_DIR ?= docs
TARGET := $(BUILD_DIR)/relativistic-starfield

export EM_CACHE ?= $(CURDIR)/$(BUILD_WEB_DIR)/.emcache

.PHONY: all configure run clean web


all: $(TARGET)

configure:
	cmake -S . -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=Release

$(TARGET): configure
	cmake --build $(BUILD_DIR) --config Release -j

run: all
	./$(TARGET)

web:
	@which emcmake > /dev/null 2>&1 || (echo "Error: 'emcmake' not found in PATH.\n\nTo install Emscripten on Mac, run:\n  brew install emscripten\n\nOr if using emsdk, activate it in your terminal:\n  source /path/to/emsdk/emsdk_env.sh\n" && exit 1)
	emcmake cmake -S . -B $(BUILD_WEB_DIR) -DCMAKE_BUILD_TYPE=Release
	cmake --build $(BUILD_WEB_DIR) --config Release -j
	mkdir -p $(DOCS_DIR)
	cp $$(if [ -f $(BUILD_WEB_DIR)/relativistic-starfield.html ]; then echo $(BUILD_WEB_DIR)/relativistic-starfield.html; else echo $(BUILD_WEB_DIR)/index.html; fi) $(DOCS_DIR)/index.html
	cp $(BUILD_WEB_DIR)/relativistic-starfield.js $(DOCS_DIR)/relativistic-starfield.js
	cp $(BUILD_WEB_DIR)/relativistic-starfield.wasm $(DOCS_DIR)/relativistic-starfield.wasm
	cp $(BUILD_WEB_DIR)/relativistic-starfield.data $(DOCS_DIR)/relativistic-starfield.data
	touch $(DOCS_DIR)/.nojekyll


clean:
	rm -rf $(BUILD_DIR) $(BUILD_WEB_DIR)


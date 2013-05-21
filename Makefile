BUILD_DIR=./build
BIN_DIR=$(BUILD_DIR)/bin
BIN_NAME=noj

all:build
	cd $(BUILD_DIR); cmake -DCMAKE_BUILD_TYPE=RELEASE .. && make

debug:build
	cd $(BUILD_DIR); cmake -DCMAKE_BUILD_TYPE=DEBUG .. && make

run:
	@cd $(BIN_DIR); ./$(BIN_NAME)

install:
	cd $(BUILD_DIR); make install

clean:
	rm -rf $(BUILD_DIR)

build:
	mkdir $(BUILD_DIR)

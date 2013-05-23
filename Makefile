BUILD_DIR=./build
BIN_DIR=$(BUILD_DIR)/bin
BIN_NAME=noj

all:debug

release:build
	cd $(BUILD_DIR); cmake -DCMAKE_BUILD_TYPE=RELEASE .. && make

debug:build
	cd $(BUILD_DIR); cmake -DCMAKE_BUILD_TYPE=DEBUG .. && make

install:
	cd $(BUILD_DIR); make install;

clean:
	rm -rf $(BUILD_DIR)

build:
	mkdir $(BUILD_DIR)

update:
	sudo apt-get update;
	sudo apt-get install cmake mysql-client libmysql++-dev libpcre3-dev

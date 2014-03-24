DAEMON_NAME = remote-runnerd
BUILD_PATH = ./build
SRC_PATH = ./src

DEFAULT_TIMEOUT = 5
LIB_SUFFIX = -mt
MAC_OS_LIB_PATH = /opt/local/lib

CPP = g++
CFLAGS = -std=c++11 -Werror -ggdb -DDEBUG
LFLAGS = -L$(MAC_OS_LIB_PATH) -lboost_system$(LIB_SUFFIX) -lboost_thread$(LIB_SUFFIX) -lboost_iostreams$(LIB_SUFFIX)

BUILD_OBJECTS = $(BUILD_PATH)/main.o $(BUILD_PATH)/Server.o $(BUILD_PATH)/Session.o $(BUILD_PATH)/ProcessRunner.o $(BUILD_PATH)/ConfigParser.o

build: $(BUILD_PATH)/$(DAEMON_NAME)

.PHONY: run
run: build
	$(BUILD_PATH)/$(DAEMON_NAME) $(DEFAULT_TIMEOUT)

$(BUILD_PATH)/$(DAEMON_NAME): $(BUILD_OBJECTS)
	$(CPP) $(LFLAGS) $(BUILD_OBJECTS) -o $@

$(BUILD_PATH)/main.o: $(SRC_PATH)/main.cpp $(SRC_PATH)/Server.h $(SRC_PATH)/settings.h
	$(CPP) $(CFLAGS) -c $< -o $@

$(BUILD_PATH)/ConfigParser.o: $(SRC_PATH)/ConfigParser.cpp $(SRC_PATH)/ConfigParser.h
	$(CPP) $(CFLAGS) -c $< -o $@

$(BUILD_PATH)/ProcessRunner.o: $(SRC_PATH)/ProcessRunner.cpp $(SRC_PATH)/ProcessRunner.h $(SRC_PATH)/settings.h
	$(CPP) $(CFLAGS) -c $< -o $@

$(BUILD_PATH)/Session.o: $(SRC_PATH)/Session.cpp $(SRC_PATH)/ProcessRunner.h $(SRC_PATH)/settings.h
	$(CPP) $(CFLAGS) -c $< -o $@

$(BUILD_PATH)/Server.o: $(SRC_PATH)/Server.cpp $(SRC_PATH)/Session.h $(SRC_PATH)/ConfigParser.h $(SRC_PATH)/settings.h
	$(CPP) $(CFLAGS) -c $< -o $@

$(BUILD_PATH)/%.o: $(SRC_PATH)/%.cpp $(SRC_PATH)/*.h
	$(CPP) $(CFLAGS) -c $< -o $@

.PHONY: clean
clean: 
	rm -rf $(BUILD_PATH)/*.o $(BUILD_PATH)/$(DAEMON_NAME)

